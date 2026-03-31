#include "CallsignParser.hpp"
#include <sstream>
#include <algorithm>

CallsignParser::CallsignParser() {
    // Inicializamos el diccionario OTAN/OACI
    radioMapping = {
        {"ALPHA", 'A'}, {"ALFA", 'A'}, {"BRAVO", 'B'}, {"CHARLIE", 'C'}, {"DELTA", 'D'},
        {"ECHO", 'E'}, {"FOXTROT", 'F'}, {"GOLF", 'G'}, {"HOTEL", 'H'}, {"INDIA", 'I'},
        {"JULIETT", 'J'}, {"JULIET", 'J'},{"QUILO",'K'}, { "KILO", 'K' }, {"LIMA", 'L'}, {"MIKE", 'M'},
        {"NOVEMBER", 'N'},{"NIKE", 'N'}, {"OSCAR", 'O'}, {"PAPA", 'P'}, {"QUEBEC", 'Q'}, {"ROMEO", 'R'},
        {"SIERRA", 'S'}, {"TANGO", 'T'}, {"UNIFORM", 'U'}, {"VICTOR", 'V'}, {"WHISKEY", 'W'},
        {"XRAY", 'X'}, {"X-RAY", 'X'}, {"YANKEE", 'Y'}, {"ZULU", 'Z'},
        {"ZERO", '0'}, {"ONE", '1'}, {"TWO", '2'}, {"THREE", '3'}, {"FOUR", '4'},
        {"FIVE", '5'}, {"FIFE", '5'}, {"SIX", '6'}, {"SEVEN", '7'}, {"EIGHT", '8'}, {"NINE", '9'}
    };

    // Regex: 1-2 letras + 1 número + 1-3 letras
    callsignRegex = std::regex("([A-Z]{1,2}[0-9][A-Z]{1,3})");
    lastDetected = "";
}

std::string CallsignParser::cleanPhonetic(std::string text) {
    std::replace(text.begin(), text.end(), '-', ' ');
    std::stringstream ss(text);
    std::string word, result;

    while (ss >> word) {
        // Limpiar puntuación
        word.erase(std::remove_if(word.begin(), word.end(), [](char c) {
            return !std::isalnum(c);
            }), word.end());

        for (char& c : word) c = toupper(c);
        if (word.empty()) continue;

        if (isdigit(word[0])) {
            result += word;
        }
        else if (radioMapping.count(word)) {
            result += radioMapping[word];
        }
    }
    return result;
}

std::string CallsignParser::parse(const std::string& rawText) {
    std::string filtered = cleanPhonetic(rawText);
    std::smatch match;

    filtered = std::regex_replace(filtered, std::regex("599|559|59"), "");
    
    if (std::regex_search(filtered, match, callsignRegex)) {
        std::string found = match[0];

        // Evitamos repetir el mismo indicativo si sigue en el buffer
        if (found.length() >= 3 && found != lastDetected) {
            lastDetected = found;
            return found;
        }
    }
    return ""; // No se detectó nada nuevo
}

void CallsignParser::reset() {
    lastDetected = "";
}