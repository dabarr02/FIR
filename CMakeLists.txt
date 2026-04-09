cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(RadioAccessTFG C CXX)

set(CMAKE_CXX_STANDARD 17)

# 1. BUSCAR LIBRERÍAS
find_package(CURL REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 2. INCLUSIONES
include_directories(include)
include_directories(third_party/portaudio/include)

# 3. EJECUTABLE
add_executable(RadioAccessTFG 
    src/main.cpp
    src/AudioEngine.cpp
    src/Transcriber.cpp
    src/CallsignParser.cpp
    src/QRZClient.cpp
    src/TTSManager.cpp
)

# 4. ENLACE (Linking)
target_link_libraries(RadioAccessTFG PRIVATE 
    CURL::libcurl        
    pugixml::pugixml
    nlohmann_json::nlohmann_json
    "${CMAKE_SOURCE_DIR}/third_party/whisper/lib/whisper.lib"
    "${CMAKE_SOURCE_DIR}/third_party/whisper/lib/ggml.lib"
    "${CMAKE_SOURCE_DIR}/third_party/whisper/lib/ggml-base.lib"
    "${CMAKE_SOURCE_DIR}/third_party/portaudio/lib/portaudio_x64.lib"
)

# 5. AUTO-COPIAR DLLs (La magia para que funcione al darle a Play)
add_custom_command(TARGET RadioAccessTFG POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/third_party/portaudio/lib/portaudio_x64.dll"
    "${CMAKE_SOURCE_DIR}/third_party/whisper/bin/whisper.dll"
    "${CMAKE_SOURCE_DIR}/third_party/whisper/bin/ggml.dll"
    "${CMAKE_SOURCE_DIR}/third_party/whisper/bin/ggml-cpu.dll"
    "${CMAKE_SOURCE_DIR}/third_party/whisper/bin/ggml-cuda.dll"
    $<TARGET_FILE_DIR:RadioAccessTFG>)