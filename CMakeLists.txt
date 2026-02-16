cmake_minimum_required(VERSION 3.15)
project(RadioAccessTFG)

set(CMAKE_CXX_STANDARD 17)

# Rutas
set(PA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/portaudio/include")
set(PA_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/portaudio/lib")

include_directories(include "${PA_INCLUDE_DIR}")

# Añadimos todos los archivos fuente
add_executable(RadioAccessTFG 
    src/main.cpp 
    src/AudioEngine.cpp
)

# Enlazamos la libreria .lib
target_link_libraries(RadioAccessTFG PRIVATE "${PA_LIB_DIR}/portaudio_x64.lib")

# Intentamos la copia, pero si falla no detendra el build
add_custom_command(TARGET RadioAccessTFG POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PA_LIB_DIR}/portaudio_x64.dll"
    "$<TARGET_FILE_DIR:RadioAccessTFG>"
    COMMENT "Copiando DLL..."
    VERBATIM
)