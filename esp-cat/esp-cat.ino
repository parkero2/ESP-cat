#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// HTML content
#include "index-min.h"

// Modify these
// Tank drive controls
const int LF = 1, LB = 2, RF = 3, RB = 4;

// WiFi credentials
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

// End of modifications

int lsp, rsp = 0;

// Webserver stuff 
WebServer server(80); // Port 80 (http://ip/)

/// @brief Set the speed of the left and right motors.
/// @param leftSpeed The speed for the left motors (-255 to 255) Negative integers indicate reverse direction.
/// @param rightSpeed The speed for the right motors (-255 to 255) Negative integers indicate reverse direction.
void setSpeed(int leftSpeed = lsp, int rightSpeed = rsp) {
    lsp = leftSpeed < -255 || leftSpeed > 255 ? lsp : leftSpeed;
    rsp = rightSpeed < -255 || rightSpeed > 255 ? rsp : rightSpeed;

    // Left motors
    if (lsp < 0) {
        analogWrite(LF, LOW);
        analogWrite(LB, abs(lsp));
    } else if (lsp > 0) {
        analogWrite(LF, abs(lsp));
        analogWrite(LB, LOW);
    } else {
        analogWrite(LF, LOW);
        analogWrite(LB, LOW);
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
    int lOpt = server.arg("l").toInt() || 0;
    int rOpt = server.arg("r").toInt() || 0;

    if (!lOpt || !rOpt) {
        server.send(400, "text/plain", "Bad Request");
        return;
    }

    setSpeed(lOpt * 255, rOpt * 255);
    server.send(200, "text/plain", "OK");
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

    // WiFi setup
    Serial.begin(115200);
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
    server.begin();
}

void loop() {
    server.handleClient();

    // Example usage: Set left motors to 100 and right motors to -100
    setSpeed(100, -100);
    delay(2000);

    setSpeed(0, 0);
    delay(2000);
}