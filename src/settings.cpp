#include "settings.h"
#include <fstream>
#include <iostream>

Settings::Settings() {
    // Default voice commands
    commands.mouseMode = {
        "jarvis move the mouse", "jarvis move mouse", "move the mouse", 
        "mouse mode", "switch to mouse mode", "control the mouse"
    };
    
    commands.textMode = {
        "jarvis stop", "stop mouse", "exit mouse mode", "back to text", 
        "text mode", "typing mode", "keyboard mode"
    };
    
    commands.continuousMode = {
        "jarvis listen continuously", "continuous mode", "listen continuously", 
        "always listen", "continuous listening", "keep listening"
    };
    
    commands.exitContinuousMode = {
        "jarvis stop listening", "stop continuous", "exit continuous mode",
        "stop continuous listening", "back to normal"
    };
}

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

    // Load voice commands if they exist in the JSON
    if (json.contains("voice_commands")) {
        // Load mouse mode commands
        if (json["voice_commands"].contains("mouse_mode")) {
            commands.mouseMode = json["voice_commands"]["mouse_mode"].get<std::vector<std::string>>();
        }
        
        // Load text mode commands
        if (json["voice_commands"].contains("text_mode")) {
            commands.textMode = json["voice_commands"]["text_mode"].get<std::vector<std::string>>();
        }
        
        // Load continuous mode commands
        if (json["voice_commands"].contains("continuous_mode")) {
            commands.continuousMode = json["voice_commands"]["continuous_mode"].get<std::vector<std::string>>();
        }
        
        // Load exit continuous mode commands
        if (json["voice_commands"].contains("exit_continuous_mode")) {
            commands.exitContinuousMode = json["voice_commands"]["exit_continuous_mode"].get<std::vector<std::string>>();
        }
    }

    return true;
}
