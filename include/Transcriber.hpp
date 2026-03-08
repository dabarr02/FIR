#pragma once
#include <string>
#include <vector>
#include "whisper.h"

class Transcriber {
public:
    Transcriber();
    ~Transcriber();
    bool init(const std::string& modelPath);
    std::string transcribe(const std::vector<float>& pcmData);
private:
    struct whisper_context* ctx = nullptr;
};