# Current compatibility status

## Classification scale

| Classification | Meaning |
|---|---|
| **IMPLEMENTED AND VALIDATED** | Implemented and verified against real data with tests appropriate to the claim |
| **PARTIAL** | Works only for a subset or a particular layer; the gap is quantified |
| **BLOCKED** | Depends on data, semantics, or an environment not available yet |
| **NOT IMPLEMENTED** | No functional implementation exists for this capability |
| **NOT EXECUTED** | The required runtime test has not been performed |

A header probe does not prove decoding; converted bytes do not prove semantics;
a PPM does not prove visual compatibility. The stages below remain separate.

## 1. BMD parser results and limits

`mutool bmdreport` measured 10.090 BMD files in the available reference tree.
The table is generated from parser fields; these counts do not, by themselves,
mean a model can be displayed correctly.

| Stage | Measurement | Files | % of 10.090 |
|---|---|---:|---:|
| 1 — structure | magic/version and declared v0x0C size consistency | **10.023** | **99,34%** |
| 2 — payload access | current-tool categories: plaintext, BUX-labelled, or v0x0C-labelled | **8.571** | **84,95%** |
| 3 — vertices | positions/normals extracted from a body layout accepted by the parser | **8.406** | **83,31%** |
| 4 — topology | files with a triangle array read | **8.405** | **83,30%** |
| 5 — loose bones | records located by the current loose criterion | **3.693** | **36,60%** |
| 5b — strict bones | conservative subset with strict names | **1.186** | **11,75%** |
| 6 — UV | per-corner mappings with semantics verified by the importer | **0** | **0,00%** |

The level-2 output comprises 1.245 plaintext, 100 BUX-labelled, 7.226
v0x0C-labelled, and 1.519 OPAQUE (15,05%). **Policy caveat:** `AGENTS.md`
continues to classify the BMD v0x0C cipher as **BLOCKED**. The existing tool's
v0x0C-labelled count is an implementation/classifier observation, not permission
to extend reverse engineering or a claim that the cipher is an officially
validated/authorized format. Do not expand that work unless a legitimate source
or authorization changes the blocker. `bmdreport` exits 2 because OPAQUE payloads
remain; it is not an all-success result.

All stages use the same denominator and do not form a fully cumulative hierarchy.
Positive topology is a subset of parsed geometry, verified by test; the bone
metric is independent. Stage 3 is counted only when the parser extracts
positions/normals from a body layout it accepts, not merely when a container
header is recognized.

### Invalid references and dropped triangles

Across 3,681,664 triangle records there are six vertex/normal reference fields
per triangle:

- 523,717 out-of-range vertex references;
- 507,068 out-of-range normal references;
- combined: 1,030,785 / 22,089,984 = **4.666% of reference fields**.

`mutool meshimport` drops a triangle once if any required reference is invalid:
3,168,360 were emitted and **513,304 (13.94%) were dropped**; zero meshes were
rejected for another reason in that stage. Dropped-triangle rate is not the same
as invalid-reference rate. The importer does not clamp indices, which could
silently deform geometry.

Full output: [`evidence/ciclo17_meshimport_final.log`](evidence/ciclo17_meshimport_final.log)
and [`evidence/ciclo17_bmdreport_final.log`](evidence/ciclo17_bmdreport_final.log).

### UVs, materials, and interpretation

**Status: BLOCKED / not semantically validated.** The importer has not established
which coordinate belongs to each triangle corner or the complete
mesh/material/texture association. Statistical float/offset candidates in
[`ASSET_FORMATS.md`](ASSET_FORMATS.md) are hypotheses, not semantic confirmation
or proof of appearance. `Mesh::has_texcoords` remains false for BMD when mapping
is unknown.

The CPU rasterizer accepts an explicit texture and uses planar mapping only as a
diagnostic. A real OZT was loaded alongside a different BMD without evidence
that the two are a matching pair. That execution proves the read/conversion path
for pixels, not association, UVs, compatibility, or GPU rendering.

### Level-4 import breakdown

The per-file helper reports **1,685** files without importable triangle topology
out of 10,090: 1,483 OPAQUE cases in that view, 67 invalid containers, 134 bodies
whose geometry was not parsed, and one zero-face file. This explains one import
criterion; the `bmdreport` OPAQUE total remains 1,519 and must not be replaced by
1,483.

A cycle-6 multi-mesh investigation measured the hypothesis that a texture name
can be located at `tri_end-4`. In 4,256 files / 12,590 meshes, the strict test
found 1,796 (14.27%); a same-class control found 89/236,677 = 0.0376%, ratio
379×. The weak test yielded 52.20% versus a 77.51% weak control (0.67×; not
discriminative); files with at least one strict hit: 38.79%. These are
statistical results for a **name-position search**, not a UV index or proof of a
material. Detailed evidence:
[`evidence/ciclo6_multimesh_validation.log`](evidence/ciclo6_multimesh_validation.log).
The consistency checker re-runs the measurement.

## 2. Asset-format compatibility

The following counts come from `mutool scan` over 29,060 files (2,372.04 MiB).
“Probe OK” does not mean pixels are decoded or an asset is renderable.

| Format | Files | Probe / reported validation | Beyond the probe |
|---|---:|---:|---|
| OZT | 3,470 | 3,404 (98.10%) | **PARTIAL** — standard 24-bit BGR and 32-bit BGRA conversion; synthetic fixtures and one real OZT loaded; no visual association verified |
| OZJ | 13,928 | 13,832 (99.31%) | **PARTIAL** — JPEG structure/markers; pixels not decoded |
| BMD | 10,090 | 10,066 recognized by probe; 10,023 structurally valid in report | **PARTIAL** — 8,406 geometries and 8,405 triangle arrays; UV/material not confirmed |
| TGA | 15 | all 15 probes accepted | **PARTIAL** — uncompressed type-2 read; RLE type 10 is not decoded |
| `.att` | 105 | all 105 probes accepted | **PARTIAL** — grids can be parsed; `Enc*` terrain remains blocked |
| `.map` | 79 | all 79 probes accepted | **PARTIAL** — container/grid parsing; presentation not validated |
| terrain `.obj` | 81 | all 81 probes accepted | **PARTIAL** — probe does not establish semantic/rendering support |

The 2,893.80 MiB reported by the scan is an **estimate from headers/payloads**
(OZJ as an RGBA8-equivalent, OZT/TGA from calculated or declared payload). It is
not decoded OZJ pixels, actual allocation, RAM, or VRAM. Full scan:
[`evidence/ciclo17_scan_final.log`](evidence/ciclo17_scan_final.log).

### Real OZT evidence

`Data/Item/Axes06.ozt` is 64×64, 32 bpp, with a declared 16,384-byte payload and
a 26-byte trailer. It passed `OztRead` and BGRA8 conversion in the recorded run.
The extension alone was not used as evidence. The converted image was not
compared with output from the original client.

## 3. Rendering and platform

| Capability | Current classification |
|---|---|
| CPU rasterizer and PPM output | **PARTIAL — diagnostic tool**; not the product renderer or compatibility proof |
| Functional Windows window, input, and loop | **NOT IMPLEMENTED / NOT EXECUTED**; only a platform skeleton exists |
| GPU renderer and device creation | **NOT IMPLEMENTED / NOT EXECUTED** |
| D3D12, Vulkan, D3D11 | **NOT EXECUTED**; backend not selected by an on-target prototype |
| Original untextured model | **PARTIAL** — parser and CPU output exist; geometric/visual equivalence to reference is not validated |
| Original textured model | **BLOCKED** — no semantic UV/material mapping or verifiable visual comparison |
| Skinning/animation | **PARTIAL/BLOCKED** — some bone records are parsed; BMD weights, transform order, and keyframe semantics are unverified |
| Complete terrain | **BLOCKED** — `Enc*` cipher, vertical scale, and appearance unresolved |
| Effects, particles, glow, sky, weather | **NOT IMPLEMENTED** in the reconstruction |
| UI/HUD/text/game input | **NOT IMPLEMENTED** |
| Audio | **NOT IMPLEMENTED** |
| Windows x86 client | **NOT EXECUTED** |
| Windows x64 client | **NOT EXECUTED** — target/compatibility needs evidence from a real build |
| Linux | Library and tools build; Linux is not the distribution target |

The first graphics milestone requires a real Windows executable, a created GPU
device, and an original model rendered with correctly interpreted geometry and
texture association. CPU-only output, checkerboard, and planar mapping do not
satisfy it.

## 4. Network, game session, and security

- Network, login, authentication, character creation, movement, inventory,
  combat, and persistence: **NOT IMPLEMENTED / NOT EXECUTED**.
- No authorized server test was performed in this audit.
- Do not bypass login, licensing, anticheat, VMProtect, or other controls of the
  reference client. Any future network validation must use authorized
  specifications and a test server.
- Parsers are exercised with synthetic fixtures, bounds tests, and ASan/UBSan;
  this is not a complete security audit or proof against malicious inputs.

## 5. Reference evidence and coverage

The reference `heldergaraujo2/Cliente` was used read-only for asset and metadata
inspection. Its commit is `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`. Verification of the
available 188-entry manifest exited 2: 116 matches, 8 size mismatches, 11 CRC
mismatches, and 53 missing paths in the available tree. Thus there was **no
188/188 verification**, and this clone cannot be called intact or installable
based on the manifest. Details: [`REFERENCE_AUDIT.md`](REFERENCE_AUDIT.md) and
[`evidence/ciclo17_reference_manifest_verification_final.log`](evidence/ciclo17_reference_manifest_verification_final.log).

## 6. Tests still absent

Windows startup/build, x86/x64 runtime, graphics API/device, equivalent visual
comparison, FPS/frametime/VRAM/RAM, input latency, long-session stability,
network, and authorized-server tests remain **NOT EXECUTED**. Do not substitute a
CPU benchmark, avoided-draw count, PPM, or memory estimate for those results.
