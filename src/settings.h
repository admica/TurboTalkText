#ifndef SETTINGS_H
#define SETTINGS_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class Settings {
public:
    Settings();
    bool load(const std::string& filename);

    // Audio settings
    std::string audioDevice;
    int sampleRate;
    float silenceThreshold;
    int silenceDurationMs;
    
    // Speech detection settings (for improved continuous mode)
    struct SpeechDetectionSettings {
        float threshold;
        int minSilenceMs;
        int maxChunkSec;
        int preSpeechBufferMs;
        bool enabled;
    };
    SpeechDetectionSettings speechDetection;

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
    
    // UI settings
    struct UISettings {
        bool enabled;
        std::string style;
        int size;
        float opacity;
        bool minimizeWhenInactive;
    };
    UISettings ui;
    
    // Voice command settings
    struct VoiceCommands {
        std::vector<std::string> mouseMode;
        std::vector<std::string> textMode;
        std::vector<std::string> continuousMode;
        std::vector<std::string> exitContinuousMode;
        std::vector<std::string> keyPress;
    };
    VoiceCommands commands;

private:
    nlohmann::json json;
};

#endif // SETTINGS_H
