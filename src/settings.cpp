#include "settings.h"
#include "logger.h"
#include <fstream>
#include <iostream>

// Initialize static variables with default values
std::string Settings::audioDevice = "default";
int Settings::sampleRate = 16000;
float Settings::silenceThreshold = 0.001f;
int Settings::silenceDurationMs = 1000;

std::string Settings::modelPath = "whisper.cpp/models/ggml-base.en.bin";
std::string Settings::language = "en";
bool Settings::translate = false;
int Settings::beamSize = 10;
int Settings::threads = 4;

std::string Settings::outputType = "keyboard";
bool Settings::addPunctuation = true;
bool Settings::capitalizeSentences = true;

nlohmann::json Settings::config;

bool Settings::load(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open settings file: " << filename << std::endl;
            return false;
        }
        
        config = nlohmann::json::parse(file);
        
        // Load audio settings
        if (config.contains("audio")) {
            const auto& audio = config["audio"];
            if (audio.contains("device")) audioDevice = audio["device"].get<std::string>();
            if (audio.contains("sample_rate")) sampleRate = audio["sample_rate"].get<int>();
            if (audio.contains("silence_threshold")) silenceThreshold = audio["silence_threshold"].get<float>();
            if (audio.contains("silence_duration_ms")) silenceDurationMs = audio["silence_duration_ms"].get<int>();
        }
        
        // Load whisper settings
        if (config.contains("whisper")) {
            const auto& whisper = config["whisper"];
            if (whisper.contains("model_path")) modelPath = whisper["model_path"].get<std::string>();
            if (whisper.contains("language")) language = whisper["language"].get<std::string>();
            if (whisper.contains("translate")) translate = whisper["translate"].get<bool>();
            if (whisper.contains("beam_size")) beamSize = whisper["beam_size"].get<int>();
            if (whisper.contains("threads")) threads = whisper["threads"].get<int>();
        }
        
        // Load output settings
        if (config.contains("output")) {
            const auto& output = config["output"];
            if (output.contains("type")) outputType = output["type"].get<std::string>();
            if (output.contains("add_punctuation")) addPunctuation = output["add_punctuation"].get<bool>();
            if (output.contains("capitalize_sentences")) capitalizeSentences = output["capitalize_sentences"].get<bool>();
        }
        
        std::cout << "Settings loaded successfully from " << filename << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse settings: " << e.what() << std::endl;
        return false;
    }
} 