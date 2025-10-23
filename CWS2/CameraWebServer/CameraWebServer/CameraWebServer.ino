#include "esp_camera.h"
#include <WiFi.h>
#include <SoftwareSerial.h>
#include <DFPlayerMini_Fast.h>

// BEGIN PIN DEF
const int LF = 2, LB = 3, RF = 4, RB = 5, LSP_PWM = 9, RSP_PWM = 10;
const bool initPins = true;  // Flag used for testing. Some pins cause issues with camera

const bool testUltrasonic = false;  // Flag to enable ultrasonic sensor testing

// Secondary board pins
const int SS_RX = 33, SS_TX = 32;  // Soft serial RX/TX pins.

// Ultrasonic sensor pins
const int TRIG = 12;  // Trig pin connected to esp 12
const int ECHO = 14;  // Echo pin connected to ESP 2

// DFPlayer Mini pins
// Refer to dfplayer.ino for DFPlayer Mini code
const int df_RX = 2, df_TX = 13;  // DFPlayer Mini SoftwareSerial pins

const int SOUND_SPEED = 343;  // Sound velocity in m/s

const double MIN_DISTANCE_CM = 10.0;  // Minimum safe distance in cm

SoftwareSerial ssSerial(SS_RX, SS_TX);  // RX, TX
//SoftwareSerial dfSerial(df_RX, df_TX);  // RX, TX

//DFPlayerMini_Fast dfPlayer;

// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
#define CAMERA_MODEL_WROVER_KIT  // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE  // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_CAMS3_UNIT  // Has PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "Valleyview";
const char *password = "Littleseven7!";

void startCameraServer();
void setupLedFlash(int pin);

// // Tank drive control functions
// void setSpeed(int l = 128, int r = 128);
// void fwd();
// void bck();
// void lft();
// void rgt();

void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
pinMode(ECHO, INPUT);
    pinMode(TRIG, OUTPUT);
  if (testUltrasonic) {
    pinMode(ECHO, INPUT);
    pinMode(TRIG, OUTPUT);
    while (true) {
      double distance = ultrasonicPulse();
      Serial.print("Distance: ");
      Serial.print(distance);
      Serial.println(" cm");
      delay(100);
    }
  }

  if (initPins) {
    // // Initialize motor control pins
    // pinMode(LF, OUTPUT);
    // pinMode(LB, OUTPUT);
    // pinMode(RF, OUTPUT);
    // pinMode(RB, OUTPUT);
    // pinMode(LSP_PWM, OUTPUT);
    // pinMode(RSP_PWM, OUTPUT);
  }

  // DF Player Mini setup
  // dfSerial.begin(9600);
  // delay(100);  // Give DFPlayer time to initialize
  // if (dfPlayer.begin(dfSerial)) {
  //   Serial.println("DFPlayer Mini initialized successfully!");
  //   dfPlayer.volume(20);  // Set volume (0-30)
  //   delay(100);
  //   Serial.println("Ready to play audio file!");
  // } else {
  //   Serial.println("Failed to initialize DFPlayer Mini!");
  //   Serial.println("Check connections and SD card");
  // }

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  // Initialize SoftwareSerial communication with Arduino Nano
  ssSerial.begin(9600);
  delay(100);  // Give nano time to initialize
  Serial.println("SoftwareSerial initialized for sub-board communication");

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

// Tank drive control functions
int leftSpeed = 128;
int rightSpeed = 128;

void setSpeed(int l, int r) {
  // return;
  // // constrain method ensures values are between 0 and 255, similar to map
  // leftSpeed = constrain(l, 0, 255);
  // rightSpeed = constrain(r, 0, 255);
  // analogWrite(LSP_PWM, leftSpeed);
  // analogWrite(RSP_PWM, rightSpeed);

  SS_digitalWrite(LSP_PWM, l);
  SS_digitalWrite(RSP_PWM, r);
}

void fwd() {
  SS_digitalWrite(LF, HIGH);
  SS_digitalWrite(LB, LOW);
  SS_digitalWrite(RF, HIGH);
  SS_digitalWrite(RB, LOW);
  setSpeed(255, 255);

  // Check distance and stop if too close
  if (ultrasonicPulse() < MIN_DISTANCE_CM) {
    setSpeed(0, 0);  // Stop the robot
    Serial.println("Obstacle detected! Stopping movement.");
  }
}

void bck() {
  SS_digitalWrite(LF, LOW);
  SS_digitalWrite(LB, HIGH);
  SS_digitalWrite(RF, LOW);
  SS_digitalWrite(RB, HIGH);
  setSpeed(255, 255);
}

void lft() {
  SS_digitalWrite(LF, LOW);
  SS_digitalWrite(LB, HIGH);
  SS_digitalWrite(RF, HIGH);
  SS_digitalWrite(RB, LOW);
  setSpeed(255, 255);
}

void rgt() {
  SS_digitalWrite(LF, HIGH);
  SS_digitalWrite(LB, LOW);
  SS_digitalWrite(RF, LOW);
  SS_digitalWrite(RB, HIGH);
  setSpeed(255, 255);
}

// Sub board controls
void SS_digitalWrite(uint8_t pin, uint8_t val) {
  // Send digital write command to Arduino Nano in format "pin:value"
  String command = String(pin) + ":" + String(val);
  ssSerial.println(command);

  // Debug output
  Serial.println("Sent to nano: " + command);
}


/// @brief Sends a PWM/analog write command to the Arduino Nano sub-controller
/// @param pin The pin number to write to
/// @param val The value to write (0-255)
void ss_analogWrite(uint8_t pin, int val) {
  // Send PWM/analog write command to Arduino Nano in format "pin:value"
  // Constrain value to 0-255 range
  val = constrain(val, 0, 255);

  String command = String(pin) + ":" + String(val);
  ssSerial.println(command);

  // Debug output
  Serial.println("Sent to nano: " + command);
}


/// @brief Sends a pulse from the ultrasonic sensor and measures the echo time
/// @return Distance measured by the ultrasonic sensor in centimeters
double ultrasonicPulse() {
  long duration;
  double distance;
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  duration = pulseIn(ECHO, HIGH, 30000);  // 30ms timeout
  Serial.println(duration);
  // duration is in microseconds and SOUND_SPEED is in m/s:
  // convert duration to seconds (duration / 1e6), multiply by speed to get meters,
  // divide by 2 for one-way, then convert to centimeters (*100).
  distance = (duration / 1e6) * (double)SOUND_SPEED / 2.0 * 100.0;  // Calculate distance in cm
  Serial.print("Distance: ");
  Serial.println(distance);
  return distance;
}

void dfPlayerPlay() {
  //dfPlayer.play(1);  // Play the first (and only) track
}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  if (ultrasonicPulse() < MIN_DISTANCE_CM) {
    setSpeed(0, 0);  // Stop the robot
    Serial.println("Obstacle detected in loop! Stopping movement.");
  }
  delay(500);
}
