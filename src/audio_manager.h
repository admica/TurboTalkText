#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "settings.h"
#include <SDL2/SDL.h>
#include <vector>
#include <atomic>
#include <chrono>

class AudioManager {
public:
    AudioManager(const Settings& settings);
    ~AudioManager();
    bool init();
    void startRecording();
    void stopRecording();
    bool isRecording() const;
    std::vector<float> getAudioData() const;
    bool checkSilence();
    
    // Continuous mode functions
    void setContinuousMode(bool enabled);
    bool isInContinuousMode() const;
    bool hasNewContinuousAudio() const;
    std::vector<float> getContinuousAudioChunk();
    void resetContinuousFlag();

private:
    static void audioCallback(void* userdata, Uint8* stream, int len);
    void processAudioData(const Uint8* stream, int len);

    const Settings& settings;
    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec obtainedSpec;
    SDL_AudioDeviceID deviceId;
    std::vector<float> audioBuffer;
    std::vector<float> continuousBuffer;
    std::atomic<bool> recording;
    std::atomic<int> silenceCounter;
    std::atomic<bool> continuousMode;
    std::atomic<bool> newContinuousAudioReady;
    int samplesPerChunk;
    int silenceChunks;
    int continuousSampleThreshold;  // Number of samples needed for continuous processing
    std::chrono::time_point<std::chrono::steady_clock> lastContinuousProcessTime;
};

#endif // AUDIO_MANAGER_H
