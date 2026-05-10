#include "TTSManager.hpp"
#include <iostream>

// Constructor: inicializa el puntero de la voz a null y arranca COM,
// que es necesario para poder usar las APIs de SAPI en Windows.
TTSManager::TTSManager() : pVoice(nullptr) {
    CoInitialize(NULL); // Inicializa COM para usar SAPI
}

// Destructor: libera la interfaz de voz si existe y cierra COM.
TTSManager::~TTSManager() {
    if (pVoice) pVoice->Release();
    CoUninitialize();
}

// Crea la instancia de la voz de SAPI que se usará para sintetizar audio.
bool TTSManager::init() {
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
	std::cout << "[*] Inicializacion TTS:  " << (SUCCEEDED(hr) ? "Exito" : "Error") << std::endl;
    return SUCCEEDED(hr);
}

// Convierte texto std::string a wchar_t y lo envía a SAPI para que lo reproduzca.
void TTSManager::speak(const std::string& text) {
    if (!pVoice) return;

    // SAPI trabaja con cadenas anchas (wide string), por eso convertimos el texto.
    std::wstring stemp = std::wstring(text.begin(), text.end());
    LPCWSTR result = stemp.c_str();

    // SPF_ASYNC hace que la reproducción sea asíncrona y no bloquee el programa.
    pVoice->Speak(result, SPF_ASYNC, NULL);
}

// Obtiene la lista de dispositivos de salida de audio disponibles en el sistema.
std::vector<AudioDevice> TTSManager::getOutputDevices() {
    std::vector<AudioDevice> devices;
    IEnumSpObjectTokens* pEnum = NULL;

    // Enumeramos los tokens de la categoría de salida de audio de SAPI.
    if (SUCCEEDED(SpEnumTokens(SPCAT_AUDIOOUT, NULL, NULL, &pEnum))) {
        ULONG count = 0;
        // Obtenemos cuántos dispositivos hay para recorrerlos uno a uno.
        pEnum->GetCount(&count);

        // Recorremos cada dispositivo disponible.
        for (ULONG i = 0; i < count; i++) {
            ISpObjectToken* pToken = NULL;
            if (SUCCEEDED(pEnum->Next(1, &pToken, NULL))) {
                LPWSTR pDescription = NULL;

                // Extraemos el nombre descriptivo del dispositivo para mostrarlo al usuario.
                if (SUCCEEDED(SpGetDescription(pToken, &pDescription))) {
                    // Convertimos de wide string a string normal para usarlo en la aplicación.
                    std::wstring ws(pDescription);
                    std::string friendlyName(ws.begin(), ws.end());

                    // Guardamos el índice y el nombre legible del dispositivo.
                    devices.push_back({ (int)i, friendlyName });

                    // Liberamos la memoria reservada por Windows para la descripción.
                    CoTaskMemFree(pDescription);
                }
                // Liberamos el token del dispositivo actual.
                pToken->Release();
            }
        }
        // Liberamos el enumerador de tokens al terminar.
        pEnum->Release();
    }

    // Si no se encontró ningún dispositivo, devolvemos un mensaje informativo.
    if (devices.empty()) {
        devices.push_back({ -1, "No se detectaron salidas de audio" });
    }

    return devices;
}

// Selecciona un dispositivo de salida concreto para que SAPI reproduzca por él.
bool TTSManager::setOutputDevice(int deviceId) {
    if (!pVoice) return false;

    IEnumSpObjectTokens* pEnum = NULL;
    if (SUCCEEDED(SpEnumTokens(SPCAT_AUDIOOUT, NULL, NULL, &pEnum))) {
        ISpObjectToken* pToken = NULL;
        // Buscamos el token del dispositivo con el índice solicitado.
        if (SUCCEEDED(pEnum->Item(deviceId, &pToken))) {
            // Asignamos ese token como dispositivo de salida de la voz.
            pVoice->SetOutput(pToken, TRUE);
            pToken->Release();
            pEnum->Release();
            return true;
        }
        // Si no se pudo obtener el dispositivo, liberamos el enumerador.
        pEnum->Release();
    }
    return false;
}