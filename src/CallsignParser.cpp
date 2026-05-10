#include "CallsignParser.hpp"
#include <sstream>
#include <algorithm>

CallsignParser::CallsignParser() {
    // Inicializamos el diccionario OTAN/OACI
    radioMapping = {
        {"ALPHA", 'A'}, {"ALFA", 'A'},{"ALF", 'A'},{"AMERICA", 'A'}, 
        {"BRAVO", 'B'}, 
        {"CHARLIE", 'C'}, 
        {"DELTA", 'D'},{"DENMARK", 'D'},
        {"ECHO", 'E'},{"AGO", 'E'},{"EGO", 'E'},
        {"FOXTROT", 'F'},{"FOXTROOT", 'F'}, 
        {"GOLF", 'G'}, 
        {"HOTEL", 'H'}, 
        {"INDIE", 'I'},{"INDIA", 'I'},{"INDIAN", 'I'},{"INDIC", 'I'},
        {"ITALY", 'I'},
        {"JULIETT", 'J'}, {"JULIET", 'J'},
        {"QUILO",'K'}, { "KILO", 'K' }, {"CAMPEQUIO", 'K'},
        {"LIMA", 'L'}, {"LONDON", 'L'},
        {"MIKE", 'M'},
        {"NOVEMBER", 'N'},{"NIKE", 'N'}, {"NORWAY", 'N'},
        {"OSCAR", 'O'}, 
        {"PAPA", 'P'}, 
        {"QUEBEC", 'Q'}, 
        {"ROMEO", 'R'},{"RADIO", 'R'},
        {"SIERRA", 'S'},{"SWITZERLAND", 'S'}, 
        {"TANGO", 'T'}, 
        {"UNIFORM", 'U'}, {"IGUERNIFORNE", 'U'}, {"ICONIFORM", 'U'}, {"UNIFORNE", 'U'},
        {"VICTOR", 'V'}, 
        {"WHISKEY", 'W'},
        {"XRAY", 'X'}, {"X-RAY", 'X'}, 
        {"YANKEE", 'Y'},
        {"ZULU", 'Z'},
        {"ZERO", '0'}, {"ONE", '1'}, {"TWO", '2'}, {"THREE", '3'}, {"FOUR", '4'},
        {"FIVE", '5'}, {"FIFE", '5'}, {"SIX", '6'}, {"SEVEN", '7'}, {"EIGHT", '8'}, {"NINE", '9'}
    };

    // Regex: 1-2 letras + 1 numero + 1-3 letras
    callsignRegex = std::regex("([A-Z]{1,2}[0-9]{1,2}[A-Z]{1,3})");
    lastDetected = "";
}

std::string CallsignParser::cleanPhonetic(std::string text) {
    std::replace(text.begin(), text.end(), '-', ' ');
    std::stringstream ss(text);
    std::string word, result;

    while (ss >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), [](char c) {
            return !std::isalnum(c);
            }), word.end());

        for (char& c : word) c = toupper(c);
        if (word.empty()) continue;

        if (isdigit(word[0])) {
            result += word[0];
        }
        else if (radioMapping.count(word)) {
            result += radioMapping[word];
        }
    }
    return result;
}

std::vector<std::string> CallsignParser::parseAll(const std::string& rawText) {
    std::string filtered = cleanPhonetic(rawText);
    std::vector<std::string> candidates;

    // Eliminamos reportes comunes que confunden a la regex
    filtered = std::regex_replace(filtered, std::regex("599|559|59"), "");

    // Usamos sregex_iterator para encontrar todas las coincidencias
    auto words_begin = std::sregex_iterator(filtered.begin(), filtered.end(), callsignRegex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::string found = i->str();

        // Solo lo anyadimos si tiene una longitud minima logica 
        if (found.length() >= 3) {
            candidates.push_back(found);
        }
    }

    return candidates;
}

void CallsignParser::reset() {
    //lastDetected = "";
    charBuffer = ""; 
}
