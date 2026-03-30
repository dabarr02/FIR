#include "Transcriber.hpp"
#include <iostream>

Transcriber::Transcriber() {}
Transcriber::~Transcriber() { if (ctx) whisper_free(ctx); }

bool Transcriber::init(const std::string& modelPath) {
    ctx = whisper_init_from_file(modelPath.c_str());
    return ctx != nullptr;
}

std::string Transcriber::transcribe(const std::vector<float>& pcmData) {
    if (!ctx) return "";
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.language = "en";
    params.n_threads = 8;
    params.translate = false;
    params.initial_prompt = "Amateur radio callsigns: Alfa, Bravo, Charlie, Delta, Echo, Foxtrot, Golf, Hotel, India, Juliett, Kilo, Lima, Mike, November, Oscar, Papa, Quebec, Romeo, Sierra, Tango, Uniform, Victor, Whiskey, X-Ray, Yankee, Zulu";

    if (whisper_full(ctx, params, pcmData.data(), pcmData.size()) != 0) return "Error";

    std::string result = "";
    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        result += whisper_full_get_segment_text(ctx, i);
    }
    return result;
}