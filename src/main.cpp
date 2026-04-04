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

#pragma warning(push, 0) 
#include "httplib.h"
#include <nlohmann/json.hpp>
#pragma warning(pop)


#include "AudioEngine.hpp"
#include "Transcriber.hpp"
#include "CallsignParser.hpp"
#include "QRZClient.hpp"

// Constants
const size_t BLOQUE_3S = 16000 * 4;
const size_t UMBRAL_LATENCIA = 16000 * 10;
const int MAX_CONTEXTO = 500;


// Estructura para compartir datos entre la Radio y la Web
struct RadioState {
    std::string lastTranscription;
    std::vector<nlohmann::json> validatedContacts;
    bool isProcessing = false;
    bool running = true;
    std::mutex mtx; 
};


RadioState globalState;

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

int qrz_test(QRZClient& qrz) {
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
    }
    else {
      
		return 1;
    }
	return 0;
}

int system_init(Transcriber& trans,AudioEngine& audio, QRZClient& qrz ) {

    //=======================INIT audio y transcriptor===============

    if (!trans.init("models/ggml-small.bin")) return 1;
    if (!audio.start()) return 1;
	

    //============ Load environment variables=======================
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

	//=======================INIT QRZ=======================
    qrz.init(qrz_user, qrz_pass);
	if(qrz_test(qrz)){
		std::cerr << "[!] Error en la conexión o consulta a QRZ. Verifica tus credenciales y conexión." << std::endl;
        return 1;
    }
	return 0;


}

void web_init() {
    
    // Usamos un puntero estático o capturamos por referencia en un hilo
    std::thread serverThread([]() {
    httplib::Server svr;

    // Ruta para enviar datos a la Web (RX)
    svr.Get("/api/status", [](const httplib::Request&, httplib::Response& res) {
        std::string body;
        try {
            {
                std::lock_guard<std::mutex> lock(globalState.mtx);
                nlohmann::json j;
                j["transcription"] = globalState.lastTranscription;
                j["contacts"] = globalState.validatedContacts;
                j["isProcessing"] = globalState.isProcessing;

                // Convertimos a string mientras aún tenemos el lock para asegurar consistencia
                body = j.dump();
            }
            //  El Mutex se libera AQUÍ automáticamente al cerrar la llave

            //Configuramos la respuesta fuera del bloqueo
            res.status = 200;
            res.set_content(body, "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");

        }
        catch (const std::exception& e) {
            std::cerr << "[WEB ERROR] Error generando JSON: " << e.what() << std::endl;
            res.status = 500;
            res.set_content("{\"error\": \"Internal Server Error\"}", "application/json");
        }
        });

    // Ruta para recibir órdenes de la Web (TX)
    svr.Post("/api/transmit", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = nlohmann::json::parse(req.body);
            std::string text = j.value("text", "");
            std::cout << "\n[WEB TX] Recibido para transmitir: " << text << std::endl;
            // Aquí irá el acople con el futuro módulo TTS
            res.set_content("OK", "text/plain");
        }
        catch (...) {
            res.status = 400;
        }
        });

    svr.Post("/api/toggle", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(globalState.mtx);
        globalState.isProcessing = !globalState.isProcessing;

        res.set_content(globalState.isProcessing ? "true" : "false", "text/plain");
        std::cout << "[SYSTEM] Motor de radio: " << (globalState.isProcessing ? "ACTIVO" : "PAUSADO") << std::endl;
        });

    svr.Post("/api/shutdown", [&](const httplib::Request&, httplib::Response& res) {
        std::cout << "[SYSTEM] Iniciando secuencia de apagado..." << std::endl;

        {
            std::lock_guard<std::mutex> lock(globalState.mtx);
            globalState.running = false; // Avisamos al motor de audio que pare
        }

        res.set_content("Sistema apagado correctamente", "text/plain");

        // Detenemos el servidor HTTP
        svr.stop(); 
        });

    std::cout << "[*] Servidor API iniciado en puerto 8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    });

    serverThread.detach(); // Separamos el hilo para que viva de forma independiente
}

// ============================================================================
// MAIN
// ============================================================================

int main() {

    // Initialize components
    Transcriber transcriber;
    AudioEngine audio;
    CallsignParser parser;
    QRZClient qrz;
    std::ofstream logFile("registro_radio.txt", std::ios::app);
    SetConsoleOutputCP(CP_UTF8);

	if(system_init(transcriber, audio, qrz)!=0){
        std::cerr << "[!] Error en la inicialización del sistema. Abortando." << std::endl;
        return 1;
	}
	web_init();

    // Main loop
    setColor(11);
    std::cout << "--- RadioAccess TFG: Sistema Activo ---" << std::endl;
    setColor(7);

    std::set<std::string> sessionHistory;

    while (globalState.running) {

		//========Comprobamos si el motor de radio está activo antes de procesar audio========
        bool motorActivo;
        {
            std::lock_guard<std::mutex> lock(globalState.mtx);
            motorActivo = globalState.isProcessing;
        }

        if (!motorActivo) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Dormimos un poco
            continue; // Saltamos al principio del bucle sin procesar audio
        }
		//====================================================================================
        size_t acumulado = audio.getQueuedSamplesCount();

        // Manage latency
        if (acumulado > UMBRAL_LATENCIA) {
            std::cout << "[!] Latencia detectada (" << acumulado / 16000 << "s). Saltando al presente..." << std::endl;
            audio.discardOldAudio(BLOQUE_3S);
            continue;
        }

        if (acumulado < BLOQUE_3S) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Transcription
        std::vector<float> pcmData = audio.getSamples(BLOQUE_3S);
        std::string textoActual = transcriber.transcribe(pcmData);

        // Filter noise
        if (textoActual.empty() || textoActual.find("[") != std::string::npos || textoActual.find("(") != std::string::npos) {
            std::cout << "[DEBUG] Silencio detectado" << std::endl;
            continue;
        }
		// Actualizamos el estado global con la última transcripción para que la Web pueda acceder a ella
        {
            std::lock_guard<std::mutex> lock(globalState.mtx);
            globalState.lastTranscription = textoActual;
        }

        std::string ahora = getTimestamp();

        std::cout << "[" << ahora << "] " << textoActual << std::endl;
        logFile << "[" << ahora << "] " << textoActual << std::endl;

        // Parse and validate callsign
        std::vector<std::string> candidates = parser.parseAll(textoActual);

       
		for (const auto& callsign : candidates) {
            if (sessionHistory.find(callsign) == sessionHistory.end()) {
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
                    {
                        std::lock_guard<std::mutex> lock(globalState.mtx);
                        nlohmann::json c;
                        c["call"] = op.callsign;
                        c["name"] = op.name;
                        c["loc"] = op.city + ", " + op.country;
                        globalState.validatedContacts.push_back(c);
                    }
                }
                else {
                    setColor(10);
                    std::cout << "[DEBUG] Candidato descartado por QRZ: " << callsign << std::endl;
                    setColor(7);
                }
            }
         }

        logFile.flush();
    }


    std::cout << "[*] Cerrando módulos de radio..." << std::endl;
    audio.stop();   // Detiene PortAudio
    logFile.close(); // Cierra el archivo de registro

    std::cout << "[*] Deteniendo Nginx..." << std::endl;
    system("taskkill /f /im nginx.exe >nul 2>&1"); //

    std::cout << "--- Sistema RadioAccess apagado. ---" << std::endl;
}
