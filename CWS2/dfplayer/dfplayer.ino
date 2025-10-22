#include <SoftwareSerial.h>
#include <DFPlayerMini_Fast.h>

// Pin definitions for Arduino Nano
const int SS_RX = 3;  // Connect to DFPlayer TX
const int SS_TX = 4;  // Connect to DFPlayer RX
const int LED_PIN = 13;  // Built-in LED for visual feedback
const int BUTTON_PIN = 2;  // Button to trigger playback

// Create SoftwareSerial and DFPlayer objects
SoftwareSerial dfSerial(SS_RX, SS_TX);
DFPlayerMini_Fast dfPlayer;

// Test variables
bool audioPlaying = false;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;  // 200ms debounce

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("DFPlayer Mini Test - Single File");
  Serial.println("=================================");
  
  // Configure pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize DFPlayer communication
  dfSerial.begin(9600);
  
  // Wait for DFPlayer to initialize
  delay(500);
  
  // Initialize DFPlayer
  if (dfPlayer.begin(dfSerial)) {
    Serial.println("✓ DFPlayer Mini initialized successfully!");
    
    // Set volume to comfortable level
    dfPlayer.volume(20);  // Set volume (0-30)
    delay(100);
    
    Serial.println("✓ Ready to play audio file!");
    blinkLED(3, 200);  // Success indication
  } else {
    Serial.println("✗ Failed to initialize DFPlayer Mini!");
    Serial.println("Check connections and SD card");
    
    // Error indication - fast blinking
    while(true) {
      blinkLED(10, 50);
      delay(1000);
    }
  }
  
  Serial.println("\nControls:");
  Serial.println("Press button on pin 2 to play/stop");
  Serial.println("Send 'p' to play, 's' to stop");
  Serial.println();
}

void loop() {
  // Check for button press
  checkButton();
  
  // Check for serial commands
  checkSerialCommands();
  
  // Update LED status
  updateStatusLED();
  
  delay(10);  // Small delay to prevent excessive polling
}

void checkButton() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  // Check for button press (falling edge with debounce)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    unsigned long currentTime = millis();
    if (currentTime - lastButtonPress > debounceDelay) {
      lastButtonPress = currentTime;
      toggleAudio();
    }
  }
  
  lastButtonState = currentButtonState;
}

void checkSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "p") {
      playAudio();
    } else if (command == "s") {
      stopAudio();
    } else {
      Serial.println("Commands: 'p' to play, 's' to stop");
    }
  }
}

void toggleAudio() {
  if (audioPlaying) {
    stopAudio();
  } else {
    playAudio();
  }
}

void playAudio() {
  Serial.println("Playing audio file...");
  dfPlayer.play(1);  // Play the first (and only) track
  audioPlaying = true;
}

void stopAudio() {
  Serial.println("Stopping audio...");
  dfPlayer.stop();
  audioPlaying = false;
}

void increaseVolume() {
  static int currentVolume = 20;
  if (currentVolume < 30) {
    currentVolume++;
    dfPlayer.volume(currentVolume);
    Serial.print("Volume increased to: ");
    Serial.println(currentVolume);
  } else {
    Serial.println("Volume already at maximum (30)");
  }
}

void decreaseVolume() {
  static int currentVolume = 20;
  if (currentVolume > 0) {
    currentVolume--;
    dfPlayer.volume(currentVolume);
    Serial.print("Volume decreased to: ");
    Serial.println(currentVolume);
  } else {
    Serial.println("Volume already at minimum (0)");
  }
}

void updateStatusLED() {
  // LED on when audio is playing, off when stopped
  digitalWrite(LED_PIN, audioPlaying);
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}