#include "QRZClient.hpp"
#include <curl/curl.h>
#include <pugixml.hpp>
#include <iostream>

// Callback necesario para que libcurl guarde la respuesta en un std::string
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

QRZClient::QRZClient(const std::string& user, const std::string& pass)
    : username(user), password(pass) {
}

std::string QRZClient::httpRequest(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

bool QRZClient::login() {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    // Escapamos los caracteres especiales del usuario y la contraseña
    char* encodedUser = curl_easy_escape(curl, username.c_str(), 0);
    char* encodedPass = curl_easy_escape(curl, password.c_str(), 0);

    // Construimos la URL con los datos seguros
    // Nota: Usamos '&' o ';' según pida la API, pero los valores van protegidos
    std::string url = "https://xmldata.qrz.com/xml/current/?username=";
    url += encodedUser;
    url += ";password=";
    url += encodedPass;

    // Limpiamos la memoria de curl_easy_escape
    curl_free(encodedUser);
    curl_free(encodedPass);
    curl_easy_cleanup(curl);

    std::string xml = httpRequest(url);

    pugi::xml_document doc;
    if (doc.load_string(xml.c_str())) {
        pugi::xml_node session = doc.child("QRZDatabase").child("Session");
        sessionKey = session.child_value("Key");

        // Si hay error, QRZ lo pone aquí
        std::string error = session.child_value("Error");
        if (!error.empty()) {
            std::cout << "[!] Error de la API de QRZ: " << error << std::endl;
        }

        return !sessionKey.empty();
    }
    return false;
}

OperatorData QRZClient::lookup(const std::string& callsign) {
    OperatorData data;
    if (sessionKey.empty()) return data;

    std::string url = "https://xmldata.qrz.com/xml/current/?s=" + sessionKey + ";callsign=" + callsign;
    std::string xml = httpRequest(url);
    //std::cout << "[DEBUG XML LOOKUP]: " << xml << std::endl;
    pugi::xml_document doc;
    if (doc.load_string(xml.c_str())) {
        pugi::xml_node callNode = doc.child("QRZDatabase").child("Callsign");
        if (callNode) {
            data.callsign = callNode.child_value("call");
            data.name = std::string(callNode.child_value("fname")) + " " + callNode.child_value("name");
            data.country = callNode.child_value("country");
            data.city = callNode.child_value("addr2");
            data.found = true;
        }
    }
    return data;
}