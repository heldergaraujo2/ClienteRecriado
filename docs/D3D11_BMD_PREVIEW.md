# Experimental D3D11 BMD geometry preview

Status: source exists; the task owner reports a local `agnecklace.bmd` preview
run, but its exact build/runtime output is not recorded here and was not
independently reproduced in this session. Treat the preview as **unverified**
until that evidence is saved. This is a diagnostic step toward the asset-to-GPU
vertical slice, not a playable client or the production renderer.

## What it does

- Accepts a path to a plaintext BMD v0x0A file only.
- Uses the existing BMD probe, body parser, and mesh importer.
- Uses only triangles that pass the importer’s existing vertex/normal index
  validation and reports the number of dropped triangles.
- Uploads positions and triangle indices to D3D11 and displays a solid-colour
  diagnostic preview, normalizing the mesh bounds to fit the window.

## Deliberate limitations

- No texture sampling, UV interpretation, material assignment, skinning,
  animation, lighting, perspective camera, or original-client equivalence.
- It does not decrypt BMD v0x0C or any encrypted container. Those formats are
  rejected by this preview.
- The automatic orthographic axis selection is for inspection only; it is not
  a claim about MU Online's coordinate convention.
- A successful compile or visible mesh alone would not prove correct texture
  mapping or full asset compatibility.

## Build on the target Windows PC

Open **Developer PowerShell for Visual Studio** with the C++ desktop workload
and Windows SDK available, from the local repository clone, on branch
`arena/1d9c1f53-clienterecriado`. Pull the branch first, then run:

```powershell
git pull --ff-only origin arena/1d9c1f53-clienterecriado
.\scripts\build_d3d11_bmd_preview.ps1
```

If the build succeeds, choose a known plaintext BMD v0x0A from the authorized
local reference asset tree and run:

```powershell
.\build\d3d11_bmd_preview.exe "C:\path\to\model.bmd"
```

Replace the example path with the real local file path. The preview will reject
other versions, encrypted files, or files with no surviving valid triangles.
Keep the full compiler output and console output. Record the exact asset path
(or a privacy-safe relative path), parser counts, dropped triangle count,
visible result, and exit status. Do not call it a pass if compilation failed,
the window never displayed the geometry, or the process did not close cleanly.

## Downloadable Windows executable artifact

The workflow
[`.github/workflows/windows-d3d11-bmd-preview.yml`](../.github/workflows/windows-d3d11-bmd-preview.yml)
builds the existing preview with MSVC x64 on a Windows runner and uploads the
artifact `d3d11-bmd-preview-windows-x64`. It runs when the preview sources/build
script change on the session branch. A manual dispatch is available after the
workflow is present on the repository's default branch. The first successful
build is [Actions run 38996411696](https://github.com/heldergaraujo2/ClienteRecriado/actions/runs/38996411696);
the artifact expires on 2026-11-16 at 02:49 UTC. Download it from that run,
then invoke the EXE with a local authorized plaintext BMD path as shown above.
The executable contains no model or texture assets; keep those on the user's
machine.

This workflow validates compilation and checks that the output is a non-empty
PE x64 executable. The hosted runner does not display a window, create a real
D3D11 device, or validate a model. The executable remains an **untextured geometry diagnostic**:
it does not decode BMD UV semantics, bind materials, load textures, or prove
visual compatibility. UV/material/texture validation still requires legitimate
semantic evidence and a real Windows/GPU test.

## Next decision

If the build and geometry preview pass, inspect the model silhouette and parser
counts, then investigate verified UV/material association separately. Do not
enable guessed UVs or infer a texture pairing from filenames or visual
plausibility. D3D11/D3D12 triangle smoke evidence remains separate from this
model-preview milestone.

## Parallel orbital-camera component and integration plan

A platform-independent controller now exists in
`src/mu/render/orbit_camera.hpp` and `.cpp`. It stores a target, yaw, pitch and
orbit distance; yaw wraps through a full revolution, pitch is clamped short of
the world-up singularity, and positive wheel steps zoom in within finite distance
bounds. It returns eye/target/up vectors only. It is **not wired into this
preview** and does not produce a view/projection matrix. Its standalone tests
build to `/tmp` so they do not replace `build/test_assets` or touch the active
Windows validation artifacts; see the parallel entry in `TEST_RESULTS.md`.

When the preview file can be changed after the owner's local validation is
complete, the D3D11 adapter should:

1. Set the initial target to the validated mesh bounds center and choose an
   initial distance from its bounding radius; retain the original object-space
   positions instead of the current CPU-side clip-space normalization.
2. Map left-button drag deltas to `Orbit(yaw_delta, pitch_delta)` and map
   `WM_MOUSEWHEEL / WHEEL_DELTA` to `Zoom(wheel_steps)`. Capture/release the
   mouse correctly, clamp pitch/distance, and update projection aspect after a
   resize. Input sensitivity belongs in the window adapter, not the camera math.
3. Build a left-handed view with `XMMatrixLookAtLH(pose.eye, pose.target,
   pose.up)` and a perspective projection for the current viewport. Upload the
   view-projection matrix via an aligned D3D11 constant buffer, respecting the
   HLSL matrix packing/transposition convention. This path has not been
   implemented or Windows-tested here.

## Texture and semantic gates

The latest parser reads triangle vertex/normal tables and retains a raw
`BmdTriangleCorner::optional` value from the candidate UV-index table. It does
not decode a verified UV coordinate array, the meaning of that index has not
been established across controlled assets, `Mesh::has_texcoords` remains false,
and the mesh model has no verified per-material texture binding. Name-like
texture bytes found while walking a body are not, by themselves, proof of
material association. Keep those values uninterpreted until a positive and
negative reference control establish their meaning.

For a future verified texture path:

- The current OZT reader handles the standard signature with uncompressed
  24-bit BGR or 32-bit BGRA pixels and converts bottom-up rows to top-down RGBA8
  via `OztRead` and `OztToRgbaTopDown`. The observed `00 4B 6D 4B`-prefixed
  variant is recognized but returns unsupported because its layout is unresolved;
  do not imply that every OZT is decoded. Only associate/upload a decoded image
  after BMD-to-texture and UV semantics are independently established. Test row
  orientation, alpha, footer/trailer variants, dimensions and truncation with
  generated fixtures.
- OZJ currently has structural JPEG marker probing only; it locates the image
  within known duplicated-header files but does **not** decode pixels. Select
  and review a decoder/API and its license/build requirements before attempting
  D3D11 texture upload. Add valid-image, orientation/color, malformed and
  truncated-input tests before using it in the viewer.
- Do not turn the raw candidate indices or filename guesses into UVs/materials.
  A real model/texture pair must be verified using independent evidence and a
  control, not plausibility alone.

## Verifiable acceptance tests

| Area | Test required | Current status |
|---|---|---|
| Orbit state | Full-turn yaw, angle wrap, pitch clamp, zoom direction/bounds, target translation and non-finite input rejection | Standalone camera unit tests pass; D3D11 input integration is not implemented |
| BMD geometry | Synthetic triangle/quad triangulation, index bounds and truncation; then rerun the complete parser suite and inspect the chosen local model | Synthetic quad regression is present in `tests/test_assets.cpp`; the full suite was not run by this parallel task |
| UV/material | Positive and negative reference controls for the coordinate array, corner index and material/texture binding; malformed/out-of-range cases | **BLOCKED** pending semantic evidence; no verified BMD UV/material mapping |
| OZT pixels | Generated 24/32-bit fixtures for origin, alpha, footer, bounds and truncation; then validate a verified model/texture pair | Reader/conversion exist; no association is claimed here |
| OZJ pixels | Decoder tests for dimensions, colors/orientation and corrupt/truncated JPEG | **NOT IMPLEMENTED**; current code is a structural probe |
| D3D11 model preview | Build the current preview on Windows, display the selected BMD, orbit all the way around, zoom both directions, resize, check debug-layer output and clean shutdown | **NOT EXECUTED in this session**. `d3d11_smoke_20261013.log` is an isolated triangle smoke test, not a BMD-preview log. The owner reports a local model run, but its exact command/output is not recorded here. |

The D3D11 triangle smoke PASS is evidence for basic window/device/present only.
It is not evidence that the BMD preview, camera controls, texture upload, or
visual compatibility works. No change was made to
`platform/windows/d3d11_bmd_preview.cpp` as part of this parallel unit.
