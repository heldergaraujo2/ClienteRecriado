# ClienteRecriado

Independent reconstruction and optimisation work for a MU Online client, plus
the diagnostic tooling used to measure it.

**Status: in progress.** This is not a playable client yet. What exists today
is a verified, tested asset-format library and a command line inspection tool
that has been run against a real 2.4 GB MU Online asset tree. Read
[`docs/CURRENT_STATE.md`](docs/CURRENT_STATE.md) for exactly what is
implemented, what is partial and what is blocked.

## Why this repository exists

The reference client (`heldergaraujo2/Cliente`) is a 32-bit Windows
OpenGL 1.1 fixed-function game client. Its render loop lives inside
`Main.exe`, which is protected with VMProtect, so the engine cannot be
modified legally or safely. That shapes the whole strategy:

- The **engine cannot be patched**. Anything that requires editing the render
  loop is out of scope.
- The **assets can be read**. Their formats are not protected, and reading them
  is what makes an independent renderer possible.
- **External optimisation is possible.** Configuration, asset hygiene, load
  order and packaging can all be improved without touching protected code.

So this project builds (a) tooling that measures the real client and its
assets, and (b) an independent asset layer that a future renderer can be built
on.

## What is here now

| Path | What it is |
|---|---|
| `src/mu/assets/` | Asset format readers: OZT, OZJ, TGA, BMD container, terrain, patch manifest, CRC-32 |
| `src/mu/fs.*` | Portable recursive directory walker (POSIX + Win32) |
| `tools/mutool.cpp` | Command line inspection tool: `scan`, `info`, `verify`, `dedupe` |
| `tests/test_assets.cpp` | 131 assertions, zero dependencies |
| `bench/bench_assets.cpp` | Micro-benchmarks with frametime-style percentile reporting |
| `docs/` | Audit, architecture, formats, rendering analysis, benchmarks, decisions |

## Building

Requires a C++17 compiler and `make`. No third-party dependencies, no package
downloads, no network access.

```sh
make            # builds build/libmuassets.a, build/mutool, build/test_assets
make test       # runs the unit tests
make bench      # builds the micro-benchmark
```

The same sources are intended to build on Windows with MSVC or MinGW; see
[`docs/BUILD.md`](docs/BUILD.md). That path has **not been executed** — there
is no Windows machine in the development environment.

## Using the tool

```sh
# Inventory an asset tree: per-format counts, parse success, VRAM estimate.
build/mutool scan /path/to/client/Data

# Classify a single file in detail.
build/mutool info /path/to/client/Data/Item/some_texture.ozt

# Verify an installed tree against the client's own patch manifest.
# Exit status: 0 clean, 1 usage error, 2 problems found.
build/mutool verify /path/to/client/ListaPatch.txt /path/to/client

# Find byte-identical duplicate assets.
build/mutool dedupe /path/to/client/Data
```

## Headline findings so far

All of these were measured against the real reference client, not estimated.

- **Renderer is fixed-function OpenGL 1.1.** `Main.exe` imports 51 `OPENGL32`
  functions including `glBegin`/`glEnd` immediate mode and `glReadPixels`.
  There is no `glGenBuffers`, no `glDrawElements` and no framebuffer object
  anywhere in the import tables. No Direct3D at all.
- **The asset set needs 2,893.80 MB of uncompressed texture memory** when every
  OZJ, OZT and TGA is expanded to RGBA. That is far more than any GPU
  contemporary with the client, and more than many modern integrated GPUs.
- **Asset formats are documented and loadable.** 99.31% of OZJ, 98.10% of OZT,
  99.76% of BMD and 100% of TGA and terrain files parse correctly.
- **The shipped client is incomplete against its own patch manifest.** 52 of
  188 manifest entries are missing from the tree, and 20 more have a size or
  CRC mismatch.
- **Two protections are confirmed and will not be circumvented**: VMProtect on
  `Main.exe`, and a commercial licensing stack (`License.hinet`,
  `*.premium`, `MHPVerify.dll`, `3DGuard.dll`).

## Documentation index

| Document | Contents |
|---|---|
| [`docs/PROJECT_OVERVIEW.md`](docs/PROJECT_OVERVIEW.md) | Goals, scope, out of scope |
| [`docs/REFERENCE_AUDIT.md`](docs/REFERENCE_AUDIT.md) | Full inventory and binary analysis of the original client |
| [`docs/ASSET_FORMATS.md`](docs/ASSET_FORMATS.md) | Every format, with the measurements that established it |
| [`docs/RENDERING.md`](docs/RENDERING.md) | Render pipeline analysis from import tables and config |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Architecture of the reconstruction |
| [`docs/PERFORMANCE_BASELINE.md`](docs/PERFORMANCE_BASELINE.md) | What was measured, and what could not be |
| [`docs/BENCHMARKS.md`](docs/BENCHMARKS.md) | Methodology and results |
| [`docs/OPTIMIZATION_LOG.md`](docs/OPTIMIZATION_LOG.md) | Each optimisation: evidence, hypothesis, change, result |
| [`docs/TEST_STRATEGY.md`](docs/TEST_STRATEGY.md) / [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md) | Test design and real results |
| [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md) | Visual and functional compatibility status |
| [`docs/DEPENDENCIES.md`](docs/DEPENDENCIES.md) / [`docs/LICENSING.md`](docs/LICENSING.md) | Third-party components and rights |
| [`docs/SECURITY.md`](docs/SECURITY.md) | Risks found, including a credential exposure |
| [`docs/DECISIONS.md`](docs/DECISIONS.md) | Architectural decisions and why |
| [`docs/CURRENT_STATE.md`](docs/CURRENT_STATE.md) | What is true right now |
| [`docs/NEXT_TASK.md`](docs/NEXT_TASK.md) | The next concrete task, with acceptance criteria |
| [`docs/CHANGELOG.md`](docs/CHANGELOG.md) | Notable changes |

## Licence

Code in this repository is MIT. That covers only the code written here. It says
nothing about the rights to the original client's assets or binaries, which are
not redistributed and remain the property of their owners. See
[`docs/LICENSING.md`](docs/LICENSING.md).
