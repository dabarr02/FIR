#include "Transcriber.hpp"
#include "ggml-backend.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>

Transcriber::Transcriber() {}
Transcriber::~Transcriber() { if (ctx) whisper_free(ctx); }

bool Transcriber::init(const std::string& modelPath) {
    const auto backendDirectory = std::filesystem::path(modelPath).parent_path().parent_path();
    ggml_backend_load_all_from_path(backendDirectory.string().c_str());

    whisper_context_params contextParams = whisper_context_default_params();
    const char* useGpu = std::getenv("FIR_USE_GPU");
    contextParams.use_gpu = useGpu != nullptr && std::string(useGpu) == "1";
    ctx = whisper_init_from_file_with_params(modelPath.c_str(), contextParams);
    return ctx != nullptr;
}

std::string Transcriber::transcribe(const std::vector<float>& pcmData) {
    if (!ctx) return "";
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    params.language = "en";
    params.n_threads = 8;
    params.translate = false;
    params.initial_prompt = "Alpha, Alfa, Bravo, Charlie, Delta, Echo, Foxtrot, Golf, Hotel, India, Juliett, Kilo, Lima, Mike, November, Oscar, Papa, Quebec, Romeo, Sierra, Tango, Uniform, Victor, Whiskey, X-Ray, Yankee, Zulu";

    if (whisper_full(ctx, params, pcmData.data(), pcmData.size()) != 0) return "Error";

    std::string result = "";
    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        result += whisper_full_get_segment_text(ctx, i);
    }
    return result;
}
