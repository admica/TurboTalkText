#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <memory>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#ifdef WHISPER_DISABLED
// Dummy implementations when WHISPER_DISABLED is defined
struct whisper_context_params { bool use_gpu; };
struct whisper_context { bool dummy; };
struct whisper_full_params { 
    bool dummy; 
    bool print_realtime;
    const char* language;
    bool translate;
    int n_threads;
    int beam_size;
};
enum whisper_sampling_strategy { WHISPER_SAMPLING_GREEDY };

inline whisper_context_params whisper_context_default_params() { return whisper_context_params{false}; }
inline whisper_context* whisper_init_from_file_with_params(const char* path, const whisper_context_params* params) { return new whisper_context(); }
inline whisper_context* whisper_init_from_file(const char* path) { return new whisper_context(); }
inline void whisper_free(whisper_context* ctx) { delete ctx; }
inline whisper_full_params whisper_full_default_params(whisper_sampling_strategy strategy) { 
    whisper_full_params params;
    params.print_realtime = false;
    params.language = "en";
    params.translate = false;
    params.n_threads = 4;
    params.beam_size = 5;
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

struct Settings {
    struct Audio {
        std::string device;
        int sample_rate = 16000;
        float silence_threshold = 0.01f;
        int silence_duration_ms = 2000;
    } audio;

    struct Whisper {
        std::string model_path = "ggml-base.en.bin";
        std::string language = "en";
        bool translate = false;
        int beam_size = 5;
        int threads = 4;
    } whisper;

    struct Output {
        std::string type = "keyboard";
        bool add_punctuation = true;
        bool capitalize_sentences = true;
    } output;
    
    static Settings load(const std::string& path) {
        Settings s;
        try {
            std::ifstream f(path);
            if (f.is_open()) {
                json j = json::parse(f);
                
                // Audio settings
                auto& ja = j["audio"];
                s.audio.device = ja.value("device", s.audio.device);
                s.audio.sample_rate = ja.value("sample_rate", s.audio.sample_rate);
                s.audio.silence_threshold = ja.value("silence_threshold", s.audio.silence_threshold);
                s.audio.silence_duration_ms = ja.value("silence_duration_ms", s.audio.silence_duration_ms);
                
                // Whisper settings
                auto& jw = j["whisper"];
                s.whisper.model_path = jw.value("model_path", s.whisper.model_path);
                s.whisper.language = jw.value("language", s.whisper.language);
                s.whisper.translate = jw.value("translate", s.whisper.translate);
                s.whisper.beam_size = jw.value("beam_size", s.whisper.beam_size);
                s.whisper.threads = jw.value("threads", s.whisper.threads);
                
                // Output settings
                auto& jo = j["output"];
                s.output.type = jo.value("type", s.output.type);
                s.output.add_punctuation = jo.value("add_punctuation", s.output.add_punctuation);
                s.output.capitalize_sentences = jo.value("capitalize_sentences", s.output.capitalize_sentences);
            }
        } catch (const std::exception& e) {
            printf("Failed to load settings: %s\n", e.what());
        }
        return s;
    }
    
    void save(const std::string& path) const {
        try {
            json j;
            
            // Audio settings
            j["audio"]["device"] = audio.device;
            j["audio"]["sample_rate"] = audio.sample_rate;
            j["audio"]["silence_threshold"] = audio.silence_threshold;
            j["audio"]["silence_duration_ms"] = audio.silence_duration_ms;
            
            // Whisper settings
            j["whisper"]["model_path"] = whisper.model_path;
            j["whisper"]["language"] = whisper.language;
            j["whisper"]["translate"] = whisper.translate;
            j["whisper"]["beam_size"] = whisper.beam_size;
            j["whisper"]["threads"] = whisper.threads;
            
            // Output settings
            j["output"]["type"] = output.type;
            j["output"]["add_punctuation"] = output.add_punctuation;
            j["output"]["capitalize_sentences"] = output.capitalize_sentences;
            
            std::ofstream f(path);
            f << j.dump(4);
        } catch (const std::exception& e) {
            printf("Failed to save settings: %s\n", e.what());
        }
    }
};

struct AudioDevice {
    std::string name;
    SDL_AudioDeviceID id;
    bool is_capture;
    SDL_AudioSpec spec;
};

struct TurboTalkText {
    whisper_context *ctx = nullptr;
    SDL_AudioDeviceID audio_dev = 0;
    SDL_AudioStream *audio_stream = nullptr;
    bool is_recording = false;
    bool is_running = true;
    std::vector<float> audio_buffer;
    std::chrono::steady_clock::time_point last_sound;
    Settings settings;
    std::vector<AudioDevice> audio_devices;

    void init() {
        settings = Settings::load("settings.json");
        
        // Initialize SDL
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            printf("Failed to initialize SDL: %s\n", SDL_GetError());
            return;
        }
        
        // List available recording devices
        int num_devices = 0;
        SDL_AudioDeviceID* recording_devices = SDL_GetAudioRecordingDevices(&num_devices);
        if (!recording_devices) {
            printf("Failed to get recording devices: %s\n", SDL_GetError());
        } else {
            printf("Available recording devices (%d):\n", num_devices);
            for (int i = 0; i < num_devices; i++) {
                const char* name = SDL_GetAudioDeviceName(recording_devices[i]);
                if (name) {
                    printf("%d: %s\n", i, name);
                    AudioDevice dev;
                    dev.name = name;
                    dev.id = recording_devices[i];
                    dev.is_capture = true;
                    audio_devices.push_back(dev);
                }
            }
            SDL_free(recording_devices);
        }
        
        // Initialize Whisper
        whisper_context_params cparams = whisper_context_default_params();
        ctx = whisper_init_from_file_with_params(settings.whisper.model_path.c_str(), &cparams);
        if (!ctx) {
            printf("Failed to initialize Whisper model\n");
            return;
        }

        setup_audio();
    }

    void setup_audio() {
        // Create audio specification
        SDL_AudioSpec spec = {};
        spec.freq = settings.audio.sample_rate;
        spec.format = SDL_AUDIO_F32;
        spec.channels = 1;
        
        printf("Opening audio device with sample rate: %d Hz\n", spec.freq);
        
        // Open audio device stream
        audio_stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &spec,
            audio_callback,
            this
        );
        
        if (!audio_stream) {
            printf("Failed to open audio device stream: %s\n", SDL_GetError());
            return;
        }

        // Get the device ID associated with the stream
        audio_dev = SDL_GetAudioStreamDevice(audio_stream);
        if (audio_dev == 0) {
            printf("Failed to get audio device: %s\n", SDL_GetError());
            SDL_DestroyAudioStream(audio_stream);
            return;
        }

        printf("Successfully opened audio device and created stream\n");
        
        // Start in paused state
        SDL_PauseAudioStreamDevice(audio_stream);
    }

    static void audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
        auto *app = static_cast<TurboTalkText*>(userdata);
        if (!app->is_recording) return;

        // Process only if we have new data
        if (additional_amount > 0) {
            std::vector<float> samples(additional_amount / sizeof(float));
            
            // Get the data from the stream
            if (SDL_GetAudioStreamData(stream, samples.data(), additional_amount) == additional_amount) {
                float energy = 0;
                for (size_t i = 0; i < samples.size(); i++) {
                    energy += samples[i] * samples[i];
                }
                energy /= samples.size();

                printf("Audio energy: %.6f (threshold: %.6f)\n", energy, app->settings.audio.silence_threshold);

                // If we detect sound
                if (energy > app->settings.audio.silence_threshold) {
                    printf("Sound detected! Buffer size: %zu samples\n", app->audio_buffer.size());
                    app->audio_buffer.insert(app->audio_buffer.end(), samples.begin(), samples.end());
                    app->last_sound = std::chrono::steady_clock::now();
                } 
                // If we have data and detect silence
                else if (!app->audio_buffer.empty()) {
                    auto now = std::chrono::steady_clock::now();
                    auto silence_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - app->last_sound).count();
                    printf("Silence duration: %lld ms (threshold: %d ms)\n", silence_duration, app->settings.audio.silence_duration_ms);
                    
                    if (silence_duration >= app->settings.audio.silence_duration_ms) {
                        printf("Processing audio buffer of size: %zu samples\n", app->audio_buffer.size());
                        app->transcribe_and_output();
                        app->audio_buffer.clear();
                    }
                }
            }
        }
    }

    void transcribe_and_output() {
        if (audio_buffer.empty()) {
            printf("Warning: Attempted to transcribe empty audio buffer\n");
            return;
        }

        printf("Starting transcription of %zu samples\n", audio_buffer.size());

        // Set up Whisper parameters
        whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        wparams.print_realtime = false;
        wparams.language = settings.whisper.language.c_str();
        wparams.translate = settings.whisper.translate;
        wparams.n_threads = settings.whisper.threads;
        wparams.beam_size = settings.whisper.beam_size;

        // Run Whisper inference
        if (whisper_full(ctx, wparams, audio_buffer.data(), audio_buffer.size()) == 0) {
            // Get number of segments
            int n_segments = whisper_full_n_segments(ctx);
            printf("Transcription completed with %d segments\n", n_segments);

            // Process each segment
            for (int i = 0; i < n_segments; ++i) {
                const char* text = whisper_full_get_segment_text(ctx, i);
                printf("Segment %d text: %s\n", i, text);
                
                // Type out the text
                if (text && strlen(text) > 0) {
                    type_text(text);
                }
            }
        } else {
            printf("Failed to run whisper inference\n");
        }
    }

    void type_text(const char *text) {
        if (!text) return;
        
        INPUT ip = {INPUT_KEYBOARD};
        for (; *text; text++) {
            ip.ki.wVk = VkKeyScanA(*text) & 0xFF;
            ip.ki.dwFlags = 0; 
            SendInput(1, &ip, sizeof(INPUT));
            ip.ki.dwFlags = KEYEVENTF_KEYUP; 
            SendInput(1, &ip, sizeof(INPUT));
        }
    }

    void toggle_recording() {
        is_recording = !is_recording;
        if (is_recording) {
            SDL_ResumeAudioStreamDevice(audio_stream);
            printf("Recording started\n");
        } else {
            SDL_PauseAudioStreamDevice(audio_stream);
            printf("Recording stopped\n");
        }
    }

    void run() {
        printf("TurboTalkText: Press Ctrl+Shift to toggle recording, Ctrl+Shift+CapsLock to exit\n");
        
        while (is_running) {
            if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                if (GetAsyncKeyState(VK_CAPITAL) & 0x8000) {
                    is_running = false;
                } else {
                    toggle_recording();
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void cleanup() {
        if (audio_stream) {
            SDL_DestroyAudioStream(audio_stream);
            audio_stream = nullptr;
            audio_dev = 0;  // Device is automatically closed when stream is destroyed
        }
        SDL_Quit();
        if (ctx) {
            whisper_free(ctx);
            ctx = nullptr;
        }
    }
};

int main() {
    TurboTalkText app;
    app.init();
    app.run();
    app.cleanup();
    return 0;
}
