#include "Transcriber.hpp"
#include <iostream>

Transcriber::Transcriber() {}
Transcriber::~Transcriber() { if (ctx) whisper_free(ctx); }

bool Transcriber::init(const std::string& modelPath) {
    ctx = whisper_init_from_file(modelPath.c_str());
    return ctx != nullptr;
}

/*
std::string Transcriber::transcribe(const std::vector<float>& pcmData) {
    if (!ctx) return "";

    // 1. Cambiamos a BEAM_SEARCH para máxima precisión (ahora que tenemos GPU)
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);

    params.language = "en"; // "auto" también funcionaría bien con el modelo medium
    params.n_threads = 4;   // En GPU no necesitas tantos hilos de CPU, con 4 sobra
    params.translate = false;

    // 2. Parámetros de Beam Search (Solo disponibles en este modo)
    params.beam_search.beam_size = 6; // Revisa 5 caminos posibles para evitar errores

    // 3. Mejoras de robustez para radio (ruido/estática)
    params.entropy_thold = 2.4f;    // Ayuda a evitar que la IA invente texto en el ruido
    params.no_speech_thold = 0.7f;  // Ignora fragmentos que solo son estática de fondo

    // 4. Prompt Inicial "Pro"
    // El prompt no debe ser una frase explicativa, sino un ejemplo de lo que esperamos oír.
    // Esto "sesga" la probabilidad de las palabras hacia el alfabeto aeronáutico.
    params.initial_prompt =
        "Alpha, Alfa, Bravo, Charlie, Delta, Echo, Foxtrot, Golf, Hotel, India, Juliett, Kilo, Lima, Mike, November, Oscar, Papa, Quebec, Romeo, Sierra, Tango, Uniform, Victor, Whiskey, X-Ray, Yankee, Zulu ";

    if (whisper_full(ctx, params, pcmData.data(), pcmData.size()) != 0) return "Error";

    std::string result = "";
    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char* text = whisper_full_get_segment_text(ctx, i);
        if (text) result += text;
    }
    return result;
}
*/

std::string Transcriber::transcribe(const std::vector<float>& pcmData) {
    if (!ctx) return "";
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    //whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
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
