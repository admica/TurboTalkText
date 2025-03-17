#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "settings.h"
#include <SDL2/SDL.h>
#include <vector>
#include <atomic>

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

private:
    static void audioCallback(void* userdata, Uint8* stream, int len);
    void processAudioData(const Uint8* stream, int len);

    const Settings& settings;
    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec obtainedSpec;
    SDL_AudioDeviceID deviceId;
    std::vector<float> audioBuffer;
    std::atomic<bool> recording;
    std::atomic<int> silenceCounter;
    int samplesPerChunk;
    int silenceChunks;
};

#endif // AUDIO_MANAGER_H
