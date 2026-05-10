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

    /***
     * Autentica al usuario y obtiene la clave de sesión
     * @return true si la autenticación es exitosa, false en caso contrario
     */
    bool login();

    /***
     * Busca un indicativo en la base de datos de QRZ
     * @param callsign Indicativo a buscar
     * @return Datos del operador si se encuentra, con el campo 'found' a false si no se encuentra o hay error
     */
    OperatorData lookup(const std::string& callsign);

private:
    std::string username;
    std::string password;
    std::string sessionKey;

    /***
     * Función auxiliar para hacer las peticiones HTTP
     * @param url URL de la petición
     * @return Respuesta de la petición
     */
    std::string httpRequest(const std::string& url);
};