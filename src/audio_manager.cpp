#include "audio_manager.h"
#include "logger.h"
#include <cmath>
#include <algorithm>

AudioManager::AudioManager(Settings& settings) 
    : settings(settings), deviceId(0), recording(false), 
      continuousMode(false), newContinuousAudioAvailable(false),
      newContinuousAudioReady(false),
      continuousSampleThreshold(settings.sampleRate * 2.5), // 2.5 seconds of audio
      silenceThreshold(settings.silenceThreshold),
      silenceDurationSamples(settings.silenceDurationMs * settings.sampleRate / 1000),
      sampleRate(settings.sampleRate),
      silenceCounter(0),
      silenceChunks(settings.silenceDurationMs * settings.sampleRate / (1000 * 1024)), // Convert ms to buffer chunks
      // Speech detection initialization
      currentSpeechState(SpeechState::SILENCE),
      speechThreshold(settings.speechDetection.threshold),
      minSilenceFrames(settings.speechDetection.minSilenceMs * settings.sampleRate / (1000 * 1024)),
      minSpeechFrames(settings.sampleRate / 50), // About 20ms of speech
      maxSpeechFrames(settings.speechDetection.maxChunkSec * settings.sampleRate),
      preSpeechBufferSize(settings.speechDetection.preSpeechBufferMs * settings.sampleRate / 1000),
      speechDetectionEnabled(settings.speechDetection.enabled),
      currentAudioLevel(0.0f) {
    
    // Initialize desired audio spec
    SDL_zero(desiredSpec);
    desiredSpec.freq = settings.sampleRate;
    desiredSpec.format = AUDIO_F32;
    desiredSpec.channels = 1;
    desiredSpec.samples = 1024; // Buffer size
    desiredSpec.callback = AudioManager::audioCallback;
    desiredSpec.userdata = this;
    
    // Pre-allocate the pre-speech buffer
    preSpeechBuffer.reserve(preSpeechBufferSize);
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
        
        // Initialize speech detection state
        currentSpeechState.store(SpeechState::SILENCE);
        silenceFrameCount = 0;
        speechFrameCount = 0;
        preSpeechBuffer.clear();
        currentSpeechBuffer.clear();
        
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
    continuousMode.store(enabled);
    
    if (enabled) {
        Logger::info("Continuous mode enabled");
        
        // Initialize speech detection state
        if (speechDetectionEnabled && settings.speechDetection.enabled) {
            Logger::info("Speech-aware chunking enabled");
            currentSpeechState.store(SpeechState::SILENCE);
            silenceFrameCount = 0;
            speechFrameCount = 0;
            preSpeechBuffer.clear();
            currentSpeechBuffer.clear();
        }
        
        // Clear any existing data
        continuousBuffer.clear();
        
        std::lock_guard<std::mutex> lock(continuousMutex);
        continuousChunks.clear();
    } else {
        Logger::info("Continuous mode disabled");
        
        // Reset speech detection state if we were using it
        if (speechDetectionEnabled && settings.speechDetection.enabled) {
            currentSpeechState.store(SpeechState::SILENCE);
            currentSpeechBuffer.clear();
        }
    }
}

bool AudioManager::isContinuousMode() const {
    return continuousMode.load();
}

bool AudioManager::hasNewContinuousAudio() {
    return newContinuousAudioAvailable.load();
}

std::vector<float> AudioManager::getContinuousAudioChunk() {
    std::lock_guard<std::mutex> lock(continuousMutex);
    
    // If no chunks available, return empty
    if (continuousChunks.empty()) {
        return {};
    }
    
    // Get the oldest chunk
    std::vector<float> chunk = std::move(continuousChunks.front());
    continuousChunks.pop_front();
    
    // Reset flag if no more chunks
    if (continuousChunks.empty()) {
        newContinuousAudioAvailable.store(false);
    }
    
    return chunk;
}

void AudioManager::resetContinuousFlag() {
    newContinuousAudioAvailable.store(false);
}

// Static audio callback
void AudioManager::audioCallback(void* userdata, Uint8* stream, int len) {
    AudioManager* am = static_cast<AudioManager*>(userdata);
    am->processAudioData(stream, len);
}

// Check if sound level indicates speech
bool AudioManager::detectSpeech(float soundLevel) {
    return soundLevel > speechThreshold;
}

// Update speech state based on current sound level
void AudioManager::updateSpeechState(float soundLevel) {
    bool isSpeech = detectSpeech(soundLevel);
    
    switch (currentSpeechState) {
        case SpeechState::SILENCE:
            if (isSpeech) {
                // Potential speech detected
                speechFrameCount++;
                if (speechFrameCount >= minSpeechFrames) {
                    // Transition to SPEAKING state
                    currentSpeechState.store(SpeechState::SPEAKING);
                    speechFrameCount = 0;
                    silenceFrameCount = 0;
                    
                    // Record when speech started
                    speechStartTime = std::chrono::steady_clock::now();
                    
                    Logger::info("Speech detected - starting new chunk");
                }
            } else {
                // Still silence, add to pre-speech buffer
                speechFrameCount = 0;
            }
            break;
            
        case SpeechState::SPEAKING:
            if (!isSpeech) {
                // Potential silence detected
                silenceFrameCount++;
                if (silenceFrameCount >= minSilenceFrames) {
                    // Transition to SILENCE state and process the speech chunk
                    currentSpeechState.store(SpeechState::SILENCE);
                    silenceFrameCount = 0;
                    speechFrameCount = 0;
                    
                    // Process the completed speech chunk
                    processSpeechBasedChunk();
                    
                    Logger::info("Silence detected - finalizing speech chunk");
                }
            } else {
                // Still speaking
                silenceFrameCount = 0;
                
                // Check if we've exceeded maximum chunk duration
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - speechStartTime).count();
                
                if (duration >= settings.speechDetection.maxChunkSec) {
                    // Force processing of the current chunk if it's too long
                    Logger::info("Max speech duration reached - splitting chunk");
                    processSpeechBasedChunk();
                    
                    // Reset timers but remain in SPEAKING state
                    speechStartTime = now;
                }
            }
            break;
            
        case SpeechState::TRANSITION:
            // Not currently used, but could implement hysteresis here
            break;
    }
}

// Process a completed speech chunk
void AudioManager::processSpeechBasedChunk() {
    if (currentSpeechBuffer.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(continuousMutex);
    
    // Create a complete chunk that includes the pre-speech buffer
    std::vector<float> completeChunk;
    
    // Add pre-speech buffer first (contains audio right before speech was detected)
    if (!preSpeechBuffer.empty()) {
        completeChunk.reserve(preSpeechBuffer.size() + currentSpeechBuffer.size());
        completeChunk.insert(completeChunk.end(), preSpeechBuffer.begin(), preSpeechBuffer.end());
    } else {
        completeChunk.reserve(currentSpeechBuffer.size());
    }
    
    // Add the actual speech buffer
    completeChunk.insert(completeChunk.end(), currentSpeechBuffer.begin(), currentSpeechBuffer.end());
    
    // Copy the complete chunk to the continuous chunks queue
    continuousChunks.push_back(std::move(completeChunk));
    
    // Clear the current speech buffer but keep the pre-speech buffer
    currentSpeechBuffer.clear();
    
    // Set flag indicating new continuous audio is available
    newContinuousAudioAvailable.store(true);
}

// Get current speech state
SpeechState AudioManager::getSpeechState() const {
    return currentSpeechState.load();
}

// Process incoming audio data
void AudioManager::processAudioData(const Uint8* stream, int len) {
    if (!recording.load()) return;
    
    // Convert Uint8 stream to float for processing
    const float* floatStream = reinterpret_cast<const float*>(stream);
    int numSamples = len / sizeof(float);
    
    // Calculate the current audio level (RMS)
    float rms = calculateRMS(floatStream, numSamples);
    currentAudioLevel.store(rms);
    
    // Log the audio level occasionally
    static int logCounter = 0;
    if (++logCounter % 100 == 0) {
        Logger::info("RMS Sound Level: " + std::to_string(rms));
    }
    
    // Add audio data to the buffer
    std::lock_guard<std::mutex> lock(audioMutex);
    size_t startPos = audioBuffer.size();
    audioBuffer.resize(startPos + numSamples);
    memcpy(audioBuffer.data() + startPos, floatStream, len);
    
    // Check for silence in regular mode
    if (!continuousMode.load()) {
        // Increment silence counter if below threshold
        if (rms < silenceThreshold) {
            silenceCounter++;
        } else {
            silenceCounter = 0;
        }
    }
    // Handle continuous mode
    else {
        if (speechDetectionEnabled && settings.speechDetection.enabled) {
            // Speech-aware continuous mode
            
            // First, update the pre-speech buffer
            if (currentSpeechState.load() == SpeechState::SILENCE) {
                // In silence mode, maintain the pre-speech buffer
                preSpeechBuffer.insert(preSpeechBuffer.end(), floatStream, floatStream + numSamples);
                
                // Keep only the most recent samples based on buffer size
                if (preSpeechBuffer.size() > preSpeechBufferSize) {
                    preSpeechBuffer.erase(
                        preSpeechBuffer.begin(),
                        preSpeechBuffer.begin() + (preSpeechBuffer.size() - preSpeechBufferSize)
                    );
                }
            }
            else if (currentSpeechState.load() == SpeechState::SPEAKING) {
                // In speaking mode, add samples to the current speech buffer
                currentSpeechBuffer.insert(currentSpeechBuffer.end(), floatStream, floatStream + numSamples);
            }
            
            // Update the speech state
            updateSpeechState(rms);
        }
        else {
            // Traditional fixed-chunk continuous mode (fallback)
            continuousBuffer.insert(continuousBuffer.end(), floatStream, floatStream + numSamples);
            
            // Check if we have enough data for continuous processing
            if (continuousBuffer.size() >= continuousSampleThreshold) {
                std::lock_guard<std::mutex> continuousLock(continuousMutex);
                
                // Copy current chunk to the continuous chunks queue
                continuousChunks.push_back(continuousBuffer);
                
                // Keep 1 second of audio for overlap
                int overlapSamples = sampleRate * 1.0; // 1 second overlap
                
                // Create a new continuous buffer with overlap
                std::vector<float> newBuffer;
                if (continuousBuffer.size() > overlapSamples) {
                    newBuffer.assign(
                        continuousBuffer.end() - overlapSamples,
                        continuousBuffer.end()
                    );
                }
                
                // Replace the continuous buffer with the overlapped portion
                continuousBuffer = std::move(newBuffer);
                
                // Set flag indicating new continuous audio is available
                newContinuousAudioAvailable.store(true);
            }
        }
    }
}

// Calculate RMS value for audio level
float AudioManager::calculateRMS(const float* samples, int sampleCount) {
    if (sampleCount <= 0) return 0.0f;
    
    float sum = 0.0f;
    for (int i = 0; i < sampleCount; i++) {
        sum += samples[i] * samples[i];
    }
    
    return std::sqrt(sum / sampleCount);
}

// Get the current audio level
float AudioManager::getCurrentAudioLevel() const {
    return currentAudioLevel.load();
}
