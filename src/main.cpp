#include "whisper.h"
#include <SDL3/SDL.h>
#include <Windows.h>
#include <vector>
#include <chrono>

struct TurboTalkText {
    whisper_context *ctx;
    SDL_AudioDeviceID audio_dev;
    bool is_recording = false;
    bool is_running = true;
    std::vector<float> audio_buffer;
    std::chrono::steady_clock::time_point last_sound;

    void init() {
        ctx = whisper_init_from_file("ggml-base.en.bin");
        SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
        setup_audio();
        setup_overlay();
    }

    void setup_audio() {
        SDL_AudioSpec spec = {16000, AUDIO_F32SYS, 1, 0, 1024, 0, 0, audio_callback, this};
        audio_dev = SDL_OpenAudioDevice(nullptr, 1, &spec, nullptr, 0);
        SDL_PauseAudioDevice(audio_dev, 0);
    }

    void setup_overlay() {
        // Stub: SDL3 window creation later
    }

    static void audio_callback(void *userdata, Uint8 *stream, int len) {
        auto *app = static_cast<TurboTalkText*>(userdata);
        if (!app->is_recording) return;

        float *samples = (float*)stream;
        int sample_count = len / sizeof(float);
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

    void run() {
        printf("TurboTalkText: Ctrl+Shift to toggle, Ctrl+Shift+CapsLock to exit\n");
        while (is_running) {
            if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                if (GetAsyncKeyState(VK_CAPITAL) & 0x8000) is_running = false;
                else {
                    is_recording = !is_recording;
                    printf("Recording %s\n", is_recording ? "ON" : "OFF");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void cleanup() {
        SDL_CloseAudioDevice(audio_dev);
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
