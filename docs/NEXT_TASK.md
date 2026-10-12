# Next task — Windows GPU renderer vertical slice

## Product target

The selected target is maximum practical performance on modern Windows PCs with
powerful dedicated GPUs, while preserving original MU Online assets and allowing
opt-in visual improvements and heavier scenes. The authoritative target and
architecture contract is [PERFORMANCE_TARGET.md](PERFORMANCE_TARGET.md).

This is a target, not a claim of achieved FPS. The current software rasterizer is
CPU-only and is a validation tool, not the production renderer. UV mapping in
that path is explicitly a placeholder.

## Why this is the next task

The repository already has substantial asset-analysis tooling, internal mesh
and skeleton representations, CPU-side buffer packing, skinning, culling and a
deterministic software-rendering proof of concept. The highest-value next step
is to prove a real Windows/GPU path with an actual original model, not to expand
side systems such as audio before the rendering foundation exists.

## Work sequence

1. **Reconcile the current state first.** Inspect branch/working-tree state and
   continuity documents. Re-run clean build, unit tests, sanitizer tests and
   documentation consistency checks where the environment supports them. Update
   stale test counts and explicitly label Windows/GPU measurements as
   NOT EXECUTED until actually run.
2. **Resolve real material coordinates.** Continue the BMD/UV investigation using
   real assets and appropriate same-class controls. Distinguish container
   recognition, usable geometry, real UVs and complete visual compatibility.
   Do not present planar/checkerboard coordinates as decoded UVs.
3. **Prototype the Windows rendering boundary.** Build a real window, input
   handling and frame loop, then a minimal GPU-backed path that draws a triangle
   and reports device/driver capabilities. Keep platform and graphics headers
   out of the asset library.
4. **Choose the graphics API from evidence.** Compare a minimal Direct3D 12 and
   Vulkan prototype on the target machine where practical; consider Direct3D 11
   as a simpler comparison if it reduces risk. Record the choice and rationale
   in DECISIONS.md before expanding the backend.
5. **Render one real asset.** Load a supported BMD through the existing asset
   pipeline and draw its geometry with correctly associated original texture
   data and real UVs. If an asset's UV or texture path is not yet understood,
   report the exact blocker instead of substituting a placeholder silently.
6. **Establish a GPU baseline.** Capture hardware, OS, driver, resolution,
   build settings, object/triangle counts, frame-time distribution and available
   CPU/GPU/RAM/VRAM metrics. Repeat runs and store raw evidence under
   docs/evidence/.
7. **Optimise only after the baseline exists.** Prioritise indexed GPU buffers,
   batching/instancing, asynchronous asset preparation, texture/resource caches,
   GPU skinning and measured culling. Validate each change under equivalent
   conditions and check p95/p99 frame time, not average FPS alone.

## Acceptance criteria

- make clean && make && make test succeeds with zero test failures and no
  compiler warnings in the supported development environment.
- make test-san succeeds with zero sanitizer reports.
- Documentation consistency checks pass when real asset counts are available.
- A Windows build produces a runnable executable and creates a real graphics
  device; a CPU-only render or source-level stub does not satisfy this criterion.
- The GPU prototype draws a known test triangle and logs adapter/API/device
  details without crashing on startup or shutdown.
- A real supported original model is rendered correctly before claiming the
  asset-to-GPU path complete.
- Benchmarks include raw evidence, at least five measured repetitions where
  feasible, median/p95/p99 frame time and hardware/software details.
- All unrun Windows/GPU/server checks remain NOT EXECUTED; do not fabricate
  FPS, latency or VRAM results.
- No original asset bytes, protected binaries, credentials or licensing/anti-cheat
  components are committed.
- CURRENT_STATE.md, CHANGELOG.md and this file are updated with the actual
  result; commit and push status is reported literally.

## Explicitly not the next priority

Do not start a broad audio subsystem or additional side features before the
Windows/GPU vertical slice is proven, unless a concrete blocker or measurement
shows that another task is more valuable. Do not patch or bypass the protected
reference client.