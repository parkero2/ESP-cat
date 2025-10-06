#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// HTML content
#include "index-min.h"

// Modify these
// Tank drive controls
const int LF = 1, LB = 2, RF = 3, RB = 4;

// Camera pin definitions (ESP32-CAM AI-Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// WiFi credentials
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

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
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // Frame size and quality settings
    config.frame_size = FRAMESIZE_VGA; // 640x480
    config.jpeg_quality = 10; // 0-63, lower means higher quality
    config.fb_count = 1;
    
    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return false;
    }
    
    return true;
}

/// @brief Set the speed of the left and right motors.
/// @param leftSpeed The speed for the left motors (-255 to 255) Negative integers indicate reverse direction.
/// @param rightSpeed The speed for the right motors (-255 to 255) Negative integers indicate reverse direction.
void setSpeed(int leftSpeed = lsp, int rightSpeed = rsp) {
    lsp = leftSpeed < -255 || leftSpeed > 255 ? lsp : leftSpeed;
    rsp = rightSpeed < -255 || rightSpeed > 255 ? rsp : rightSpeed;

    // Left motors
    if (lsp < 0) {
        analogWrite(LF, 0);  // stop
        analogWrite(LB, abs(lsp));
    } else if (lsp > 0) {
        analogWrite(LF, abs(lsp));
        analogWrite(LB, 0);  // stop
    } else {
        analogWrite(LF, 0);  // stop
        analogWrite(LB, 0);  // stop
    }

    // Right motors
    if (rsp < 0) {
        analogWrite(RF, LOW);
        analogWrite(RB, abs(rsp));
    } else if (rsp > 0) {
        analogWrite(RF, abs(rsp));
        analogWrite(RB, LOW);
    } else {
        analogWrite(RF, LOW);
        analogWrite(RB, LOW);   
    }
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