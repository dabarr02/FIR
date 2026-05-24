#include <string>
#include <vector>
#include <map>
#include <regex>


class CallsignParser {
public:
    CallsignParser();

    /***
     * Parsea el texto transcrito para extraer indicativos de radioaficionado. Devuelve un vector con todos los indicativos detectados.
     * @param rawText Texto transcrito sin procesar
     * @return Vector con los indicativos detectados en el texto
     */
	std::vector<std::string> parseAll(const std::string& rawText);

   

    /***
     * Reinicia el parser, limpiando el indicativo detectado
     */
    void reset();

private:
    std::string cleanPhonetic(std::string text);
    std::string lastDetected;
    std::map<std::string, char> radioMapping;
    std::regex callsignRegex;
    std::string charBuffer; 
    const size_t MAX_BUFFER = 40;
};