#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include "AudioEngine.hpp"
#include "Transcriber.hpp"

int main() {
    Transcriber transcriber;
    AudioEngine engine;
    int tiempoTotalPrueba = 30; // Segundos que durará la prueba
    int intervaloCaptura = 3;   // Procesamos de 3 en 3 segundos

    std::cout << "--- RadioAccess TFG: Prueba Temporizada (" << tiempoTotalPrueba << "s) ---" << std::endl;

    if (!transcriber.init("models/ggml-base.bin")) return 1;
    if (!engine.start()) return 1;

    // Calculamos cuántas veces debe ejecutarse el bucle
    int iteraciones = tiempoTotalPrueba / intervaloCaptura;

    for (int i = 0; i < iteraciones; ++i) {
        std::cout << "\n[Ciclo " << i + 1 << "/" << iteraciones << "] Capturando audio..." << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(intervaloCaptura));

        // Obtenemos solo lo último capturado
        std::vector<float> pcmData = engine.getAvailableSamples();
        
        if (!pcmData.empty()) {
            // --- TRUCO TFG: Si hay demasiado audio acumulado, solo nos quedamos con los últimos 3 segundos ---
            // Esto evita el efecto "bola de nieve" si la IA es lenta.
            size_t maxSamples = 16000 * intervaloCaptura;
            if (pcmData.size() > maxSamples) {
                pcmData.erase(pcmData.begin(), pcmData.end() - maxSamples);
            }

            std::cout << " > Procesando " << pcmData.size() << " muestras..." << std::flush;
            
            auto start = std::chrono::high_resolution_clock::now();
            std::string texto = transcriber.transcribe(pcmData);
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double> diff = end - start;
            std::cout << " (Inferencia: " << diff.count() << "s)" << std::endl;
            std::cout << " [IA]: " << (texto.empty() ? "..." : texto) << std::endl;
        }
    }

    std::cout << "\n--- Finalizando programa y liberando recursos ---" << std::endl;
    engine.stop();
    // PortAudio y Whisper se cierran automáticamente por los destructores
    
    std::cout << "Programa terminado correctamente." << std::endl;
    return 0;
}