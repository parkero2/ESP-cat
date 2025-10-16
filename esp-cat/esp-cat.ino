#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// HTML content
#include "index-min.h"

// Use the standard camera model definition
#define CAMERA_MODEL_WROVER_KIT
#include "camera_pins.h"

// Tank drive controls (Updated to avoid camera pin conflicts)
const int LF = 2, LB = 15, RF = 16, RB = 17, LSP_PWM = 12, RSP_PWM = 13;

// WiFi credentials
const char *ssid = "Valleyview";
const char *password = "Littleseven7!";

// End of modifications

int lsp, rsp = 0;

// Webserver stuff 
WebServer server(80); // Port 80 (http://ip/)

// Camera initialization function
bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;  // Fixed typo: was pin_sscb_sda
    config.pin_sccb_scl = SIOC_GPIO_NUM;  // Fixed typo: was pin_sscb_scl
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA; // Start with smaller frame size
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 15; // Higher number = lower quality, less memory
    config.fb_count = 1; // Start with single buffer
    
    // if PSRAM IC present, init with higher JPEG quality
    if (config.pixel_format == PIXFORMAT_JPEG) {
        if (psramFound()) {
            Serial.println("PSRAM found - using high quality settings");
            config.jpeg_quality = 12;
            config.fb_count = 1; // Keep single buffer to avoid memory issues
            config.grab_mode = CAMERA_GRAB_LATEST;
            config.frame_size = FRAMESIZE_SVGA; // Try medium size first
        } else {
            Serial.println("No PSRAM found - using reduced settings");
            // Limit the frame size when PSRAM is not available
            config.frame_size = FRAMESIZE_QVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    }
    
    // Initialize camera
    Serial.println("Attempting camera initialization...");
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x (%s)\n", err, esp_err_to_name(err));
        Serial.println("Check camera module connection and pin definitions!");
        return false;
    }
    
    // Get camera sensor and apply settings like the working example
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        Serial.printf("Camera sensor detected: PID=0x%02x\n", s->id.PID);
        
        // Apply sensor-specific settings from working example
        if (s->id.PID == OV3660_PID) {
            s->set_vflip(s, 1);        // flip it back
            s->set_brightness(s, 1);   // up the brightness just a bit
            s->set_saturation(s, -2);  // lower the saturation
        }
        
        // Set initial frame size for better frame rate
        if (config.pixel_format == PIXFORMAT_JPEG) {
            s->set_framesize(s, FRAMESIZE_QVGA);
        }
    }
    
    return true;
}

/// @brief Set the speed of the left and right motors.
/// @param leftSpeed The speed for the left motors (-255 to 255) Negative integers indicate reverse direction.
/// @param rightSpeed The speed for the right motors (-255 to 255) Negative integers indicate reverse direction.
void setSpeed(int leftSpeed = lsp, int rightSpeed = rsp) {
    // Normalise values

    lsp = leftSpeed < -255 || leftSpeed > 255 ? lsp : leftSpeed;
    rsp = rightSpeed < -255 || rightSpeed > 255 ? rsp : rightSpeed;
    analogWrite(LSP_PWM, lsp);
    analogWrite(RSP_PWM, rsp);

    // Determine direction of each side including left/right turns
    if (lsp > 0 && rsp > 0) {
        setFWD();
    } else if (lsp < 0 && rsp < 0) {
        setREV();
    } else if (lsp > 0 && rsp < 0) {
        setRGT();
    } else if (lsp < 0 && rsp > 0) {
        setLFT();
    } else {
        setSTOP();
    }

}

void setFWD() {
    digitalWrite(RF, HIGH);
    digitalWrite(LF, HIGH);
    digitalWrite(RB, LOW);
    digitalWrite(LB, LOW);
}

void setREV() {
    digitalWrite(RF, LOW);
    digitalWrite(LF, LOW);
    digitalWrite(RB, HIGH);
    digitalWrite(LB, HIGH);
}

void setLFT() {
    digitalWrite(RF, HIGH);
    digitalWrite(LF, LOW);
    digitalWrite(RB, LOW);
    digitalWrite(LB, HIGH);
}

void setRGT() {
    digitalWrite(RF, LOW);
    digitalWrite(LF, HIGH);
    digitalWrite(RB, HIGH);
    digitalWrite(LB, LOW);
}



void setSTOP() {
    digitalWrite(RF, LOW);
    digitalWrite(LF, LOW);
    digitalWrite(RB, LOW);
    digitalWrite(LB, LOW);
}

/* Server Routes */
// '/'
void handleRoot() {
    server.send(200, "text/html", index_html);
}

// '/speed?l=[int]&r=[int]' 
void handleSpeed() {
    int lOpt = server.arg("l").toInt();
    int rOpt = server.arg("r").toInt();

    if (server.arg("l") == "" || server.arg("r") == "") {
        server.send(400, "text/plain", "Bad Request");
        return;
    }

    setSpeed(lOpt * 255, rOpt * 255);
    server.send(200, "text/plain", "OK");
}

// '/video' - Capture and return image
void handleVideo() {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        server.send(500, "text/plain", "Camera Error");
        return;
    }
    
    // Send JPEG image with proper headers
    server.sendHeader("Content-Type", "image/jpeg");
    server.sendHeader("Content-Length", String(fb->len));
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    
    // Send the image data
    server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
    
    // Return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
}
/* End of Server Routes */

void setup() {
    // Initialize motor pins
    pinMode(LF, OUTPUT);
    pinMode(RF, OUTPUT);
    pinMode(LB, OUTPUT);
    pinMode(RB, OUTPUT);

    // Set initial speed to 0
    setSpeed(0, 0);

    // Initialize Serial
    Serial.begin(115200);
    
    // Initialize camera
    Serial.println("Initializing camera...");
    if (!initCamera()) {
        Serial.println("Camera initialization failed!");
        return;
    }
    Serial.println("Camera initialized successfully");

    // WiFi setup
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Start web server
    server.on("/", handleRoot);
    server.on("/speed", handleSpeed);
    server.on("/video", handleVideo);
    server.begin();
    Serial.println("Web server started");
}

void loop() {
    server.handleClient();

    // Example usage: Set left motors to 100 and right motors to -100
    // setSpeed(100, -100);
    // delay(2000);

    // setSpeed(0, 0);
    // delay(2000);
}