#ifndef TRANSCRIPTION_H
#define TRANSCRIPTION_H

#include "settings.h"
#include <whisper.h>
#include <vector>
#include <string>

class Transcription {
public:
    Transcription(const Settings& settings);
    ~Transcription();
    bool init();
    std::string transcribe(const std::vector<float>& audioData);

private:
    const Settings& settings;
    whisper_context* ctx;
};

#endif // TRANSCRIPTION_H
