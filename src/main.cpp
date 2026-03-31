#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <regex>
#include <algorithm> // Para transform
#include <Windows.h> // Necesario para los colores en Windows
#include <cctype>
#include "AudioEngine.hpp"
#include "Transcriber.hpp"
#include "CallsignParser.hpp"
#include <map>
#include <set>


// Función para cambiar el color de la consola
void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// Función de tiempo
std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}


int main() {
    Transcriber transcriber;
    AudioEngine engine;
    CallsignParser parser;
    std::ofstream logFile("registro_radio.txt", std::ios::app);
    const size_t BLOQUE_3S = 16000 * 3;
    const size_t UMBRAL_LATENCIA = 16000 * 10; // Si hay >10s acumulados, saltamos

    // Ruta absoluta para evitar fallos en Visual Studio
    if (!transcriber.init("models/ggml-tiny.bin")) return 1;
    if (!engine.start()) return 1;

    setColor(11); // Cian para el inicio
    std::cout << "--- RadioAccess TFG: Sistema de Vigilancia Activo ---" << std::endl;
    setColor(7);
    std::string contextoGlobal = "";
    const int MAX_CONTEXTO = 100; // Caracteres máximos de memoria

    while (true) {
        size_t acumulado = engine.getQueuedSamplesCount();
        if (acumulado > UMBRAL_LATENCIA) {
            std::cout << "[!] Latencia detectada (" << acumulado / 16000 << "s). Saltando al presente..." << std::endl;
            engine.discardOldAudio(BLOQUE_3S); // Tiramos todo menos los últimos 3s
            continue;
        }
        if (acumulado < BLOQUE_3S) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        std::vector<float> pcmData = engine.getSamples(BLOQUE_3S);
        std::string textoActual = transcriber.transcribe(pcmData);

        if (!textoActual.empty() && textoActual.find("[") == std::string::npos && textoActual.find("(") == std::string::npos) {
            std::string ahora = getTimestamp();
            contextoGlobal += " " + textoActual;
            if (contextoGlobal.length() > MAX_CONTEXTO) {
                contextoGlobal.erase(0, contextoGlobal.length() - MAX_CONTEXTO);
            }
            std::cout << "[" << ahora << "] " << textoActual << std::endl;
            logFile << "[" << ahora << "] " << textoActual << std::endl;

            std::string callsign = parser.parse(contextoGlobal);

            if (!callsign.empty()) {
                setColor(10);
                std::cout << "\n    [!] DETECTADO: " << callsign << " [!]" << std::endl;
                setColor(7);
            }

            logFile.flush();
        }
        
    }

    engine.stop();
    return 0;
}