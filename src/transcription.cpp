#include "transcription.h"
#include "logger.h"
#include <whisper.h>

Transcription::Transcription(const Settings& settings) : settings(settings), ctx(nullptr) {}

Transcription::~Transcription() {
    if (ctx) {
        whisper_free(ctx);
    }
}

bool Transcription::init() {
    // Use the correct structure and function for Whisper.cpp context initialization
    struct whisper_context_params params = whisper_context_default_params();
    ctx = whisper_init_from_file_with_params(settings.modelPath.c_str(), params);
    if (!ctx) {
        Logger::error("Failed to initialize Whisper context");
        return false;
    }
    return true;
}

std::string Transcription::transcribe(const std::vector<float>& audioData) {
    if (!ctx) {
        Logger::error("Whisper context not initialized");
        return "";
    }

    // Set up transcription parameters (adjust based on Whisper.cpp API)
    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.language = settings.language.c_str();
    params.translate = settings.translate;
    params.n_threads = settings.threads;
    // Note: beam_size may need to be set differently; see notes below
    // params.beam_search.beam_size = settings.beamSize;  // Example if using beam search

    if (whisper_full(ctx, params, audioData.data(), audioData.size()) != 0) {
        Logger::error("Transcription failed");
        return "";
    }

    // Extract and return the transcription text
    std::string result;
    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char* text = whisper_full_get_segment_text(ctx, i);
        result += text;
    }
    return result;
}
