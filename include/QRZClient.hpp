#include <string>

struct OperatorData {
    std::string callsign;
    std::string name;
    std::string country;
    std::string city;
    bool found = false;
};

class QRZClient {
public:

    QRZClient();
    void init(const std::string& user, const std::string& pass);

    // Paso 1: Autenticarse y obtener la Session Key
    bool login();

    // Paso 2: Consultar un indicativo
    OperatorData lookup(const std::string& callsign);

private:
    std::string username;
    std::string password;
    std::string sessionKey;

    // Función auxiliar para hacer las peticiones HTTP
    std::string httpRequest(const std::string& url);
};