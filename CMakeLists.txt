cmake_minimum_required(VERSION 3.15)
project(RadioAccessTFG C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# 1. DEFINICIONES GLOBALES (Extraídas del archivo original)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DWHISPER_VERSION="1.8.3")
add_definitions(-DGGML_VERSION="0.9.7") # Según tu archivo 
add_definitions(-DGGML_COMMIT="unknown")
add_definitions(-DGGML_USE_CPU)

if(WIN32)
    add_definitions(-D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR) # Vital para MSVC 
    add_definitions(-DGGML_BACKEND_DL_WINDOWS) # [cite: 31]
endif()

# 2. DIRECTORIOS DE INCLUSIÓN
include_directories(include)
include_directories(third_party/portaudio/include)
include_directories(third_party/whisper/ggml-cpu)
include_directories(third_party/whisper)

# 3. LIBRERÍA GGML_CORE (Archivos base)
add_library(ggml_core STATIC
    third_party/whisper/ggml.c
    third_party/whisper/ggml-alloc.c
    third_party/whisper/ggml-backend.cpp
    third_party/whisper/ggml-backend-reg.cpp
    third_party/whisper/ggml-backend-dl.cpp
    third_party/whisper/ggml-quants.c
    third_party/whisper/gguf.cpp
    third_party/whisper/ggml-threading.cpp
)

# 4. LIBRERÍA GGML_CPU (La que te daba los 39 errores)
# El archivo original separa estas optimizaciones 
add_library(ggml_cpu_x86 OBJECT
    third_party/whisper/ggml-cpu/arch/x86/cpu-feats.cpp
    third_party/whisper/ggml-cpu/arch/x86/quants.c
    third_party/whisper/ggml-cpu/arch/x86/repack.cpp
)

add_library(ggml_cpu STATIC
    third_party/whisper/ggml-cpu/ggml-cpu.c
    third_party/whisper/ggml-cpu/ggml-cpu.cpp
    third_party/whisper/ggml-cpu/quants.c
    third_party/whisper/ggml-cpu/repack.cpp
    third_party/whisper/ggml-cpu/traits.cpp
    third_party/whisper/ggml-cpu/binary-ops.cpp
    third_party/whisper/ggml-cpu/unary-ops.cpp
    third_party/whisper/ggml-cpu/vec.cpp
    third_party/whisper/ggml-cpu/ops.cpp
    third_party/whisper/ggml-cpu/hbm.cpp
    $<TARGET_OBJECTS:ggml_cpu_x86>
)

add_library(whisper STATIC third_party/whisper/whisper.cpp)

# 5. EJECUTABLE FINAL
add_executable(RadioAccessTFG 
    src/main.cpp 
    src/AudioEngine.cpp 
    src/Transcriber.cpp
    third_party/whisper/whisper.cpp # Whisper depende de las dos de arriba
)

# 6. ENLACE DE PIEZAS
target_link_libraries(RadioAccessTFG PRIVATE 
    whisper
    ggml_cpu 
    ggml_core
    "${CMAKE_SOURCE_DIR}/third_party/portaudio/lib/portaudio_x64.lib"
)

# 7. SUPRESIÓN DE ERRORES MSVC (Copiado del original )
if (MSVC)
   # /arch:AVX2 es lo que hara que bajes de 5s a 0.5s de procesado
    set(MSVC_FLAGS /wd4005 /wd4244 /wd4267 /wd4305 /wd4566 /wd4996 /wd4702 /arch:AVX2)
    target_compile_options(ggml_core PRIVATE ${MSVC_FLAGS})
    target_compile_options(ggml_cpu PRIVATE ${MSVC_FLAGS})
    target_compile_options(ggml_cpu_x86 PRIVATE ${MSVC_FLAGS})
    target_compile_options(RadioAccessTFG PRIVATE ${MSVC_FLAGS})
endif()

# 8. DLL DE PORTAUDIO
add_custom_command(TARGET RadioAccessTFG POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/third_party/portaudio/lib/portaudio_x64.dll"
    $<TARGET_FILE_DIR:RadioAccessTFG>)