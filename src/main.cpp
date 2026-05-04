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


// Constantes
const size_t BLOQUE_3S = 16000 * 4;
const size_t UMBRAL_LATENCIA = 16000 * 10;
const int MAX_CONTEXTO = 500;

struct RadioState {
    std::string lastTranscription;
    std::vector<nlohmann::json> validatedContacts;
    bool isProcessing = false;
    bool running = true;
    bool needsConfig = true;
    std::mutex mtx;

    std::string qrzUser;
    std::string qrzPass;
    std::string myCallsign;
    std::string currentBand = "2M";
};

TTSManager tts;
RadioState globalState;
QRZClient qrz; // Cliente global

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool esAlucinacion(std::string texto) {
    // Convertimos a minúsculas para comparar fácil
    std::transform(texto.begin(), texto.end(), texto.begin(), ::tolower);

    const std::vector<std::string> blacklist = {
        "and the rest of the world",
        "thank you for watching",
        "subtitles by",
        "watching!",
        "please subscribe",
        "and zero ventura" // He visto que te sale algo parecido
    };

    for (const auto& f : blacklist) {
        if (texto.find(f) != std::string::npos) return true;
    }
    return false;
}


void updateEnvFile(const std::string& user, const std::string& pass, const std::string& call) {
    std::ofstream envFile(".env", std::ios::trunc);
    if (envFile.is_open()) {
        envFile << "QRZ_USER=" << user << "\n";
        envFile << "QRZ_PASS=" << pass << "\n";
        envFile << "MY_CALLSIGN=" << call << "\n";
        envFile.close();

        _putenv_s("QRZ_USER", user.c_str());
        _putenv_s("QRZ_PASS", pass.c_str());
        _putenv_s("MY_CALLSIGN", call.c_str());
    }
}

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

void web_init() {
    std::thread serverThread([]() {
        CoInitialize(NULL);
        httplib::Server svr;

        svr.Get("/api/report", [](const httplib::Request&, httplib::Response& res) {
            std::stringstream adif;
            {
                std::lock_guard<std::mutex> lock_report(globalState.mtx);
                adif << "ADIF Export from RadioAccess\n<ADIF_VER:5>3.1.4\n<PROGRAMID:11>RadioAccess\n";
                adif << "<STATION_CALLSIGN:" << globalState.myCallsign.length() << ">" << globalState.myCallsign << "\n<EOH>\n\n";

                for (const auto& c : globalState.validatedContacts) {
                    std::string call = c["call"], name = c["name"], loc = c["loc"];
                    adif << "<CALL:" << call.length() << ">" << call << " <NAME:" << name.length() << ">" << name 
                         << " <QTH:" << loc.length() << ">" << loc << " <BAND:" << globalState.currentBand.length() << ">" << globalState.currentBand 
                         << " <MODE:2>FM <EOR>\n";
                }
            }
            res.set_content(adif.str(), "text/plain");
            res.set_header("Content-Disposition", "attachment; filename=logbook_radio.adi");
        });

        svr.Get("/api/status", [](const httplib::Request&, httplib::Response& res) {
            nlohmann::json j;
            {
                std::lock_guard<std::mutex> lock_status(globalState.mtx);
                j["transcription"] = globalState.lastTranscription;
                j["contacts"] = globalState.validatedContacts;
                j["isProcessing"] = globalState.isProcessing;
                j["needsConfig"] = globalState.needsConfig;
            }
            res.set_content(j.dump(), "application/json");
        });

        svr.Get("/api/settings", [](const httplib::Request&, httplib::Response& res) {
            nlohmann::json j;
            {
                std::lock_guard<std::mutex> lock_settings(globalState.mtx);
                j["user"] = globalState.qrzUser; j["pass"] = globalState.qrzPass;
                j["myCall"] = globalState.myCallsign; j["band"] = globalState.currentBand;
                j["needsConfig"] = globalState.needsConfig;
            }
            res.set_content(j.dump(), "application/json");
        });

        svr.Post("/api/settings", [](const httplib::Request& req, httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                std::string u = j.at("user"), p = j.at("pass"), c = j.at("myCall"), b = j.at("band");

                if (j.contains("deviceId")) {
                    int devId = j.at("deviceId").get<int>();
                    tts.setOutputDevice(devId);
                    std::cout << "[SYSTEM] Salida de audio cambiada al dispositivo ID: " << devId << std::endl;
                }
                
                qrz.init(u, p);
                if (qrz.login()) {
                    {
                        std::lock_guard<std::mutex> lock_save(globalState.mtx);
                        globalState.qrzUser = u; 
                        globalState.qrzPass = p;
                        globalState.myCallsign = c; 
                        globalState.currentBand = b;
                        globalState.needsConfig = false; 
                        
                    }
                    updateEnvFile(u, p, c);
                    res.set_content("{\"status\":\"ok\"}", "application/json");
                } else {
                    res.status = 401;
                    res.set_content("{\"error\":\"QRZ Login failed\"}", "application/json");
                }
            } catch (...) { res.status = 400; }
        });

        svr.Post("/api/toggle", [](const httplib::Request&, httplib::Response& res) {
            std::lock_guard<std::mutex> lock_toggle(globalState.mtx);
            if (globalState.needsConfig) {
                res.status = 403;
                res.set_content("config_required", "text/plain");
                return;
            }
            globalState.isProcessing = !globalState.isProcessing;
            std::cout << "[SYSTEM] Motor de radio: " << (globalState.isProcessing ? "ACTIVO" : "PAUSADO") << std::endl;
            res.set_content(globalState.isProcessing ? "true" : "false", "text/plain");
        });

        svr.Post("/api/shutdown", [](const httplib::Request&, httplib::Response& res) {
            { std::lock_guard<std::mutex> lock_stop(globalState.mtx); globalState.running = false; }
            res.set_content("OK", "text/plain");
        });

        svr.Post("/api/transmit", [](const httplib::Request& req, httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                std::string textoParaHablar = j.at("text").get<std::string>();

                std::cout << "[TX] Sintetizando: " << textoParaHablar << std::endl;
                tts.speak(textoParaHablar);

                res.set_content("{\"status\":\"ok\"}", "application/json");
            }
            catch (...) { res.status = 400; }
            });
        svr.Get("/api/devices", [](const httplib::Request&, httplib::Response& res) {
            auto devices = tts.getOutputDevices();
            std::cout << "[DEBUG] Web solicitó dispositivos. Encontrados: " << devices.size() << std::endl;
            nlohmann::json j = nlohmann::json::array();
            for (const auto& d : devices) {
                j.push_back({ {"id", d.id}, {"name", d.name} });
				std::cout << "[DEBUG] Dispositivo encontrado: ID=" << d.id << ", Name=\"" << d.name << "\"" << std::endl;
            }
            res.set_content(j.dump(), "application/json");
            });

        svr.Post("/api/lookup", [](const httplib::Request& req, httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                std::string call = j.at("call").get<std::string>();

                // Realizamos la consulta real a QRZ usando el cliente existente
                OperatorData op = qrz.lookup(call);

                if (op.found) {
                    std::lock_guard<std::mutex> lock(globalState.mtx);

                    // Añadimos al historial para que salga en el ADIF
                    globalState.validatedContacts.push_back({
                        {"call", op.callsign},
                        {"name", op.name},
                        {"loc", op.city + ", " + op.country}
                        });

                    res.set_content("{\"status\":\"ok\"}", "application/json");
                }
                else {
                    res.status = 404; // No encontrado en QRZ
                }
            }
            catch (...) {
                res.status = 400;
            }
            });

        svr.listen("0.0.0.0", 8080);
    });
    serverThread.detach();
}

void startFrontend() {
    std::cout << "[*] Iniciando Nginx..." << std::endl;
    // Terminamos cualquier instancia previa que se haya quedado colgada
    system("taskkill /f /im nginx.exe >nul 2>&1");

    // Construir la ruta al ejecutable de nginx y su directorio base
    // Asumiendo que el .exe está en "build/Release/" o "build/Debug/", 
    // necesitamos retroceder 2 carpetas.
    std::string nginxExePath = "..\\..\\tools\\nginx\\nginx.exe";
    std::string nginxDirArgs = "-p ..\\..\\tools\\nginx";

    // Usamos ShellExecute para lanzar nginx en modo oculto (SW_HIDE)
    // sin bloquear el flujo principal de nuestro C++
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
    web_init();

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

        if (audio.getQueuedSamplesCount() < BLOQUE_3S) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::vector<float> pcmData = audio.getSamples(BLOQUE_3S);
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
                        globalState.validatedContacts.push_back({ {"call", op.callsign}, {"name", op.name}, {"loc", op.city + ", " + op.country} });
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