# Next task — first verifiable Windows/GPU vertical slice

## Immediate task — finish one verifiable BMD-to-GPU vertical slice

The latest Linux validation is recorded in
`docs/evidence/ciclo20_main_loop_validation_20261016.log`: an out-of-tree C++17
build passed without warnings; `make test` and `make test-san` each passed 560
asset checks plus 82 frame-loop checks. The frame-loop foundation now accepts
update/render/present callbacks, but no application, screen manager, window,
renderer, or gameplay callback is connected. Its timings are callback durations,
not full-frame, GPU, or FPS measurements. These Linux tests do not validate
real-asset interpretation or Windows/GPU rendering.

**Muserver handoff:** read [`NETWORK.md`](NETWORK.md) before implementing server
compatibility. No client networking module or verified wire protocol exists in
this repository. Do not guess the original packet header, opcodes, transport,
or cryptography. A shared authorized specification—or an explicit decision to
define a new client/server protocol—must precede compatibility claims and joint
implementation.

1. **Preserve and reconcile the Windows/WSL working copy first.** Record
   `git status --short --branch`, inspect unstaged/staged diffs for
   `platform/windows/d3d11_bmd_preview.cpp` and its `.backup`, and save any local
   version before fetching or updating. Do not run `git reset --hard` or
   `git clean`, and do not overwrite files merely because the sandbox copy
   matches the published version. The separate external copy is not visible
   here; its state is unknown.
2. **Run reference-data checks where authorized data exists.** The sandbox has
   no `/home/user/Cliente_ref` clone or `Data/` tree. On the authorized local
   reference clone, record its HEAD/status before and after, run
   `python3 tools/check_docs_consistency.py` and the documented full-tree
   `mutool scan`/manifest verification, and keep asset bytes outside Git.
3. **Build and run the actual Windows model preview.** After comparing/preserving
   local edits, run `.\scripts\build_d3d11_bmd_preview.ps1` from Developer
   PowerShell, then run the resulting preview with the same authorized
   plaintext `agnecklace.bmd`. Record compiler output, exact invocation,
   parser counts, dropped triangles, visible result, exit status, and final
   Git status. The D3D11 triangle smoke log is not a BMD preview result.
4. **Integrate camera input only after that reconciliation.** Retain
   object-space positions; derive target/range from validated bounds; map drag
   and wheel input to `OrbitCamera`; add D3D11 view/projection, resize/aspect,
   depth and clean-shutdown handling. Test full orbit and zoom on Windows. Do
   not modify the BMD parser or existing parser tests for this integration.
5. **Keep texture semantics separate.** Establish UV/material binding with
   positive and negative controls before using candidate indices or names.
   OZT supports only its documented standard 24/32-bit uncompressed subset;
   the prefixed variant remains unsupported. OZJ still needs an approved pixel
   decoder and tests. BMD v0x0C and `Enc*` remain BLOCKED; do not bypass them.
6. **Measure before optimizing.** After a real model scene renders correctly,
   record repeatable frame-time/CPU/GPU/memory baseline data on the target PC.
   Optimize measured bottlenecks; do not report FPS or compatibility from a
   software renderer, triangle smoke, or unverified preview.

## Product target

An independent, playable MU Online Season 6.20 client for Windows, preserving
correct interpretation of original assets and aiming for high performance on
modern PCs with dedicated GPUs. The target and measurement contract are in
[`PERFORMANCE_TARGET.md`](PERFORMANCE_TARGET.md). This is the **target**, not an
achieved result.

The current implementation is a C++17 asset library and inspection tools, a
CPU-only diagnostic rasterizer, and a callback-driven frame-loop foundation.
There is no runnable Windows client, production GPU backend, functional
window/input path, screen manager, or playable game session. This session is fixed to
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
8. **Implement gameplay networking only after the shared contract is agreed.**
   Follow [`NETWORK.md`](NETWORK.md): cite an authorized protocol source, or
   obtain explicit approval for a new versioned client/server protocol. Use an
   authorized loopback test server and synthetic fixtures. Do not infer opcodes,
   framing, transport, or crypto from generic MU conventions or the protected
   executable. Until the contract and implementation exist, login,
   authentication, character creation, movement, inventory, combat, persistence,
   and network compatibility remain `NOT IMPLEMENTED` / `NOT EXECUTED`.

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
