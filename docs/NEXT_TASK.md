# Next task — first verifiable Windows/GPU vertical slice

## Product target

An independent, playable MU Online Season 6.20 client for Windows, preserving
correct interpretation of original assets and aiming for high performance on
modern PCs with dedicated GPUs. The target and measurement contract are in
[`PERFORMANCE_TARGET.md`](PERFORMANCE_TARGET.md). This is the **target**, not an
achieved result.

The current implementation is a C++17 asset library and inspection tools, plus a
CPU-only diagnostic rasterizer. There is no runnable Windows client, GPU backend,
functional window/input path, or playable game session. This session is fixed to
branch `arena/1d9c1f53-clienterecriado`.

## Prerequisites and blockers

The next product milestone requires evidence from Windows with a GPU. This
sandbox has no Windows, Windows cross-compiler, Wine, or GPU. Therefore:

- do not call Linux builds or PPM output evidence of target execution;
- do not report FPS, frametime, VRAM, input latency, or server compatibility
  until the relevant test has actually run and its log is saved;
- if Windows/GPU hardware remains unavailable, limit work to safe parser and
  documentation tasks, leave the milestone blocked, and record why.

The reference `heldergaraujo2/Cliente` remains read-only. Do not modify or
redistribute protected executables/assets; do not unpack VMProtect, remove
licensing/authentication, or bypass anticheat. The BMD v0x0C cipher and `Enc*`
terrain cipher remain **BLOCKED** under `AGENTS.md`. Do not expand that work
unless legitimate source/authorization changes the documented blocker. For other
BMD semantics, use legitimate/licensed sources or independent asset evidence and
controls; statistical plausibility alone does not validate UV, materials,
animation, or appearance.

## Work sequence

1. **Close each audit unit with evidence.** Confirm branch/HEAD, clean build,
   tests, sanitizers, documentation checker, available full manifest, and the
   reference hash/status before and after. Save exact outputs under
   `docs/evidence/` and update `CURRENT_STATE.md`.
2. **Resolve semantics needed for one original asset.** Select a BMD using a
   body layout that can be investigated without crossing the cipher blocker.
   Trace counts, vertices, normals, triangle corners, material/texture
   association, and UVs. Keep probe, structural checks, decoded pixels, meaning,
   and rendering separate. Do not claim a supplied texture belongs to a model
   without evidence. OZJ still needs a pixel decoder; OZT conversion exists, but
   visual association/appearance is unverified.
3. **Produce a real Windows build and platform boundary.** On a Windows machine
   or runner, produce a runnable 64-bit executable as the initial candidate and
   document any ABI/data evidence that requires 32-bit compatibility. Do not
   assume the original 32-bit client forces the reconstruction to be 32-bit.
   Keep shared C++17 asset code isolated from platform/graphics headers.
4. **Prove window, input, and GPU device creation.** Implement minimal prototypes
   and draw one known test triangle. Compare D3D12 and Vulkan on target hardware
   where practical; include D3D11 as a simpler fallback comparison. Record API,
   adapter, driver/runtime, capabilities, and rationale in `DECISIONS.md`. Do
   not select an API based only on imports from the old client.
5. **Load and render an original model.** Integrate supported BMD geometry,
   indexed buffers, and the correct material/texture with semantically verified
   UVs. Compare against a legitimate visual reference with controlled scene,
   camera, and resolution; list unexplained differences. CPU rasterization,
   planar mapping, checkerboard, or a manually supplied unassociated texture do
   not satisfy this step.
6. **Measure a GPU baseline.** Record hardware/OS/driver/API, build settings,
   scene, object/triangle counts, resolution, cache state, and at least five
   repetitions where feasible. Report median, p95/p99, variability, stutter,
   and available CPU/GPU/RAM/VRAM metrics. This is a baseline, not a promise.
7. **Optimize only after the baseline.** Prioritize indexed buffers, material
   batching, instancing, resource streaming/caching, culling, and GPU skinning
   when real measurements identify a bottleneck. Repeat equivalent tests and
   validate visual correctness and stability after each change.
8. **Implement gameplay networking only with authorization.** Use authorized
   protocol documentation/implementations and an authorized test server. Until
   then, login, authentication, character creation, movement, inventory, combat,
   persistence, and network compatibility remain `NOT IMPLEMENTED` /
   `NOT EXECUTED`.

## Acceptance criteria for the graphics milestone

- `make clean && make` has no warnings; `make test` and `make test-san` have no
  failures; the documentation checker has run after metric changes.
- A Windows build produces a runnable executable; a real window/input path and
  GPU device are recorded. A source stub, Linux build, or CPU rasterizer does
  not count.
- A known test triangle is drawn on the GPU; device/window shutdown is clean.
- At least one **original** model is drawn through the GPU backend with verified
  geometry interpretation, material/texture association, and coordinates; the
  result is compared against a documented visual reference.
- Baseline conditions and raw results are saved under `docs/evidence/`; no FPS
  or memory improvement is claimed without repeatable comparison.
- Every unrun network/server/platform test remains marked `NOT EXECUTED`.
- No proprietary asset, protected executable, credential, or secret is added to
  Git.
- Update `CURRENT_STATE.md`, `TEST_RESULTS.md`, `CHANGELOG.md`, and this file;
  make a small validated commit on the fixed session branch, push only to
  `arena/1d9c1f53-clienterecriado`, and record literal hashes/status.

## Explicitly not the immediate priority

Do not add broad audio, UI, or gameplay systems before a measurable Windows/GPU
graphics path exists, unless a concrete dependency blocks the slice. Do not
continue scanning BMD offsets without a discriminating hypothesis and
same-class control; do not label statistical scores as semantic discoveries.
Do not modify or bypass the reference client.
