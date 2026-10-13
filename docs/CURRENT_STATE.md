# Current state — cycle 17 audit (2026-10-12)

This snapshot separates **probe**, structural validation, decoding, semantics,
and verified rendering. The repository contains a C++17 asset library and
inspection tools; **it is not yet a playable Windows client**.

## Executive summary

- Reference `heldergaraujo2/Cliente` is read-only. Its observed HEAD at the start
  and end of this audit is `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`. No reference
  branch, file, or history was changed. The temporary clone contains `Data/`;
  the Git-tree search found no client C/C++ source.
- `mutool scan` inventories the whole tree but performs **format probes**; it does
  not establish that pixels, UVs, materials, or images were decoded.
- There is a BMD parser for layouts accepted by the current implementation, an
  internal mesh representation, CPU-side buffer preparation, and a **CPU-only**
  rasterizer. PPM output is not the product's graphics milestone and does not
  prove visual compatibility.
- A real OZT was converted to BGRA pixels and explicitly passed to the render
  CLI. The BMD used in that run is a different file; no association between the
  two or visual correctness was demonstrated. BMD UVs still lack a verified
  semantic mapping.
- There is no Windows executable, GPU device, or measured FPS, frametime, or
  VRAM. Windows x86/x64, D3D12/Vulkan/D3D11, game startup, audio, UI, and network
  remain without runtime validation.

## Implemented capability and measured results

### BMD and meshes

`mutool bmdreport` measured 10,090 BMD files. Full output:
[`evidence/ciclo17_bmdreport_final.log`](evidence/ciclo17_bmdreport_final.log).

| Stage / metric | Result | Interpretation |
|---|---:|---|
| Structurally valid containers | **10.023 (99,34%)** | Signature/version and declared v0x0C size consistency |
| Payload access classified by current tool | **8.571 (84,95%)** | 1.245 plaintext + 100 BUX + 7.226 v0x0C-labelled (classifier only) |
| OPAQUE | **1.519 (15,05%)** | Not recovered by the current classifier |
| Geometry extracted by current parser | **8.406 (83,31%)** | Vertices/normals read for accepted body layouts |
| Files with triangle records | **8.405 (83,30%)** | 3.681.664 triangle records read |
| Files with loose bone records | **3.693 (36,60%)** | 7.879 bones; discoverability is not animation semantics |
| Out-of-range vertex references | **523,717** | Reference fields, not triangles |
| Out-of-range normal references | **507,068** | Reference fields, not triangles |
| Combined invalid references | **1,030,785 / 22,089,984 (4.666%)** | Six index fields per triangle |

**Policy caveat:** `AGENTS.md` still classifies the BMD v0x0C cipher as
**BLOCKED**. The counts labelled v0x0C-decryptable by the existing tool are an
observed implementation/classifier result, not permission to further reverse,
claim an officially validated cipher, or redistribute results as an authorized
format. Do not expand that work unless a legitimate source/authorization changes
the documented blocker. The remaining `Enc*` terrain cipher is also BLOCKED.

The stages use the same denominator of 10,090 and are not a fully cumulative
hierarchy. Positive topology is a subset of parsed geometry, verified by test.
`bmdreport` exits with code 2 because OPAQUE payloads remain; this is not reported
as an entirely successful decode.

`mutool meshimport` read 8,406 meshes, 8,327 with triangles. Of 3,681,664 body
triangles, **3,168,360** were emitted and **513,304 (13.94%)** were dropped due
to at least one invalid vertex/normal reference. Zero meshes were rejected for
another reason in that stage. A triangle counts once as dropped even if it has
multiple invalid references. This differs from 4.666%, which counts individual
references.

The UV-per-corner index and complete mesh/material/texture association are not
known. Statistical float/offset candidates documented in
[`ASSET_FORMATS.md`](ASSET_FORMATS.md) are not semantic validation; the importer
does not expose usable BMD UVs (`has_texcoords` remains false when mapping is
unknown).

### Textures and diagnostic rendering

- `mutool scan`: 29,060 files, 2,372.04 MiB. Probes accepted 13,832/13,928 OZJ
  and 3,404/3,470 OZT. The BMD scan counter is container recognition, not
  geometry extraction. Header/payload-derived image-byte totals are **estimates**,
  not measured RAM/VRAM. See
  [`evidence/ciclo17_scan_final.log`](evidence/ciclo17_scan_final.log).
- The standard OZT conversion path supports uncompressed 24-bit BGR and 32-bit
  BGRA input to BGRA8; unit tests use synthetic fixtures. Real file
  `Axes06.ozt` was loaded as 64×64 BGRA8 (declared payload 16,384 bytes, 26-byte
  trailer).
- A real `mutool render` call explicitly loaded that OZT alongside
  `Player/Axe11.bmd` (1,364 vertices, 2,665 triangles), output 800×600, exit 0.
  The OZT was provided explicitly and was **not associated** with the model by
  evidence. Without decoded BMD UVs, the rasterizer reports a planar diagnostic
  mapping. Loaded/rendered pixels do not prove a faithful appearance. Logs:
  [`ciclo17_ozt_real_asset_info.log`](evidence/ciclo17_ozt_real_asset_info.log),
  [`ciclo17_ozt_real_asset_load.log`](evidence/ciclo17_ozt_real_asset_load.log),
  and [`ciclo17_ozt_real_asset_status.log`](evidence/ciclo17_ozt_real_asset_status.log).
- OZJ: the code probes JPEG structure/markers; it **does not decode pixels**.
  TGA: probe and uncompressed type-2 read are implemented; RLE type 10 is not
  decoded. Recognition is not decoding.
- `--checkerboard` and planar mapping are CPU diagnostic tools, not UVs or
  correctly textured original rendering.

### Other measurements that are not product metrics

- `mutool terrain`: 76/76 complete grids; 72/76 are below the 30.00 smoothness
  heuristic. Exit code 2 is due to four values at/above the heuristic threshold.
  This does not prove those files invalid or validate terrain units/scale.
  Evidence: [`ciclo17_terrain_after_audit.log`](evidence/ciclo17_terrain_after_audit.log).
- `mutool bmdanim`: 500 files have bytes after bones; 385 are divisible by
  `boneCount × 12`. This is a **size test**, not animation decoding or
  identification of the three floats.
- `mutool culltest`: synthetic scene; 8,086/8,406 bounds were classified as
  culled under artificial camera/placement parameters. It counts avoided draws
  in that scenario, **not FPS or frametime**.
- `build/bench_assets`: CPU microbenchmarks for parsing/conversion/preparation.
  They do not measure GPU, full-client loading, FPS, or latency. Latest single-run output:
  [`ciclo17_bench_final.log`](evidence/ciclo17_bench_final.log).

## Build and tests

| Check | State in this audit |
|---|---|
| `make clean && make` | **PASS** — full build, no warnings |
| `make test` | **PASS** — **539 checks, 0 failures** |
| `make test-san` | **PASS** — **539 checks, 0 failures**, no ASan/UBSan report |
| `python3 tools/check_docs_consistency.py /tmp/cliente_ref/Data` | **PASS (exit 0)** — 54 checks, 0 failures against reference HEAD `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` |
| Full available patch-manifest check | **INCOMPLETE / exit 2**: 188 entries, 116 match, 8 size mismatches, 11 CRC mismatches, 53 missing in the available tree; not 188/188 coverage |
| Windows build/run and GPU device | **NOT EXECUTED** — no Windows, cross-toolchain, or GPU here |
| Equivalent visual comparison with reference client | **NOT EXECUTED** |
| FPS, frametime, VRAM, runtime RAM, and latency | **NOT EXECUTED** |
| Authorized server/network compatibility | **NOT EXECUTED** — no authorized test server |

Manifest output:
[`evidence/ciclo17_reference_manifest_verification_final.log`](evidence/ciclo17_reference_manifest_verification_final.log).
Manifest verification read asset bytes to compare sizes/CRCs; protected/licensing
files were not otherwise inspected. No such file was changed. The reference tree
remains read-only.

## Product compatibility matrix

| System | Actual status |
|---|---|
| Runnable Windows client / startup / functioning window and input | **NOT IMPLEMENTED / NOT EXECUTED** |
| Windows x86 and x64 | **NOT EXECUTED**; architecture choice still needs a validated target build |
| Renderer API policy | **OPEN / BLOCKED** — DEC-008 adopts OpenGL VBO/VAO; the newer target plan proposes D3D12/Vulkan/D3D11 evaluation; see DEC-014 |
| GPU backend and device creation | **NOT IMPLEMENTED / NOT EXECUTED** — no Windows/GPU prototype |
| BMD model assets | **PARTIAL** — supported subset and counts above; 1,519 OPAQUE |
| OZT textures | Pixel conversion implemented; one real OZT loaded, no visual association validated |
| OZJ textures | JPEG probe only; pixel decoding **NOT IMPLEMENTED** |
| BMD UV/material/texture association | **BLOCKED** — no semantic mapping or verifiable visual comparison |
| Terrain | Partial parsing; `Enc*` cipher, vertical scale, and appearance unresolved |
| Bones/skinning/animation | Partial bone-record parsing; no verified BMD weights, transform order, or keyframe semantics |
| UI, HUD, fonts, effects, particles | **NOT IMPLEMENTED** |
| Audio | **NOT IMPLEMENTED** |
| Network, authentication, game session | **NOT IMPLEMENTED / NOT EXECUTED**; no bypass or unauthorized-server testing |
| Client performance and long-session stability | **NOT EXECUTED** |

The first graphics milestone remains: **a real Windows executable, a created GPU
device, and at least one original model rendered with correctly interpreted
geometry and texture association, verified**. CPU rasterization, PPM output,
checkerboard, or planar UV do not satisfy it.

## Key evidence

- Final whole-tree scan, BMD report, and mesh import:
  `ciclo17_scan_final.log`, `ciclo17_bmdreport_final.log`,
  `ciclo17_meshimport_final.log`.
- Real OZT diagnostic run: `ciclo17_ozt_real_asset_*.log` (explicitly not a
  verified model/texture pair or GPU render).
- Six differing local PPM variants were kept outside Git; hash/size comparison
  only: `ciclo17_ppm_recovery_comparison.log`; neither set proves GPU rendering.
- 188-entry patch-manifest comparison:
  `ciclo17_reference_manifest_verification_final.log`.
- Clean build, normal/sanitizer tests, benchmark build/run, and documentation
  check: corresponding `ciclo17_*_final.log` files under [`evidence/`](evidence/).

## Next task

Follow [`NEXT_TASK.md`](NEXT_TASK.md): prepare the first verifiable Windows/GPU
vertical slice on an appropriate machine/toolchain, without claiming runtime
progress in this environment. In parallel, advance BMD/UV/material semantics
only with legitimate sources and controls capable of testing meaning, not just
statistical plausibility.

## Recovery audit baseline and publication

This read-only Git audit began on 2026-10-12, before the documentation-only
reconciliation recorded in this snapshot. At audit start:

- Fixed session branch: `arena/1d9c1f53-clienterecriado`; local `HEAD` and its
  upstream both matched `f839757d7b0b56d0b0a8d542aae9c67df1628d37`.
- `git status --short --branch`:
  `## arena/1d9c1f53-clienterecriado...origin/arena/1d9c1f53-clienterecriado`.
  Staged/unstaged diffs and untracked paths were empty;
  `git rev-list --left-right --count @{upstream}...HEAD` returned `0 0`.
- `git ls-remote` and GitHub REST branch queries agreed:
  `arena/1d9c1f53-clienterecriado` =
  `f839757d7b0b56d0b0a8d542aae9c67df1628d37`;
  `arena/519f8fc9-clienterecriado` =
  `4c36befab01263bf95a509d1c8306c56a1e876c5`;
  `main` = `972fabcea1b0d4e80345f0f5abd287b46d819455`.
  No checkout, push, or other write to branch 519 or `main` occurred.
- The fixed branch was two commits beyond the published 519 snapshot at the
  audit baseline: `be4f79ae5a3c003502e68a5cc245f8f0e6cdba5f`
  (`recovery: reconcile cycle-17 asset audit`) and
  `f839757d7b0b56d0b0a8d542aae9c67df1628d37`
  (`docs: record cycle-17 recovery publication state`). Relative to 519, the
  published diff was 78 files, 3,950 insertions and 1,684 deletions.
- Recovery safety copies remain local and untouched: `stash@{0}`; the workspace
  archive `/home/user/ClienteRecriado_cycle17_workspace_before_fastforward.tar.gz`
  (SHA-256 `cbbe233b47f79c97f1d5a94241e6623f1f1da214304c6f20e88b4bb9e135bc1e`);
  and `/home/user/ClienteRecriado_recovery_before_fastforward.tar.gz` (SHA-256
  `4a9ca9872e55de44fde65d138e649da5aa9e884f9b5c88d66744092f1f6304f7`). The
  six divergent CPU PPM variants remain outside Git at
  `/home/user/ClienteRecriado_recovery_ppm/`; their hash/size-only comparison is
  `evidence/ciclo17_ppm_recovery_comparison.log`. None is GPU-render evidence.
- The reference clone remained read-only at
  `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` with no tracked or staged diff.
  The test-count and renderer-API documentation discrepancies found in this
  audit are reconciled in `TEST_RESULTS.md` and DEC-014; no Windows/GPU
  milestone was executed. Full recovery evidence:
  [`evidence/ciclo17_git_recovery.log`](evidence/ciclo17_git_recovery.log).

The documentation-only reconciliation for this audit is committed on the fixed
session branch; the final branch tip and clean post-push status are reported in
the cycle summary.
