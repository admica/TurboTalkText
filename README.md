TurboTalkText

TurboTalkText is a real-time speech-to-text application for Windows, built with SDL3 for audio input and Whisper.cpp for transcription. Toggle recording with Ctrl+Shift, and it types out what you say after a 2-second silence. Exit with Ctrl+Shift+CapsLock. Perfect for hands-free typing or accessibility.
Features
Real-Time Transcription: Converts speech to text as you speak, with output typed via Windows keyboard emulation.

Hotkey Control: Ctrl+Shift to start/stop recording, Ctrl+Shift+CapsLock to quit.

Lightweight: Uses Whisper.cpp’s base.en model for efficient English transcription.

Cross-Platform Build: Built on WSL Ubuntu for Windows using MinGW.

Prerequisites

WSL2 with Ubuntu (tested on 22.04).

Windows 10/11 (for running the .exe).

Tools: wget, cmake, make, mingw-w64 (installed automatically by build.sh if missing).

Installation

Clone the Repo:
git clone https://github.com/yourusername/TurboTalkText.git
cd TurboTalkText

Run the Build Script:
./build.sh
Downloads SDL3 (v3.2.8) and Whisper.cpp (v1.7.4).

Fetches the ggml-base.en.bin model.

Cross-compiles for Windows using MinGW.

Copies TurboTalkText.exe, SDL3.dll, and ggml-base.en.bin to your Windows Desktop (/mnt/c/Users/<YourUser>/Desktop).

Optional: Specify a custom output directory:
./build.sh /mnt/c/Users/YourUser/Path

