#pragma once
#include <string>
#include <vector>
#include "whisper.h"

class Transcriber {
public:
    Transcriber();
    ~Transcriber();
    /***
     * Inicializa el transcriptor con un modelo de voz
     * @param modelPath Ruta al archivo del modelo
     * @return true si se inicializó correctamente, false en caso contrario
     */
    bool init(const std::string& modelPath);
    /***
     * Transcribe un bloque de audio PCM a texto
     * @param pcmData Vector de floats con los datos de audio PCM (16kHz mono)
     * @return El texto transcrito, o "Error" si hubo un problema durante la transcripción
     */
    std::string transcribe(const std::vector<float>& pcmData);
private:
    struct whisper_context* ctx = nullptr;
};