#ifndef SETTINGS_H
#define SETTINGS_H

#include <nlohmann/json.hpp>
#include <string>

class Settings {
public:
    Settings();
    bool load(const std::string& filename);

    // Audio settings
    std::string audioDevice;
    int sampleRate;
    float silenceThreshold;
    int silenceDurationMs;

    // Whisper settings
    std::string modelPath;
    std::string language;
    bool translate;
    int beamSize;
    int threads;

    // Output settings
    std::string outputType;
    bool addPunctuation;
    bool capitalizeSentences;

private:
    nlohmann::json json;
};

#endif // SETTINGS_H
