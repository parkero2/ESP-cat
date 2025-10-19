/*
 * Arduino Nano Sub-Controller for ESP32 Camera Robot
 * Receives pin:value commands via SoftwareSerial and controls local pins
 * Communication format: pin:value (e.g., "2:255", "A0:128", "9:0")
 * 
 * SoftwareSerial Connections (as per Notes.md):
 * ESP32 -> Arduino Nano
 * TX (GPIO 32) -> Pin 11 (RX)
 * RX (GPIO 33) -> Pin 12 (TX)
 * GND -> GND
 * 
 * Motor Driver (TB6612FNG) Connections:
 * IN1: D2, IN2: D3, IN3: D4, IN4: D5
 * PWM A: D9, PWM B: D10
 */

#include <SoftwareSerial.h>

// Pin mapping and state tracking
#define MAX_PINS 20
struct PinState {
  int pin;
  int value;
  bool isAnalog;
  bool isOutput;
};

PinState pinStates[MAX_PINS];
int activePins = 0;

// SoftwareSerial Communication settings
SoftwareSerial espSerial(11, 12); // RX=11, TX=12 (connect to ESP32 TX=32, RX=33)
#define BUFFER_SIZE 32
char inputBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// Status LED (built-in LED on pin 13)
#define STATUS_LED 13
unsigned long lastHeartbeat = 0;
bool ledState = false;

void setup() {
  Serial.begin(115200); // For debugging output
  espSerial.begin(9600); // SoftwareSerial communication with ESP32
  
  // Initialize status LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  // Initialize pin states array
  for (int i = 0; i < MAX_PINS; i++) {
    pinStates[i].pin = -1;
    pinStates[i].value = 0;
    pinStates[i].isAnalog = false;
    pinStates[i].isOutput = false;
  }
  
  Serial.println("Arduino Nano SoftwareSerial Sub-Controller Ready");
  Serial.println("Waiting for commands from ESP32...");
  Serial.println("Motor pins: IN1=D2, IN2=D3, IN3=D4, IN4=D5, PWMA=D9, PWMB=D10");
}

void loop() {
  // Handle incoming SoftwareSerial data
  handleSerialInput();
  
  // Heartbeat LED (blink every 2 seconds when idle)
  if (millis() - lastHeartbeat > 2000) {
    ledState = !ledState;
    digitalWrite(STATUS_LED, ledState);
    Serial.print("Heartbeat - LED is now ");
    Serial.println(ledState ? "ON" : "OFF");
    lastHeartbeat = millis();
  }
  
  delay(10); // Small delay to prevent overwhelming the processor
}

void handleSerialInput() {
  while (espSerial.available()) {
    char inChar = espSerial.read();
    
    // Debug: Show received character
    Serial.print("Received char: '");
    Serial.print(inChar);
    Serial.print("' (ASCII: ");
    Serial.print((int)inChar);
    Serial.println(")");
    
    // Handle newline or carriage return as command terminator
    if (inChar == '\n' || inChar == '\r') {
      if (bufferIndex > 0) {
        inputBuffer[bufferIndex] = '\0'; // Null terminate
        Serial.print("Processing command: '");
        Serial.print(inputBuffer);
        Serial.println("'");
        processCommand(inputBuffer);
        bufferIndex = 0; // Reset buffer
      }
    }
    // Handle regular characters
    else if (bufferIndex < BUFFER_SIZE - 1) {
      inputBuffer[bufferIndex++] = inChar;
      Serial.print("Buffer now: '");
      for (int i = 0; i < bufferIndex; i++) {
        Serial.print(inputBuffer[i]);
      }
      Serial.println("'");
    }
    // Buffer overflow protection
    else {
      Serial.println("ERROR: Command too long");
      bufferIndex = 0;
    }
  }
}

void processCommand(char* command) {
  // Convert to uppercase for case-insensitive commands
  String cmd = String(command);
  cmd.toUpperCase();
  
  Serial.println("=== COMMAND PROCESSING ===");
  Serial.print("Original: '");
  Serial.print(command);
  Serial.println("'");
  Serial.print("Uppercase: '");
  Serial.print(cmd);
  Serial.println("'");
  
  // Handle special commands
  if (cmd == "PING") {
    Serial.println("Responding to PING with PONG");
    Serial.println("PONG");
    return;
  }
  
  if (cmd == "STATUS") {
    Serial.println("Processing STATUS command");
    printStatus();
    return;
  }
  
  if (cmd == "RESET") {
    Serial.println("Processing RESET command");
    resetAllPins();
    Serial.println("OK: All pins reset");
    return;
  }
  
  // Parse pin:value command
  int colonIndex = cmd.indexOf(':');
  if (colonIndex == -1) {
    Serial.println("ERROR: Invalid format. Use pin:value");
    return;
  }
  
  String pinStr = cmd.substring(0, colonIndex);
  String valueStr = cmd.substring(colonIndex + 1);
  
  Serial.print("Pin string: '");
  Serial.print(pinStr);
  Serial.print("', Value string: '");
  Serial.print(valueStr);
  Serial.println("'");
  
  // Parse pin number (handle analog pins like A0, A1, etc.)
  int pinNumber;
  bool isAnalogPin = false;
  
  if (pinStr.startsWith("A")) {
    // Analog pin (A0 = 14, A1 = 15, etc. on Nano)
    int analogNum = pinStr.substring(1).toInt();
    if (analogNum >= 0 && analogNum <= 7) {
      pinNumber = A0 + analogNum;
      isAnalogPin = true;
    } else {
      Serial.println("ERROR: Invalid analog pin");
      return;
    }
  } else {
    // Digital pin
    pinNumber = pinStr.toInt();
    if (pinNumber < 0 || pinNumber > 19) {
      Serial.println("ERROR: Invalid digital pin");
      return;
    }
  }
  
  // Parse value
  int value = valueStr.toInt();
  if (value < 0 || value > 255) {
    Serial.print("ERROR: Value must be 0-255, got: ");
    Serial.println(value);
    return;
  }
  
  Serial.print("Parsed - Pin: ");
  Serial.print(pinNumber);
  Serial.print(", Value: ");
  Serial.print(value);
  Serial.print(", IsAnalog: ");
  Serial.println(isAnalogPin ? "YES" : "NO");
  
  // Execute pin command
  setPinValue(pinNumber, value, isAnalogPin);
  Serial.println("=== COMMAND COMPLETE ===");
}

void setPinValue(int pin, int value, bool isAnalog) {
  Serial.print(">> setPinValue called: Pin ");
  Serial.print(pin);
  Serial.print(", Value ");
  Serial.print(value);
  Serial.print(", Analog: ");
  Serial.println(isAnalog ? "YES" : "NO");
  
  // Skip status LED pin to avoid conflicts
  if (pin == STATUS_LED) {
    Serial.println("ERROR: Pin 13 reserved for status LED");
    return;
  }
  
  // Determine if this is a digital or PWM operation
  bool isOutput = true;
  bool usePWM = (value > 1 && value < 255);
  
  Serial.print("Will use PWM: ");
  Serial.println(usePWM ? "YES" : "NO");
  
  // Configure pin mode if not already set
  updatePinState(pin, value, isAnalog, isOutput);
  
  // Set pin mode
  pinMode(pin, OUTPUT);
  Serial.print("Set pin ");
  Serial.print(pin);
  Serial.println(" to OUTPUT mode");
  
  // Set pin value
  if (usePWM && !isAnalog) {
    // PWM output for digital pins
    analogWrite(pin, value);
    Serial.print("SUCCESS: Pin ");
    Serial.print(pin);
    Serial.print(" PWM = ");
    Serial.println(value);
  } else {
    // Digital output
    int digitalValue = (value > 0) ? HIGH : LOW;
    digitalWrite(pin, digitalValue);
    Serial.print("SUCCESS: Pin ");
    Serial.print(pin);
    Serial.print(" = ");
    Serial.println(digitalValue == HIGH ? "HIGH" : "LOW");
  }
  
  // Brief flash of status LED to confirm command
  digitalWrite(STATUS_LED, HIGH);
  delay(50);
  digitalWrite(STATUS_LED, LOW);
  Serial.println("Status LED flashed - command executed!");
}

void updatePinState(int pin, int value, bool isAnalog, bool isOutput) {
  // Find existing pin state or create new one
  int stateIndex = -1;
  
  for (int i = 0; i < activePins; i++) {
    if (pinStates[i].pin == pin) {
      stateIndex = i;
      break;
    }
  }
  
  // Create new pin state if not found
  if (stateIndex == -1 && activePins < MAX_PINS) {
    stateIndex = activePins++;
  }
  
  // Update pin state
  if (stateIndex != -1) {
    pinStates[stateIndex].pin = pin;
    pinStates[stateIndex].value = value;
    pinStates[stateIndex].isAnalog = isAnalog;
    pinStates[stateIndex].isOutput = isOutput;
  }
}

void printStatus() {
  Serial.println("=== Arduino Nano Status ===");
  Serial.print("Active pins: ");
  Serial.println(activePins);
  
  for (int i = 0; i < activePins; i++) {
    Serial.print("Pin ");
    if (pinStates[i].pin >= A0) {
      Serial.print("A");
      Serial.print(pinStates[i].pin - A0);
    } else {
      Serial.print(pinStates[i].pin);
    }
    Serial.print(": ");
    Serial.print(pinStates[i].value);
    Serial.print(" (");
    Serial.print(pinStates[i].isOutput ? "OUT" : "IN");
    Serial.println(")");
  }
  Serial.println("========================");
}

void resetAllPins() {
  // Reset all tracked pins to LOW
  for (int i = 0; i < activePins; i++) {
    if (pinStates[i].pin != STATUS_LED) {
      digitalWrite(pinStates[i].pin, LOW);
      pinStates[i].value = 0;
    }
  }
  activePins = 0;
  
  // Clear pin states
  for (int i = 0; i < MAX_PINS; i++) {
    pinStates[i].pin = -1;
    pinStates[i].value = 0;
    pinStates[i].isAnalog = false;
    pinStates[i].isOutput = false;
  }
}