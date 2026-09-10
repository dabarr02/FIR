# FIR

Aplicacion de acceso por radio con transcripcion local, consulta de indicativos,
texto a voz y una interfaz web local.

## Estado del repositorio

Este repositorio contiene el codigo fuente y los headers necesarios para estudiar
y compilar el proyecto. Los binarios generados, los modelos de IA y las DLL/LIB
de terceros no forman parte del repositorio fuente.

## Requisitos

- Windows x64
- Visual Studio 2022 con herramientas C++
- CMake 3.15 o posterior
- vcpkg con `curl`, `pugixml` y `nlohmann-json`
- Binarios x64 de Whisper/ggml y PortAudio colocados en `third_party/`
- Un modelo compatible con whisper.cpp en `models/`

Las DLL y LIB de terceros deben obtenerse desde sus proyectos oficiales o desde
un artefacto de release cuyo origen y avisos legales estén documentados. No se
deben reutilizar binarios encontrados en `build/` o `out/` sin comprobar su
version, arquitectura y licencia.

## Compilacion

Configura CMake indicando el toolchain de vcpkg, por ejemplo:

```powershell
cmake -S . -B build -G Ninja `
	-DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

El proyecto falla de forma intencionada si faltan los binarios de Whisper/ggml o
PortAudio. Consulta `THIRD_PARTY_NOTICES.md` antes de redistribuirlos.

## Ejecucion desde un clon

Actualmente una persona que clone el repositorio debe completar estos pasos:

1. Obtener los binarios x64 de Whisper/ggml y PortAudio desde sus fuentes
	oficiales y colocarlos en las rutas esperadas bajo `third_party/`.
2. Descargar el modelo compatible y guardarlo como
	`build/Release/models/ggml-small.bin`. El modelo no se incluye en GitHub.
3. Configurar y compilar el proyecto siguiendo la seccion anterior.
4. Comprobar que los puertos 80 y 8080 estan libres y que hay un microfono
	disponible en Windows.
5. Ejecutar el programa desde la carpeta de salida:

```powershell
Set-Location build\Release
.\FIR.exe
```

El programa localiza el modelo y Nginx junto al ejecutable, inicia el servidor
interno y abre `http://localhost` en el navegador. El paquete incluye un `.env`
inicial con los campos vacios. La interfaz permite guardar el indicativo local
y, si se desea, las credenciales de QRZ. En el repositorio solo se mantiene
[.env.example](.env.example); nunca se deben subir credenciales reales.

Por tanto, la release actual es para desarrolladores. Para ofrecer una descarga
lista para usar habrá que crear un paquete Windows que incluya solo binarios
externos con versiones y licencias verificadas, además del modelo distribuido
segun sus condiciones.

## Crear paquetes Windows

Despues de compilar en Release y verificar las licencias de los binarios, instala
Inno Setup 6 y ejecuta desde la raiz del repositorio:

```powershell
.\scripts\package-release.ps1 -Version v0.1.0
```

El script crea ambos archivos en `dist/`:

- `FIR-v0.1.0-portable.zip`: se descomprime y se ejecuta directamente.
- `FIR-v0.1.0-setup.exe`: instala FIR con accesos directos.

El script no crea paquetes incompletos: requiere el ejecutable, las DLL, Nginx,
el modelo, la licencia y los avisos de terceros.

## Licencia

El codigo original de FIR se distribuye bajo la licencia MIT. Las
dependencias externas mantienen sus propias licencias y avisos; ver
`THIRD_PARTY_NOTICES.md`.

## Releases

Las releases de GitHub se generan al publicar un tag `v*.*.*`. La automatizacion
adjunta el archivo fuente y una copia de los avisos legales. No incluye DLL/LIB
precompiladas hasta que exista un artefacto reproducible con procedencia y
licencias verificadas.