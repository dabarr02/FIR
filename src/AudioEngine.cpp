#include "AudioEngine.hpp"
#include <iostream>
#include <algorithm>

AudioEngine::AudioEngine(size_t bufferSize) : stream(nullptr), capacity(bufferSize) {
    ringBuffer.resize(capacity, 0.0f);
    Pa_Initialize();
}

AudioEngine::~AudioEngine() {
    stop();
    Pa_Terminate();
}

int AudioEngine::paCallback(const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData) {
    AudioEngine* engine = static_cast<AudioEngine*>(userData);
    const float* in = static_cast<const float*>(inputBuffer);

    if (in != nullptr) {
        size_t currentWrite = engine->writeIdx.load(std::memory_order_relaxed);
        
        for (unsigned int i = 0; i < framesPerBuffer; i++) {
            engine->ringBuffer[currentWrite] = in[i];
            // Aritmética modular para circularidad
            currentWrite = (currentWrite + 1) % engine->capacity;
        }
        
        // Actualizamos el índice de escritura de forma atómica
        engine->writeIdx.store(currentWrite, std::memory_order_release);
    }
    return paContinue;
}

// Función para que el "Background" lea los datos acumulados
std::vector<float> AudioEngine::getAvailableSamples() {
    size_t currentWrite = writeIdx.load(std::memory_order_acquire);
    size_t currentRead = readIdx.load(std::memory_order_relaxed);
    
    std::vector<float> samples;
    
    // Calculamos cuántas muestras hay nuevas desde la última lectura
    while (currentRead != currentWrite) {
        samples.push_back(ringBuffer[currentRead]);
        currentRead = (currentRead + 1) % capacity;
    }
    
    readIdx.store(currentRead, std::memory_order_relaxed);
    return samples;
}

bool AudioEngine::start() {
    PaError err = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, 16000, 512, paCallback, this);
    if (err != paNoError) return false;
    return Pa_StartStream(stream) == paNoError;
}

void AudioEngine::stop() {
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        stream = nullptr;
    }
}