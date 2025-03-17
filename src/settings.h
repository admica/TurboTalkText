#pragma once

#include <nlohmann/json.hpp>
#include <string>

class Settings {
public:
    static bool load(const std::string& filename = "settings.json");
    
    // Audio settings
    static std::string getAudioDevice() { return audioDevice; }
    static int getSampleRate() { return sampleRate; }
    static float getSilenceThreshold() { return silenceThreshold; }
    static int getSilenceDurationMs() { return silenceDurationMs; }
    
    // Whisper settings
    static std::string getModelPath() { return modelPath; }
    static std::string getLanguage() { return language; }
    static bool getTranslate() { return translate; }
    static int getBeamSize() { return beamSize; }
    static int getThreads() { return threads; }
    
    // Output settings
    static std::string getOutputType() { return outputType; }
    static bool getAddPunctuation() { return addPunctuation; }
    static bool getCapitalizeSentences() { return capitalizeSentences; }
    
private:
    // Audio settings
    static std::string audioDevice;
    static int sampleRate;
    static float silenceThreshold;
    static int silenceDurationMs;
    
    // Whisper settings
    static std::string modelPath;
    static std::string language;
    static bool translate;
    static int beamSize;
    static int threads;
    
    // Output settings
    static std::string outputType;
    static bool addPunctuation;
    static bool capitalizeSentences;
    
    static nlohmann::json config;
}; 