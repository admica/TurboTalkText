#include "audio_manager.h"
#include "logger.h"
#include <cmath>
#include <algorithm>

AudioManager::AudioManager(const Settings& settings) 
    : settings(settings), deviceId(0), recording(false), silenceCounter(0),
      continuousMode(false), newContinuousAudioReady(false) {
    samplesPerChunk = settings.sampleRate / 1000 * 64; // 64ms chunks
    silenceChunks = settings.silenceDurationMs / 64; // Chunks for silence duration
    
    // For continuous mode, process audio every 2.5 seconds with 1 second overlap (increased from 0.5s)
    continuousSampleThreshold = settings.sampleRate * 2.5; // 2.5 seconds of audio (reduced from 3s)
    
    // Initialize desired audio spec
    SDL_zero(desiredSpec);
    desiredSpec.freq = settings.sampleRate;
    desiredSpec.format = AUDIO_F32;
    desiredSpec.channels = 1;
    desiredSpec.samples = samplesPerChunk;
    desiredSpec.callback = AudioManager::audioCallback;
    desiredSpec.userdata = this;
    
    // Initialize lastContinuousProcessTime
    lastContinuousProcessTime = std::chrono::steady_clock::now();
}

AudioManager::~AudioManager() {
    if (deviceId != 0) {
        SDL_CloseAudioDevice(deviceId);
    }
}

bool AudioManager::init() {
    // List all available audio input devices
    int numDevices = SDL_GetNumAudioDevices(1); // 1 indicates capture devices
    Logger::info("Available audio input devices:");
    for (int i = 0; i < numDevices; i++) {
        const char* name = SDL_GetAudioDeviceName(i, 1);
        Logger::info(std::to_string(i) + ": " + name);
    }

    // Determine which device to use
    std::string deviceToUse = settings.audioDevice;
    if (deviceToUse.empty() && numDevices > 0) {
        // If the device setting is empty, use the first available
        deviceToUse = SDL_GetAudioDeviceName(0, 1);
        Logger::info("Using first available device: " + deviceToUse);
    }

    // If the device is "default", pass NULL to use the system's default device
    const char* deviceName = (deviceToUse == "default") ? NULL : deviceToUse.c_str();
    Logger::info("Opening audio device: " + std::string(deviceName ? deviceName : "default"));

    // Attempt to open the audio device
    deviceId = SDL_OpenAudioDevice(deviceName, 1, &desiredSpec, &obtainedSpec, 0);
    if (deviceId == 0) {
        // If it fails, log the error and list devices again for reference
        Logger::error("Failed to open audio device: " + std::string(SDL_GetError()));
        Logger::error("Available audio input devices:");
        for (int i = 0; i < numDevices; i++) {
            const char* name = SDL_GetAudioDeviceName(i, 1);
            Logger::error(std::to_string(i) + ": " + name);
        }
        return false;
    }

    Logger::info("Audio device opened successfully");
    Logger::info("Sample rate: " + std::to_string(obtainedSpec.freq));
    Logger::info("Channels: " + std::to_string(obtainedSpec.channels));
    Logger::info("Format: " + std::to_string(obtainedSpec.format));
    Logger::info("Samples per chunk: " + std::to_string(obtainedSpec.samples));

    return true;
}

void AudioManager::startRecording() {
    if (!recording) {
        audioBuffer.clear();
        continuousBuffer.clear();
        silenceCounter = 0;
        SDL_PauseAudioDevice(deviceId, 0);
        recording = true;
        lastContinuousProcessTime = std::chrono::steady_clock::now();
        Logger::info("Recording started" + std::string(continuousMode ? " (continuous mode)" : ""));
    }
}

void AudioManager::stopRecording() {
    if (recording) {
        SDL_PauseAudioDevice(deviceId, 1);
        recording = false;
        continuousMode = false;
        newContinuousAudioReady = false;
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

void AudioManager::setContinuousMode(bool enabled) {
    continuousMode = enabled;
    if (enabled) {
        Logger::info("Continuous mode enabled");
        continuousBuffer.clear();
        newContinuousAudioReady = false;
        lastContinuousProcessTime = std::chrono::steady_clock::now();
    } else {
        Logger::info("Continuous mode disabled");
    }
}

bool AudioManager::isInContinuousMode() const {
    return continuousMode;
}

bool AudioManager::hasNewContinuousAudio() const {
    return newContinuousAudioReady;
}

std::vector<float> AudioManager::getContinuousAudioChunk() {
    newContinuousAudioReady = false;
    
    // Create a copy to return
    std::vector<float> result = continuousBuffer;
    
    // Keep the last 1 second (overlap) for context (increased from 0.5 seconds)
    size_t overlapSamples = settings.sampleRate * 1; // 1 second overlap
    
    if (continuousBuffer.size() > overlapSamples) {
        // Keep only the overlap samples
        continuousBuffer.erase(continuousBuffer.begin(), 
                              continuousBuffer.end() - overlapSamples);
    } else {
        // If we don't have enough samples, just clear it
        continuousBuffer.clear();
    }
    
    return result;
}

void AudioManager::resetContinuousFlag() {
    newContinuousAudioReady = false;
}

void AudioManager::audioCallback(void* userdata, Uint8* stream, int len) {
    AudioManager* self = static_cast<AudioManager*>(userdata);
    self->processAudioData(stream, len);
}

void AudioManager::processAudioData(const Uint8* stream, int len) {
    const float* floatStream = reinterpret_cast<const float*>(stream);
    int numSamples = len / sizeof(float);
    
    // Add to regular audio buffer
    audioBuffer.insert(audioBuffer.end(), floatStream, floatStream + numSamples);
    
    // If continuous mode is active, also add to continuous buffer
    if (continuousMode) {
        continuousBuffer.insert(continuousBuffer.end(), floatStream, floatStream + numSamples);
        
        // Check if we have enough samples for continuous processing
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastContinuousProcessTime).count();
            
        // Process approximately every 3 seconds
        if (continuousBuffer.size() >= continuousSampleThreshold || elapsed >= 3000) {
            newContinuousAudioReady = true;
            lastContinuousProcessTime = now;
        }
    }

    // RMS for silence detection
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += floatStream[i] * floatStream[i];
    }
    float rms = std::sqrt(sum / numSamples);

    // Log RMS at human intervals
    static int chunkCounter = 0;
    chunkCounter++;
    if (chunkCounter % 8 == 0) {
        Logger::info("RMS Sound Level: " + std::to_string(rms));
    }

    if (rms < settings.silenceThreshold) {
        silenceCounter++;
    } else {
        silenceCounter = 0;
    }
}
