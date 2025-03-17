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

    Logger::info("TurboTalkText started");
    Logger::info("Press Ctrl+Shift+A to toggle recording");
    Logger::info("Press Ctrl+Shift+CapsLock to exit");

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

        // Check for exit hotkey press
        if (hotkey.isExitHotkeyPressed()) {
            Logger::info("Exit hotkey pressed: Shutting down application");
            running = false;
            hotkey.resetExitHotkeyPressed();
            continue;
        }

        // Check for hotkey press
        if (hotkey.isHotkeyPressed()) {
            if (audioManager.isRecording()) {
                Logger::info("Hotkey pressed: STOP recording");
                audioManager.stopRecording();
                Logger::info("Transcribing audio");
                std::string transcribedText = transcription.transcribe(audioManager.getAudioData());
                Logger::info("Transcription complete");
                keyboard.typeText(transcribedText);
            } else {
                Logger::info("Hotkey pressed: START recording");
                audioManager.startRecording();
            }
            hotkey.resetHotkeyPressed();
        }

        // Check for auto-stop due to silence
        if (audioManager.isRecording() && audioManager.checkSilence()) {
            Logger::info("Silence detected while recording, STOP recording");
            audioManager.stopRecording();
            Logger::info("Transcribing audio");
            std::string transcribedText = transcription.transcribe(audioManager.getAudioData());
            Logger::info("Transcription complete");
            keyboard.typeText(transcribedText);
        }

        // Sleep to avoid high CPU usage
        Sleep(10);
    }
    Logger::info("No longer running, doing cleanup");

    // Cleanup
    hotkey.unregisterHotkey();
    SDL_Quit();

    return 0;
}
