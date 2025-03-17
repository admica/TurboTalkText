#include "settings.h"
#include "logger.h"
#include "audio_manager.h"
#include "transcription.h"
#include "keyboard.h"
#include "hotkey.h"

#include <windows.h>
#include <iostream>

int main() {
    // Initialize logger
    Logger::init();

    // Load settings
    Settings settings;
    if (!settings.load("settings.json")) {
        Logger::error("Failed to load settings.json");
        return 1;
    }

    // Initialize SDL2 for audio
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        Logger::error("SDL_Init failed: " + std::string(SDL_GetError()));
        return 1;
    }

    // Initialize audio manager
    AudioManager audioManager(settings);
    if (!audioManager.init()) {
        Logger::error("AudioManager initialization failed");
        SDL_Quit();
        return 1;
    }

    // Initialize transcription engine
    Transcription transcription(settings);
    if (!transcription.init()) {
        Logger::error("Transcription initialization failed");
        SDL_Quit();
        return 1;
    }

    // Initialize keyboard simulator
    Keyboard keyboard;

    // Initialize hotkey manager
    Hotkey hotkey;
    if (!hotkey.registerHotkey()) {
        Logger::error("Failed to register hotkey");
        SDL_Quit();
        return 1;
    }

    // Main loop
    bool running = true;
    while (running) {
        // Process Windows messages
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Check for hotkey press
        if (hotkey.isHotkeyPressed()) {
            if (audioManager.isRecording()) {
                // Stop recording
                audioManager.stopRecording();
                std::string transcribedText = transcription.transcribe(audioManager.getAudioData());
                keyboard.typeText(transcribedText);
            } else {
                // Start recording
                audioManager.startRecording();
            }
            hotkey.resetHotkeyPressed();
        }

        // Check for auto-stop due to silence
        if (audioManager.isRecording() && audioManager.checkSilence()) {
            audioManager.stopRecording();
            std::string transcribedText = transcription.transcribe(audioManager.getAudioData());
            keyboard.typeText(transcribedText);
        }

        // Sleep to avoid high CPU usage
        Sleep(10);
    }

    // Cleanup
    hotkey.unregisterHotkey();
    SDL_Quit();

    return 0;
}
