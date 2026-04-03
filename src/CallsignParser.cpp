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
            result += word[0];
        }
        else if (radioMapping.count(word)) {
            result += radioMapping[word];
        }
    }
    return result;
}
/*
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
*/
std::string CallsignParser::parse(const std::string& rawText) {
    // 1. Extraemos SOLO caracteres de radio del texto que acaba de llegar
    std::string newChars = cleanPhonetic(rawText);

    if (newChars.empty()) return "";

    // 2. Los añadimos al buffer acumulativo
    charBuffer += newChars;

    // 3. Limpieza: Eliminamos reportes de señal (59, 599, 559) para que no estorben
    charBuffer = std::regex_replace(charBuffer, std::regex("599|559|59"), "");

    // 4. Mantenemos el buffer en un tamaño razonable (ventana deslizante)
    if (charBuffer.length() > MAX_BUFFER) {
        charBuffer.erase(0, charBuffer.length() - MAX_BUFFER);
    }

    // 5. Buscamos el indicativo en el buffer acumulado
    std::smatch match;
    // Buscamos desde el final para pillar el indicativo más reciente
    std::string result = "";
    auto words_begin = std::sregex_iterator(charBuffer.begin(), charBuffer.end(), callsignRegex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::string found = (*i).str();
        // Solo devolvemos si es distinto al último para no repetir
        if (found != lastDetected) {
            lastDetected = found;
            reset();
            result = found;
        }
    }

    return result;
}

void CallsignParser::reset() {
    //lastDetected = "";
    charBuffer = ""; // También limpiamos el buffer
}
