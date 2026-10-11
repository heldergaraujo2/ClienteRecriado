# Test results

Every result is classified `PASS`, `FAIL`, `BLOCKED`, `NOT EXECUTED` or
`NOT APPLICABLE`, with evidence or a reason. A blank is never left where a
classification belongs.

Last run: `make test` — **171 checks, 0 failures**.

## 1. Unit tests

Command: `make test` → `build/test_assets`.
Environment: g++ 12.2.0, `-std=c++17 -O2`, Linux x86_64.
Fixtures are generated at run time; no original asset bytes are involved.

| Test | Checks | Result | Notes |
|---|---|---|---|
| crc32 known vectors | 46 | `PASS` | Includes values computed independently with Python `zlib.crc32`, and a one-shot-vs-incremental sweep over lengths 0–40 |
| ozt round trip | 20 | `PASS` | Write-then-read, with and without the 26-byte footer, non-power-of-two 37x21 |
| ozt variant with extra prefix | 4 | `PASS` | Asserts the `00 4B 6D 4B` variant is recognised and named, not misparsed |
| ozt rejects bad input | 14 | `PASS` | Truncation, bad signature, unsupported image type, absurd trailer |
| ozj probe | 8 | `PASS` | Geometry, precision, components, EOI, truncation, non-JPEG |
| ozj duplicated header prefix | 5 | `PASS` | The real OZJ container shape; asserts the image is located at offset 24 |
| bux cipher is self inverse | 8 | `PASS` | Includes the property that zero plaintext encrypts to the key itself |
| bmd probe | 17 | `PASS` | v0x0C, v0x0A, bux container, bux non-model, high-entropy rejection, non-model rejection |
| entropy separates plaintext from ciphertext | 2 | `PASS` | Flat data < 0.01 bits, pseudorandom > 7.9 bits |
| terrain probe | 12 | `PASS` | All three grid families, plaintext vs encrypted verdicts, auto-decrypt |
| manifest parse | 15 | `PASS` | CRLF, blank lines, spaces in paths, malformed lines, bad hex |
| manifest verify | 6 | `PASS` | Match, size mismatch, missing, unreadable, exit classification |
| extension helpers | 6 | `PASS` | Case folding, dot-in-directory, no extension |

Total: **171 checks, 0 failures.**

The build is warning-clean under `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion -Wold-style-cast -Wuseless-cast -Wnon-virtual-dtor
-Woverloaded-virtual -Wcast-align -Wdouble-promotion -Wformat=2`.

Also run under AddressSanitizer and UndefinedBehaviorSanitizer
(`-fsanitize=address,undefined`): no reports.

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
