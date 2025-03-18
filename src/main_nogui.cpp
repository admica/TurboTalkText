#include "settings.h"
#include "logger.h"
#include "audio_manager.h"
#include "transcription.h"
#include "keyboard.h"
#include "mouse.h"
#include "hotkey.h"

#ifdef USE_OVERLAY_UI
#include "overlay_ui.h"
#endif

#include <windows.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <deque>
#include <string>
#include <regex>
#include <sstream>

// Application input modes
enum InputMode {
    TEXT_MODE,
    MOUSE_MODE
};

// Voice command state tracking
struct VoiceCommands {
    // No need for wake word state tracking anymore
    // We directly check for the wake word in each command
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

// Helper function to clean up transcription text
static std::string cleanTranscription(const std::string& text) {
    // Remove [BLANK_AUDIO] markers and other noise indicators
    std::string result = text;
    
    // Define regex patterns for various noise indicators
    std::regex noisePattern("\\s*\\[(BLANK_AUDIO|silence|keyboard|background|noise|typing|clicking|inaudible|music|sound|sounds).*?\\]\\s*");
    
    // Replace matches with a space
    result = std::regex_replace(result, noisePattern, " ");
    
    // Trim extra whitespace
    result = std::regex_replace(result, std::regex("\\s+"), " ");
    
    // Trim leading/trailing whitespace
    result = std::regex_replace(result, std::regex("^\\s+|\\s+$"), "");
    
    return result;
}

// Remove duplicate words at the boundary of two strings
static std::string mergeContinuousText(const std::string& previousText, const std::string& newText) {
    if (previousText.empty()) {
        return newText;
    }
    
    // Convert both texts to lowercase for comparison
    std::string prevLower = normalizeText(previousText);
    std::string newLower = normalizeText(newText);
    
    // Extract last few words from previous text (up to 8 words for better context)
    std::vector<std::string> prevWords;
    size_t wordCount = 0;
    
    // Tokenize previous text into words
    std::istringstream prevStream(prevLower);
    std::string word;
    std::vector<std::string> allPrevWords;
    
    while (prevStream >> word && wordCount < 20) { // Get up to 20 words
        allPrevWords.push_back(word);
        wordCount++;
    }
    
    // Get last 8 words or all available words if less than 8
    int wordsToUse = std::min(8, static_cast<int>(allPrevWords.size()));
    prevWords.assign(allPrevWords.end() - wordsToUse, allPrevWords.end());
    
    // Find the best overlap point
    size_t bestOverlapPos = 0;
    size_t bestOverlapLength = 0;
    size_t bestWordCount = 0;
    
    // Try different sequence lengths
    for (size_t startIdx = 0; startIdx < prevWords.size(); startIdx++) {
        std::string sequence;
        
        for (size_t i = startIdx; i < prevWords.size(); i++) {
            if (!sequence.empty()) {
                sequence += " ";
            }
            sequence += prevWords[i];
            
            // Check if this sequence is at the start of the new text
            size_t pos = newLower.find(sequence);
            if (pos == 0 && sequence.length() > bestOverlapLength) {
                bestOverlapLength = sequence.length();
                bestWordCount = i - startIdx + 1;
                
                // Find the position after the last matched word
                size_t spacePos = newLower.find(' ', bestOverlapLength);
                bestOverlapPos = (spacePos != std::string::npos) ? spacePos + 1 : newLower.length();
            }
        }
    }
    
    // Preserve proper capitalization and punctuation
    if (bestOverlapLength > 0 && bestOverlapPos > 0) {
        // Use the new text after the overlap to maintain original capitalization
        std::string mergedText = previousText;
        
        // If the last character is not punctuation or space, add a space
        if (!mergedText.empty() && !std::ispunct(mergedText.back()) && !std::isspace(mergedText.back())) {
            mergedText += " ";
        }
        
        // Add remaining text from new chunk
        if (bestOverlapPos < newText.length()) {
            mergedText += newText.substr(bestOverlapPos);
        }
        
        // Ensure proper spacing after punctuation
        std::regex multipleSpaces("\\s+");
        mergedText = std::regex_replace(mergedText, multipleSpaces, " ");
        
        return mergedText;
    }
    
    // No overlap found, just append with a space or punctuation-aware join
    if (!previousText.empty() && !std::ispunct(previousText.back()) && !std::isspace(previousText.back())) {
        // If the new text starts with lowercase, just add a space
        if (!newText.empty() && std::islower(newText[0])) {
            return previousText + " " + newText;
        }
        // If new text starts with uppercase, it might be a new sentence
        else if (!newText.empty() && std::isupper(newText[0])) {
            // Add period if not already ending with punctuation
            return previousText + ". " + newText;
        } else {
            return previousText + " " + newText;
        }
    } else {
        // Previous text already ends with punctuation or space
        return previousText + newText;
    }
}

// Check if text contains the wake word
static bool containsWakeWord(const std::string& text, const Settings& settings) {
    // Simple implementation: check if text starts with "jarvis"
    return text.find("jarvis") != std::string::npos;
}

bool processText(const std::string& text, VoiceCommands& voiceCommands, Mouse& mouse, Keyboard& keyboard, Settings& settings) {
    Logger::info("Processing text: " + text);

    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(),
        [](unsigned char c) { return std::tolower(c); });

    // Check if this contains the wake word
    if (!containsWakeWord(lowerText, settings)) {
        // No wake word, so don't process as a command
        return false;
    }
    
    Logger::info("Wake word detected in: '" + text + "', processing command...");

    // Check for key press commands
    if (containsAnyCommand(lowerText, settings.commands.keyPress) ||
        lowerText.find("push ") != std::string::npos || 
        lowerText.find("press ") != std::string::npos || 
        lowerText.find("key ") != std::string::npos) {
        
        if (keyboard.processKeyCommand(lowerText)) {
            Logger::info("Executed key press command: " + text);
            return true;
        }
    }

    // Process other commands based on the current mode
    std::string normalizedText = normalizeText(lowerText);
    
    // Prepare references to command lists from settings
    const std::vector<std::string>& MOUSE_MODE_COMMANDS = settings.commands.mouseMode;
    const std::vector<std::string>& TEXT_MODE_COMMANDS = settings.commands.textMode;
    const std::vector<std::string>& CONTINUOUS_MODE_COMMANDS = settings.commands.continuousMode;
    const std::vector<std::string>& EXIT_CONTINUOUS_MODE_COMMANDS = settings.commands.exitContinuousMode;
    
    // Check for exit command
    if (normalizedText.find("exit") != std::string::npos || 
        normalizedText.find("quit") != std::string::npos ||
        normalizedText.find("stop listening") != std::string::npos) {
        std::cout << "Exit command recognized." << std::endl;
        return true;
    }
    
    // Important: Return false to let the main loop check for mode switch commands
    // This is critical for commands like "jarvis continuous mode" to work
    return false;
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

#ifdef USE_OVERLAY_UI
    // Initialize overlay UI if enabled in settings
    OverlayUI& overlayUI = OverlayUI::getInstance();
    if (settings.ui.enabled) {
        Logger::info("Initializing overlay UI");
        if (!overlayUI.initialize(GetModuleHandle(NULL), settings)) {
            Logger::error("Failed to initialize overlay UI");
            // Continue without UI
        }
    }
#endif

    // Current input mode and continuous mode status
    InputMode currentInputMode = TEXT_MODE;
    bool continuousModeActive = false;
    
    // Track speech detection state for UI and logging
    SpeechState previousSpeechState = SpeechState::SILENCE;
    
    // Voice commands state tracking
    VoiceCommands voiceCommands;
    
    // Buffer for continuous mode
    std::string continuousTextBuffer;
    
    // Use commands from settings
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
    if (!EXIT_CONTINUOUS_MODE_COMMANDS.empty()) {
        Logger::info("Say '" + EXIT_CONTINUOUS_MODE_COMMANDS[0] + "' to exit continuous listening mode");
    }
    Logger::info("In mouse mode, say 'up', 'down', 'left', 'right', 'click', etc.");
    
    // Log speech detection status
    if (settings.speechDetection.enabled) {
        Logger::info("Speech-aware chunking enabled for continuous mode");
        Logger::info("Speech threshold: " + std::to_string(settings.speechDetection.threshold));
        Logger::info("Min silence duration: " + std::to_string(settings.speechDetection.minSilenceMs) + "ms");
        Logger::info("Max chunk duration: " + std::to_string(settings.speechDetection.maxChunkSec) + "s");
    } else {
        Logger::info("Using fixed-size chunking for continuous mode");
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

        // Get audio level for UI
        float audioLevel = 0.0f;
        if (audioManager.isRecording()) {
            audioLevel = audioManager.getCurrentAudioLevel();
        }

#ifdef USE_OVERLAY_UI
        // Update overlay UI if enabled
        if (settings.ui.enabled) {
            overlayUI.update(audioManager.isRecording(), continuousModeActive, 
                            currentInputMode == MOUSE_MODE, audioLevel);
        }
#endif

        // Track speech state changes for logging
        if (continuousModeActive && settings.speechDetection.enabled) {
            SpeechState currentSpeechState = audioManager.getSpeechState();
            
            // Log state transitions for debugging
            if (currentSpeechState != previousSpeechState) {
                if (currentSpeechState == SpeechState::SPEAKING) {
                    Logger::info("Speech detection: Started speaking");
                } else if (currentSpeechState == SpeechState::SILENCE && previousSpeechState == SpeechState::SPEAKING) {
                    Logger::info("Speech detection: Stopped speaking");
                }
                previousSpeechState = currentSpeechState;
            }
        }

        // Check for exit hotkey press
        if (hotkey.isExitHotkeyPressed()) {
            Logger::info("Exit hotkey pressed: Shutting down application");
            running = false;
            hotkey.resetExitHotkeyPressed();
            continue;
        }

        // Check for hotkey press - this toggles recording
        if (hotkey.isHotkeyPressed()) {
            if (audioManager.isRecording()) {
                Logger::info("Hotkey pressed: STOP recording");
                audioManager.stopRecording();
                
                // If we were in continuous mode, just exit the continuous mode
                if (continuousModeActive) {
                    continuousModeActive = false;
                    audioManager.setContinuousMode(false);
                    continuousTextBuffer.clear();
                    Logger::info("Exited CONTINUOUS MODE");
                } else {
                    // Normal transcription for regular recording
                    Logger::info("Transcribing audio");
                    std::string transcribedText = transcription.transcribe(audioManager.getAudioData());
                    
                    // Clean the transcription text
                    transcribedText = cleanTranscription(transcribedText);
                    
                    Logger::info("Transcription complete: \"" + transcribedText + "\"");
                    
                    // First check for key press commands
                    bool isCommand = processText(transcribedText, voiceCommands, mouse, keyboard, settings);
                    if (isCommand) {
                        // Command was processed, continue to the next loop iteration
                        hotkey.resetHotkeyPressed();
                        continue;
                    }
                    
                    // If processText returns false, it might still be a wake word command
                    // that needs to be processed for mode switching
                    std::string normalizedText = normalizeText(transcribedText);
                    
                    // Check for mode switch commands
                    if (containsAnyCommand(normalizedText, CONTINUOUS_MODE_COMMANDS)) {
                        // Enable continuous mode
                        continuousModeActive = true;
                        audioManager.startRecording();
                        audioManager.setContinuousMode(true);
                        continuousTextBuffer.clear();
                        Logger::info("Enabled CONTINUOUS MODE (current input: " + 
                                    std::string(currentInputMode == TEXT_MODE ? "TEXT" : "MOUSE") + ")");
                    } else if (containsAnyCommand(normalizedText, MOUSE_MODE_COMMANDS)) {
                        // Switch to mouse mode
                        currentInputMode = MOUSE_MODE;
                        Logger::info("Switched to MOUSE MODE");
                    } else if (containsAnyCommand(normalizedText, TEXT_MODE_COMMANDS)) {
                        // Switch to text mode
                        currentInputMode = TEXT_MODE;
                        Logger::info("Switched to TEXT MODE");
                    } else {
                        // Process based on current input mode
                        if (currentInputMode == TEXT_MODE) {
                            // Normal text input
                            keyboard.typeText(transcribedText);
                        } else { // MOUSE_MODE
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

        // Check for auto-stop due to silence (only in regular recording mode)
        if (audioManager.isRecording() && !continuousModeActive && audioManager.checkSilence()) {
            Logger::info("Silence detected while recording, STOP recording");
            audioManager.stopRecording();
            Logger::info("Transcribing audio");
            std::string transcribedText = transcription.transcribe(audioManager.getAudioData());
            
            // Clean the transcription text
            transcribedText = cleanTranscription(transcribedText);
            
            Logger::info("Transcription complete: \"" + transcribedText + "\"");
            
            // First check for key press commands
            bool isCommand = processText(transcribedText, voiceCommands, mouse, keyboard, settings);
            if (isCommand) {
                // Command was processed, continue to the next loop iteration
                continue;
            }
            
            // If processText returns false, it might still be a wake word command
            // that needs to be processed for mode switching
            std::string normalizedText = normalizeText(transcribedText);
            
            // Check for mode switch commands
            if (containsAnyCommand(normalizedText, CONTINUOUS_MODE_COMMANDS)) {
                // Enable continuous mode
                continuousModeActive = true;
                audioManager.startRecording();
                audioManager.setContinuousMode(true);
                continuousTextBuffer.clear();
                Logger::info("Enabled CONTINUOUS MODE (current input: " + 
                            std::string(currentInputMode == TEXT_MODE ? "TEXT" : "MOUSE") + ")");
            } else if (containsAnyCommand(normalizedText, MOUSE_MODE_COMMANDS)) {
                // Switch to mouse mode
                currentInputMode = MOUSE_MODE;
                Logger::info("Switched to MOUSE MODE");
            } else if (containsAnyCommand(normalizedText, TEXT_MODE_COMMANDS)) {
                // Switch to text mode
                currentInputMode = TEXT_MODE;
                Logger::info("Switched to TEXT MODE");
            } else {
                // Process based on current input mode
                if (currentInputMode == TEXT_MODE) {
                    // Normal text input
                    keyboard.typeText(transcribedText);
                } else { // MOUSE_MODE
                    // Process as mouse command
                    if (!mouse.processCommand(transcribedText)) {
                        Logger::info("Unrecognized mouse command: " + transcribedText);
                    }
                }
            }
        }
        
        // Handle continuous mode processing
        if (continuousModeActive && audioManager.isRecording()) {
            // Check if we have a new chunk of audio to process
            if (audioManager.hasNewContinuousAudio()) {
                std::vector<float> audioChunk = audioManager.getContinuousAudioChunk();
                
                if (!audioChunk.empty()) {
                    Logger::info("Processing continuous audio chunk");
                    std::string transcribedChunk = transcription.transcribe(audioChunk);
                    
                    if (!transcribedChunk.empty()) {
                        // Clean the transcription text
                        transcribedChunk = cleanTranscription(transcribedChunk);
                        
                        Logger::info("Continuous chunk transcribed: \"" + transcribedChunk + "\"");
                        
                        // First check for key press commands
                        bool isCommand = processText(transcribedChunk, voiceCommands, mouse, keyboard, settings);
                        if (isCommand) {
                            // Command was processed, continue to the next loop iteration
                            continue;
                        }
                        
                        // If processText returns false, it might still be a wake word command
                        // that needs to be processed for mode switching
                        std::string normalizedChunk = normalizeText(transcribedChunk);
                        
                        // Check for exit continuous mode command
                        if (containsAnyCommand(normalizedChunk, EXIT_CONTINUOUS_MODE_COMMANDS)) {
                            Logger::info("Exiting continuous mode");
                            continuousModeActive = false;
                            audioManager.stopRecording();
                            audioManager.setContinuousMode(false);
                            continuousTextBuffer.clear();
                        } 
                        // Check for mode switch commands
                        else if (containsAnyCommand(normalizedChunk, MOUSE_MODE_COMMANDS)) {
                            // Check if we need to type out accumulated text before switching modes
                            bool wasInTextMode = (currentInputMode == TEXT_MODE);
                            
                            // Switch to mouse mode but stay in continuous mode
                            currentInputMode = MOUSE_MODE;
                            Logger::info("Switched to MOUSE MODE (continuous listening active)");
                            
                            // Type any accumulated text before switching to mouse mode
                            if (wasInTextMode && !continuousTextBuffer.empty()) {
                                keyboard.typeText(continuousTextBuffer);
                                continuousTextBuffer.clear();
                            }
                        } 
                        else if (containsAnyCommand(normalizedChunk, TEXT_MODE_COMMANDS)) {
                            // Switch to text mode but stay in continuous mode
                            currentInputMode = TEXT_MODE;
                            Logger::info("Switched to TEXT MODE (continuous listening active)");
                        }
                        // Process based on current input mode
                        else if (currentInputMode == TEXT_MODE) {
                            // In text mode, accumulate text
                            continuousTextBuffer = mergeContinuousText(continuousTextBuffer, transcribedChunk);
                            
                            // Type when enough text has accumulated
                            if (continuousTextBuffer.length() > 150) {
                                Logger::info("Typing accumulated text: \"" + continuousTextBuffer + "\"");
                                keyboard.typeText(continuousTextBuffer);
                                continuousTextBuffer.clear();
                            }
                        } 
                        else { // MOUSE_MODE
                            // In mouse mode, process as mouse command
                            if (!mouse.processCommand(transcribedChunk)) {
                                Logger::info("Unrecognized mouse command: " + transcribedChunk);
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

#ifdef USE_OVERLAY_UI
    // Shutdown overlay UI
    if (settings.ui.enabled) {
        overlayUI.shutdown();
    }
#endif

    SDL_Quit();
    return 0;
}
