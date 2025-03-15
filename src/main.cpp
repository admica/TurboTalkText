#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <memory>
#include <algorithm>
#include <chrono>
#include <thread>

#ifdef WHISPER_DISABLED
// Dummy implementations when WHISPER_DISABLED is defined
struct whisper_context_params { bool use_gpu; };
struct whisper_context { bool dummy; };
struct whisper_full_params { 
    bool dummy; 
    bool print_realtime;
};
enum whisper_sampling_strategy { WHISPER_SAMPLING_GREEDY };

inline whisper_context_params whisper_context_default_params() { return whisper_context_params{false}; }
inline whisper_context* whisper_init_from_file_with_params(const char* path, const whisper_context_params* params) { return new whisper_context(); }
inline whisper_context* whisper_init_from_file(const char* path) { return new whisper_context(); }
inline void whisper_free(whisper_context* ctx) { delete ctx; }
inline whisper_full_params whisper_full_default_params(whisper_sampling_strategy strategy) { 
    whisper_full_params params;
    params.print_realtime = false;
    return params;
}
inline int whisper_full(whisper_context* ctx, whisper_full_params params, const float* samples, int n_samples) { return 0; }
inline int whisper_full_n_segments(whisper_context* ctx) { return 1; }
inline const char* whisper_full_get_segment_text(whisper_context* ctx, int i) { return "Dummy transcription"; }
#else
#include "whisper.h"
#endif

#include <SDL3/SDL.h>
#include <windows.h>

struct TurboTalkText {
    whisper_context *ctx;
    SDL_AudioDeviceID audio_dev;
    SDL_AudioStream *audio_stream;
    bool is_recording = false;
    bool is_running = true;
    std::vector<float> audio_buffer;
    std::chrono::steady_clock::time_point last_sound;

    void init() {
        // Initialize Whisper with the recommended new API
        whisper_context_params cparams = whisper_context_default_params();
        ctx = whisper_init_from_file_with_params("ggml-base.en.bin", &cparams);
        if (!ctx) {
            // Fallback to the deprecated function if needed
            ctx = whisper_init_from_file("ggml-base.en.bin");
            if (!ctx) {
                printf("Failed to initialize Whisper model\n");
                return;
            }
        }

        SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
        setup_audio();
        setup_overlay();
    }

    void setup_audio() {
        // Create audio specification
        SDL_AudioSpec spec = {};
        spec.freq = 16000;
        spec.format = SDL_AUDIO_F32;
        spec.channels = 1;
        
        // Open audio device for recording
        audio_dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec);
        if (audio_dev == 0) {
            printf("Failed to open audio device: %s\n", SDL_GetError());
            return;
        }
        
        // Create audio stream
        audio_stream = SDL_CreateAudioStream(&spec, &spec);
        if (!audio_stream) {
            printf("Failed to create audio stream: %s\n", SDL_GetError());
            SDL_CloseAudioDevice(audio_dev);
            return;
        }
        
        // Set a callback for the audio stream
        SDL_SetAudioStreamPutCallback(audio_stream, audio_callback, this);
        
        // Bind the stream to the device
        if (SDL_BindAudioStream(audio_dev, audio_stream) < 0) {
            printf("Failed to bind audio stream: %s\n", SDL_GetError());
            SDL_DestroyAudioStream(audio_stream);
            SDL_CloseAudioDevice(audio_dev);
            return;
        }
        
        // Initially paused
        SDL_PauseAudioDevice(audio_dev);
    }

    void setup_overlay() {
        // Stub: SDL3 window creation later
    }

    static void audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
        auto *app = static_cast<TurboTalkText*>(userdata);
        if (!app->is_recording) return;

        // The PutCallback is called after data is added to the stream
        if (additional_amount > 0) {
            float *samples = new float[additional_amount / sizeof(float)];
            int sample_count = additional_amount / sizeof(float);
            
            // Get the data from the stream
            if (SDL_GetAudioStreamData(stream, samples, additional_amount) == additional_amount) {
                float energy = 0;
                for (int i = 0; i < sample_count; i++) energy += samples[i] * samples[i];
                energy /= sample_count;

                if (energy > 0.01f) {
                    app->audio_buffer.insert(app->audio_buffer.end(), samples, samples + sample_count);
                    app->last_sound = std::chrono::steady_clock::now();
                } else if (!app->audio_buffer.empty()) {
                    auto now = std::chrono::steady_clock::now();
                    if (std::chrono::duration_cast<std::chrono::seconds>(now - app->last_sound).count() >= 2) {
                        app->transcribe_and_output();
                        app->audio_buffer.clear();
                    }
                }
            }
            delete[] samples;
        }
    }

    void transcribe_and_output() {
        whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        wparams.print_realtime = false;
        if (whisper_full(ctx, wparams, audio_buffer.data(), audio_buffer.size()) == 0) {
            int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; i++) {
                const char *text = whisper_full_get_segment_text(ctx, i);
                type_text(text);
            }
        }
    }

    void type_text(const char *text) {
        INPUT ip = {INPUT_KEYBOARD};
        for (; *text; text++) {
            ip.ki.wVk = VkKeyScanA(*text) & 0xFF;
            ip.ki.dwFlags = 0; SendInput(1, &ip, sizeof(INPUT));
            ip.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &ip, sizeof(INPUT));
        }
    }

    void toggle_recording() {
        is_recording = !is_recording;
        if (is_recording) {
            SDL_ResumeAudioDevice(audio_dev);
        } else {
            SDL_PauseAudioDevice(audio_dev);
        }
        printf("Recording %s\n", is_recording ? "ON" : "OFF");
    }

    void run() {
        printf("TurboTalkText: Ctrl+Shift to toggle, Ctrl+Shift+CapsLock to exit\n");
        while (is_running) {
            if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                if (GetAsyncKeyState(VK_CAPITAL) & 0x8000) is_running = false;
                else {
                    toggle_recording();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void cleanup() {
        if (audio_stream) {
            SDL_DestroyAudioStream(audio_stream);
        }
        if (audio_dev) {
            SDL_CloseAudioDevice(audio_dev);
        }
        SDL_Quit();
        whisper_free(ctx);
    }
};

int main() {
    TurboTalkText app;
    app.init();
    app.run();
    app.cleanup();
    return 0;
}
