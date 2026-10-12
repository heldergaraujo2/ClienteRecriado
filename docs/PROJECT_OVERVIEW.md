# Project overview

## 1. Goal

Reconstruct and progressively optimise a MU Online client, targeting maximum
practical performance on modern Windows PCs with powerful dedicated GPUs while
preserving original assets and recognizable visuals. The client should support
heavier models, textures and crowded scenes through a modern resource and
rendering architecture, with opt-in visual enhancements where appropriate.

The concrete target, architecture principles and benchmark contract are defined
in [PERFORMANCE_TARGET.md](PERFORMANCE_TARGET.md).

## 2. The constraint that shapes everything

The audit established one fact that determines the entire strategy: **the
original engine cannot be modified.**

Main.exe in the reference client is packed with VMProtect — sections .vmp0 and
.vmp1, with .text, .rdata and .data having zero size on disk. Its import table
shows 51 OPENGL32 functions including wglCreateContext, glBegin/glEnd and
SwapBuffers, which places the render loop inside the protected module. Around
it sits a commercial licensing and anti-cheat stack: License.hinet,
main.premium, 3D.Premium, Checksum.premium, MHPClient.dll, MHPVerify.dll and
3DGuard.dll.

Bypassing any of that is out of scope and will not be attempted. The available
routes are:

1. **Read the assets.** Interpret original formats without modifying or
   redistributing proprietary source files.
2. **Build an independent renderer and engine.** Consume the original assets
   through a validated, backend-independent representation and modern GPU path.
3. **Optimise the asset pipeline.** Improve loading, caching, packaging and
   diagnostics without touching the protected engine.

## 3. Scope

### In scope

- Independent asset readers with tests against the real asset tree.
- Diagnostic and measurement tools that produce reproducible results.
- An independent Windows renderer and client, developed incrementally.
- Original BMD, OZJ, OZT, TGA, terrain and other supported assets as inputs.
- Optional visual improvements that preserve a faithful rendering mode.
- Support for heavier content through memory management, batching, instancing,
  asynchronous loading and other measured optimisations.
- Reproducible builds, tests, benchmarks and regression detection.
- Evaluation of a 64-bit process architecture while preserving protocol
  compatibility where lawful and technically possible.

### Out of scope

- Unpacking, patching, keygenning or defeating VMProtect, licence or anti-cheat.
- Reverse engineering the protected module.
- Redistributing original game assets, binaries or components without rights.
- Weakening server-side validation to improve benchmark results.
- Claiming performance or compatibility that has not been measured.

## 4. Performance priorities

1. Eliminate crashes, memory corruption, deadlocks and freezes.
2. Remove bottlenecks that cap frame rate and input responsiveness.
3. Reduce frame-time spikes and loading pauses, not only average frame time.
4. Improve input-to-response, simulation, render and presentation latency.
5. Reduce startup, map, model and texture load times.
6. Use CPU, GPU, RAM and VRAM efficiently while handling heavier scenes.
7. Preserve original geometry, UVs, materials, animation and visual identity.
8. Maintain stability in long sessions and crowded scenes.
9. Preserve content and authorised network/protocol compatibility.
10. Automate testing, measurement and performance regression detection.

A higher average FPS is not automatically an improvement if p95/p99 frame time,
stutter or input latency worsens. Compare equivalent scenes and settings.

## 5. Current state in one paragraph

The repository has an asset-format library and analysis tools, internal mesh
and skeleton representations, CPU-side buffer packing, skinning and frustum
culling, plus a deterministic CPU software-rasterizer proof of concept with
placeholder texturing. It does **not** yet have a production Windows/GPU
renderer or a playable client. The documented asset scan reports 13,832/13,928
OZJ files recognised, 3,404/3,470 OZT files recognised, 10,023/10,090 BMD
files structurally valid, and geometry parsed from 8,406 BMD files. Those
categories are distinct: structural recognition is not complete model support,
real UV decoding, visual parity or proof of runtime performance. Real Windows
FPS, frame-time, VRAM, input latency, long-session stability and authorised
server compatibility remain unexecuted until measured on the target platform.

Full detail: [CURRENT_STATE.md](CURRENT_STATE.md).

## 6. What "done" means

The project is complete only when evidence demonstrates:

- A documented independent architecture with implemented and pending components
  clearly separated.
- A reproducible Windows build and runnable client executable.
- Required original assets loading correctly, with correct UV/material mapping.
- Declared functionality actually implemented — not stubbed or simulated.
- A GPU renderer validated with real assets and representative scenes.
- Reproducible performance baselines and before/after optimisation comparisons.
- Frame-time distribution, FPS, latency and available resource metrics recorded
  with hardware, settings and conditions.
- Stability and regression tests passing for the scenarios that can be run.
- Dependencies and licences documented, with no secrets in the package.
- Build, run and validation instructions available.
- The next task recorded while work remains.

**No absolute performance claim will be made.** Results must state the hardware,
conditions, known limitations and what remains uninvestigated.
