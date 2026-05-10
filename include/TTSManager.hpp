#ifndef TTS_MANAGER_HPP
#define TTS_MANAGER_HPP

#include <string>
#include <vector>
#include <sapi.h>
#include <sphelper.h>

struct AudioDevice {
    int id;
    std::string name;
};

class TTSManager {
public:
    TTSManager();
    ~TTSManager();

    bool init();
    /***
     * Lee en voz alta el texto proporcionado
     * @param text Texto a leer
     */
    void speak(const std::string& text);

    /***
     * Obtiene la lista de dispositivos de salida disponibles
     * @return Vector con los dispositivos de salida
     */
    std::vector<AudioDevice> getOutputDevices();
    /***
     * Establece el dispositivo de salida actual
     * @param deviceId ID del dispositivo de salida
     * @return true si se estableció correctamente, false en caso contrario
     */
    bool setOutputDevice(int deviceId);

private:
    ISpVoice* pVoice;
    int currentDeviceId;
};

#endif