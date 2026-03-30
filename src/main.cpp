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
#include <map>
#include <set>

std::string ultimoCallsign = "";

std::string procesarFonetico(std::string texto) {
    // 1. Diccionario de mapeo
    static const std::map<std::string, char> radioMapping = {
        {"ALPHA", 'A'}, {"ALFA", 'A'}, {"BRAVO", 'B'}, {"CHARLIE", 'C'}, {"DELTA", 'D'},
        {"ECHO", 'E'}, {"FOXTROT", 'F'}, {"GOLF", 'G'}, {"HOTEL", 'H'}, {"INDIA", 'I'},
        {"JULIETT", 'J'}, {"JULIET", 'J'}, {"KILO", 'K'}, {"LIMA", 'L'}, {"MIKE", 'M'},
        {"NOVEMBER", 'N'}, {"OSCAR", 'O'}, {"PAPA", 'P'}, {"QUEBEC", 'Q'}, {"ROMEO", 'R'},
        {"SIERRA", 'S'}, {"TANGO", 'T'}, {"UNIFORM", 'U'}, {"VICTOR", 'V'}, {"WHISKEY", 'W'},
        {"XRAY", 'X'}, {"X-RAY", 'X'}, {"YANKEE", 'Y'}, {"ZULU", 'Z'},
        // Números en texto
        {"ZERO", '0'}, {"ONE", '1'}, {"TWO", '2'}, {"THREE", '3'}, {"FOUR", '4'},
        {"FIVE", '5'}, {"FIFE", '5'}, {"SIX", '6'}, {"SEVEN", '7'}, {"EIGHT", '8'}, {"NINE", '9'}
    };

    std::replace(texto.begin(), texto.end(), '-', ' ');
    std::replace(texto.begin(), texto.end(), ',', ' ');

    std::stringstream ss(texto);
    std::string palabra, resultado;

    while (ss >> palabra) {
        // Limpieza de puntuación
        palabra.erase(std::remove_if(palabra.begin(), palabra.end(), [](char c) {
            return !std::isalnum(c);
            }), palabra.end());

        for (char& c : palabra) c = toupper(c);
        if (palabra.empty()) continue;

        // CASO A: Es un número literal (ej: "5")
        if (isdigit(palabra[0])) {
            resultado += palabra;
        }
        // CASO B: Es una palabra del alfabeto radiofónico (ej: "INDIA")
        else if (radioMapping.count(palabra)) {
            resultado += radioMapping.at(palabra);
        }
        // CUALQUIER OTRA PALABRA ("Hello", "Thank", "Radio") SE IGNORA
    }
    return resultado;
}




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

void detectarCallsigns(const std::string& contexto, std::ofstream& log) {
    std::string filtrado = procesarFonetico(contexto);

    // Regex mejorada: busca prefijo (1-2 letras/núms), un número obligatorio, y sufijo (1-3 letras)
    // También permitimos que el indicativo esté en medio de los reportes de señal (59)
    std::regex pattern("([A-Z0-9]{1,2}[0-9][A-Z]{1,3})");
    std::smatch match;

    if (std::regex_search(filtrado, match, pattern)) {
        std::string detectado = match[0];

        if (detectado != ultimoCallsign) {
            setColor(10); // Verde
            std::cout << "\n    [!] CONTACTO IDENTIFICADO: " << detectado << " [!]" << std::endl;
            setColor(7);
            log << "[" << getTimestamp() << "] [CALLSIGN]: " << detectado << std::endl;
            ultimoCallsign = detectado;
        }
    }
}

int main() {
    Transcriber transcriber;
    AudioEngine engine;
    std::ofstream logFile("registro_radio.txt", std::ios::app);
    const size_t BLOQUE_3S = 16000 * 3;
    const size_t UMBRAL_LATENCIA = 16000 * 10; // Si hay >10s acumulados, saltamos

    // Ruta absoluta para evitar fallos en Visual Studio
    if (!transcriber.init("models/ggml-base.bin")) return 1;
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

            detectarCallsigns(contextoGlobal, logFile);
            logFile.flush();
        }
        
    }

    engine.stop();
    return 0;
}