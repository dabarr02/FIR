#pragma once
#include <cstdint>

#pragma pack(push, 1) // Importante: evita que Windows añada relleno entre datos
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 3; // 3 = float (necesario para la IA de transcripcion)
    uint16_t numChannels = 1;
    uint32_t sampleRate = 16000;
    uint32_t byteRate = 16000 * 4;
    uint16_t blockAlign = 4;
    uint16_t bitsPerSample = 32;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2Size;
};
#pragma pack(pop)