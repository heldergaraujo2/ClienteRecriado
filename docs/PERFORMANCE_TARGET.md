# Performance and compatibility target

## 1. Product goal

Build an independent, playable MU Online client that preserves the original game's
assets and recognizable appearance while using a modern rendering and resource
pipeline. The primary target is maximum practical performance on modern Windows
PCs with capable dedicated GPUs, including scenes containing substantially more
or heavier content than the reference client handles comfortably.

This is a target, not a claim that the current repository has achieved it. The
current implementation and measured results remain those documented in
[CURRENT_STATE.md](CURRENT_STATE.md), [RENDERING.md](RENDERING.md), and
[PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md).

## 2. Product principles

1. **Original assets remain the source of truth.** Load original BMD, OZJ, OZT,
   TGA, terrain and other supported resources without destructive modification.
   Keep source files external; do not commit or redistribute proprietary assets.
2. **Preserve the original look by default.** Correct geometry, UVs, texture
   orientation, material behavior, animation and scene scale take priority over
   speculative visual effects.
3. **Allow optional enhancements.** Higher-resolution rendering, improved
   filtering, configurable shadows/lighting and higher-quality effects may be
   added as opt-in features when they do not silently change gameplay or asset
   meaning. Keep a compatibility/faithful-rendering mode.
4. **Optimise measured bottlenecks.** Every material optimisation needs a
   reproducible baseline, equivalent test conditions and before/after results.
   Mean FPS alone is insufficient; frame-time distribution, spikes, input
   responsiveness, load times, CPU/GPU usage and RAM/VRAM also matter.
5. **Use modern hardware without needless waste.** Prefer GPU-side indexed
   rendering, resource batching/instancing, asynchronous loading, explicit
   resource lifetime management and conservative culling where measurements
   support them.
6. **Fail safely on malformed assets.** File counts, dimensions, vertex/index
   ranges and memory allocations must be validated before use.
7. **Keep the reference client read-only.** Never patch or bypass VMProtect,
   licensing or anti-cheat protections. Network compatibility must be validated
   through an authorised server.

## 3. Target platform and performance goals

- **Primary platform:** Windows on modern PCs with dedicated GPUs.
- **Architecture direction:** evaluate a 64-bit client to avoid the address-space
  ceiling of a 32-bit process when handling larger asset sets. Preserve the
  original network protocol where lawful and technically possible; process bitness
  must not be confused with protocol compatibility.
- **Frame pacing:** expose configurable frame-rate limits, including 60, 120 and
  144 FPS where the display and hardware support them. These are operating targets,
  not promised results. Avoid regressions in p95/p99 frame time or stutter while
  raising average FPS.
- **Heavy-scene capacity:** build repeatable scenes with increasing visible model,
  triangle, texture, animation and effect counts. Record the point at which CPU,
  GPU, RAM or VRAM becomes the bottleneck.
- **Responsiveness:** measure input-to-response where practical, plus simulation,
  render and presentation times separately.
- **Loading:** measure cold and warm startup, model/texture decode, map transition
  and peak memory during loading.
- **Stability:** test long sessions, repeated map transitions, resource churn and
  device/backend errors.

No FPS, latency, VRAM or compatibility claim is considered achieved until a real
Windows/GPU run records hardware, driver, build, scene, settings, run duration and
raw output in `docs/evidence/`.

## 4. Intended architecture

```
Original files (read-only, external)
        |
Format readers + validation (BMD / OZJ / OZT / TGA / terrain / audio)
        |
Canonical CPU representation (Mesh / Skeleton / Material / Texture / Terrain)
        |
Resource manager (cache, async jobs, budgets, lifetime, diagnostics)
        |
Renderer interface (backend-independent scene and draw data)
        |
GPU backend (API selected by prototype evidence)
        |
Window / input / frame loop / UI / audio / authorised network integration
```

Keep format parsing independent of the rendering API. The existing CPU-side
`Mesh`, `Skeleton`, buffer packing, skinning and frustum-culling code should
remain testable without a GPU. Add GPU-specific code behind a renderer boundary
rather than making asset readers depend on Windows or graphics headers.

### Renderer API decision

Do **not** select a graphics API based only on reputation or theoretical maximum
performance. Prototype the smallest real workload on the target Windows hardware
and compare implementation risk, driver coverage, frame time, upload cost,
resource management and debugging support. Evaluate Direct3D 12 and Vulkan for
the high-performance path; consider Direct3D 11 as a simpler baseline if it helps
separate renderer problems from API complexity. Record the decision and evidence
in `DECISIONS.md` before committing to a long-lived backend.

**Status clarification (DEC-014):** This is a proposed evaluation plan, not an
adopted API change. DEC-008/DEC-008-A formally adopt OpenGL VBO/VAO and reject
D3D12/Vulkan based on older-hardware compatibility, which conflicts with this
plan's proposed evaluation. Resolve whether that compatibility requirement still
applies before writing an API-specific backend. No choice has been validated by
a Windows/GPU prototype.

## 5. Large-content strategy

- Use 64-bit indices only where required; validate index ranges and choose compact
  formats when they are sufficient.
- Keep meshes indexed and avoid CPU expansion of indexed geometry per frame.
- Batch by material/state and instance repeated objects where appropriate.
- Upload immutable geometry once and update dynamic buffers selectively.
- Cache decoded textures and GPU resources with explicit memory budgets and
  observable eviction behavior.
- Decode and prepare resources on worker threads; perform graphics API operations
  according to the selected API's threading rules.
- Use mipmaps, texture filtering and compressed GPU formats where they preserve
  appearance and the source data/license pipeline permits them.
- Add LOD and occlusion culling only with correctness tests and representative
  scenes. Do not trade away nearby visual fidelity or gameplay visibility silently.
- Treat malformed, unusually large or adversarial assets as untrusted input and
  enforce allocation/count limits.

## 6. Benchmark contract

Every performance report should include:

- exact commit/build configuration, OS, CPU, GPU, RAM, driver and resolution;
- scene identifier and counts of visible objects, meshes, triangles and textures;
- warm-up policy, run duration and at least five measured repetitions where feasible;
- median, p95 and p99 frame time, FPS derived from frame-time data, plus stutter
  counts and CPU/GPU/RAM/VRAM metrics when available;
- cold/warm load timing and input-latency methodology where relevant;
- raw command output or capture in `docs/evidence/`;
- a comparison under equivalent conditions and an explicit PASS/FAIL/BLOCKED/
  NOT EXECUTED classification.

Asset micro-benchmarks are useful but do not establish whole-client FPS. A CPU
software-renderer result is useful for validating geometry and determinism, but
does not establish GPU performance.

## 7. Implementation order

1. Reconcile continuity documents and the current test/build counts; preserve
   historical evidence but make the current state unambiguous.
2. Resolve and test real UV-coordinate interpretation and texture/material
   association for original assets. Keep placeholder UV mapping explicitly
   separate from production rendering.
3. Create a minimal Windows executable with a real window, input, frame loop and
   one GPU backend prototype.
4. Render a real original model with correct geometry and original texture data;
   compare screenshots against the reference client under documented conditions.
5. Establish GPU baseline captures and a repeatable heavy-scene benchmark.
6. Implement resource caching/async loading, indexed draw submission, batching,
   instancing and GPU skinning as evidence indicates.
7. Expand from one model to terrain, scene objects, characters/animation, effects,
   UI and audio; then validate long-session stability and authorised network
   compatibility.
8. Only after the vertical slice is measured and stable, expand support for
   substantially heavier custom content (including glTF where appropriate).

## 8. Current limitations that remain explicit

- The CPU software rasterizer is a validation tool, not the production renderer.
- The current planar/checkerboard texture path is a placeholder, not real BMD UV
  decoding or visual compatibility.
- A working Windows GPU executable, measured FPS/frametime, actual VRAM use,
  input latency, long-session stability and authorised server compatibility must
  not be marked complete until run and evidenced on target hardware.
- Unknown or opaque asset variants remain unsupported until understood through
  legitimate format information or compatible open implementations.
