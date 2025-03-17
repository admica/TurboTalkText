# TurboTalkText

A real-time speech-to-text application that transcribes your voice and types it automatically. Perfect for hands-free text input and mouse control in any application.

## Features

- **Real-time Speech Recognition**: Uses OpenAI's Whisper model for accurate speech recognition
- **Automatic Typing**: Simulates keyboard input to type transcribed text
- **Voice-Controlled Mouse**: Control mouse movement and clicks with voice commands
- **Keyboard Shortcuts**: Trigger keyboard shortcuts and key presses with voice commands
- **Precise Mouse Control**: Specify exact pixel distances (e.g., "right 250")
- **Continuous Listening Mode**: Transcribe speech continuously without needing to toggle recording
- **Dual-Mode Operation**: Switch between text input and mouse control seamlessly
- **Smart Silence Detection**: Automatically detects speech segments and processes them efficiently
- **Customizable Voice Commands**: Configure keyword phrases for all modes via settings.json
- **Configurable Settings**: Easy JSON-based configuration for all parameters
- **Multiple Audio Devices**: Supports selection of different audio input devices
- **Hotkey Controls**: Simple keyboard shortcuts for controlling recording

## Requirements

- Windows operating system
- SDL2 library
- Whisper GGML model file (`ggml-base.en.bin`)
- C++ compiler with C++17 support

## Installation

1. Clone the repository:
```bash
git clone https://github.com/admica/TurboTalkText.git
cd TurboTalkText
```

2. Build the application:
The bash script handles all downloading of dependencies automatically.
```bash
./build.sh
```

## Usage

1. Start the application
2. Use the following hotkeys:
   - `Ctrl+Shift+A`: Toggle recording on/off
   - `Ctrl+Shift+CapsLock`: Exit application

3. Speak clearly into your microphone
4. The application will automatically:
   - Detect when you're speaking
   - Process the speech after you pause
   - Type the transcribed text

### Voice Commands

#### Mode Switching
- Say **"Jarvis move the mouse"** to enter mouse control mode
- Say **"Jarvis listen continuously"** to enable continuous listening mode
- Say **"Jarvis stop"** to return to text input mode
- Say **"Jarvis stop listening"** to exit continuous mode

#### Mouse Control
- Basic directions: "up", "down", "left", "right"
- Precise movement: "right 120", "up 50", "left 200", etc.
- Mouse actions: "click", "right click", "double click"
- Speed adjustment: "faster", "slower"

#### Keyboard Shortcuts
- Press a single key: **"Jarvis press enter"**, **"Jarvis push space"**
- Press key combinations: **"Jarvis press control alt delete"**, **"Jarvis push control c"**
- Function keys: **"Jarvis press f5"**, **"Jarvis push control f4"**
- Special keys: **"Jarvis press escape"**, **"Jarvis push tab"**

### Operating Modes

#### Text Input Mode
The default mode. Speech is transcribed and typed into your active application.

#### Mouse Control Mode
Your voice commands move the cursor and perform clicks.

#### Continuous Listening Mode
Works with both text and mouse modes. Instead of toggling recording, it continuously listens and:
- Transcribes and types text when in text mode
- Follows mouse commands when in mouse mode
- Allows switching between text and mouse modes without exiting continuous listening

## Configuration

Edit `settings.json` to customize:

```json
{
    "audio": {
        "device": "default",
        "sample_rate": 16000,
        "silence_threshold": 0.011,
        "silence_duration_ms": 2000
    },
    "whisper": {
        "model_path": "ggml-base.en.bin",
        "language": "en",
        "translate": false,
        "beam_size": 5,
        "threads": 4
    },
    "output": {
        "type": "keyboard",
        "add_punctuation": true,
        "capitalize_sentences": true
    },
    "voice_commands": {
        "mouse_mode": [
            "jarvis move the mouse", 
            "mouse mode", 
            "move the mouse", 
            "control the mouse"
        ],
        "text_mode": [
            "jarvis stop", 
            "text mode", 
            "typing mode", 
            "keyboard mode"
        ],
        "continuous_mode": [
            "jarvis listen continuously", 
            "continuous mode", 
            "always listen", 
            "keep listening"
        ],
        "exit_continuous_mode": [
            "jarvis stop listening", 
            "stop continuous", 
            "exit continuous mode", 
            "back to normal"
        ]
    }
}
```

## Troubleshooting

### Audio Issues
- Ensure your microphone is properly connected and selected
- Check Windows sound settings for proper input levels
- Adjust `silence_threshold` if speech detection is too sensitive/insensitive

### Transcription Issues
- Verify the Whisper model file is present and correct
- Try adjusting `beam_size` for better accuracy
- Ensure you're speaking clearly and at a reasonable volume

### Mouse Control Issues
- Speak direction commands clearly and distinctly
- For numeric movements, make sure to clearly say both the direction and number
- If movements are too fast or slow, adjust with "faster" or "slower" commands

## Technical Details

- Audio Processing: SDL2 for real-time audio capture
- Speech Recognition: Whisper.cpp for efficient CPU-based inference
- Text & Input Simulation: Windows API for keyboard and mouse control
- Configuration: JSON-based settings management with customizable voice commands
- Continuous Processing: Implements overlapping audio segments with smart text merging

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

Apache-2.0 license

## Acknowledgments

- OpenAI for the Whisper model
- SDL team for the audio framework
- Whisper.cpp team for the efficient C++ implementation 
