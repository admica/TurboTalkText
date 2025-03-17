#include "audio_manager.h"
#include "logger.h"
#include <cmath>

AudioManager::AudioManager(const Settings& settings) 
    : settings(settings), deviceId(0), recording(false), silenceCounter(0) {
    samplesPerChunk = settings.sampleRate / 1000 * 64; // 64ms chunks
    silenceChunks = settings.silenceDurationMs / 64;   // Chunks for silence duration
}

AudioManager::~AudioManager() {
    if (deviceId != 0) {
        SDL_CloseAudioDevice(deviceId);
    }
}

bool AudioManager::init() {
    desiredSpec.freq = settings.sampleRate;
    desiredSpec.format = AUDIO_F32SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = 1024;
    desiredSpec.callback = audioCallback;
    desiredSpec.userdata = this;

    deviceId = SDL_OpenAudioDevice(settings.audioDevice.c_str(), 1, &desiredSpec, &obtainedSpec, 0);
    if (deviceId == 0) {
        Logger::error("Failed to open audio device: " + std::string(SDL_GetError()));
        return false;
    }
    return true;
}

void AudioManager::startRecording() {
    if (!recording) {
        audioBuffer.clear();
        silenceCounter = 0;
        SDL_PauseAudioDevice(deviceId, 0);
        recording = true;
        Logger::info("Recording started");
    }
}

void AudioManager::stopRecording() {
    if (recording) {
        SDL_PauseAudioDevice(deviceId, 1);
        recording = false;
        Logger::info("Recording stopped");
    }
}

bool AudioManager::isRecording() const {
    return recording;
}

std::vector<float> AudioManager::getAudioData() const {
    return audioBuffer;
}

bool AudioManager::checkSilence() {
    return silenceCounter >= silenceChunks;
}

void AudioManager::audioCallback(void* userdata, Uint8* stream, int len) {
    AudioManager* self = static_cast<AudioManager*>(userdata);
    self->processAudioData(stream, len);
}

void AudioManager::processAudioData(const Uint8* stream, int len) {
    const float* floatStream = reinterpret_cast<const float*>(stream);
    int numSamples = len / sizeof(float);
    audioBuffer.insert(audioBuffer.end(), floatStream, floatStream + numSamples);

    // RMS for silence detection
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += floatStream[i] * floatStream[i];
    }
    float rms = std::sqrt(sum / numSamples);
    if (rms < settings.silenceThreshold) {
        silenceCounter++;
    } else {
        silenceCounter = 0;
    }
}
