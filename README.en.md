# FIR

[Version en español](README.md)

FIR is an amateur radio assistant with local speech-to-text transcription, callsign parsing, optional QRZ integration, text-to-speech, and a local web interface.

## Repository status

This repository contains the source code and required headers. Generated binaries, AI models, and third-party DLL/LIB files are not part of the source repository.

## Requirements

- Windows x64
- Visual Studio 2022 with C++ tools
- CMake 3.15 or later
- vcpkg packages: `curl`, `pugixml`, and `nlohmann-json`
- x64 Whisper/ggml and PortAudio binaries under `third_party/`
- A whisper.cpp-compatible model under `models/`

Third-party DLL and LIB files must be obtained from their official projects or from a release artifact with documented provenance and licenses. Do not reuse unverified binaries from `build/` or `out/`.

## Build

Configure CMake with your vcpkg toolchain:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

The build intentionally fails when the required Whisper/ggml or PortAudio files are missing. Read `THIRD_PARTY_NOTICES.md` before redistributing them.

## Run from a clone

For a source checkout, obtain the required dependencies and model, then run:

```powershell
Set-Location build\Release
.\FIR.exe
```

FIR locates the model and Nginx next to the executable, starts the internal server, and opens `http://localhost`. The package contains an initial `.env` file. CPU packages use `FIR_USE_GPU=0`; CUDA packages use `FIR_USE_GPU=1`. Never commit real QRZ credentials. Only `.env.example` belongs in the source repository.

## Windows packages

The release process creates CPU and CUDA variants:

- CPU: for AMD, Intel, or unknown hardware. No CUDA is required.
- CUDA: for NVIDIA hardware only. Includes the CUDA runtime and provides better performance.

See `scripts/package-release.ps1` for packaging commands. Inno Setup 6 is required to create the installer.

## User manual

- [User manual in English](docs/manual/manual-en.md)
- [Manual de usuario en español](docs/manual/manual-es.md)

Place manual images in [docs/manual/images](docs/manual/images).

## License

FIR original code is distributed under the MIT License. External dependencies retain their own licenses and notices; see `THIRD_PARTY_NOTICES.md`.

## Releases

GitHub releases are created when a `v*.*.*` tag is pushed. Source archives and verified Windows package artifacts are published as release assets.
