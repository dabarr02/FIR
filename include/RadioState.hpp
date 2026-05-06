#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

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