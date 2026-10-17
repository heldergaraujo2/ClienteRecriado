# Test results — latest status as of cycle 21 (2026-10-16)

Results are classified as `PASS`, `FAIL`, `BLOCKED`, `NOT EXECUTED`, or
`INCOMPLETE`. A probe, a parser acceptance, a sanitizer run, and a product
compatibility test answer different questions and are reported separately.

## Cycle 21 — Windows D3D11 preview executable artifact pipeline (2026-10-16)

This change adds a Windows x64 CI build/artifact workflow for the existing
preview plus documentation. It does not edit C++ source, the parser, tests, or
assets. The workflow was added on top of the cycle-20 branch; the first remote
Windows run is pending.

| Check | Result | Evidence / limitation |
|---|---|---|
| Linux `make` build with repository warning flags | **PASS** — exit 0, no warnings | `evidence/ciclo21_linux_validation_20261016.log`; output under `/tmp` |
| Linux `make test` | **PASS** — 560 asset checks + 82 frame-loop checks, 0 failures | Same log; generated/unit fixtures only |
| Linux `make test-san` | **PASS** — 560 asset checks + 82 frame-loop checks, 0 failures; no sanitizer report | Same log |
| Windows x64 MSVC preview build and artifact upload | **NOT YET RUN** | Workflow: `.github/workflows/windows-d3d11-bmd-preview.yml`; artifact name `d3d11-bmd-preview-windows-x64`; pending push/run |
| `python3 tools/check_docs_consistency.py` | **BLOCKED (exit 2)** | `/home/user/Cliente_ref/Data` is absent |
| D3D11 device/window, real BMD geometry on GPU | **NOT EXECUTED** | A hosted compile job cannot prove device creation, window display, or real-asset rendering |
| BMD UV/material semantics, associated texture decode/render | **BLOCKED / NOT IMPLEMENTED** | Existing preview is untextured; UV semantics and material/texture association are unverified |
| End-user Windows/GPU validation with an authorized real model and its texture | **NOT EXECUTED** | Requires the user's Windows/GPU and verified model/texture pair; the CI artifact contains no assets |

A successful CI build means only that MSVC produced a non-empty PE x64
executable. It must not be described as validation of a D3D11 device/window,
UVs, materials, texture binding, or rendering compatibility.

## Cycle 20 — callback-driven frame-loop foundation (2026-10-16)

The source was the working tree on `arena/1d9c1f53-clienterecriado`, based on
`9d3de3b469e1c2d57112a356d1b63c1aa22b2383`, with the cycle-20 frame-loop
changes. All build outputs used `/tmp`; repository `build/` remained absent.

| Check | Result | Evidence / limitation |
|---|---|---|
| `make` with repository warning flags | **PASS** — exit 0, no warnings | `evidence/ciclo20_main_loop_validation_20261016.log` |
| `make test` | **PASS** — 560 asset checks + 82 frame-loop checks, 0 failures | Same log; generated fixtures and callback tests only |
| `make test-san` | **PASS** — 560 asset checks + 82 frame-loop checks, 0 failures; no ASan/UBSan report | Same log |
| Strict-warning standalone frame-loop test | **PASS** — 82 checks, 0 failures | Same log; binary in `/tmp` |
| `python3 tools/check_docs_consistency.py` | **BLOCKED (exit 2)** | `/home/user/Cliente_ref/Data` is absent; reference data unavailable |
| Windows executable, window/input, GPU render, real BMD preview | **NOT EXECUTED** | This task did not modify the preview or build a Windows client |
| FPS, end-to-end frame time, GPU time, or optimization gain | **NOT MEASURED** | MainLoop records callback durations only; no real application callbacks are connected |
| Client/Muserver integration | **NOT EXECUTED** | No client networking implementation or authorized test server |

These tests verify orchestration callbacks, truthful hook timing, bounds on
history, invalid frame-rate handling, and prediction gating. They do not measure
full-frame performance or establish a functional client.

## Cycle 19 — Muserver handoff documentation (2026-10-16)

This was a documentation-only change. No C++ source or server implementation
changed; the cycle-18 Linux build and synthetic test results below remain the
latest code validation. The reference clone was absent, so a reference HEAD
could not be recorded at the start or end of this cycle.

| Check | Result | Evidence / limitation |
|---|---|---|
| C++ build, unit tests, sanitizers | **NOT RUN in this docs-only cycle** | No code changed; latest results remain in the cycle-18 section below |
| `python3 tools/check_docs_consistency.py` | **BLOCKED (exit 2)** | `/home/user/Cliente_ref/Data` is absent; see `evidence/ciclo19_muserver_handoff_20261016.log` |
| Client/Muserver protocol or loopback test | **NOT EXECUTED** | No `src/mu/net` implementation or authorized test server is available |
| Original-client network inspection | **NOT EXECUTED / OUT OF SCOPE** | The handoff intentionally does not infer protocol details from protected code or traffic |

## Cycle 18 — latest fixed-branch validation (2026-10-16)

The source tested was commit `ce8d3bd6da6c8bf2c226ad8b037f55773f848d7c` on
`arena/1d9c1f53-clienterecriado`. Build outputs were redirected to `/tmp` so
this run did not write to the repository's `build/` directory.

| Check | Result | Evidence / limitation |
|---|---|---|
| Clean C++17 build with repository warning flags | **PASS** — exit 0, no compiler warnings | `evidence/ciclo18_linux_validation_20261016.log` |
| `make test` | **PASS** — 560 checks, 0 failures | Same evidence log; generated/unit fixtures only |
| `make test-san` | **PASS** — 560 checks, 0 failures; no ASan/UBSan report | Same evidence log |
| Standalone `OrbitCamera`, strict warnings | **PASS** — 57 checks, 0 failures | Same evidence log |
| Standalone `OrbitCamera`, ASan/UBSan | **PASS** — 57 checks, 0 failures; no sanitizer report | Same evidence log |
| `python3 tools/check_docs_consistency.py` | **BLOCKED (exit 2)** | `/home/user/Cliente_ref/Data` is absent; the reference clone and real-tree scan are unavailable. Output is in the evidence log |
| Windows D3D11 BMD preview, camera input, GPU render | **NOT EXECUTED in this cycle** | The owner previously reported a local preview, but no exact build/run log is available here. The committed D3D11 smoke test is triangle-only |
| UV/material semantics and verified model/texture association | **BLOCKED** | No positive/negative control or verified mapping is available |

These passes validate the current synthetic test suite and isolated camera math;
they do not establish full asset-tree compatibility, the meaning of the BMD
candidate UV/material fields, or a genuine Windows/GPU model render.

## Parallel orbital-camera component (2026-10-13)

This task added an asset-free camera state helper and deliberately did not run
or rebuild the parser suite or the D3D11 preview, to avoid touching the active
Windows validation artifacts. The standalone test was compiled into `/tmp`, not
`build/`.

| Command | Result | Evidence / limitation |
|---|---|---|
| Strict-warning standalone C++17 build and run of `orbit_camera.cpp` plus `test_orbit_camera_standalone.cpp` | **PASS** — 57 checks, 0 failures | `evidence/orbit_camera_standalone_20261013.log` |
| ASan/UBSan standalone camera build and run | **PASS** — 57 checks, 0 failures; no sanitizer report | `evidence/orbit_camera_standalone_san_20261013.log` |
| `make`, `make test`, `make test-san`, `make bench` | **NOT EXECUTED in this task** | Explicitly left project build and parser test binaries untouched while Windows validation is in progress |
| `python3 tools/check_docs_consistency.py` | **BLOCKED (exit 2)** | No `/home/user/Cliente_ref` clone or `Data/` tree is present; reference HEAD start/end could not be checked. Exact output: `evidence/orbit_camera_docs_consistency_blocked_20261013.log` |
| D3D11 BMD-preview build/run and camera input | **NOT EXECUTED in this task** | Windows-only; the committed D3D11 smoke log is for a triangle, not the BMD preview |
| BMD UV/material semantics and verified texture pair | **BLOCKED** | Raw candidate fields and a user-reported preview are not semantic proof; no verified association in repository evidence |

The source-level camera tests verify angle wrapping, pitch/distance clamps,
zoom direction, target translation and rejection of non-finite inputs. They do
not test DirectX matrix packing, Win32 mouse capture, the D3D11 preview, BMD
parsing, textures, or visual compatibility. Do not treat this isolated unit as
completion of the model-preview milestone.

## Historical BMD triangle-offset follow-up — status as of 2026-10-13

This is the pre-cycle-18 record and is superseded by the current validation
above. The owner first reported a pre-fix baseline of `560 checks, 8 failures`.
After pulling commit `bbea18e`, the owner ran `make -B test` and
`make test-san`. Both rebuilt successfully but stopped at
`TestBmdTriangles` (`tests/test_assets.cpp:1197`): the single-mesh fixture
expected one triangle but the parser returned zero. ASan/UBSan then reported
a null-vector-element dereference at line 1198 caused by the test continuing
after its failed size assertion.

Inspection of the generated fixtures shows that both single- and multi-mesh
fixtures currently model a four-byte adjustment at the UV/normal boundary.
The just-published follow-up restores that adjustment and guards triangle
indexing so failures remain diagnostic rather than becoming a segmentation
fault. The fixture's convention still requires validation against real assets.

| Check after the latest offset/test follow-up | Status | Required evidence |
|---|---|---|
| `make -B test` | **NOT YET RUN on latest commit** | Fresh build and full test output; target is zero failures |
| `make test-san` | **NOT YET RUN on latest commit** | ASan/UBSan output; target is zero failures |
| Documentation consistency checker | **NOT EXECUTED** | Exit 0 against an available read-only reference `Data/` tree |
| Windows D3D11 preview build/run | **NOT EXECUTED after this fix** | Build output, parser counts, visible model, clean exit and Git status |

The preceding 560-check/8-failure output must not be treated as a test of
commit `34e7207`. No original asset bytes are stored in Git. BMD UV/material
semantics remain unverified; BMD v0x0C and `Enc*` terrain cipher work remains
BLOCKED under `AGENTS.md`.

## 1. Build and automated tests

Environment: Linux x86_64, g++ 12.2.0, C++17, no third-party dependencies.
Build flags include the repository warning set (`-Wall -Wextra -Wpedantic
-Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wuseless-cast
-Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wdouble-promotion
-Wformat=2`).

| Command | Result | Evidence |
|---|---|---|
| `make clean && make` | **PASS (exit 0)** — full clean rebuild; no compiler warnings | `evidence/ciclo17_clean_build_final.log` |
| `make test` | **PASS (exit 0)** — **539 checks, 0 failures** | `evidence/ciclo17_make_test_final.log` |
| `make test-san` | **PASS (exit 0)** — **539 checks, 0 failures**; no ASan/UBSan report | `evidence/ciclo17_make_test_san_final.log` |
| `make bench` | **PASS (exit 0)** — builds CPU microbenchmark | `evidence/ciclo17_bench_build_final.log` |
| `build/bench_assets` | **PASS (exit 0) as one CPU microbenchmark run only**; not FPS/frametime/GPU measurement | `evidence/ciclo17_bench_final.log` |
| `python3 tools/check_docs_consistency.py /tmp/cliente_ref/Data` | **PASS (exit 0)** — 54 checks, 0 failures against reference HEAD `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` | `evidence/ciclo17_docs_consistency_final.log` |

### Historical test-count reconciliation

The published `arena/519f8fc9-clienterecriado` snapshot at
`4c36befab01263bf95a509d1c8306c56a1e876c5` has conflicting historical counts:
its `TEST_RESULTS.md` says 389 (Cycle 10, citing the older
`ciclo9_item3_security_test.log`), while its README and CURRENT_STATE say 500
and its Cycle-15 report also claims 500. Neither number describes the recovered
Cycle-17 source state. In the cycle17 audit, `make test` and `make test-san`
were rerun on the source tree whose baseline was
`f839757d7b0b56d0b0a8d542aae9c67df1628d37`; both exited 0 with **539 checks,
0 failures**. That is the cycle17 count, not the latest count. Cycle18 validation
at the top of this file reports **560 checks, 0 failures** on `ce8d3bd`.
Counts 389 and 500 are historical. Test counts are not statements of 100% code
coverage.

The test executable covers CRC/format helpers, OZT probes/read/conversion
(including 24-bit conversion), OZJ marker probes, BMD container/body/topology,
mesh import/index validation, bounds/culling, skinning validation, terrain
helpers, manifest checks, software framebuffer/camera/triangle/z-buffer, OZT
pixel loading, and use of explicitly supplied mesh UVs. Unit-test fixtures are
generated by the test code; no original asset bytes are committed.

For the cycle17 run, `make test-san` built a separate ASan/UBSan-instrumented
binary under `build-san/` and matched `make test` at 539 checks. Cycle18's
current sanitizer result is 560 checks, as recorded above. Neither count is a
statement of 100% code coverage.

## 2. Real-tree asset commands

The data source was the read-only reference tree at `/tmp/cliente_ref/Data`.
The full scan is a **probe/inventory**, not a full decoder for all formats.

| Command | Result | Evidence / interpretation |
|---|---|---|
| `build/mutool scan /tmp/cliente_ref/Data` | exit 0; 29,060 files, 2,372.04 MiB; 2,893.80 MiB header-derived image-byte estimate | `evidence/ciclo17_scan_final.log`; probes/header estimates only, not decoded allocation or VRAM |
| `build/mutool bmdreport /tmp/cliente_ref/Data` | exit 2 because OPAQUE payloads remain | `evidence/ciclo17_bmdreport_final.log`; 10,023 structurally valid, 8,571 tool-classified payloads, 8,406 geometry, 8,405 with triangles, 3,681,664 triangle records, 3,693 loose-bone files; classifier labels do not validate or authorize v0x0C cipher work |
| `build/mutool meshimport /tmp/cliente_ref/Data` | exit 0 | `evidence/ciclo17_meshimport_final.log`; 513,304 triangles dropped once each for invalid references, 3,168,360 emitted, zero verified BMD UV coordinates |
| `build/mutool info /tmp/cliente_ref/Data/Item/Axes06.ozt` | exit 0 | `evidence/ciclo17_ozt_real_asset_info.log`; 64×64, 32 bpp, declared 16,384-byte payload, 26-byte trailer |
| `build/mutool render /tmp/cliente_ref/Data/Player/Axe11.bmd /tmp/ciclo17_ozt_texture_test.ppm 800 600 --texture /tmp/cliente_ref/Data/Item/Axes06.ozt` | exit 0 | `evidence/ciclo17_ozt_real_asset_load.log`; real pixels loaded, but the BMD and OZT are separate assets with no verified association; planar mapping is diagnostic only |
| `build/mutool verify /tmp/cliente_ref/ListaPatch.txt /tmp/cliente_ref` | exit 2; **INCOMPLETE** | `evidence/ciclo17_reference_manifest_verification_final.log`; all 188 entries checked: 116 match, 8 size mismatch, 11 CRC mismatch, 53 missing, 0 unreadable |
| `build/mutool terrain /tmp/cliente_ref/Data` | exit 2 under current heuristic | `evidence/ciclo17_terrain_after_audit.log`; all 76 grids parse, 72/76 pass smoothness threshold; 4 heuristic outliers are not automatically invalid |
| `build/mutool bmdanim /tmp/cliente_ref/Data` | exit 0, **size-only** | `evidence/ciclo17_bmdanim_after_audit.log`; 500 files with trailing bytes, 385 divisible by `boneCount × 12`; animation values not interpreted |
| `build/mutool culltest /tmp/cliente_ref/Data` | exit 0, **synthetic scenario** | `evidence/ciclo17_culltest_after_audit.log`; counts bounds culled under artificial camera/placements, not FPS |

`mutool info` and `mutool render` prove only that one real OZT was read/converted
and pixels reached the CPU diagnostic path. They do not establish a correct
BMD/texture pair or visual equivalence.

Six PPM files in the interrupted workspace differed byte-for-byte from the
published `4c36bef` versions. The published files remain unchanged; the recovered
variants are preserved outside Git. Their size/SHA-256 comparison is recorded in
[`evidence/ciclo17_ppm_recovery_comparison.log`](evidence/ciclo17_ppm_recovery_comparison.log).
Neither set proves GPU rendering, UV semantics, or visual compatibility.

### BMD index metrics

The full parser report distinguishes reference-field counts from rejected
triangles:

- 523,717 vertex references and 507,068 normal references were out of range;
- 1,030,785 / 22,089,984 reference fields = 4.666%;
- mesh import emitted 3,168,360 triangles and dropped 513,304 / 3,681,664 =
  13.94% because one or more references were invalid.

A triangle is counted once as dropped even if several of its reference fields are
invalid. The importer rejects rather than clamps. Full details are in
[`COMPATIBILITY.md`](COMPATIBILITY.md).

**Cipher policy:** `AGENTS.md` continues to mark the BMD v0x0C cipher and the
`Enc*` terrain cipher **BLOCKED**. Counts labelled v0x0C-decryptable by the
existing tool are implementation/classifier output, not an authorization to
extend reverse engineering or a claim of officially validated compatibility.

## 3. CPU microbenchmark

`build/bench_assets` measures CPU-side operations and data preparation. The
latest single-run raw log is `evidence/ciclo17_bench_final.log`; it includes
medians/tail percentiles for the microbench operations and explicitly states
that it is **not a frame-rate measurement**. It is not evidence of FPS,
frametime, GPU/driver cost, VRAM, or whole-client performance. The hypothetical
indexed-versus-expanded data-volume section is not runtime timing. Do not compare
single-run figures to a different machine/build as a speedup claim.

## 4. Not executed / blocked

| Test | Status | Reason |
|---|---|---|
| Windows x86/x64 build and client startup | **NOT EXECUTED** | No Windows or cross-compiler in this environment |
| GPU API/device creation or rendering | **NOT IMPLEMENTED / NOT EXECUTED** | No GPU and no production backend |
| Original-client visual comparison | **NOT EXECUTED** | No equivalent authorized runtime comparison; CPU PPM is insufficient |
| FPS, frametime distribution, VRAM/RAM at runtime | **NOT EXECUTED** | No Windows/GPU client run |
| Input latency and long-session stability | **NOT EXECUTED** | Required target hardware/test harness unavailable |
| Authorized server/network compatibility | **NOT EXECUTED** | No authorized test server |
| Full 188-entry manifest match | **INCOMPLETE** | The available reference tree has 53 missing and 19 mismatching entries |
| Full OZT/OZJ pixel decode over the tree | **NOT EXECUTED** | Scan is not a full pixel decoder; OZJ pixel decode is not implemented |
| BMD UV/material association and correctly textured original model | **BLOCKED** | Semantics not verified |
| BMD v0x0C cipher / `Enc*` terrain cipher | **BLOCKED** | Standing `AGENTS.md` constraint |

## 5. Reference immutability

The read-only sparse reference clone is at HEAD
`88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`; `git status --short --branch`
reported `## main...origin/main` with no worktree changes. It contains `Data/`
and the root `ListaPatch.txt` used by the audit. No reference file, branch, or
history was modified. The exact post-validation HEAD/status/diff checks are in
[`evidence/ciclo17_reference_status_final.log`](evidence/ciclo17_reference_status_final.log).
The manifest command compared sizes/CRCs; protected and licensing paths were
not otherwise inspected. See [`REFERENCE_AUDIT.md`](REFERENCE_AUDIT.md).


## Renderer API spike (2026-10-12) — prepared, not executed

| Check | Result | Evidence / limitation |
|---|---|---|
| API decision reconciled with product target | **DOCUMENTED** | DEC-014; target is modern Windows + dedicated GPU |
| D3D12 smoke-test source and PowerShell build helper added | **PREPARED, NOT COMPILED** | platform/windows/d3d12_triangle.cpp, scripts/build_d3d12_smoke.ps1 |
| MSVC/Windows SDK compilation | **NOT EXECUTED** | Authoring environment is Linux and has no Windows SDK |
| Hardware adapter/device creation | **NOT EXECUTED** | Requires target Windows PC and compatible driver |
| GPU triangle / swap-chain presentation | **NOT EXECUTED** | Source code is not runtime evidence |
| D3D11 comparison / Vulkan evaluation | **NOT EXECUTED** | Gate depends on first successful D3D12 run |
| FPS / frametime / VRAM / speedup | **NOT MEASURED** | A triangle smoke test is not a benchmark |

Do not mark this milestone passed until the Windows build and runtime logs are saved and reviewed.


## 6. Latest owner-run BMD regression result — 2026-10-13

After pulling through `751d9b6`, the owner ran:

```text
make -B test
560 checks, 6 failures
```

Compilation succeeded. `bmd triangles` now passes; all six remaining
assertions are in `topology is a strict subset of geometry`. Further source
inspection found that the single-mesh topology fixture placed the triangle
record four bytes after the parser's currently accepted cursor-adjusted start,
and the multi-mesh report case merely changed the global `meshCount` field on
a single-mesh body. Those are invalid fixtures for the layouts being tested.

Commit `49b2be3` corrects the single-mesh offset in the synthetic fixture and
uses `MakeMultiMeshBody` to construct the multi-mesh case. This commit has not
yet been run on the owner's PC.

| Check after `49b2be3` | Status |
|---|---|
| `make -B test` | **NOT YET RUN** |
| `make test-san` | **NOT YET RUN** |
| Documentation consistency checker | **NOT EXECUTED** |
| Windows BMD D3D11 preview build/run | **NOT EXECUTED after latest parser/test changes** |

Do not mark the parser correction accepted until the regular suite and
ASan/UBSan suite both pass. Passing synthetic fixtures does not replace
validation against authorized real BMD assets.
