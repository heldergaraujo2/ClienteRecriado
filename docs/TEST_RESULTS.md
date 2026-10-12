# Test results

Every result is classified `PASS`, `FAIL`, `BLOCKED`, `NOT EXECUTED` or
`NOT APPLICABLE`, with evidence or a reason. A blank is never left where a
classification belongs.

Last run: `make test` — **389 checks, 0 failures** (ciclo10, após detecção ciclo e vetorização bux).

- `make test-san` — 389 checks, 0 failures, ASan 0 reports, UBSan 0 reports (evidência `ciclo9_item3_security_test.log`).
- `check_docs_consistency.py` — 56 verificações, 0 falhas (tool vs docs, breakdown 1685=1483+67+134+1, taxas multi-mesh 14,27%/0,0376%/379× etc, nivel4 ≤ nivel3, zero includes gráficos).
- `mutool bmdreport /home/user/Cliente_ref/Data` — total 10090, structure 10023 99,34%, cleartext 8571 84,95% (plaintext 1245+bux100+v0c7226), OPAQUE 1519 15,05%, geom 8406 83,31%, topo 8405 83,30% 3681664 tris, bones loose 3693 36,60% 7879 bones ratio per file 1846×, strict 1186 11,75% taxa falso-positivo 0/25218, limite superior ~0,004% (1/25218), ratio >3558×, meshCount 0:1 1:4149 2:2280 etc, 0 multi-mesh fail.
- `bench_assets` — BuxCrypt 1 MiB med 290.9us 3437.8 MB/s (antes 880 MB/s) 3.9x speedup evidência `ciclo10_opt002_bux_benchmark.log`, OztProbe 0.0us, OztRead 6.0us, etc, full log `ciclo10_opt002_bux_benchmark.log`.
- Tools Fase A-B: `validate_A1_fixed` total geom 8406 real loose 3693 43,93% strict 1186 14,11% control trials 25218 loose 2 rate 0,0079% per file 2 rate 0,0238% ratio per file 1846×, strict taxa falso-positivo 0/25218, limite superior ~0,004% (1/25218), ratio >3558×; `validate_A3_subset` strict ⊂ loose PASS 6570 janelas 0 strict_not_loose, files 1186⊂3693 0 falhas; `test_uv_offset2` platô 9-26× 4149 files; `scan_anim` 3693 with bones 764 extra 231 divisível bc*12 62 plausíveis rotação [-pi,pi]; `brute_opaque` 1503 opaque tried 20 found 1 IceGolem step 0x33.

## 1. Unit tests

Command: `make test` → `build/test_assets`.
Environment: g++ 12.2.0, `-std=c++17 -O2`, Linux x86_64.
Fixtures are generated at run time; no original asset bytes are involved.

| Test | Result | Notes |
|---|---|---|
| crc32 known vectors | `PASS` | Includes values computed independently with Python `zlib.crc32`, and a one-shot-vs-incremental sweep over lengths 0–40 |
| ozt round trip | `PASS` | Write-then-read, with and without the 26-byte footer, non-power-of-two 37x21 |
| ozt variant with extra prefix | `PASS` | Asserts the `00 4B 6D 4B` variant is recognised and named, not misparsed |
| ozt rejects bad input | `PASS` | Truncation, bad signature, unsupported image type, absurd trailer |
| ozj probe | `PASS` | Geometry, precision, components, EOI, truncation, non-JPEG |
| ozj duplicated header prefix | `PASS` | The real OZJ container shape; asserts the image is located at offset 24 |
| bux cipher is self inverse | `PASS` | Includes the property that zero plaintext encrypts to the key itself, plus byte-for-byte equality scalar vs vectorised for phases 0..2 lengths 0..40 (OPT-003) |
| bmd probe | `PASS` | v0x0C, v0x0A, bux container, bux non-model, high-entropy rejection, non-model rejection |
| entropy separates plaintext from ciphertext | `PASS` | Flat data < 0.01 bits, pseudorandom > 7.9 bits |
| terrain probe | `PASS` | All three grid families, plaintext vs encrypted verdicts, auto-decrypt |
| manifest parse | `PASS` | CRLF, blank lines, spaces in paths, malformed lines, bad hex |
| manifest verify | `PASS` | Match, size mismatch, missing, unreadable, exit classification |
| extension helpers | `PASS` | Case folding, dot-in-directory, no extension |
| bmd v0x0c cipher | `PASS` | Decrypts v0C payload, printable check, entropy |
| bmd triangles | `PASS` | Topology parsing, out-of-range refs counted |
| topology is a strict subset of geometry | `PASS` | Level4 ≤ Level3 verified over whole tree |
| mesh from bmd | `PASS` | Import BMD to internal mesh representation |
| compute bounds | `PASS` | Bounding box calculation |
| frustum cull | `PASS` | Frustum culling by bounding box |
| frustum from view projection | `PASS` | Frustum extraction from VP matrix |
| terrain height | `PASS` | Height parsing and smoothness |
| bmd body parses verified layout | `PASS` | Round-trips synthetic v0A body: 5 verts 7 normals, source name, aux fields |
| bmd body rejects bad input | `PASS` | Truncated header, counts exceeding payload, non-printable name (932 real files have no name), non-finite coordinate, null output |
| bmd report levels are separate | `PASS` | Structure-valid vs cleartext vs geometry separate counters |
| bmd geometry needs a real body | `PASS` | Level3 must be earned, v0C never parsed without decrypt |
| bmd percent helper | `PASS` | Division by zero, 0%, 100%, 1/3 |
| bmd report accumulates kinds | `PASS` | v0A / v0C / other / unrecognised bucketed |
| bmd multi-mesh large counts (regression 8406) | `PASS` | Regression 8406: ntri 11771-17694 not rejected by fixed limit 10k |
| bmd multi-mesh extreme meshCount 0xFFFF | `PASS` | OOM-safe, overflow check |
| bmd multi-mesh truncated payload | `PASS` | Truncated in middle of mesh list |
| bmd multi-mesh overflow check | `PASS` | meshCount*record overflow |
| bmd bones circular parent (cycle detection) | `PASS` | 0->1->0 cycle must fail gracefully, no crash no loop (ciclo9 item3) |
| bmd bones extreme boneCount | `PASS` | bc=0xFFFF OOM-safe |
| bmd bones truncated payload | `PASS` | Truncated in middle of bone list |
| bmd bones overflow check | `PASS` | bc*59 overflow, bc>64 reject |

Total: **389 checks, 0 failures.** Verified via `./build/test_assets` output `389 checks, 0 failures`.

The build is warning-clean under `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion -Wold-style-cast -Wuseless-cast -Wnon-virtual-dtor
-Woverloaded-virtual -Wcast-align -Wdouble-promotion -Wformat=2`.

Also run under AddressSanitizer and UndefinedBehaviorSanitizer:
**389 checks, 0 failures, 0 sanitizer reports, exit code 0.**

Reproduce with `make test-san`, which builds a separate instrumented binary in
`build-san/` (`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`). It
is a separate binary on purpose, so that `make test` can never silently run an
instrumented build.

**Correction recorded ciclo10:** previous revision had 231 checks, now 389 after adding multi-mesh (4 tests) and bones (4 tests) and other modules (mesh, bounds, cull, terrain height, etc). Every number now comes from binary output, not source reading.

## 2. Real-asset validation

Run against the full reference tree — 29,060 files, 2,372.04 MB. Not a sample.

### 2.1 Format parsing over the whole tree

| Format | Files | Parsed | Failed | Result |
|---|---|---|---|---|
| OZJ | 13,928 | 13,832 | 96 | `PASS` (99.31%) |
| OZT | 3,470 | 3,404 | 66 | `PASS` (98.10%) |
| BMD | 10,090 | 10,066 | 24 | `PASS` (99.76%) |
| TGA | 15 | 15 | 0 | `PASS` (100%) |
| `.att` | 105 | 105 | 0 | `PASS` (100%) |
| `.map` | 79 | 79 | 0 | `PASS` (100%) |
| `.obj` | 81 | 81 | 0 | `PASS` (100%) |

Failure breakdown, all accounted for:

```
OZJ  96  : "missing JPEG SOI" — files beginning FF 4B 6D, a distinct variant
OZT  66  : 43 "00 4B 6D 4B variant, layout not resolved"
           11 "image type != 2"
            6 "signature not present"
            6 "trailer larger than any observed variant"
BMD  24  : "neither a BMD magic nor a bux-encrypted container"
```

Every failure category is understood. None is an unexplained crash or a silent
misparse.

### 2.2 Integrity verification against the client's own manifest

```
mutool verify ListaPatch.txt .
  total 188   match 116   size mismatch 9   crc mismatch 11   missing 52
  exit code 2
```

Result: `PASS` for the tool — it correctly classified all 188 entries and
returned exit code 2 as designed. The **client** fails its own manifest: 72 of
188 entries do not match the tree. That is a finding about the reference client,
recorded in [`REFERENCE_AUDIT.md`](REFERENCE_AUDIT.md) section 6, not a tool
defect.

### 2.3 Cross-validation of the CRC implementation

`Data/Item/capesnowlucasnuut02.ozt` — the tool computes `f2db1635`, and
`ListaPatch.txt` records `Data/Item/capesnowlucasnuut02.ozt f2db1635 262192`.
File size on disk: 262,192 bytes. Result: `PASS`. Three independent values
agree.

## 3. Blocked items

### 3.1 BMD v0x0C payload decryption — `BLOCKED`

8,570 of 10,090 BMD files have payload entropy above 7.5 bits/byte. The cipher
is not recovered. Ruled out: repeating XOR at every period from 1 to 600, CFB
chaining, rolling subtraction, index-derived keys, zlib. See
[`ASSET_FORMATS.md`](ASSET_FORMATS.md) for the full test table.

Recovering it requires the loader, which is inside the VMProtect-protected
`Main.exe`. Per `AGENTS.md` that is out of scope.

**Impact:** models cannot be loaded, so there is no geometry, no skeletal
animation and no rendering of characters or items. This is the single largest
functional gap in the project.

**What would unblock it:** the algorithm, obtained legitimately — for example
from an open-source MU client implementation under a compatible licence, or from
the format's author. Not from unpacking the protected module.

### 3.2 `Enc*` terrain decryption — `BLOCKED`

219 of 264 terrain files use the `Enc*` naming and are **not** bux-encrypted.
Evidence: decrypting 60 of them with the bux key improved the low-byte density
of exactly 1. `EncTerrain1.att` measures entropy 7.9926 and small-byte density
0.255, which is what uniform random data looks like.

**Impact:** most terrain cannot be loaded. The 45 non-`Enc` terrain files can be
read, including 8 plaintext attribute files.

## 4. Not executed

Everything below requires Windows, a GPU, or an authorised server. None has
been run. The harness exists in `scripts/` but **has never been executed**, and
its output has never been seen.

| Item | Status | Blocker |
|---|---|---|
| Build on Windows (MSVC) | `NOT EXECUTED` | No Windows toolchain in this environment |
| Build on Windows (MinGW cross-compile) | `NOT EXECUTED` | No `mingw-w64`; `apt-get` unavailable without root |
| Client startup | `NOT EXECUTED` | No Windows |
| FPS and frametime, all scenarios | `NOT EXECUTED` | No Windows, no GPU |
| VRAM measurement | `NOT EXECUTED` | No GPU |
| Map load timing | `NOT EXECUTED` | No Windows |
| Long-session stability | `NOT EXECUTED` | No Windows |
| Network compatibility with a server | `NOT EXECUTED` | No authorised test server |
| Visual comparison against the original | `NOT EXECUTED` | Requires both clients running |
| Input latency | `NOT EXECUTED` | Requires dedicated measurement hardware |

These are recorded as `NOT EXECUTED` rather than omitted, because an absent line
reads as "not relevant" when the truth is "not yet possible".

## 5. Not applicable

| Item | Reason |
|---|---|
| Mobile or console testing | Windows x86 target only |
| 64-bit build testing | The reference client is entirely 32-bit; a 64-bit path is not part of the current scope |
| WebGL or Vulkan backends | Not part of the compatibility target |

## 6. Defects found by real data that sampling missed

Recorded because they are the strongest argument for whole-tree validation.

**The OZJ container prefix.** A 120-file sample showed 119 files beginning with
`FF D8 FF` and supported the conclusion "OZJ is a plain JPEG". Running the real
parser over the whole tree:

```
before:  .ozj  13928 files,  1 ok, 13927 fail
after:   .ozj  13928 files,  13832 ok,    96 fail
```

A 99.99% failure rate behind a conclusion that sampling actively supported. The
rule is now in `AGENTS.md`.

**The Makefile header dependency gap.** Adding a field to `OztImage` left
`test_assets.o` compiled against the old layout linked against a library built
with the new one. The result was a segfault with no connection to the edit.
Fixed with `-MMD -MP` and `-include` of the generated `.d` files, and verified
by touching `ozt.hpp` and confirming exactly the three dependent translation
units rebuild.
