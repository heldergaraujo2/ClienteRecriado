# Asset-to-GPU vertical-slice audit

Date: 2026-10-13
Scope: read-only inspection of the repository source and existing cycle-17 evidence. No original asset bytes were copied, and no claim is made that a Windows build or local reference-tree command ran in this session.

## Decision

Do not start a second renderer or add guessed UV/material interpretation. The shortest defensible route is to make the asset import path expose and validate the semantics already present in supported body layouts, then connect that result to the existing isolated Windows GPU smoke application.

## Findings confirmed in source/docs

1. `src/mu/assets/bmd_body.cpp` parses supported BMD body layouts and retains triangle corner vertex/normal references. The parser also contains texture-name recognition helpers, but the existence of a name-like byte sequence alone is not proof of the mesh's texture association.
2. `src/mu/assets/bmd_body.hpp` explicitly states that UV semantics, corner-to-UV indices, material/texture association, skinning weights and animation meaning are not established. Its candidate fields remain raw/unnamed.
3. `src/mu/assets/mesh.cpp` imports finite positions and validated triangle indices, computes bounds, and deliberately leaves texture coordinates empty (`has_texcoords = false`). It skips invalid vertex/normal references rather than clamping them.
4. `docs/ASSET_FORMATS.md` and `docs/TEST_RESULTS.md` report 8,406 files with extracted geometry, 8,405 with triangle records, 3,168,360 emitted triangles and 513,304 dropped triangles in the existing mesh-import pass. These are parser/importer counts, not visual correctness.
5. The real OZT diagnostic run loaded `Data/Item/Axes06.ozt` alongside `Data/Player/Axe11.bmd`, but the repository explicitly records that this pairing was supplied manually and is not verified. The render was CPU diagnostic output using planar mapping, not an original-asset GPU render.
6. OZT conversion exists for supported uncompressed true-colour layouts. OZJ parsing is structural only; it does not decode JPEG pixels.
7. The BMD v0x0C and `Enc*` terrain cipher work remain BLOCKED by `AGENTS.md`. This task must not extend that work.

## Required next actions

1. Choose a supported BMD candidate using existing inventory/report output, without inferring that a texture belongs to it from a filename or visual guess.
2. Trace the candidate's existing parser path and report: container/version classification, body layout accepted, vertex/normal/triangle counts, mesh count, source texture-name fields if actually parsed, and all rejected/out-of-range references.
3. Identify a legitimate, independently verifiable control for UV corner mapping and material/texture association. A float-range score or plausible texture filename is insufficient. If no discriminating control/source exists, keep UV/material mapping BLOCKED and record the exact missing evidence instead of guessing.
4. Add tests only for a newly justified interpretation, including a negative/control case and bounds/truncation cases. Do not alter current raw fields into semantic names until this passes.
5. Once semantics are verified, build an asset-to-GPU integration that reuses the existing Windows smoke path, loads actual decoded pixels, and records the exact model/texture pair, command, output, and visual reference. Keep the existing D3D11/D3D12 smoke results separate from this milestone.
6. Run `make clean && make`, `make test`, `make test-san`, and the documentation consistency checker on a machine with the repository and the reference tree. Windows GPU integration and visual comparison must be run on the target PC; this GitHub-only inspection cannot execute commands in the user's `D:\MU-REENGINEER` clone or read files that exist only on that PC.

## Explicit non-results

- No original BMD has been verified as correctly textured.
- No semantic UV/material mapping was discovered by this audit.
- No integrated GPU model rendering, playable client, FPS, frametime, or runtime VRAM measurement was performed.
- No code or original asset files were changed as part of this audit.
