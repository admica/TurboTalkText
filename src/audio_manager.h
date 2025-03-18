#pragma once

#include "settings.h"
#include <SDL2/SDL.h>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <chrono>

// Speech detection states
enum class SpeechState {
    SILENCE,     // No speech detected
    SPEAKING,    // Active speech
    TRANSITION   // Transitioning between states
};

class AudioManager {
public:
    AudioManager(Settings& settings);
    ~AudioManager();
    
    bool init();
    void startRecording();
    void stopRecording();
    bool isRecording() const;
    std::vector<float> getAudioData() const;
    
    // Continuous mode methods
    void setContinuousMode(bool enabled);
    bool isContinuousMode() const;
    bool hasNewContinuousAudio();
    std::vector<float> getContinuousAudioChunk();
    void resetContinuousFlag();
    
    // Silence detection
    bool checkSilence();
    
    // Get current audio level (for UI visualization)
    float getCurrentAudioLevel() const;
    
    // Get current speech state
    SpeechState getSpeechState() const;

private:
    static void audioCallback(void* userdata, Uint8* stream, int len);
    void processAudioData(const Uint8* stream, int len);
    float calculateRMS(const float* samples, int sampleCount);
    
    // Speech detection for continuous mode
    bool detectSpeech(float soundLevel);
    void updateSpeechState(float soundLevel);
    void processSpeechBasedChunk();
    
    // Audio device and buffers
    SDL_AudioDeviceID deviceId = 0;
    std::vector<float> audioBuffer;
    std::vector<float> continuousBuffer;
    std::atomic<bool> recording{false};
    std::mutex audioMutex;
    
    // Continuous mode support
    std::atomic<bool> continuousMode{false};
    std::atomic<bool> newContinuousAudioAvailable{false};
    std::atomic<bool> newContinuousAudioReady{false};
    std::deque<std::vector<float>> continuousChunks;
    mutable std::mutex continuousMutex;
    int continuousSampleThreshold;
    std::chrono::steady_clock::time_point lastContinuousProcessTime;
    
    // Silence detection
    int silenceCounter = 0;
    int silenceChunks = 0;
    
    // Speech detection variables
    std::atomic<SpeechState> currentSpeechState{SpeechState::SILENCE};
    std::vector<float> preSpeechBuffer;
    std::vector<float> currentSpeechBuffer;
    int silenceFrameCount = 0;
    int speechFrameCount = 0;
    float speechThreshold = 0.02f;
    int minSilenceFrames = 50;
    int minSpeechFrames = 10;
    int maxSpeechFrames = 0;
    int preSpeechBufferSize = 0;
    bool speechDetectionEnabled = true;
    std::chrono::steady_clock::time_point speechStartTime;
    
    // Audio specs
    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec obtainedSpec;
    
    // Settings
    Settings& settings;
    float silenceThreshold;
    int silenceDurationSamples;
    int sampleRate;
    
    // Current audio level
    std::atomic<float> currentAudioLevel{0.0f};
};
