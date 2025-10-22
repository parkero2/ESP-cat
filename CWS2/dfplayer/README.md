# DFPlayer Mini Test for Arduino Nano

This test program is designed to comprehensively test a DFPlayer Mini module with an Arduino Nano.

## Hardware Requirements

- Arduino Nano
- DFPlayer Mini module
- MicroSD card with at least one audio file (MP3 or WAV)
- Push button (optional, for manual control)
- Jumper wires
- Breadboard (optional)

## Wiring Connections

| DFPlayer Mini | Arduino Nano | Notes |
|---------------|--------------|-------|
| VCC | 5V | Power supply |
| GND | GND | Ground |
| TX | Pin 3 (SS_RX) | DFPlayer transmit to Arduino receive |
| RX | Pin 4 (SS_TX) | DFPlayer receive from Arduino transmit |
| SPK+ | Speaker + | Connect speaker |
| SPK- | Speaker - | Connect speaker |

**Optional Button:**
| Button | Arduino Nano |
|--------|--------------|
| One terminal | Pin 2 |
| Other terminal | GND |

## Library Requirements

Install the following library in Arduino IDE:
- **DFPlayerMini_Fast** by PowerBroker2

To install:
1. Open Arduino IDE
2. Go to Tools > Manage Libraries
3. Search for "DFPlayerMini_Fast"
4. Install the library by PowerBroker2

## SD Card Setup

1. Format a microSD card (FAT32 recommended)
2. Create a folder named `mp3` (optional but recommended)
3. Copy your audio file(s) to the root directory or `mp3` folder
4. Name the file `001.mp3` (or `001.wav`) for the first track
5. Insert the SD card into the DFPlayer Mini

## Test Features

### Automatic Tests
- ✅ DFPlayer initialization check
- ✅ SD card file detection
- ✅ Volume control verification
- ✅ Audio playback test

### Manual Controls
- **Button on Pin 2**: Play/Stop toggle
- **Serial Commands**:
  - `p` - Play audio
  - `s` - Stop audio
  - `v+` - Increase volume
  - `v-` - Decrease volume
  - `t` - Run full automated test

### Visual Feedback
- **Built-in LED (Pin 13)**:
  - 3 quick blinks: Successful initialization
  - 5 fast blinks: Error (no files found)
  - Continuous fast blinking: Initialization failed
  - Slow blinking: Audio is playing

## Usage Instructions

1. Upload the code to your Arduino Nano
2. Open Serial Monitor (9600 baud)
3. Observe the initialization messages
4. Use serial commands or button to control playback
5. Run full test with `t` command

## Troubleshooting

### "Failed to initialize DFPlayer Mini!"
- Check all wiring connections
- Ensure 5V power supply is adequate
- Verify DFPlayer Mini is functional

### "No audio files found on SD card!"
- Check SD card formatting (FAT32)
- Ensure audio file is named correctly (001.mp3)
- Try a different SD card
- Verify file format (MP3 or WAV)

### No audio output
- Check speaker connections
- Verify volume level (try `v+` command)
- Test with headphones instead of speaker
- Check audio file integrity

### Button not responding
- Verify button wiring
- Check for proper pull-up resistor (internal pull-up is enabled in code)
- Test with serial commands instead

## Expected Serial Output

```
DFPlayer Mini Test - Arduino Nano
==================================
✓ DFPlayer Mini initialized successfully!
Files found on SD card: 1
✓ Audio file detected!

Test Commands:
Press button on pin 2 to play/stop audio
Send 'p' via Serial Monitor to play
Send 's' via Serial Monitor to stop
Send 'v+' to increase volume
Send 'v-' to decrease volume
Send 't' to run full test
```

## Notes

- Default volume is set to 20 (out of 30)
- Button includes software debouncing (200ms)
- Audio file numbering starts from 001
- SoftwareSerial is used for DFPlayer communication
- Hardware serial (USB) is used for debugging/commands
