#include "settings.h"
#include <fstream>
#include <iostream>

Settings::Settings() {}

bool Settings::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open settings file: " << filename << std::endl;
        return false;
    }
    file >> json;
    file.close();

    // Load audio settings
    audioDevice = json["audio"]["device"].get<std::string>();
    sampleRate = json["audio"]["sample_rate"].get<int>();
    silenceThreshold = json["audio"]["silence_threshold"].get<float>();
    silenceDurationMs = json["audio"]["silence_duration_ms"].get<int>();

    // Load whisper settings
    modelPath = json["whisper"]["model_path"].get<std::string>();
    language = json["whisper"]["language"].get<std::string>();
    translate = json["whisper"]["translate"].get<bool>();
    beamSize = json["whisper"]["beam_size"].get<int>();
    threads = json["whisper"]["threads"].get<int>();

    // Load output settings
    outputType = json["output"]["type"].get<std::string>();
    addPunctuation = json["output"]["add_punctuation"].get<bool>();
    capitalizeSentences = json["output"]["capitalize_sentences"].get<bool>();

    return true;
}
