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
            printf("Attempting to load settings from: %s\n", path.c_str());
            fflush(stdout);
            
            std::string content;
            try {
                // Open and read the file
                std::ifstream f(path, std::ios::binary);  // Use binary mode
                printf("File open attempt complete\n");
                fflush(stdout);
                
                if (!f.is_open()) {
                    DWORD error = GetLastError();
                    char error_msg[256];
                    FormatMessageA(
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        error,
                        0,
                        error_msg,
                        sizeof(error_msg),
                        NULL
                    );
                    printf("Could not open settings file for reading. Error %lu: %s\n", error, error_msg);
                    fflush(stdout);
                    return s;
                }
                
                printf("File opened successfully\n");
                fflush(stdout);
                
                // Read the file into a stringstream first
                std::stringstream buffer;
                buffer << f.rdbuf();
                content = buffer.str();
                
                printf("File read into string, size: %zu bytes\n", content.length());
                fflush(stdout);
                
                f.close();
                printf("File closed\n");
                fflush(stdout);
                
            } catch (const std::ios_base::failure& e) {
                printf("File I/O error: %s\n", e.what());
                fflush(stdout);
                return s;
            } catch (const std::exception& e) {
                printf("Error reading file: %s\n", e.what());
                fflush(stdout);
                return s;
            }
            
            if (content.empty()) {
                printf("Settings file is empty\n");
                fflush(stdout);
                return s;
            }
            
            printf("About to parse JSON: %s\n", content.c_str());
            fflush(stdout);
            
            json j = json::parse(content);
            printf("JSON parsed successfully\n");
            fflush(stdout);
            
            // Audio settings
            if (j.contains("audio")) {
                auto& ja = j["audio"];
                if (ja.contains("device")) s.audio.device = ja["device"];
                if (ja.contains("sample_rate")) s.audio.sample_rate = ja["sample_rate"];
                if (ja.contains("silence_threshold")) s.audio.silence_threshold = ja["silence_threshold"];
                if (ja.contains("silence_duration_ms")) s.audio.silence_duration_ms = ja["silence_duration_ms"];
            }
            
            // Whisper settings
            if (j.contains("whisper")) {
                auto& jw = j["whisper"];
                if (jw.contains("model_path")) s.whisper.model_path = jw["model_path"];
                if (jw.contains("language")) s.whisper.language = jw["language"];
                if (jw.contains("translate")) s.whisper.translate = jw["translate"];
                if (jw.contains("beam_size")) s.whisper.beam_size = jw["beam_size"];
                if (jw.contains("threads")) s.whisper.threads = jw["threads"];
            }
            
            // Output settings
            if (j.contains("output")) {
                auto& jo = j["output"];
                if (jo.contains("type")) s.output.type = jo["type"];
                if (jo.contains("add_punctuation")) s.output.add_punctuation = jo["add_punctuation"];
                if (jo.contains("capitalize_sentences")) s.output.capitalize_sentences = jo["capitalize_sentences"];
            }
            
            printf("Settings loaded successfully\n");
            fflush(stdout);
            
        } catch (const json::parse_error& e) {
            printf("JSON parse error: %s\n", e.what());
            fflush(stdout);
        } catch (const std::exception& e) {
            printf("Error loading settings: %s\n", e.what());
            fflush(stdout);
        } catch (...) {
            printf("Unknown error while loading settings\n");
            fflush(stdout);
        }
        return s;
    }
    
    void save(const std::string& path) const {
        try {
            printf("Attempting to save settings to: %s\n", path.c_str());
            
            // Create JSON object first to catch any serialization errors early
            json j;
            try {
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
                
                printf("JSON object created successfully\n");
            } catch (const json::exception& e) {
                printf("Error creating JSON object: %s\n", e.what());
                return;
            }
            
            // Try to create the file
            printf("Opening file for writing...\n");
            std::ofstream f(path, std::ios::out | std::ios::binary);
            if (!f.is_open()) {
                DWORD error = GetLastError();
                char error_msg[256];
                FormatMessageA(
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    error,
                    0,
                    error_msg,
                    sizeof(error_msg),
                    NULL
                );
                printf("Failed to open file for writing. Error %lu: %s\n", error, error_msg);
                return;
            }
            
            printf("File opened successfully, writing settings...\n");
            std::string json_str = j.dump(4);
            f.write(json_str.c_str(), json_str.length());
            
            if (f.fail()) {
                printf("Failed to write to file: %s\n", strerror(errno));
                f.close();
                return;
            }
            
            f.close();
            if (f.fail()) {
                printf("Error occurred while closing the file: %s\n", strerror(errno));
                return;
            }
            
            printf("Settings saved successfully to %s\n", path.c_str());
            
            // Verify the file was written correctly
            std::ifstream verify(path);
            if (verify.is_open()) {
                verify.seekg(0, std::ios::end);
                size_t size = verify.tellg();
                printf("Verified file size: %zu bytes\n", size);
                verify.close();
            } else {
                printf("Warning: Could not verify file was written\n");
            }
            
        } catch (const std::exception& e) {
            printf("Error saving settings: %s\n", e.what());
        } catch (...) {
            printf("Unknown error while saving settings\n");
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
        printf("\n=== Initialization Started ===\n");
        fflush(stdout);
        
        // Get executable directory
        char exe_path[MAX_PATH];
        if (!GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
            printf("Failed to get executable path! Error: %lu\n", GetLastError());
            fflush(stdout);
            return;
        }
        std::string exe_dir = std::string(exe_path);
        exe_dir = exe_dir.substr(0, exe_dir.find_last_of("\\/"));
        printf("Executable directory: %s\n", exe_dir.c_str());
        fflush(stdout);
        
        // Construct settings path relative to executable
        std::string settings_path = exe_dir + "\\settings.json";
        printf("Looking for settings at: %s\n", settings_path.c_str());
        fflush(stdout);
        
        // Check if settings file exists first
        WIN32_FIND_DATAA findData;
        HANDLE handle = FindFirstFileA(settings_path.c_str(), &findData);
        if (handle == INVALID_HANDLE_VALUE) {
            printf("Settings file does not exist! Error: %lu\n", GetLastError());
            fflush(stdout);
            return;
        }
        FindClose(handle);
        printf("Settings file exists\n");
        fflush(stdout);
        
        // Load settings
        settings = Settings::load(settings_path);
        printf("Settings load attempt complete\n");
        fflush(stdout);
        
        // Initialize SDL
        printf("Initializing SDL...\n");
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            printf("Failed to initialize SDL: %s\n", SDL_GetError());
            return;
        }
        printf("SDL initialized successfully\n");
        
        // List available recording devices
        printf("\nEnumerating recording devices...\n");
        int num_devices = 0;
        SDL_AudioDeviceID* recording_devices = SDL_GetAudioRecordingDevices(&num_devices);
        if (!recording_devices) {
            printf("Failed to get recording devices: %s\n", SDL_GetError());
            SDL_Quit();
            return;
        }
        
        if (num_devices == 0) {
            printf("No recording devices found!\n");
            SDL_free(recording_devices);
            SDL_Quit();
            return;
        }
        
        printf("Found %d recording devices:\n", num_devices);
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
        
        // Initialize Whisper
        printf("\nInitializing Whisper model...\n");
        
        // Check for model in executable directory first
        std::string model_path = exe_dir + "\\" + settings.whisper.model_path;
        if (!file_exists(model_path)) {
            printf("Model not found at: %s\n", model_path.c_str());
            
            // Try just the filename in executable directory
            model_path = exe_dir + "\\ggml-base.en.bin";
            if (!file_exists(model_path)) {
                printf("Model not found at: %s\n", model_path.c_str());
                printf("Please ensure ggml-base.en.bin is in the same directory as the executable\n");
                SDL_Quit();
                return;
            }
        }
        
        printf("Using model: %s\n", model_path.c_str());
        settings.whisper.model_path = model_path;  // Update settings with full path
        whisper_context_params cparams = whisper_context_default_params();
        ctx = whisper_init_from_file_with_params(model_path.c_str(), cparams);
        if (!ctx) {
            printf("Failed to initialize Whisper model\n");
            SDL_Quit();
            return;
        }
        printf("Whisper model initialized successfully\n");

        printf("\nSetting up audio...\n");
        if (!setup_audio()) {
            printf("Failed to set up audio\n");
            whisper_free(ctx);
            SDL_Quit();
            return;
        }
        printf("=== Initialization Complete ===\n\n");
    }

    // Helper function to check if a file exists
    bool file_exists(const std::string& path) {
        WIN32_FIND_DATAA findData;
        HANDLE handle = FindFirstFileA(path.c_str(), &findData);
        if (handle != INVALID_HANDLE_VALUE) {
            FindClose(handle);
            return true;
        }
        return false;
    }

    bool setup_audio() {
        printf("Creating audio specification...\n");
        // Create audio specification
        SDL_AudioSpec spec = {};
        spec.freq = settings.audio.sample_rate;
        spec.format = SDL_AUDIO_F32;
        spec.channels = 1;
        
        printf("Opening audio device with settings:\n");
        printf("- Sample rate: %d Hz\n", spec.freq);
        printf("- Format: SDL_AUDIO_F32\n");
        printf("- Channels: %d\n", spec.channels);
        
        // Open audio device stream
        audio_stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &spec,
            audio_callback,
            this
        );
        
        if (!audio_stream) {
            printf("Failed to open audio device stream: %s\n", SDL_GetError());
            return false;
        }
        printf("Audio stream created successfully\n");

        // Get the device ID associated with the stream
        audio_dev = SDL_GetAudioStreamDevice(audio_stream);
        if (audio_dev == 0) {
            printf("Failed to get audio device: %s\n", SDL_GetError());
            SDL_DestroyAudioStream(audio_stream);
            audio_stream = nullptr;
            return false;
        }
        printf("Audio device ID obtained: %u\n", audio_dev);

        printf("Audio setup completed successfully\n");
        
        // Start in paused state
        SDL_PauseAudioStreamDevice(audio_stream);
        printf("Audio stream paused and ready for recording\n");
        return true;
    }

    static void audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
        auto *app = static_cast<TurboTalkText*>(userdata);
        if (!app->is_recording) return;

        // Process only if we have new data
        if (additional_amount > 0) {
            printf("Received %d bytes of audio data\n", additional_amount);
            std::vector<float> samples(additional_amount / sizeof(float));
            
            // Get the data from the stream
            if (SDL_GetAudioStreamData(stream, samples.data(), additional_amount) == additional_amount) {
                float energy = 0;
                float peak = 0;
                for (size_t i = 0; i < samples.size(); i++) {
                    energy += samples[i] * samples[i];
                    peak = std::max(peak, std::abs(samples[i]));
                }
                energy /= samples.size();

                printf("Audio energy: %.6f (threshold: %.6f) Peak: %.6f\n", energy, app->settings.audio.silence_threshold, peak);

                // If we detect sound
                if (energy > app->settings.audio.silence_threshold) {
                    printf("Sound detected! Buffer size before: %zu samples, adding %zu new samples\n", 
                           app->audio_buffer.size(), samples.size());
                    app->audio_buffer.insert(app->audio_buffer.end(), samples.begin(), samples.end());
                    app->last_sound = std::chrono::steady_clock::now();
                } else {
                    // Check if we should process due to silence
                    auto silence_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - app->last_sound
                    ).count();

                    if (!app->audio_buffer.empty()) {
                        printf("Silence duration: %lld ms (threshold: %d ms)\n", 
                               silence_duration, app->settings.audio.silence_duration_ms);
                        
                        if (silence_duration >= app->settings.audio.silence_duration_ms) {
                            printf("Silence threshold reached, processing buffer\n");
                            app->transcribe_and_output();
                            app->audio_buffer.clear();
                        }
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

        printf("\n=== Starting Transcription ===\n");
        printf("Buffer size: %zu samples (%.2f seconds)\n", 
               audio_buffer.size(), 
               static_cast<float>(audio_buffer.size()) / settings.audio.sample_rate);
        printf("Peak amplitude: %.6f\n", 
               *std::max_element(audio_buffer.begin(), audio_buffer.end(), 
                               [](float a, float b) { return std::abs(a) < std::abs(b); }));

        // Check for valid whisper context
        if (!ctx) {
            printf("Error: Whisper context is null\n");
            return;
        }

        // Set up Whisper parameters
        whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
        wparams.print_realtime = true;  // Enable real-time printing
        wparams.language = settings.whisper.language.c_str();
        wparams.translate = settings.whisper.translate;
        wparams.n_threads = settings.whisper.threads;
        wparams.beam_search.beam_size = settings.whisper.beam_size;

        printf("Running Whisper inference with:\n");
        printf("- Language: %s\n", wparams.language);
        printf("- Beam size: %d\n", wparams.beam_search.beam_size);
        printf("- Threads: %d\n", wparams.n_threads);

        // Run Whisper inference
        printf("\nStarting inference...\n");
        int result = whisper_full(ctx, wparams, audio_buffer.data(), audio_buffer.size());
        
        if (result == 0) {
            int n_segments = whisper_full_n_segments(ctx);
            printf("\nTranscription completed with %d segments\n", n_segments);

            if (n_segments == 0) {
                printf("Warning: No segments detected in the audio\n");
                return;
            }

            // Process each segment
            for (int i = 0; i < n_segments; ++i) {
                const char* text = whisper_full_get_segment_text(ctx, i);
                if (!text || strlen(text) == 0) {
                    printf("Warning: Empty text in segment %d\n", i);
                    continue;
                }
                
                printf("Segment %d text: %s\n", i, text);
                printf("Typing text...\n");
                type_text(text);
                printf("Text typed.\n");
            }
        } else {
            printf("Failed to run whisper inference (error code: %d)\n", result);
        }
        printf("=== Transcription Complete ===\n\n");
    }

    void type_text(const char *text) {
        if (!text) return;
        
        printf("Typing: %s\n", text);
        
        // Add a small delay before typing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        INPUT ip = {INPUT_KEYBOARD};
        for (; *text; text++) {
            char c = *text;
            SHORT vk = VkKeyScanA(c);
            
            // Handle shift key for uppercase letters and symbols
            bool need_shift = (vk >> 8) & 1;
            if (need_shift) {
                ip.ki.wVk = VK_SHIFT;
                ip.ki.dwFlags = 0;
                SendInput(1, &ip, sizeof(INPUT));
            }
            
            // Press and release the character key
            ip.ki.wVk = vk & 0xFF;
            ip.ki.dwFlags = 0;
            SendInput(1, &ip, sizeof(INPUT));
            ip.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &ip, sizeof(INPUT));
            
            // Release shift if it was pressed
            if (need_shift) {
                ip.ki.wVk = VK_SHIFT;
                ip.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &ip, sizeof(INPUT));
            }
            
            // Add a small delay between characters
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void toggle_recording() {
        is_recording = !is_recording;
        if (is_recording) {
            printf("Recording started\n");
            audio_buffer.clear();
        } else {
            printf("Recording stopped\n");
            // If we have collected any audio data, process it immediately
            if (!audio_buffer.empty()) {
                printf("Processing final buffer of %zu samples\n", audio_buffer.size());
                transcribe_and_output();
                audio_buffer.clear();
            }
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
    printf("\n=== Starting TurboTalkText ===\n");
    fflush(stdout);
    
    // Get and print current working directory
    char cwd[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
        printf("Working directory: %s\n", cwd);
        fflush(stdout);
    } else {
        printf("Failed to get working directory\n");
        fflush(stdout);
    }
    
    TurboTalkText app;
    
    printf("Initializing application...\n");
    fflush(stdout);
    
    try {
        app.init();
        printf("Initialization complete, starting main loop...\n");
        fflush(stdout);
        app.run();
    } catch (const std::exception& e) {
        printf("ERROR: Application crashed with exception: %s\n", e.what());
        fflush(stdout);
    } catch (...) {
        printf("ERROR: Application crashed with unknown exception!\n");
        fflush(stdout);
    }
    
    printf("Cleaning up...\n");
    fflush(stdout);
    app.cleanup();
    
    printf("=== Application terminated ===\n");
    fflush(stdout);
    
    // Wait for a key press before exiting
    printf("\nPress Enter to exit...");
    fflush(stdout);
    getchar();
    
    return 0;
}
