#include "TTSManager.hpp"
#include <iostream>

TTSManager::TTSManager() : pVoice(nullptr) {
    CoInitialize(NULL); // Inicializa COM para usar SAPI
}

TTSManager::~TTSManager() {
    if (pVoice) pVoice->Release();
    CoUninitialize();
}

bool TTSManager::init() {
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
	std::cout << "[*] Inicializacion TTS:  " << (SUCCEEDED(hr) ? "Exito" : "Error") << std::endl;
    return SUCCEEDED(hr);
}

void TTSManager::speak(const std::string& text) {
    if (!pVoice) return;

    // Convertimos string normal awstring (necesario para SAPI)
    std::wstring stemp = std::wstring(text.begin(), text.end());
    LPCWSTR result = stemp.c_str();

    // SPF_ASYNC permite que el programa no se congele mientras habla
    pVoice->Speak(result, SPF_ASYNC, NULL);
}

std::vector<AudioDevice> TTSManager::getOutputDevices() {
    std::vector<AudioDevice> devices;
    IEnumSpObjectTokens* pEnum = NULL;

    // 1. Buscamos los tokens de la categoría AUDIO OUT (Salidas)
    if (SUCCEEDED(SpEnumTokens(SPCAT_AUDIOOUT, NULL, NULL, &pEnum))) {
        ULONG count = 0;
        pEnum->GetCount(&count);

        for (ULONG i = 0; i < count; i++) {
            ISpObjectToken* pToken = NULL;
            if (SUCCEEDED(pEnum->Next(1, &pToken, NULL))) {
                LPWSTR pDescription = NULL;

                // 2. CLAVE: Obtenemos la descripción real del dispositivo
                if (SUCCEEDED(SpGetDescription(pToken, &pDescription))) {
                    // Convertimos de Wide String (Windows) a String normal (C++)
                    std::wstring ws(pDescription);
                    std::string friendlyName(ws.begin(), ws.end());

                    // 3. Guardamos el ID real y el nombre real
                    devices.push_back({ (int)i, friendlyName });

                    // Limpiamos la memoria de la cadena de Windows
                    CoTaskMemFree(pDescription);
                }
                pToken->Release();
            }
        }
        pEnum->Release();
    }

    // Si por lo que sea sale vacío, metemos un aviso
    if (devices.empty()) {
        devices.push_back({ -1, "No se detectaron salidas de audio" });
    }

    return devices;
}

bool TTSManager::setOutputDevice(int deviceId) {
    if (!pVoice) return false;

    IEnumSpObjectTokens* pEnum = NULL;
    if (SUCCEEDED(SpEnumTokens(SPCAT_AUDIOOUT, NULL, NULL, &pEnum))) {
        ISpObjectToken* pToken = NULL;
        // Saltamos hasta el ID seleccionado
        if (SUCCEEDED(pEnum->Item(deviceId, &pToken))) {
            // CAMBIO CRÍTICO: Asignamos la salida a la voz de SAPI
            pVoice->SetOutput(pToken, TRUE);
            pToken->Release();
            pEnum->Release();
            return true;
        }
        pEnum->Release();
    }
    return false;
}