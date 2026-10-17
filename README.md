# ClienteRecriado

Independent reconstruction and optimisation work for a MU Online client, plus
the diagnostic tooling used to measure it.

**Status: in progress; not yet a playable client.** The repository contains a
tested C++17 asset-analysis library, mesh/skeleton processing, CPU-side rendering
components, a software rasterizer proof of concept and command-line tools.
There is no production Windows/GPU renderer yet. Read
[docs/CURRENT_STATE.md](docs/CURRENT_STATE.md) for implemented, partial and
blocked work.

## Product target

The goal is maximum practical performance on modern Windows PCs with capable
dedicated GPUs, preserving original MU Online assets and their recognizable
appearance while allowing opt-in visual improvements and substantially heavier
scenes. This is a target, not an achieved performance claim. The architecture,
benchmark contract and implementation order are documented in
[docs/PERFORMANCE_TARGET.md](docs/PERFORMANCE_TARGET.md).

## Why this repository exists

The reference client (heldergaraujo2/Cliente) is a 32-bit Windows OpenGL 1.1
fixed-function game client. Its render loop lives inside Main.exe, which is
protected with VMProtect. This project does not patch the protected engine or
bypass licensing or anti-cheat protections. Instead, it builds independent
asset readers, diagnostic tools and a renderer that can consume the original
assets from an external reference tree.

The original assets and binaries are not redistributed. They must remain
outside this repository; see [docs/LICENSING.md](docs/LICENSING.md).

## What is here now

| Path | What it is |
|---|---|
| src/mu/assets/ | C++17 readers/classifiers for OZT, OZJ, TGA, BMD, terrain and patch manifests |
| src/mu/assets/mesh.* | Canonical CPU-side mesh and skeleton representation |
| src/mu/render/ | CPU-side buffer packing, skinning, culling, software rasterizer and texture proof of concept |
| src/mu/platform/ | Callback-driven frame-loop coordinator; Win32 window and production GPU execution remain incomplete/unverified |
| src/mu/fs.* | Portable recursive directory walker (POSIX + Win32) |
| tools/mutool.cpp | Command-line inspection, validation and software-rendering tools |
| tests/test_assets.cpp / tests/test_main_loop_standalone.cpp | Asset suite plus callback-driven frame-loop tests; cycle-20 build/test/sanitizer evidence is in `docs/evidence/ciclo20_main_loop_validation_20261016.log` |
| bench/bench_assets.cpp | Asset micro-benchmarks with percentile reporting |
| docs/ | Audits, architecture, compatibility, measurements and continuity plan |

## Building

Requires a C++17 compiler and make. The core asset library has no third-party
dependencies and can be tested headlessly.

```sh
make
make test
make test-san
make bench
```

The Windows/MSVC path and actual GPU execution must be validated on Windows
hardware. See [docs/BUILD.md](docs/BUILD.md); do not treat an unexecuted build
path as verified.

## Using the tool

```sh
# Inventory an asset tree.
build/mutool scan /path/to/client/Data

# Classify a single file.
build/mutool info /path/to/client/Data/Item/some_texture.ozt

# Verify an installed tree against the client's patch manifest.
build/mutool verify /path/to/client/ListaPatch.txt /path/to/client

# Find byte-identical duplicate assets.
build/mutool dedupe /path/to/client/Data

# Inspect/render supported model geometry with the CPU proof of concept.
build/mutool render /path/to/client/Data/Object1/Beer01.bmd /tmp/beer.ppm 800 600
```

## Headline findings so far

The figures below are repository-documented asset-tree results, not runtime
FPS or GPU benchmarks.

- **Reference renderer:** fixed-function OpenGL 1.1. The original executable's
  imports include immediate-mode calls such as glBegin/glEnd and glReadPixels.
- **Texture bytes:** 2,893.80 MiB is a header/payload-derived image-byte estimate
  (OZJ as RGBA8-equivalent; OZT/TGA declared or calculated payload bytes), not
  decoded allocation, RAM, or VRAM. The BMD v0x0C classifier label is not cipher
  validation or format compatibility; `AGENTS.md` keeps that work BLOCKED.
- **Asset processing:** the current state document reports 13,832/13,928 OZJ
  files recognised, 3,404/3,470 OZT files recognised, and 10,023/10,090 BMD
  files structurally valid. Of the BMD tree, 8,406 files have parsed geometry;
  container recognition must not be confused with usable geometry or UV support.
- **Reference-tree integrity:** the documented patch-manifest audit found
  missing entries and size/CRC mismatches. See docs/REFERENCE_AUDIT.md for
  details.
- **Protected components:** VMProtect and commercial licensing/anti-cheat
  components remain out of scope for bypass or modification.

## Documentation index

| Document | Contents |
|---|---|
| [docs/PROJECT_OVERVIEW.md](docs/PROJECT_OVERVIEW.md) | Goals, scope and completion criteria |
| [docs/PERFORMANCE_TARGET.md](docs/PERFORMANCE_TARGET.md) | Selected performance target, architecture and benchmark contract |
| [docs/REFERENCE_AUDIT.md](docs/REFERENCE_AUDIT.md) | Inventory and binary analysis of the original client |
| [docs/ASSET_FORMATS.md](docs/ASSET_FORMATS.md) | Asset formats and measurements |
| [docs/RENDERING.md](docs/RENDERING.md) | Reference and reconstructed rendering pipeline |
| [docs/D3D11_BMD_PREVIEW.md](docs/D3D11_BMD_PREVIEW.md) | Build/download the Windows D3D11 geometry-preview executable and its validation limits |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Software architecture |
| [docs/PERFORMANCE_BASELINE.md](docs/PERFORMANCE_BASELINE.md) | What has and has not been measured |
| [docs/BENCHMARKS.md](docs/BENCHMARKS.md) | Benchmark methodology and results |
| [docs/OPTIMIZATION_LOG.md](docs/OPTIMIZATION_LOG.md) | Optimisation hypotheses and measured outcomes |
| [docs/TEST_STRATEGY.md](docs/TEST_STRATEGY.md) / [docs/TEST_RESULTS.md](docs/TEST_RESULTS.md) | Test design and results |
| [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md) | Visual and functional compatibility |
| [docs/NETWORK.md](docs/NETWORK.md) | Network evidence and the Muserver interoperability handoff; no network implementation exists yet |
| [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md) / [docs/LICENSING.md](docs/LICENSING.md) | Dependencies and rights |
| [docs/SECURITY.md](docs/SECURITY.md) | Security risks and mitigations |
| [docs/DECISIONS.md](docs/DECISIONS.md) | Architectural decisions |
| [docs/CURRENT_STATE.md](docs/CURRENT_STATE.md) | Current implementation status |
| [docs/NEXT_TASK.md](docs/NEXT_TASK.md) | Next task and acceptance criteria |
| [docs/CHANGELOG.md](docs/CHANGELOG.md) | Notable changes |

## Licence

The repository's MIT licence covers code written here only. It does not grant
rights to original MU Online assets or binaries, which remain their owners'
property. See [docs/LICENSING.md](docs/LICENSING.md).
