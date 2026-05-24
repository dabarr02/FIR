#include "CallsignParser.hpp"
#include <sstream>
#include <algorithm>
#include <set>

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



#include <set>

bool validarIndicativoSintactico(const std::string& callsign, std::string& prefix, std::string& suffix) {
    // Validación por longitudes estándar de la UIT
    if (callsign.empty() || callsign.length() < 3 || callsign.length() > 10) return false;

    // Analizar de atrás hacia adelante para localizar el primer dígito
    auto it = std::find_if(callsign.rbegin(), callsign.rend(), [](unsigned char ch) {
        return std::isdigit(ch);
        });

    // Si no contiene números
    if (it == callsign.rend()) return false;

    // Calcular la posición del dígito divisor
    size_t digitIdx = std::distance(it, callsign.rend()) - 1;

    //Separar el prefijo base del sufijo
    std::string prefijoPuro = callsign.substr(0, digitIdx);
    suffix = callsign.substr(digitIdx + 1);

    // El prefijo previo al número debe tener obligatoriamente 1 o 2 caracteres
    if (prefijoPuro.empty() || prefijoPuro.length() > 2) return false;

    // Verificar que todos los caracteres sean alfanuméricos puros
    for (char c : prefijoPuro) if (!std::isalnum(static_cast<unsigned char>(c))) return false;
    for (char c : suffix) if (!std::isalnum(static_cast<unsigned char>(c))) return false;

    // Reconstituimos el prefijo completo con su distrito
    prefix = callsign.substr(0, digitIdx + 1);

    
    static const std::set<std::string> TABLA_PREFIJOS_UIT = {
        // Europa Occidental y Mediterráneo
        "EA", "EB", "EC", "ED", "EE", "EF", "EG", "EH", "AM", "AN", "AO", // España
        "CT", "CQ", "CR", "CS", // Portugal
        "F", "HW", "HX", "HY", "TH", "TO", "TP", "TV", "TX", // Francia
        "I", "IK", "IZ", "IU", "IA", "IB", "ID", "IE", "IF", "IG", "IH", "II", "IO", "IP", "IQ", "IR", "IS", // Italia
        "ON", "OR", "OS", "OT", // Bélgica
        "PA", "PB", "PC", "PD", "PE", "PF", "PG", "PH", "PI", // Países Bajos
        "HB", "HE", // Suiza
        "LX", // Luxemburgo
        "SV", "SW", "SX", "SY", // Grecia

        // Europa Central, Norte y Bálticos
        "DA", "DB", "DC", "DD", "DE", "DF", "DG", "DH", "DI", "DJ", "DK", "DL", "DM", "DN", "DO", "DP", "DR", // Alemania
        "OE", // Austria
        "LA", "LB", "LC", "LD", "LE", "LF", "LG", "LH", // Noruega
        "SM", "SA", "SB", "SC", "SD", "SE", "SF", "7S", "8S", // Suecia
        "OH", "OF", "OG", "OI", // Finlandia
        "OZ", "OU", "OV", // Dinamarca

        // Europa del Este y Asia Central (Aquí se incluye el bloque solicitado)
        "UR", "US", "UT", "UU", "UV", "UW", "UX", "UY", "UZ", "EM", "EN", "EO", // Ucrania (¡Arreglado!)
        "UA", "RA", "RB", "RC", "RD", "RE", "RF", "RG", "RN", "RU", "RV", "RW", "RX", "RY", "RZ", // Rusia
        "SP", "SN", "SO", "SQ", "3Z", // Polonia
        "OK", "OL", // República Checa
        "OM", // Eslovaquia
        "HA", "HG", // Hungría
        "YO", "YR", // Rumanía
        "LZ", // Bulgaria
        "YL", // Letonia
        "ES", // Estonia
        "LY", // Lituania
        "EW", // Bielorrusia
        "UN", "UO", "UP", "UQ", // Kazajistán

        // Balcanes
        "9A", // Croacia
        "S5", // Eslovenia
        "E7", // Bosnia y Herzegovina
        "YU", "YT", // Serbia
        "Z3", // Macedonia

        // Angloamérica y Pacífico
        "W", "K", "N", "AA", "AB", "AC", "AD", "AE", "AF", "AG", "AI", "AJ", "AK", "AL", // EE.UU.
        "VE", "VA", "VO", "VY", // Canadá
        "G", "M", "2A", "2B", "2E", "2I", "2M", "2U", "2W", // Reino Unido
        "VK", "AX", // Australia
        "ZL", "ZM", // Nueva Zelanda
        "JA", "JB", "JC", "JD", "JE", "JF", "JG", "JH", "JI", "JJ", "JK", "JL", "JM", "JN", "JO", "JP", "JQ", "JR", "JS", // Japón

        // América Latina
        "PY", "PP", "PR", "PS", "PT", "PU", "PV", "PW", "PX", // Brasil
        "LU", "LO", "LP", "LQ", "LR", "LS", "LT", "LV", "LW", // Argentina
        "CE", "XQ", "XR", // Chile
        "HK", "HJ", // Colombia
        "YV", "YW", "YX", "YY", // Venezuela
        "XE", "XA", "XB", "XC", // México
        "OA", "OB", "OC", // Perú
        "HP", // Panamá
        "TI", // Costa Rica
        "CX", // Uruguay
        "ZP"  // Paraguay
    };

    // Validación si el bloque entero coincide en la tabla
    if (TABLA_PREFIJOS_UIT.find(prefijoPuro) != TABLA_PREFIJOS_UIT.end()) {
        return true;
    }

    // Validación especial para prefijos de una sola letra 
    if (prefijoPuro.length() == 2) {
        std::string primeraLetra = prefijoPuro.substr(0, 1);
        if (TABLA_PREFIJOS_UIT.find(primeraLetra) != TABLA_PREFIJOS_UIT.end()) {
            return true;
        }
    }

    return false; 
}

void CallsignParser::reset() {
    //lastDetected = "";
    charBuffer = ""; 
}
