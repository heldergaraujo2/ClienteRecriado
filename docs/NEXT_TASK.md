# Next task — first verifiable Windows/GPU vertical slice

## Immediate task — validate BMD topology fixture correction

The owner pulled through `751d9b6` and ran `make -B test`: compilation
succeeded, but the suite reported 560 checks and 6 failures, all in
`topology is a strict subset of geometry`. Inspection found two invalid
fixtures: the single-mesh triangle record was four bytes beyond the parser's
accepted cursor-adjusted start, and the multi-mesh case only changed the global
`meshCount` field on a single-mesh body.

Commit `49b2be3` fixes both fixture layouts on
`arena/1d9c1f53-clienterecriado`. It has not yet been validated on the owner's
PC. Preserve the active D3D11 preview edits and backup; do not use reset/clean
or overwrite the preview file.

Run:

```powershell
git status --short --branch
git pull --ff-only origin arena/1d9c1f53-clienterecriado
wsl -d Ubuntu -- bash -lc "cd /mnt/d/MU-REENGINEER && make -B test"
```

If the regular suite passes, continue with:

```powershell
wsl -d Ubuntu -- bash -lc "cd /mnt/d/MU-REENGINEER && make test-san"
```

If either test fails, capture the complete failing assertions and fix the
specific defect before proceeding. Then run the documentation consistency
checker against the available read-only reference data. Only after these
checks pass should the Windows preview be rebuilt and rerun against
`agnecklace.bmd`. Record exact compiler output, parser counts,
dropped-triangle count, visible result, exit status, and final Git status. The
green debug clear colour and other local preview diagnostics are not part of
the parser/test fix and must remain local until reviewed separately.
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

**Renderer API policy was reopened by the product owner in DEC-014.** The primary target is modern Windows PCs with dedicated GPUs; old-hardware compatibility is not a hard requirement. D3D12 is the first isolated prototype, not yet the final production-backend selection. Compare it against a small D3D11 baseline after the D3D12 triangle runs on the target PC; evaluate Vulkan if portability or evidence justifies it. See docs/RENDERER_API_SPIKE.md.

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
2. **Resolve semantics needed for one original asset.** Start with the source-level findings and exact missing evidence in [`ASSET_GPU_SLICE_AUDIT.md`](ASSET_GPU_SLICE_AUDIT.md). Select a supported BMD body layout without crossing the cipher blocker. Trace counts, vertices, normals, triangle corners, any actually parsed texture-name fields, material/texture association, and UVs. Keep probe, structural checks, decoded pixels, meaning, and rendering separate. Do not claim a supplied texture belongs to a model without evidence. OZJ still needs a pixel decoder; OZT conversion exists, but visual association/appearance is unverified. The audit is repository inspection only; local builds, full-tree commands, and target-PC GPU integration must still be executed in their respective environments.
3. **Produce a real Windows build and platform boundary.** On a Windows machine
   or runner, produce a runnable 64-bit executable as the initial candidate and
   document any ABI/data evidence that requires 32-bit compatibility. Do not
   assume the original 32-bit client forces the reconstruction to be 32-bit.
   Keep shared C++17 asset code isolated from platform/graphics headers.
4. **Windows/GPU smoke spike — completed, isolated only.** D3D12 and D3D11 triangle programs were run on the target Windows PC with an AMD Radeon RX 6600. Both visibly displayed a triangle and reported clean shutdown. Logs: `docs/evidence/d3d12_smoke_20261012.log` and `docs/evidence/d3d11_smoke_20261013.log`. This validates basic device/window/present paths only; it is not a playable client or benchmark. Keep the production API decision open until a real asset scene and equivalent measurements exist. Vulkan remains conditional.
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


## Experimental BMD preview and parallel camera helper

The D3D11 preview source and build script are
`platform/windows/d3d11_bmd_preview.cpp` and
`scripts/build_d3d11_bmd_preview.ps1`. The preview accepts plaintext BMD v0x0A
only and draws validated geometry in a solid diagnostic colour. The separately
recorded D3D11 triangle smoke is not a model-preview result. The task owner
reports a local `agnecklace.bmd` run, but this checkout has no exact preview
build/run log; record the command, counts, visible result and exit status before
using it as acceptance evidence. Preserve the owner's working preview file and
backup; do not change it during the current parallel validation.

A standalone `mu::render::OrbitCamera` helper now exists in
`src/mu/render/orbit_camera.hpp/.cpp`. Its Linux C++17 tests pass, but it is not
wired into D3D11 and does not handle window messages. After the pending parser
validation is complete and the preview edits are safe to integrate, the next
viewer step is to retain object-space vertices, derive the target/initial range
from validated bounds, feed left-drag and wheel deltas to the helper, and create
D3D11 left-handed view/projection matrices in the preview adapter. Validate the
full orbit, clamped pitch, zoom limits, resize/aspect handling, depth range and
clean shutdown on Windows. Do not modify the BMD parser or existing parser tests
for this camera integration.

Texture work remains a separate gate: BMD candidate UV indices and texture-like
names are not verified coordinate/material semantics, `Mesh::has_texcoords`
remains false, OZT conversion is only useful after a verified association, and
OZJ still lacks pixel decoding. Follow [`D3D11_BMD_PREVIEW.md`](D3D11_BMD_PREVIEW.md)
for the test matrix; do not enable guessed UVs or claim a textured original
model until a positive/negative reference control and a Windows visual run are
recorded.
