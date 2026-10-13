# Experimental D3D11 BMD geometry preview

Status: source added; **Windows compilation and runtime are not yet verified**.
This is a diagnostic step toward the asset-to-GPU vertical slice, not a playable
client or the production renderer.

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
.scriptsuild_d3d11_bmd_preview.ps1
```

If the build succeeds, choose a known plaintext BMD v0x0A from the authorized
local reference asset tree and run:

```powershell
.uild\d3d11_bmd_preview.exe "C:\path\to\model.bmd"
```

Replace the example path with the real local file path. The preview will reject
other versions, encrypted files, or files with no surviving valid triangles.
Keep the full compiler output and console output. Record the exact asset path
(or a privacy-safe relative path), parser counts, dropped triangle count,
visible result, and exit status. Do not call it a pass if compilation failed,
the window never displayed the geometry, or the process did not close cleanly.

## Next decision

If the build and geometry preview pass, inspect the model silhouette and parser
counts, then investigate verified UV/material association separately. Do not
enable guessed UVs or infer a texture pairing from filenames or visual
plausibility. D3D11/D3D12 triangle smoke evidence remains separate from this
model-preview milestone.
