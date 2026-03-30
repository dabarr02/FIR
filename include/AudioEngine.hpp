#pragma once
#include <vector>
#include <atomic>
#include <portaudio.h>

class AudioEngine {
public:
    // Reservamos por defecto 10 segundos de audio a 16kHz
    AudioEngine(size_t bufferSize = 16000 * 30);
    ~AudioEngine();

    bool start();
    void stop();

    // El Background usará esto para extraer datos para la IA
    std::vector<float> getAvailableSamples();
    size_t getQueuedSamplesCount();
    std::vector<float> AudioEngine::getSamples(size_t count);
    void AudioEngine::discardOldAudio(size_t keepLastSamples);

private:
    static int paCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags, void *userData);

    PaStream *stream;
    
    // Memoria pre-asignada (No crece en tiempo de ejecución)
    std::vector<float> ringBuffer;
    size_t capacity;

    // Índices atómicos para comunicación segura entre hilos
    std::atomic<size_t> writeIdx{0};
    std::atomic<size_t> readIdx{0};
};