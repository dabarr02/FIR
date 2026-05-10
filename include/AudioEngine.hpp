#pragma once
#include <vector>
#include <atomic>
#include <portaudio.h>

class AudioEngine {
public:
    // Reservamos por defecto 30 segundos de audio a 16kHz
    AudioEngine(size_t bufferSize = 16000 * 30);
    ~AudioEngine();

    bool start();
    void stop();

    /***
     * Devuelve la cantidad de muestras actualmente disponibles para procesar
     * @return Cantidad de muestras disponibles en el buffer para ser procesadas
     */
    size_t getQueuedSamplesCount();
    /***
     * Extrae muestras del buffer de audio. El número de muestras extraídas será como máximo 'count', pero puede ser menor si no hay suficientes.
     * @param count Cantidad máxima de muestras a extraer
     * @return Vector con las muestras extraídas     
     * */
    std::vector<float> AudioEngine::getSamples(size_t count);
    /** 
     * Si el buffer se ha llenado demasiado (por ejemplo, si el procesamiento no va lo suficientemente rápido), esta función permite saltar audio antiguo para volver a la sincronización con el tiempo real.
     * @param keepLastSamples Cantidad de muestras recientes que queremos conservar
     * */
    void AudioEngine::discardOldAudio(size_t keepLastSamples);
  

private:
    /***
     * Función de callback para el stream de audio de PortAudio
     * @param inputBuffer Buffer de entrada
     * @param outputBuffer Buffer de salida
     * @param framesPerBuffer Número de frames por buffer
     * @param timeInfo Información de tiempo
     * @param statusFlags Banderas de estado
     * @param userData Datos del usuario
     * @return Código de error
     */
    static int paCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags, void *userData);

    PaStream *stream;
    
    std::vector<float> ringBuffer;
    size_t capacity;
    std::atomic<size_t> writeIdx{0};
    std::atomic<size_t> readIdx{0};
};