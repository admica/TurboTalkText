#include "settings.h"
#include "logger.h"
#include "audio_manager.h"
#include "transcription.h"
#include "keyboard.h"
#include "mouse.h"
#include "hotkey.h"

#include <windows.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <deque>
#include <string>

// Application modes
enum InputMode {
    TEXT_MODE,
    MOUSE_MODE
};

// Helper function to normalize text for command matching (static to limit scope to this file)
static std::string normalizeText(const std::string& input) {
    // Convert to lowercase
    std::string result;
    result.resize(input.size());
    std::transform(input.begin(), input.end(), result.begin(), 
                  [](unsigned char c){ return std::tolower(c); });
    
    // Remove punctuation
    result.erase(std::remove_if(result.begin(), result.end(), 
                 [](unsigned char c){ return std::ispunct(c); }), result.end());
                 
    return result;
}

// Check if text contains any command from a list of possible commands
static bool containsAnyCommand(const std::string& text, const std::vector<std::string>& commands) {
    for (const auto& cmd : commands) {
        if (text.find(cmd) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Remove duplicate words at the boundary of two strings
static std::string mergeContinuousText(const std::string& previousText, const std::string& newText) {
    if (previousText.empty()) {
        return newText;
    }
    
    // Convert both texts to lowercase for comparison
    std::string prevLower = normalizeText(previousText);
    std::string newLower = normalizeText(newText);
    
    // Extract last few words from previous text (up to 5 words)
    std::vector<std::string> prevWords;
    size_t start = 0;
    size_t wordCount = 0;
    
    // Find the last 5 words of previous text
    for (size_t i = prevLower.size(); i > 0 && wordCount < 5; i--) {
        if (i == 1 || std::isspace(prevLower[i-1])) {
            std::string word = prevLower.substr(i, prevLower.find_first_of(" \t\n", i) - i);
            if (!word.empty()) {
                prevWords.insert(prevWords.begin(), word);
                wordCount++;
            }
        }
    }
    
    // Find the best overlap point
    size_t bestOverlapPos = 0;
    size_t bestOverlapLength = 0;
    
    for (size_t i = 0; i < prevWords.size(); i++) {
        // Try to match a sequence of words
        std::string sequence;
        for (size_t j = i; j < prevWords.size(); j++) {
            if (!sequence.empty()) {
                sequence += " ";
            }
            sequence += prevWords[j];
            
            // Check if this sequence is at the start of the new text
            if (newLower.find(sequence) == 0 && sequence.length() > bestOverlapLength) {
                bestOverlapLength = sequence.length();
                bestOverlapPos = newLower.find_first_of(" \t\n", bestOverlapLength) + 1;
            }
        }
    }
    
    // If we found an overlap, merge the texts
    if (bestOverlapLength > 0 && bestOverlapPos > 0) {
        return previousText + " " + newText.substr(bestOverlapPos);
    }
    
    // No overlap found, just append with a space
    return previousText + " " + newText;
}

int main() {
    // Initialize logger
    Logger::init();

    // Load settings
    Settings settings;
    // Try to load settings.json from current directory first
    if (!settings.load("settings.json")) {
        // If that fails, try to load from executable directory
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir = exePath;
        size_t lastSlash = exeDir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            exeDir = exeDir.substr(0, lastSlash + 1);
            if (!settings.load(exeDir + "settings.json")) {
                Logger::error("Failed to load settings.json from current directory or executable directory");
                return 1;
            }
            Logger::info("Loaded settings.json from executable directory");
        } else {
            Logger::error("Failed to load settings.json");
            return 1;
        }
    } else {
        Logger::info("Loaded settings.json from current directory");
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

    // Initialize mouse controller
    Mouse mouse;

    // Initialize hotkey manager
    Hotkey hotkey;
    if (!hotkey.registerHotkey()) {
        Logger::error("Failed to register hotkey");
        SDL_Quit();
        return 1;
    }

    // Current application mode
    InputMode currentMode = TEXT_MODE;
    
    // Buffer for continuous mode
    std::string continuousTextBuffer;
    
    // Use commands from settings instead of hardcoded values
    const std::vector<std::string>& MOUSE_MODE_COMMANDS = settings.commands.mouseMode;
    const std::vector<std::string>& TEXT_MODE_COMMANDS = settings.commands.textMode;
    const std::vector<std::string>& CONTINUOUS_MODE_COMMANDS = settings.commands.continuousMode;
    const std::vector<std::string>& EXIT_CONTINUOUS_MODE_COMMANDS = settings.commands.exitContinuousMode;

    Logger::info("TurboTalkText started");
    Logger::info("Press Ctrl+Shift+A to toggle recording");
    Logger::info("Press Ctrl+Shift+CapsLock to exit");
    
    // Log some example voice commands for each mode from settings
    if (!MOUSE_MODE_COMMANDS.empty()) {
        Logger::info("Say '" + MOUSE_MODE_COMMANDS[0] + "' to enter mouse control mode");
    }
    if (!CONTINUOUS_MODE_COMMANDS.empty()) {
        Logger::info("Say '" + CONTINUOUS_MODE_COMMANDS[0] + "' to enter continuous listening mode");
    }
    if (!TEXT_MODE_COMMANDS.empty()) {
        Logger::info("Say '" + TEXT_MODE_COMMANDS[0] + "' to return to text mode");
    }
    Logger::info("In mouse mode, say 'up', 'down', 'left', 'right', 'click', etc.");

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

        // Check for hotkey press - this toggles recording regardless of mode
        if (hotkey.isHotkeyPressed()) {
            if (audioManager.isRecording()) {
                Logger::info("Hotkey pressed: STOP recording");
                audioManager.stopRecording();
                
                // If we were in continuous mode, reset the buffer
                if (currentMode == TEXT_MODE) {
                    continuousTextBuffer.clear();
                    currentMode = MOUSE_MODE;
                    Logger::info("Exited TEXT MODE");
                } else {
                    // Normal transcription for other modes
                    Logger::info("Transcribing audio");
                    std::string transcribedText = transcription.transcribe(audioManager.getAudioData());
                    Logger::info("Transcription complete: \"" + transcribedText + "\"");
                    
                    // Process the transcribed text based on current mode
                    std::string normalizedText = normalizeText(transcribedText);
                    
                    if (currentMode == TEXT_MODE) {
                        // Check for mode switch commands
                        if (containsAnyCommand(normalizedText, MOUSE_MODE_COMMANDS)) {
                            currentMode = MOUSE_MODE;
                            Logger::info("Switched to MOUSE MODE");
                        } else if (containsAnyCommand(normalizedText, TEXT_MODE_COMMANDS)) {
                            currentMode = TEXT_MODE;
                            continuousTextBuffer.clear();
                            audioManager.startRecording();
                            Logger::info("Switched to TEXT MODE");
                        } else {
                            // Normal text input
                            keyboard.typeText(transcribedText);
                        }
                    } else if (currentMode == MOUSE_MODE) {
                        // Check for exit command
                        if (containsAnyCommand(normalizedText, TEXT_MODE_COMMANDS)) {
                            currentMode = TEXT_MODE;
                            Logger::info("Switched to TEXT MODE");
                        } else {
                            // Process as mouse command
                            if (!mouse.processCommand(transcribedText)) {
                                Logger::info("Unrecognized mouse command: " + transcribedText);
                            }
                        }
                    }
                }
            } else {
                Logger::info("Hotkey pressed: START recording");
                audioManager.startRecording();
            }
            hotkey.resetHotkeyPressed();
        }

        // Check for auto-stop due to silence (only in regular recording modes)
        if (audioManager.isRecording() && currentMode != TEXT_MODE && audioManager.checkSilence()) {
            Logger::info("Silence detected while recording, STOP recording");
            audioManager.stopRecording();
            Logger::info("Transcribing audio");
            std::string transcribedText = transcription.transcribe(audioManager.getAudioData());
            Logger::info("Transcription complete: \"" + transcribedText + "\"");
            
            // Process the transcribed text based on current mode
            std::string normalizedText = normalizeText(transcribedText);
            
            if (currentMode == TEXT_MODE) {
                // Check for mode switch commands
                if (containsAnyCommand(normalizedText, MOUSE_MODE_COMMANDS)) {
                    currentMode = MOUSE_MODE;
                    Logger::info("Switched to MOUSE MODE");
                } else if (containsAnyCommand(normalizedText, TEXT_MODE_COMMANDS)) {
                    currentMode = TEXT_MODE;
                    continuousTextBuffer.clear();
                    audioManager.startRecording();
                    Logger::info("Switched to TEXT MODE");
                } else {
                    // Normal text input
                    keyboard.typeText(transcribedText);
                }
            } else if (currentMode == MOUSE_MODE) {
                // Check for exit command
                if (containsAnyCommand(normalizedText, TEXT_MODE_COMMANDS)) {
                    currentMode = TEXT_MODE;
                    Logger::info("Switched to TEXT MODE");
                } else {
                    // Process as mouse command
                    if (!mouse.processCommand(transcribedText)) {
                        Logger::info("Unrecognized mouse command: " + transcribedText);
                    }
                }
            }
        }
        
        // Handle continuous mode processing
        if (currentMode == TEXT_MODE && audioManager.isRecording()) {
            // Check if we have a new chunk of audio to process
            if (audioManager.hasNewContinuousAudio()) {
                std::vector<float> audioChunk = audioManager.getContinuousAudioChunk();
                
                if (!audioChunk.empty()) {
                    Logger::info("Processing continuous audio chunk");
                    std::string transcribedChunk = transcription.transcribe(audioChunk);
                    
                    if (!transcribedChunk.empty()) {
                        Logger::info("Continuous chunk transcribed: \"" + transcribedChunk + "\"");
                        
                        // Check for exit commands
                        std::string normalizedChunk = normalizeText(transcribedChunk);
                        
                        if (containsAnyCommand(normalizedChunk, EXIT_CONTINUOUS_MODE_COMMANDS)) {
                            Logger::info("Exiting continuous mode");
                            currentMode = TEXT_MODE;
                            audioManager.stopRecording();
                            continuousTextBuffer.clear();
                        } else if (containsAnyCommand(normalizedChunk, TEXT_MODE_COMMANDS)) {
                            // Switch to text mode but keep continuous mode active
                            Logger::info("Command detected but continuing in continuous mode");
                            
                            // Type the accumulated text so far
                            if (!continuousTextBuffer.empty()) {
                                keyboard.typeText(continuousTextBuffer);
                                continuousTextBuffer.clear();
                            }
                        } else {
                            // Merge with previous text, avoiding duplicates at boundaries
                            continuousTextBuffer = mergeContinuousText(continuousTextBuffer, transcribedChunk);
                            
                            // Check if enough text has accumulated to type
                            if (continuousTextBuffer.length() > 100) {
                                Logger::info("Typing accumulated text: \"" + continuousTextBuffer + "\"");
                                keyboard.typeText(continuousTextBuffer);
                                continuousTextBuffer.clear();
                            }
                        }
                    }
                }
            }
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
