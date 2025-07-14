#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>

// HTML content
#include "index-min.h"

// Modify these
// Tank drive controls
const int LF = 1, LB = 2, RF = 3, RB = 4;

const char *ssid = "";
const char *password = "";

// End of modifcations

int lsp, rsp = 0;

// Webserver stuff 
WebServer server(80); // Port 80 (http://ip/)

/// @brief Set the speed of the left and right motors.
/// @param leftSpeed The speed for the leftheader motors (-255 to 255) Negative integers indicate reverse direction.
/// @param rightSpeed The speed for the right motors (-255 to 255) Negative integers indicate reverse direction.
void setSpeed(int leftSpeed = lsp, int rightSpeed = rsp) {
    // Discard values out of range (-255 to 255)
    lsp = leftSpeed < -255 || leftSpeed > 255 ? lsp : leftSpeed;
    rsp = rightSpeed < -255 || rightSpeed > 255 ? rsp : rightSpeed;

    // Confirm directions
    // Left motors
    if (lsp < 0) {
        // Left motors reverse
        analogWrite(LF, LOW);
        analogWrite(LB, abs(lsp));
    } else if (lsp > 0) {
        // Left motors forward
        analogWrite(LF, abs(lsp));
        analogWrite(LB, LOW);
    } else {
        // Left motors stop
        analogWrite(LF, LOW);
        analogWrite(LB, LOW);
    }

    // Right motors
    if (rsp < 0) {
        // Right motors reverse
        analogWrite(RF, LOW);
        analogWrite(RB, abs(rsp));
    } else if (rsp > 0) {
        // Right motors forward
        analogWrite(RF, abs(rsp));
        analogWrite(RB, LOW);
    } else {
        // Right motors stop
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
// -1, 0, 1
void handleSpeed() {
    // l and r query params
    int lOpt = server.arg("l").toInt() || 0;
    int rOpt = server.arg("r").toInt() || 0;

    if (!lOpt || !rOpt) {
        // HTTP 400 bad request
        server.send(400, "text/plain", "Bad Request");
        return;
    }

    // Set speeds at high or low
    setSpeed(lOpt * 255, rOpt * 255);

    // 200 OK
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
}

void loop() {
    // Example usage: Set left motors to 100 and right motors to -100
    setSpeed(100, -100);
    delay(2000); // Run for 2 seconds

    // Stop the motors
    setSpeed(0, 0);
    delay(2000); // Wait for 2 seconds before next command
}