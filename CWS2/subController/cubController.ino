/*
 * Arduino Nano Sub-Controller for ESP32 Camera Robot
 * Receives pin:value commands via SPI and controls local pins
 * Communication format: pin:value (e.g., "13:1", "A0:255", "2:0")
 * 
 * SPI Connections:
 * ESP32 -> Arduino Nano
 * MOSI (GPIO 23) -> Pin 11 (MOSI)
 * MISO (GPIO 19) -> Pin 12 (MISO) 
 * SCK  (GPIO 18) -> Pin 13 (SCK)
 * CS   (GPIO 5)  -> Pin 10 (SS)
 * GND  -> GND
 */

#include <SPI.h>

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

// SPI Communication settings
#define BUFFER_SIZE 32
char spiBuffer[BUFFER_SIZE];
volatile bool dataReceived = false;
volatile int bufferIndex = 0;

// Status LED (built-in LED on pin 9, since pin 13 is used for SPI SCK)
#define STATUS_LED 9
unsigned long lastHeartbeat = 0;
bool ledState = false;

void setup() {
  Serial.begin(115200); // For debugging output
  
  // Initialize SPI as slave
  pinMode(MISO, OUTPUT); // Set MISO as output
  SPCR |= _BV(SPE);      // Enable SPI in slave mode
  SPCR |= _BV(SPIE);     // Enable SPI interrupt
  
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
  
  Serial.println("Arduino Nano SPI Sub-Controller Ready");
  Serial.println("Waiting for SPI commands from ESP32...");
}

// SPI interrupt service routine
ISR(SPI_STC_vect) {
  byte receivedByte = SPDR;
  
  // Handle command terminator (newline or null)
  if (receivedByte == '\n' || receivedByte == '\0' || receivedByte == '\r') {
    if (bufferIndex > 0) {
      spiBuffer[bufferIndex] = '\0';
      dataReceived = true;
    }
  }
  // Handle regular characters
  else if (bufferIndex < BUFFER_SIZE - 1) {
    spiBuffer[bufferIndex++] = receivedByte;
  }
  // Buffer overflow protection
  else {
    bufferIndex = 0; // Reset on overflow
  }
  
  // Echo back for confirmation (optional)
  SPDR = receivedByte;
}

void loop() {
  // Handle received SPI data
  if (dataReceived) {
    processCommand(spiBuffer);
    dataReceived = false;
    bufferIndex = 0;
  }
  
  // Heartbeat LED (blink every 2 seconds when idle)
  if (millis() - lastHeartbeat > 2000) {
    ledState = !ledState;
    digitalWrite(STATUS_LED, ledState);
    lastHeartbeat = millis();
  }
  
  delay(10); // Small delay to prevent overwhelming the processor
}

void handleSerialInput() {
  // This function is no longer needed for SPI communication
  // Kept for potential debugging via Serial if needed
}

void processCommand(char* command) {
  // Convert to uppercase for case-insensitive commands
  String cmd = String(command);
  cmd.toUpperCase();
  
  // Handle special commands
  if (cmd == "PING") {
    Serial.println("PONG");
    return;
  }
  
  if (cmd == "STATUS") {
    printStatus();
    return;
  }
  
  if (cmd == "RESET") {
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
    Serial.println("ERROR: Value must be 0-255");
    return;
  }
  
  // Execute pin command
  setPinValue(pinNumber, value, isAnalogPin);
}

void setPinValue(int pin, int value, bool isAnalog) {
  // Skip SPI pins to avoid conflicts
  if (pin == 10 || pin == 11 || pin == 12 || pin == 13) {
    Serial.println("ERROR: SPI pins (10-13) are reserved");
    return;
  }
  
  // Skip status LED pin to avoid conflicts
  if (pin == STATUS_LED) {
    Serial.println("ERROR: Pin 9 reserved for status LED");
    return;
  }
  
  // Determine if this is a digital or PWM operation
  bool isOutput = true;
  bool usePWM = (value > 1 && value < 255);
  
  // Configure pin mode if not already set
  updatePinState(pin, value, isAnalog, isOutput);
  
  // Set pin mode
  pinMode(pin, OUTPUT);
  
  // Set pin value
  if (usePWM && !isAnalog) {
    // PWM output for digital pins
    analogWrite(pin, value);
    Serial.print("OK: Pin ");
    Serial.print(pin);
    Serial.print(" PWM = ");
    Serial.println(value);
  } else {
    // Digital output
    int digitalValue = (value > 0) ? HIGH : LOW;
    digitalWrite(pin, digitalValue);
    Serial.print("OK: Pin ");
    Serial.print(pin);
    Serial.print(" = ");
    Serial.println(digitalValue == HIGH ? "HIGH" : "LOW");
  }
  
  // Brief flash of status LED to confirm command
  digitalWrite(STATUS_LED, HIGH);
  delay(50);
  digitalWrite(STATUS_LED, LOW);
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