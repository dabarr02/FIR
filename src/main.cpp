#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <mutex>
#include <shellapi.h>

#pragma warning(push, 0) 
#include "httplib.h"
#include <nlohmann/json.hpp>
#pragma warning(pop)

#include "AudioEngine.hpp"
#include "Transcriber.hpp"
#include "CallsignParser.hpp"
#include "QRZClient.hpp"
#include "TTSManager.hpp"
#include "RadioState.hpp"
#include "WebHandlers.hpp"


// Constantes
const size_t BLOQUE_4S = 16000 * 4;
const size_t UMBRAL_LATENCIA = 16000 * 10; //Dejamos un maximo de 2,5 bloques de margen
const int MAX_CONTEXTO = 500;


TTSManager tts;
RadioState globalState;
QRZClient qrz; 

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

//Comprueba si el texto obtenido tiene alucionaciones mas observadas durantes las pruebas. (Sobre todo salen en silencios)
bool esAlucinacion(std::string texto) {
    // Convertimos a minúsculas para comparar fácil
    std::transform(texto.begin(), texto.end(), texto.begin(), ::tolower);

    const std::vector<std::string> blacklist = { //Alucionanciones mas observadas en las pruebas del modelo
        "and the rest of the world",
        "thank you for watching",
        "subtitles by",
        "watching!",
        "please subscribe",
        "and zero ventura" 
    };

    for (const auto& f : blacklist) {
        if (texto.find(f) != std::string::npos) return true;
    }
    return false;
}

//Carga el fichero .env
void loadEnv(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;

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

//Obtiene los valores de los ajustes del fichero
void loadPersistentSettings() {
    char* u = nullptr; char* p = nullptr; char* c = nullptr; size_t sz = 0;
    _dupenv_s(&u, &sz, "QRZ_USER");
    _dupenv_s(&p, &sz, "QRZ_PASS");
    _dupenv_s(&c, &sz, "MY_CALLSIGN");
    
    std::lock_guard<std::mutex> lock(globalState.mtx);
    globalState.qrzUser = u ? u : "";
    globalState.qrzPass = p ? p : "";
    globalState.myCallsign = c ? c : "";
    globalState.needsConfig = (globalState.qrzUser.empty() || globalState.qrzPass.empty());

    std::cout << "[DEBUG] .env cargado -> USER: " << (u ? std::string(u) : "VACIO") 
              << ", PASS: " << (p ? "***" : "VACIO") 
              << ", CALLSIGN: " << (c ? std::string(c) : "VACIO") << std::endl;

    if (u) free(u); if (p) free(p); if (c) free(c);
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

//Devuelve la fecha en el formato necesario para el informe ADIF
std::string getADIFDate() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d");
    return ss.str();
}

//Devuelve la hora en el formato necesario para el informe ADIF
std::string getADIFTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H%M%S");
    return ss.str();
}

//Inicializacion del sistema 
int system_init(Transcriber& trans, AudioEngine& audio, QRZClient& qrz_instance) {
    if (!trans.init("models/ggml-small.bin")) return 1;
    if (!audio.start()) return 1;
    loadEnv(".env");
    loadPersistentSettings();

    if (!globalState.needsConfig) {
        qrz_instance.init(globalState.qrzUser, globalState.qrzPass);
        if (qrz_instance.login()) {
            std::cout << "[+] QRZ Conectado." << std::endl;
        }
        else {
            std::cout << "[!] Credenciales guardadas invalidas. Esperando configuracion web..." << std::endl;
            std::lock_guard<std::mutex> lock(globalState.mtx);
            globalState.needsConfig = true;
        }
    }
    if (!tts.init()) {
        std::cout << "[!] Error inicializando motor de voz." << std::endl;
        return 1;
    }
    return 0;
}

//=================================== Inicial el servidor web y lanza una ventana en el navegador predeterminado =========================================
void startFrontend() {
    std::cout << "[*] Iniciando Nginx..." << std::endl;
    
    system("taskkill /f /im nginx.exe >nul 2>&1");

    std::string nginxExePath = "..\\..\\tools\\nginx\\nginx.exe";
    std::string nginxDirArgs = "-p ..\\..\\tools\\nginx";

    
    HINSTANCE hInst = ShellExecuteA(NULL, "open", nginxExePath.c_str(), nginxDirArgs.c_str(), NULL, SW_HIDE);

    if ((reinterpret_cast<INT_PTR>(hInst)) <= 32) {
        std::cerr << "[!] Error de lanzamiento: Nginx no se encontro en " << nginxExePath << std::endl;
    }

    std::cout << "[*] Abriendo navegador en http://localhost..." << std::endl;
    // Damos un par de segundos para que el servidor Nginx y el de C++ estén 100% levantados
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Abre http://localhost utilizando el navegador por defecto del sistema
    ShellExecuteA(NULL, "open", "http://localhost", NULL, NULL, SW_SHOWNORMAL);
}

int main() {
    Transcriber transcriber;
    AudioEngine audio;
    CallsignParser parser;
    SetConsoleOutputCP(CP_UTF8);

    
    if (system_init(transcriber, audio, qrz) != 0) {
        audio.stop();
        system("taskkill /f /im nginx.exe >nul 2>&1");
        return 1;
    }
    // Arranca el servidor web desde la librería de handlers
    WebHandlers handlers(globalState, tts, qrz);
    handlers.start_server_detached(8080);
        

    startFrontend();

    std::set<std::string> sessionHistory;
    bool isRunning = true;

    while (isRunning) {
        bool puedeProcesar = false;
        
        // Comprobación segura del estado
        {
            std::lock_guard<std::mutex> lock_loop(globalState.mtx);
            isRunning = globalState.running;
            puedeProcesar = (globalState.isProcessing && !globalState.needsConfig);
        }

        if (!isRunning) break;

        if (!puedeProcesar) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
		int sampleCount=audio.getQueuedSamplesCount();
        if (sampleCount < BLOQUE_4S) { //Bloque demasiado pequeño para procesar
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
		if(sampleCount >=UMBRAL_LATENCIA){ //Demasiado grande, resincronizamos los punteros
			audio.discardOldAudio(BLOQUE_4S);
		}
		
        std::vector<float> pcmData = audio.getSamples(BLOQUE_4S);
        std::string textoActual = transcriber.transcribe(pcmData);

       

        if (textoActual.empty() || textoActual.find("[") != std::string::npos || textoActual.find("(") != std::string::npos || esAlucinacion(textoActual)) continue;

        {
            std::lock_guard<std::mutex> lock_text(globalState.mtx);
            globalState.lastTranscription = textoActual.empty() ? "... Escuchando ..." : textoActual;
        }

        std::cout << "[" << getTimestamp() << "] " << textoActual << std::endl;

        auto candidates = parser.parseAll(textoActual);
        for (const auto& callsign : candidates) {
            if (sessionHistory.find(callsign) == sessionHistory.end()) {
                OperatorData op = qrz.lookup(callsign);
                if (op.found) {
                    setColor(10);
                    std::cout << ">>> CONTACTO: " << op.callsign << " (" << op.name << ")" << std::endl;
                    setColor(7);
                    sessionHistory.insert(callsign);
                    {
                        std::lock_guard<std::mutex> lock_contact(globalState.mtx);
                        globalState.validatedContacts.push_back({
                            {"call", op.callsign}, 
                            {"name", op.name}, 
                            {"loc", op.city + ", " + op.country},
                            { "date", getADIFDate() }, 
							{"time", getADIFTime()}
                            });
                    }
                }
            }
        }
    }

    audio.stop();
    system("taskkill /f /im nginx.exe >nul 2>&1");
    std::cout << "[SYSTEM] Aplicación cerrada correctamente." << std::endl;
    return 0;
}
