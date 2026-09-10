# RadioAccessTFG

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

## Licencia

El codigo original de RadioAccessTFG se distribuye bajo la licencia MIT. Las
dependencias externas mantienen sus propias licencias y avisos; ver
`THIRD_PARTY_NOTICES.md`.

## Releases

Las releases de GitHub se generan al publicar un tag `v*.*.*`. La automatizacion
adjunta el archivo fuente y una copia de los avisos legales. No incluye DLL/LIB
precompiladas hasta que exista un artefacto reproducible con procedencia y
licencias verificadas.