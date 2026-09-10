cmake_minimum_required(VERSION 3.15)
project(RadioAccessTFG C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "CMAKE_TOOLCHAIN_FILE is not set; configure vcpkg or another dependency provider before building.")
endif()

# 1. BUSCAR LIBRERÍAS
find_package(CURL REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 2. INCLUSIONES
include_directories(include)
include_directories(third_party/portaudio/include)
include_directories(third_party/whisper/include)

# 3. EJECUTABLE
add_executable(RadioAccessTFG 
    src/main.cpp
    src/AudioEngine.cpp
    src/Transcriber.cpp
    src/CallsignParser.cpp
    src/QRZClient.cpp
    src/TTSManager.cpp
    src/WebHandlers.cpp
)

# 4. ENLACE (Linking)
set(WHISPER_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/whisper/lib")
set(WHISPER_BIN_DIR "${CMAKE_SOURCE_DIR}/third_party/whisper/bin")
set(PORTAUDIO_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/portaudio/lib")

foreach(required_file
    "${WHISPER_LIB_DIR}/whisper.lib"
    "${WHISPER_LIB_DIR}/ggml.lib"
    "${WHISPER_LIB_DIR}/ggml-base.lib"
    "${PORTAUDIO_LIB_DIR}/portaudio_x64.lib"
    "${PORTAUDIO_LIB_DIR}/portaudio_x64.dll"
    "${WHISPER_BIN_DIR}/whisper.dll"
    "${WHISPER_BIN_DIR}/ggml.dll"
    "${WHISPER_BIN_DIR}/ggml-cpu.dll")
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Missing third-party binary: ${required_file}. See README.md for setup instructions.")
    endif()
endforeach()

set(RUNTIME_DLLS
    "${PORTAUDIO_LIB_DIR}/portaudio_x64.dll"
    "${WHISPER_BIN_DIR}/whisper.dll"
    "${WHISPER_BIN_DIR}/ggml.dll"
    "${WHISPER_BIN_DIR}/ggml-cpu.dll")
if(EXISTS "${WHISPER_BIN_DIR}/ggml-cuda.dll")
    list(APPEND RUNTIME_DLLS "${WHISPER_BIN_DIR}/ggml-cuda.dll")
endif()

target_link_libraries(RadioAccessTFG PRIVATE 
    CURL::libcurl        
    pugixml::pugixml
    nlohmann_json::nlohmann_json
    "${WHISPER_LIB_DIR}/whisper.lib"
    "${WHISPER_LIB_DIR}/ggml.lib"
    "${WHISPER_LIB_DIR}/ggml-base.lib"
    "${PORTAUDIO_LIB_DIR}/portaudio_x64.lib"
)

# 5. AUTO-COPIAR DLLs (La magia para que funcione al darle a Play)
add_custom_command(TARGET RadioAccessTFG POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${RUNTIME_DLLS}
    $<TARGET_FILE_DIR:RadioAccessTFG>)