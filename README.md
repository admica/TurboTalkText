# TurboTalkText

A real-time speech-to-text application that transcribes your voice and types it automatically. Perfect for hands-free text input in any application.

## Features

- **Real-time Speech Recognition**: Uses OpenAI's Whisper model for accurate speech recognition
- **Automatic Typing**: Simulates keyboard input to type transcribed text
- **Smart Silence Detection**: Automatically detects speech segments and processes them efficiently
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

3. Build the application:
The bash script handles all downloading of dependencies automatically.
```bash
./build.sh
```

## Usage

1. Start the application
2. Use the following hotkeys:
   - `Ctrl+Shift`: Toggle recording on/off
   - `Ctrl+Shift+CapsLock`: Exit application

3. Speak clearly into your microphone
4. The application will automatically:
   - Detect when you're speaking
   - Process the speech after you pause
   - Type the transcribed text

## Configuration

Edit `settings.json` to customize:

```json
{
    "audio": {
        "device": "Line In (High Definition Audio Device)",
        "sample_rate": 16000,
        "silence_threshold": 0.001000,
        "silence_duration_ms": 1000
    },
    "whisper": {
        "model_path": "ggml-base.en.bin",
        "language": "en",
        "translate": false,
        "beam_size": 10,
        "threads": 4
    },
    "output": {
        "type": "keyboard",
        "add_punctuation": true,
        "capitalize_sentences": true
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

## Technical Details

- Audio Processing: SDL2 for real-time audio capture
- Speech Recognition: Whisper.cpp for efficient CPU-based inference
- Text Output: Windows API for keyboard simulation
- Configuration: JSON-based settings management

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

Apache-2.0 license

## Acknowledgments

- OpenAI for the Whisper model
- SDL team for the audio framework
- Whisper.cpp team for the efficient C++ implementation 
