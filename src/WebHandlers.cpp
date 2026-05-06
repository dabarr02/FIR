#include "WebHandlers.hpp"

#include <thread>
#include <functional>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <iostream>

#include "TTSManager.hpp"
#include "QRZClient.hpp"

using namespace std::placeholders;


WebHandlers::WebHandlers(RadioState& state, TTSManager& tts, QRZClient& qrz)
    : state_(state), tts_(tts), qrz_(qrz) {}

//Devuelve la fecha en el formato necesario para el informe ADIF
static std::string getADIFDate() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d");
    return ss.str();
}

//Devuelve la hora en el formato necesario para el informe ADIF
static std::string getADIFTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H%M%S");
    return ss.str();
}

//Actualizacion de los ajustes persisntentes del programa en el fichero .env
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

//===============================Handlers de la API ====================================
void WebHandlers::handle_report(const httplib::Request& /*req*/, httplib::Response& res){
    std::stringstream adif;
{
    std::lock_guard<std::mutex> lock_report(state_.mtx);
    adif << "ADIF Export from RadioAccess\n<ADIF_VER:5>3.1.4\n<PROGRAMID:3>FIR\n";
    adif << "<STATION_CALLSIGN:" << state_.myCallsign.length() << ">" << state_.myCallsign << "\n<EOH>\n\n";

    for (const auto& c : state_.validatedContacts) {
        std::string call = c["call"], name = c["name"], loc = c["loc"];
        std::string date = c["date"], time = c["time"];

        adif << "<CALL:" << call.length() << ">" << call
            << " <QSO_DATE:" << date.length() << ">" << date
            << " <TIME_ON:" << time.length() << ">" << time
            << " <NAME:" << name.length() << ">" << name
            << " <QTH:" << loc.length() << ">" << loc
            << " <BAND:" << state_.currentBand.length() << ">" << state_.currentBand
            << " <MODE:2>FM <EOR>\n";
    }
}
res.set_content(adif.str(), "text/plain");
res.set_header("Content-Disposition", "attachment; filename=report_FIR.adi");

}

void WebHandlers::handle_status(const httplib::Request& /*req*/, httplib::Response& res){
    nlohmann::json j;
    {
        std::lock_guard<std::mutex> lock_status(state_.mtx);
        j["transcription"] = state_.lastTranscription;
        j["contacts"] = state_.validatedContacts;
        j["isProcessing"] = state_.isProcessing;
        j["needsConfig"] = state_.needsConfig;
    }
    res.set_content(j.dump(), "application/json");
}

void WebHandlers::handle_get_settings(const httplib::Request& /*req*/, httplib::Response& res){
    nlohmann::json j;
    {
        std::lock_guard<std::mutex> lock_settings(state_.mtx);
        j["user"] = state_.qrzUser;
        j["pass"] = state_.qrzPass;
        j["myCall"] = state_.myCallsign;
        j["band"] = state_.currentBand;
        j["needsConfig"] = state_.needsConfig;
    }
    res.set_content(j.dump(), "application/json");
}

void WebHandlers::handle_post_settings(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = nlohmann::json::parse(req.body);
        std::string u = j.at("user"), p = j.at("pass"), c = j.at("myCall"), b = j.at("band");

        if (j.contains("deviceId")) {
            int devId = j.at("deviceId").get<int>();
            tts_.setOutputDevice(devId);
            std::cout << "[SYSTEM] Salida de audio cambiada al dispositivo ID: " << devId << std::endl;
        }

        qrz_.init(u, p);
        if (qrz_.login()) {
            {
                std::lock_guard<std::mutex> lock_save(state_.mtx);
                state_.qrzUser = u;
                state_.qrzPass = p;
                state_.myCallsign = c;
                state_.currentBand = b;
                state_.needsConfig = false;
            }
            updateEnvFile(u, p, c);
            res.set_content("{\"status\":\"ok\"}", "application/json");
        } else {
            res.status = 401;
            res.set_content("{\"error\":\"QRZ Login failed\"}", "application/json");
        }
    } catch (...) {
        res.status = 400;
    }

}

void WebHandlers::handle_toggle(const httplib::Request& /*req*/, httplib::Response& res) {
    std::lock_guard<std::mutex> lock_toggle(state_.mtx);
    if (state_.needsConfig) {
        res.status = 403;
        res.set_content("config_required", "text/plain");
        return;
    }
    state_.isProcessing = !state_.isProcessing;
    std::cout << "[SYSTEM] Motor de radio: " << (state_.isProcessing ? "ACTIVO" : "PAUSADO") << std::endl;
    res.set_content(state_.isProcessing ? "true" : "false", "text/plain");

}

void WebHandlers::handle_shutdown(const httplib::Request& /*req*/, httplib::Response& res) {
    {
    std::lock_guard<std::mutex> lock_stop(state_.mtx);
    state_.running = false;
    }
    res.set_content("OK", "text/plain");
}

void WebHandlers::handle_transmit(const httplib::Request& req, httplib::Response& res){
    try {
        auto j = nlohmann::json::parse(req.body);
        std::string textoParaHablar = j.at("text").get<std::string>();

        std::cout << "[TX] Sintetizando: " << textoParaHablar << std::endl;
        tts_.speak(textoParaHablar);

        res.set_content("{\"status\":\"ok\"}", "application/json");
    }
    catch (...) { 
        res.status = 400; 
    }
}

void WebHandlers::handle_devices(const httplib::Request& /*req*/, httplib::Response& res) {
    auto devices = tts_.getOutputDevices();
    std::cout << "[DEBUG] Web solicitó dispositivos. Encontrados: " << devices.size() << std::endl;
    nlohmann::json j = nlohmann::json::array();
    for (const auto& d : devices) {
        j.push_back({ {"id", d.id}, {"name", d.name} });
        std::cout << "[DEBUG] Dispositivo encontrado: ID=" << d.id << ", Name=\"" << d.name << "\"" << std::endl;
    }
    res.set_content(j.dump(), "application/json");
}

void WebHandlers::handle_lookup(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = nlohmann::json::parse(req.body);
        std::string call = j.at("call").get<std::string>();

        OperatorData op = qrz_.lookup(call);

        if (op.found) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.validatedContacts.push_back({
                {"call", op.callsign},
                {"name", op.name},
                {"loc", op.city + ", " + op.country},
                {"date", getADIFDate()},
                {"time", getADIFTime()}
            });
            res.set_content("{\"status\":\"ok\"}", "application/json");
        } else {
            res.status = 404;
        }
    } catch (...) {
        res.status = 400;
    }
}

void WebHandlers::start_server_detached(unsigned short port) {
    std::thread serverThread([this, port]() {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) {
            std::cerr << "[!] CoInitializeEx falló: " << std::hex << hr << std::dec << std::endl;
            return;
        }

        httplib::Server svr;

        // Registrar handlers usando lambdas que llaman a los métodos del objeto
        svr.Get("/api/report", [this](const httplib::Request& req, httplib::Response& res) { this->handle_report(req, res); });
        svr.Get("/api/status", [this](const httplib::Request& req, httplib::Response& res) { this->handle_status(req, res); });
        svr.Get("/api/settings", [this](const httplib::Request& req, httplib::Response& res) { this->handle_get_settings(req, res); });
        svr.Post("/api/settings", [this](const httplib::Request& req, httplib::Response& res) { this->handle_post_settings(req, res); });
        svr.Post("/api/toggle", [this](const httplib::Request& req, httplib::Response& res) { this->handle_toggle(req, res); });
        svr.Post("/api/shutdown", [this](const httplib::Request& req, httplib::Response& res) { this->handle_shutdown(req, res); });
        svr.Post("/api/transmit", [this](const httplib::Request& req, httplib::Response& res) { this->handle_transmit(req, res); });
        svr.Get("/api/devices", [this](const httplib::Request& req, httplib::Response& res) { this->handle_devices(req, res); });
        svr.Post("/api/lookup", [this](const httplib::Request& req, httplib::Response& res) { this->handle_lookup(req, res); });

        std::string addr = "0.0.0.0";
        svr.listen(addr.c_str(), static_cast<int>(port));

        CoUninitialize();
    });

    serverThread.detach();
}
