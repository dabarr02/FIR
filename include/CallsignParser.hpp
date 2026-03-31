#include <string>
#include <vector>
#include <map>
#include <regex>


class CallsignParser {
public:
    CallsignParser();

    // Procesa el texto bruto y devuelve el indicativo si encuentra uno válido
    std::string parse(const std::string& rawText);

    // Limpia la memoria del último indicativo (útil tras silencios)
    void reset();

private:
    std::string cleanPhonetic(std::string text);
    std::string lastDetected;
    std::map<std::string, char> radioMapping;
    std::regex callsignRegex;
};