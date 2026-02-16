#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include "AudioEngine.hpp"
#include "WavHeader.hpp"

// La función de guardado se mantiene igual
void saveWav(const std::string& filename, const std::vector<float>& data) {
    std::ofstream file(filename, std::ios::binary);
    WavHeader header;
    header.subchunk2Size = static_cast<uint32_t>(data.size() * sizeof(float));
    header.chunkSize = 36 + header.subchunk2Size;

    file.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
    file.write(reinterpret_cast<const char*>(data.data()), header.subchunk2Size);
}

int main() {
    AudioEngine engine; // El constructor reserva la memoria (Pre-allocation)
    std::vector<float> grabaciónCompleta;
    
    // Mostramos info de dispositivos (opcional, para depurar)
    Pa_Initialize(); // Necesario para consultar dispositivos antes de start()
    int defaultInput = Pa_GetDefaultInputDevice();
    if (defaultInput != paNoDevice) {
        std::cout << ">>> Capturando desde: " << Pa_GetDeviceInfo(defaultInput)->name << std::endl;
    }

    std::cout << "Grabando con Buffer Circular (Real-Time Safe)..." << std::endl;

    if (engine.start()) {
        // Grabamos durante 5 segundos, pero extrayendo datos segundo a segundo
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Background: Consumimos lo que el Foreground ha escrito en el Ring Buffer
            std::vector<float> nuevasMuestras = engine.getAvailableSamples();
            
            // Las acumulamos en nuestro vector para el archivo final
            grabaciónCompleta.insert(grabaciónCompleta.end(), nuevasMuestras.begin(), nuevasMuestras.end());
            
            std::cout << "Segundo " << i + 1 << ": capturadas " << nuevasMuestras.size() << " muestras nuevas." << std::endl;
        }

        engine.stop();
        
        // Guardamos el resultado acumulado
        if (!grabaciónCompleta.empty()) {
            saveWav("prueba_circular.wav", grabaciónCompleta);
            std::cout << "¡Hecho! Archivo 'prueba_circular.wav' generado con " << grabaciónCompleta.size() << " muestras." << std::endl;
        } else {
            std::cout << "No se capturaron muestras. Revisa la configuración del micro/cable virtual." << std::endl;
        }
    } else {
        std::cerr << "Error al iniciar el motor de audio" << std::endl;
    }

    return 0;
}