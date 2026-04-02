#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <regex>
#include <algorithm>
#include <Windows.h>
#include <cctype>
#include <map>
#include <set>

#include "AudioEngine.hpp"
#include "Transcriber.hpp"
#include "CallsignParser.hpp"
#include "QRZClient.hpp"

// Constants
const size_t BLOQUE_3S = 16000 * 4;
const size_t UMBRAL_LATENCIA = 16000 * 10;
const int MAX_CONTEXTO = 500;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void loadEnv(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[!] Advertencia: No se pudo abrir el archivo .env en " << path << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);
            _putenv_s(key.c_str(), value.c_str());
        }
    }
    file.close();
}

void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    SetConsoleOutputCP(CP_UTF8);
    
    // Initialize components
    Transcriber transcriber;
    AudioEngine engine;
    CallsignParser parser;
    std::ofstream logFile("registro_radio.txt", std::ios::app);

    if (!transcriber.init("models/ggml-small.bin")) return 1;
    if (!engine.start()) return 1;

    // Load environment variables
    loadEnv(".env");

    char* user_ptr = nullptr;
    char* pass_ptr = nullptr;
    size_t sz = 0;

    _dupenv_s(&user_ptr, &sz, "QRZ_USER");
    _dupenv_s(&pass_ptr, &sz, "QRZ_PASS");

    if (user_ptr == nullptr || pass_ptr == nullptr) {
        std::cerr << "Error: No se han configurado QRZ_USER o QRZ_PASS en el .env" << std::endl;
        return 1;
    }

    std::string qrz_user(user_ptr);
    std::string qrz_pass(pass_ptr);

    free(user_ptr);
    free(pass_ptr);

    QRZClient qrz(qrz_user, qrz_pass);

    // QRZ Login test
    std::cout << "[*] Conectando a QRZ XML Service..." << std::endl;
    if (!qrz.login()) {
        std::cerr << "[!] Error de Login en QRZ. Revisa tu .env y conexión." << std::endl;
        return 1;
    }

    std::cout << "[+] Login exitoso. Session Key obtenida." << std::endl;
    std::cout << "[*] Verificando base de datos (Test: EA4IAX)..." << std::endl;

    OperatorData testOp = qrz.lookup("EA4IAX");
    if (testOp.found) {
        std::cout << "\n>>> TEST EXITOSO <<<" << std::endl;
        std::cout << "Nombre:   " << testOp.name << std::endl;
        std::cout << "País:     " << testOp.country << std::endl;
        std::cout << "Ciudad:   " << testOp.city << "\n" << std::endl;
    } else {
        std::cout << "\n[!] El test ha fallado. Revisa el DEBUG XML de arriba.\n" << std::endl;
    }

    // Main loop
    setColor(11);
    std::cout << "--- RadioAccess TFG: Sistema Activo ---" << std::endl;
    setColor(7);

    std::string contextoGlobal = "";
    std::set<std::string> sessionHistory;

    while (true) {
        size_t acumulado = engine.getQueuedSamplesCount();

        // Manage latency
        if (acumulado > UMBRAL_LATENCIA) {
            std::cout << "[!] Latencia detectada (" << acumulado / 16000 << "s). Saltando al presente..." << std::endl;
            engine.discardOldAudio(BLOQUE_3S);
            continue;
        }

        if (acumulado < BLOQUE_3S) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Transcription
        std::vector<float> pcmData = engine.getSamples(BLOQUE_3S);
        std::string textoActual = transcriber.transcribe(pcmData);

        // Filter noise
        if (textoActual.empty() || textoActual.find("[") != std::string::npos || textoActual.find("(") != std::string::npos) {
            std::cout << "[DEBUG] Silencio detectado" << std::endl;
            continue;
        }

        std::string ahora = getTimestamp();
        contextoGlobal += " " + textoActual;

        if (contextoGlobal.length() > MAX_CONTEXTO) {
            contextoGlobal.erase(0, contextoGlobal.length() - 100);
        }

        std::cout << "[" << ahora << "] " << textoActual << std::endl;
        logFile << "[" << ahora << "] " << textoActual << std::endl;

        // Parse and validate callsign
        std::string callsign = parser.parse(textoActual);

        if (!callsign.empty() && sessionHistory.find(callsign) == sessionHistory.end()) {
            OperatorData op = qrz.lookup(callsign);

            if (op.found) {
                setColor(10);
                std::cout << "\n==========================================" << std::endl;
                std::cout << "  [!] CONTACTO VALIDADO: " << op.callsign << std::endl;
                std::cout << "  NOMBRE:    " << op.name << std::endl;
                std::cout << "  UBICACIÓN: " << op.city << " (" << op.country << ")" << std::endl;
                std::cout << "==========================================\n" << std::endl;
                setColor(7);

                logFile << ">>> CONTACTO VALIDADO: " << op.callsign << " - " << op.name << " (" << op.country << ")" << std::endl;
                sessionHistory.insert(callsign);
            } else {
                setColor(10);
                std::cout << "[DEBUG] Candidato descartado por QRZ: " << callsign << std::endl;
                setColor(7);
            }
        }

        logFile.flush();
    }
}
