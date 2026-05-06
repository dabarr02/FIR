#pragma once

#include "RadioState.hpp"
#pragma warning(push, 0) 
#include "httplib.h"
#include <nlohmann/json.hpp>
#pragma warning(pop)

class TTSManager;
class QRZClient;

class WebHandlers {
public:
    WebHandlers(RadioState& state, TTSManager& tts, QRZClient& qrz);

    //================ Rellena el fichero ADIF ================================

    void handle_report(const httplib::Request& req, httplib::Response& res);

    //====================== Devulve la informacion acutal del sistema, ultima transcripción, contactos y estado del motos de procesado =================================
  
    void handle_status(const httplib::Request& req, httplib::Response& res);

    //=================================== Devuelve los ajustes actuales del sistema ============================================  

    void handle_get_settings(const httplib::Request& req, httplib::Response& res);

    //=============================== Actualiza los ajustes del sistema =========================================

    void handle_post_settings(const httplib::Request& req, httplib::Response& res);

    //====================== Activa/Desactiva la transcripcion de audio =====================================
  
    void handle_toggle(const httplib::Request& req, httplib::Response& res);

    //========================= Apaga el sistema completo ===================================================

    void handle_shutdown(const httplib::Request& req, httplib::Response& res);

    //=========================== Genera el audio apartir del texto recibido (TTS) =============================

    void handle_transmit(const httplib::Request& req, httplib::Response& res);

    //============================== Devuelve los dispositivos de audio disponibles ==============================
 
    void handle_devices(const httplib::Request& req, httplib::Response& res);

    //============================== Consulta un indicativo a QRZ y lo añade a los contactos validados ==============================

    void handle_lookup(const httplib::Request& req, httplib::Response& res);

    // ==============Inicia el servidor en un hilo detach y retorna inmediatamente ===========================
    void start_server_detached(unsigned short port = 8080);

private:
    RadioState& state_;
    TTSManager& tts_;
    QRZClient& qrz_;
};