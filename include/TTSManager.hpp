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
    void speak(const std::string& text);

    // Para el selector de la web
    std::vector<AudioDevice> getOutputDevices();
    bool setOutputDevice(int deviceId);

private:
    ISpVoice* pVoice;
    int currentDeviceId;
};

#endif