

// Tank drive controls
#define LF 1;
#define RF 2;
#define LB 3;
#define RB 4;

int lsp, rsp = 0;



/// @brief Set the speed of the left and right motors.
/// @param leftSpeed The speed for the left motors (-255 to 255) Negative integers indicate reverse direction.
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
        analogWrite(LB, Math.abs(lsp));
    } else if (lsp > 0) {
        // Left motors forward
        analogWrite(LF, Math.abs(lsp));
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
        analogWrite(RB, Math.abs(rsp));
    } else if (rsp > 0) {
        // Right motors forward
        analogWrite(RF, Math.abs(rsp));
        analogWrite(RB, LOW);
    } else {
        // Right motors stop
        analogWrite(RF, LOW);
        analogWrite(RB, LOW);   
    }

}

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