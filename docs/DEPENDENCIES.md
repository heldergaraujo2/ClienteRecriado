# Dependencies

## 1. This repository has none

The library, the tool, the tests and the benchmarks build with a C++17 compiler
and the standard library. Nothing is downloaded, vendored or linked.

```
Build requires:  g++ (or clang++, or MSVC) with C++17, GNU make, ar
Runtime requires: nothing beyond the C++ standard library
```

Verified: the build succeeds in a container with no OpenGL headers, no zlib
headers, no libpng, no libjpeg, no SDL and no network access to a package
registry. `/usr/include/GL/gl.h`, `zlib.h`, `png.h` and `jpeglib.h` are all
absent from the development environment and the build does not notice.

### Why

Three reasons, in order of weight:

1. **The Windows build cannot be tested here.** There is no Windows machine and
   no MinGW cross-compiler in the development environment. The one build path
   that cannot be verified must be the simplest possible one. A vcpkg manifest
   that has never been run on Windows is a way of shipping a broken build.
2. **No licence obligations to track.** See [`LICENSING.md`](LICENSING.md).
3. **Reproducible builds.** Any machine with a C++17 compiler produces the same
   result.

### What it costs

There is no JPEG pixel decoder. `mu/assets/ozj.cpp` parses the JPEG marker
structure — geometry, precision, component count, termination — which is
everything the inventory and VRAM tooling needs, but it does not produce pixels.
This is a deliberate, recorded gap rather than an oversight.

## 2. Dependencies identified in the reference client

Recorded because they constrain compatibility work, and because several are
third-party products whose redistribution rights are unconfirmed.

| Component | Version evidence | Role | Rights |
|---|---|---|---|
| `msvcr100.dll`, `msvcp100.dll` | Visual Studio 2010 runtime | CRT for `Main.dll` and most plugins | Microsoft, redistributable under MS terms |
| `MSVCP140.dll`, `VCRUNTIME140.dll` | VS2015+ runtime, imported by `Audio.dll` | CRT for the audio module | Microsoft, redistributable under MS terms |
| `vorbisfile.dll`, `ogg.dll` | Xiph | Ogg Vorbis playback | BSD-style (Xiph) |
| `3DGuard.dll`, `3DPlugin.dll`, `hinetworks.dll`, `License.hinet` | Hinet Networks | Licensing / anti-cheat | **Commercial. Not redistributable.** |
| `MHPClient.dll`, `MHPVerify.dll` | Unknown vendor | Verification | **Commercial. Not redistributable.** |
| `main.premium`, `3D.Premium`, `Checksum.premium` | Unknown | Licensing data | **Commercial. Not redistributable.** |
| `PS.dll`, `PS/License.ps` | Delphi build | External tool with its own licence file | **Unknown. Not redistributable.** |
| `arcadia.dll` | MSVC with control-flow guard (VS2019+) | OpenGL effects | Unknown |
| `AutoPotion.dll`, `AutoPotion.pcd` | Unknown | Third-party helper | Unknown |
| `Camera.dll`, `CEffect.dll`, `Audio.dll`, `wzAudio.dll` | Mixed | Client modules | Unknown |
| `Launcher.exe` | .NET (`mscoree.dll`) | Launcher | Unknown |
| Cg shaders under `Data/Effect/` | NVIDIA Cg toolkit | Glow post-process | Cg runtime is NVIDIA's; the shader source is the client author's |

Two Visual Studio runtime generations are present (2010 and 2015+), plus a
module built with MSVC control-flow guard. The client has been extended over
many years by different authors, which matters for any compatibility claim.

## 3. Candidates for future dependencies

Each is a decision to be made explicitly, not a default.

| Need | Candidate | Licence | Notes |
|---|---|---|---|
| JPEG pixel decoding | `stb_image.h` | Public domain / MIT | Single header, no build integration. Strongest candidate. |
| JPEG pixel decoding | libjpeg-turbo | IJG / BSD-style | Faster, but a real build dependency on every platform. |
| PNG support | `stb_image` or libpng | PD/MIT, or libpng licence | Only if PNG assets become relevant. |
| Ogg Vorbis playback | stb_vorbis, or libvorbis | PD/MIT, or BSD | Needed for the 61 MP3 and 3 OGG music files — note MP3 needs a separate decoder. |
| Windowing and input | Win32 directly, or SDL2 | —, or zlib | Win32 keeps the zero-dependency property on the actual target platform. |
| OpenGL loading | hand-written WGL loader, or GLAD | —, or MIT | The client uses only GL 1.1 entry points; a hand-written loader is ~100 lines. |
| Test framework | none — keep the built-in harness | — | Adding one buys little over 40 working lines. |

**Recommendation.** Add `stb_image.h` for JPEG decoding behind a pluggable
interface, so the choice stays reversible. Keep everything else dependency-free.
Record the decision in [`DECISIONS.md`](DECISIONS.md) when it is made.

## 4. Supply chain posture

- No package manager in the build.
- No download step.
- No vendored third-party code.
- No network access required to build.
- No transitive dependencies, therefore no transitive licence obligations.

If a dependency is added, this document must be updated with its name, version,
licence, and the reason the dependency-free property was given up.
