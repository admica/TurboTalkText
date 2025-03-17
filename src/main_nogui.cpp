#include "logger.h"
#include "settings.h"
#include <SDL2/SDL.h>
#include "../whisper.cpp/include/whisper.h"
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

// Global variables
static struct whisper_context* whisper_ctx = nullptr;
static std::vector<float> audio_buffer;
static SDL_AudioDeviceID audio_dev = 0;
static bool is_recording = false;
static bool is_processing = false;
static std::string transcription_text;

// Forward declarations
void processAudio();

// Audio callback function
void audio_callback(void* userdata, Uint8* stream, int len) {
    float* samples = reinterpret_cast<float*>(stream);
    int sample_count = len / sizeof(float);
    audio_buffer.insert(audio_buffer.end(), samples, samples + sample_count);
}

// Start audio recording
void startRecording() {
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = Settings::getSampleRate();
    spec.format = AUDIO_F32SYS;
    spec.channels = 1;
    spec.samples = 4096;
    spec.callback = audio_callback;
    spec.userdata = nullptr;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, SDL_TRUE, &spec, NULL, 0);
    if (dev == 0) {
        Logger::error("Failed to open audio device: {}", SDL_GetError());
        return;
    }
    audio_dev = dev;
    SDL_PauseAudioDevice(audio_dev, 0);
    is_recording = true;
    Logger::info("Recording started");
}

// Stop audio recording
void stopRecording() {
    SDL_PauseAudioDevice(audio_dev, 1);
    SDL_CloseAudioDevice(audio_dev);
    is_recording = false;
    is_processing = true;
    Logger::info("Recording stopped, processing audio...");
    
    // Process in the main thread for now
    processAudio();
}

// Process audio with whisper
void processAudio() {
    // Initialize whisper context if not already done
    if (!whisper_ctx) {
        whisper_context_params ctx_params = whisper_context_default_params();
        whisper_ctx = whisper_init_from_file_with_params(Settings::getModelPath().c_str(), ctx_params);
        if (!whisper_ctx) {
            Logger::error("Failed to initialize whisper context");
            is_processing = false;
            return;
        }
    }

    if (audio_buffer.empty()) {
        Logger::error("Audio buffer is empty");
        is_processing = false;
        return;
    }

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_realtime = false;
    params.print_progress = false;
    params.language = Settings::getLanguage().c_str();
    params.translate = Settings::getTranslate();
    params.n_threads = Settings::getThreads();
    // Beam size parameter was removed or renamed in this version of whisper 
    // params.beam_size = Settings::getBeamSize();
    
    // Check if we can set beam size via sampling parameters
    if (params.strategy == WHISPER_SAMPLING_BEAM_SEARCH && Settings::getBeamSize() > 0) {
        params.greedy.best_of = Settings::getBeamSize();
    }

    if (whisper_full(whisper_ctx, params, audio_buffer.data(), audio_buffer.size()) != 0) {
        Logger::error("Transcription failed");
        is_processing = false;
        return;
    }

    // Process results
    transcription_text.clear();
    int n_segments = whisper_full_n_segments(whisper_ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char* text = whisper_full_get_segment_text(whisper_ctx, i);
        transcription_text += text;
        transcription_text += " ";
    }

    Logger::info("Transcription: {}", transcription_text);
    is_processing = false;
    audio_buffer.clear();
}

int main(int argc, char* argv[]) {
    // Initialize logger
    Logger::init();
    Logger::info("TurboTalkText starting up (console mode)");
    
    // Load settings
    if (!Settings::load()) {
        Logger::error("Failed to load settings, using defaults");
    }
    
    // Initialize SDL audio
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        Logger::error("SDL_Init failed: {}", SDL_GetError());
        return 1;
    }
    
    Logger::info("Press 'r' to start/stop recording");
    Logger::info("Press 'q' to quit");
    
    // Main loop (simple keyboard input for testing)
    bool running = true;
    while (running) {
        // Read from stdin
        if (std::cin.rdbuf()->in_avail()) {
            char c = std::cin.get();
            if (c == 'r' || c == 'R') {
                if (is_recording) {
                    Logger::info("Stopping recording...");
                    stopRecording();
                } else if (!is_processing) {
                    Logger::info("Starting recording...");
                    startRecording();
                }
            } else if (c == 'q' || c == 'Q') {
                running = false;
            }
        }
        
        // Sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    Logger::info("Shutting down");
    if (whisper_ctx) {
        whisper_free(whisper_ctx);
    }
    SDL_Quit();
    
    return 0;
} 