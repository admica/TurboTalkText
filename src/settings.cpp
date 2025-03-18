#include "settings.h"
#include "logger.h"
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
    
    commands.keyPress = {
        "jarvis press", "jarvis push", "jarvis key"
    };
    
    // Default speech detection settings
    speechDetection.threshold = 0.02f;
    speechDetection.minSilenceMs = 1000;
    speechDetection.maxChunkSec = 15;
    speechDetection.preSpeechBufferMs = 500;
    speechDetection.enabled = true;
    
    // Default UI settings
    ui.enabled = true;
    ui.style = "circle";
    ui.size = 200;
    ui.opacity = 0.8f;
    ui.minimizeWhenInactive = true;
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
    
    // Load speech detection settings if they exist
    if (json.contains("speech_detection")) {
        if (json["speech_detection"].contains("threshold")) {
            speechDetection.threshold = json["speech_detection"]["threshold"].get<float>();
        }
        
        if (json["speech_detection"].contains("min_silence_ms")) {
            speechDetection.minSilenceMs = json["speech_detection"]["min_silence_ms"].get<int>();
        }
        
        if (json["speech_detection"].contains("max_chunk_sec")) {
            speechDetection.maxChunkSec = json["speech_detection"]["max_chunk_sec"].get<int>();
        }
        
        if (json["speech_detection"].contains("pre_speech_buffer_ms")) {
            speechDetection.preSpeechBufferMs = json["speech_detection"]["pre_speech_buffer_ms"].get<int>();
        }
        
        if (json["speech_detection"].contains("enabled")) {
            speechDetection.enabled = json["speech_detection"]["enabled"].get<bool>();
        }
    } else {
        // If speech detection settings don't exist, log it and keep using defaults
        Logger::info("Speech detection settings not found in config, using defaults");
        Logger::info("Default speech threshold: " + std::to_string(speechDetection.threshold));
        Logger::info("Default min silence: " + std::to_string(speechDetection.minSilenceMs) + "ms");
        Logger::info("Default max chunk: " + std::to_string(speechDetection.maxChunkSec) + "s");
    }

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
    
    // Load UI settings if they exist
    if (json.contains("ui")) {
        if (json["ui"].contains("enabled")) {
            ui.enabled = json["ui"]["enabled"].get<bool>();
        }
        
        if (json["ui"].contains("style")) {
            ui.style = json["ui"]["style"].get<std::string>();
        }
        
        if (json["ui"].contains("size")) {
            ui.size = json["ui"]["size"].get<int>();
        }
        
        if (json["ui"].contains("opacity")) {
            ui.opacity = json["ui"]["opacity"].get<float>();
        }
        
        if (json["ui"].contains("minimize_when_inactive")) {
            ui.minimizeWhenInactive = json["ui"]["minimize_when_inactive"].get<bool>();
        }
    }

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
        
        // Load key press commands
        if (json["voice_commands"].contains("key_press")) {
            commands.keyPress = json["voice_commands"]["key_press"].get<std::vector<std::string>>();
        }
    }

    return true;
}
