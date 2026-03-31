
#include <string>
#include <vector>

// Estructura para guardar lo que nos diga QRZ
struct OperatorData {
    std::string callsign;
    std::string name;
    std::string country;
    std::string city;
    std::string image_url;
    bool found = false;
};

class QRZClient {
public:
    QRZClient(std::string user, std::string pass);

    // Hace el login y guarda la sessionKey
    bool authenticate();

    // Busca un indicativo y devuelve los datos
    OperatorData lookup(const std::string& callsign);

private:
    std::string username;
    std::string password;
    std::string sessionKey;

    // Función interna para hacer la petición HTTP (usará libcurl)
    std::string makeRequest(const std::string& url);
};