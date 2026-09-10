# Third-party notices

FIR uses or bundles interfaces and binaries from the following
projects. These components are not covered by the project license above.

| Component | License | Official source |
| --- | --- | --- |
| whisper.cpp / ggml | MIT | https://github.com/ggerganov/whisper.cpp |
| PortAudio | MIT-style | https://github.com/PortAudio/portaudio |
| libcurl | curl license | https://curl.se/docs/copyright.html |
| pugixml | MIT | https://github.com/zeux/pugixml |
| nlohmann/json | MIT | https://github.com/nlohmann/json |

Before distributing a binary release, record the exact upstream version or
commit, target architecture, build options, and the complete license text for
each redistributed component. Keep those notices next to the executable or in
the release archive.

The current repository intentionally does not commit prebuilt DLL/LIB files.
The files under `build/` and `out/` are local build products and must not be
treated as release artifacts. CUDA runtime libraries and NVIDIA driver
components require a separate redistribution review and are not covered by
this notice.

The `tools/nginx/` directory is also not part of the application license. Do
not redistribute its executable or configuration as part of a release until
its exact upstream version and notices have been verified.