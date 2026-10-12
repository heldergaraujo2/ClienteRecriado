# Asset formats

Every layout in this document was established by measuring bytes in the
reference client's asset tree, not by assuming what a MU Online format looks
like. Each section states the evidence, the implementation that reads it, and
the measured success rate over the **entire** tree, not a sample.

Reproduce the numbers with:

```sh
build/mutool scan /path/to/Cliente_ref/Data
```

Reference tree measured: 29,060 files, 2,372.04 MB.

| Format | Files | Parsed | Rate |
|---|---|---|---|
| OZJ (JPEG texture) | 13,928 | 13,832 | 99.31% |
| OZT (BGRA texture) | 3,470 | 3,404 | 98.10% |
| BMD (model container) | 10,090 | 10,066 | 99.76% |
| TGA | 15 | 15 | 100% |
| `.att` terrain attribute | 105 | 105 | 100% |
| `.map` terrain map | 79 | 79 | 100% |
| `.obj` terrain object list | 81 | 81 | 100% |

**Estimated uncompressed texture memory: 2,893.80 MB** — see
[`PERFORMANCE_BASELINE.md`](PERFORMANCE_BASELINE.md).

---

## OZJ — JPEG textures

13,928 files, 971.7 MB. The largest single format in the client.

### Layout

An OZJ is a **plain JPEG with its own leading 24 bytes duplicated in front of
it**.

```
offset 0    : 24 bytes, a byte-for-byte copy of the JPEG's first 24 bytes
offset 24   : the complete JPEG, starting with its own FF D8 FF
```

### Evidence

Scanned every OZJ in the tree for start-of-image markers in the first 64 bytes:

```
files with >= 2 SOI markers in first 64 bytes: 200/200 sampled
SOI positions: always (0, 24)
prefix[0:24] == bytes[24:48]: True for 200/200
```

The 24 bytes are `FF D8` (SOI) + a 16-byte JFIF APP0 segment + the two-byte
APP1 marker and its two-byte length.

### Why this matters

This was found by a full scan, not by sampling. An earlier inventory sampled 120
OZJ files, saw that 119 began with `FF D8 FF`, and concluded "OZJ is a plain
JPEG". Running the real parser over the whole tree produced:

```
.ozj  13928 files, 1 ok, 13927 fail
      note: expected a marker prefix 0xFF (13750)
```

A parser that walks the marker chain from offset 0 reads the APP1 length out of
the duplicated region, lands mid-segment, and reports a healthy file as corrupt.
**99.99% failure rate from a conclusion that a 120-file sample supported.** After
locating the real image start: 13,832 of 13,928 parse.

The lesson is recorded in `AGENTS.md`: verify format conclusions against the
whole tree.

### Implementation

`mu/assets/ozj.cpp`. `OzjFindImageStart()` walks the markers from offset 0; if
that fails it searches the first 4096 bytes for a second SOI and re-walks from
there. It never assumes the prefix is 24 bytes.

This module does **not** decode JPEG pixels. It extracts geometry, precision,
component count and whether the file terminates correctly — everything the
inventory and VRAM-budget tooling needs. Pixel decoding needs a real JPEG
decoder, which is a dependency decision recorded in
[`DEPENDENCIES.md`](DEPENDENCIES.md).

### JPEG is big-endian

One real bug worth recording. JPEG stores every length field big-endian, unlike
every other format in this client. The first implementation read segment lengths
with the little-endian helper, so a DQT length of `00 43` (67) was read as
17152 and the parser reported "segment runs past the end of the file" on a
perfectly valid file. `mu::ReadU16BE` exists specifically so this cannot recur.

### 96 files that do not parse

These begin `FF 4B 6D` rather than `FF D8 FF`. A separate container variant. Not
resolved; recorded in [`TEST_RESULTS.md`](TEST_RESULTS.md).

---

## OZT — 32-bit BGRA textures

3,470 files, 442.0 MB. Every one measured is 32 bits per pixel.

### Layout

```
offset 0    : 4-byte signature  00 00 02 00
offset 4    : 18-byte standard TGA image header
                [4]  colour map length
                [5]  colour map type
                [6]  image type            2 = uncompressed truecolour
                [7]  colour map first index (u16)
                [9]  colour map length      (u16)
                [11] colour map entry size
                [12] x origin (u16)
                [14] y origin (u16)
                [16] width  (u16, little endian)
                [18] height (u16, little endian)
                [20] bits per pixel   32 in 400/400 sampled
                [21] image descriptor  8 = 8 alpha bits
offset 22   : width * height * 4 bytes, packed BGRA, bottom row first
trailer     : variable length, see below
```

So a complete file is `22 + w*h*4 + trailer`.

### Evidence

Over 400 sampled files:

```
signature[0:4]: 00000200 in 395, 004b6d4b in 5
image type:     2 in 400/400
bits per pixel: 32 in 400/400
trailer length: 26 bytes in 362 (90.5%)
                 0 bytes in  22 (5.5%)
                30 bytes in  12 (3.0%)
                34 bytes in   1
```

Worked example, `Data/Item/capesnowlucasnuut02.ozt`, 262,192 bytes:
`22 + 256*256*4 + 26 = 22 + 262144 + 26 = 262192`. Exact.

### Trailer handling — a deliberate decision

The first implementation rejected any trailer that was not 0 or 26 bytes. That
fails around 3% of the asset base for no benefit: the pixel payload is fully
determined by the header, so trailing metadata can be ignored safely. The loader
now accepts any trailer up to 4096 bytes, reports its length in
`OztImage::trailer_size`, and rejects only implausibly large ones.

Compatibility with the existing asset base is a stated project priority, and
this is a case where strictness would have cost real files.

### The `00 4B 6D 4B` variant — recognised, not decoded

43 files (for example `Data/Custom/Interface/Rank01.OZT`, `Next.OZT`,
`Main_IE.ozt`) carry an extra 4-byte marker ahead of the signature. Three
measurements on them:

1. Image type at the usual TGA offset reads **0**, not 2.
2. Dimensions sit 4 bytes later than in the standard form: `Rank01.OZT` is
   78x78 with the dimensions at file offset 20 rather than 16.
3. File length is 24,358 bytes. Header-plus-payload is 26 + 78*78*4 = 26,362 —
   **exactly 4 bytes more than the file**.

Those three facts cannot all be satisfied by a single consistent layout. It is
possible to pick a pixel offset that makes the arithmetic close, but it would
overlap the bits-per-pixel field, and the resulting pixels would be silently
wrong. The loader therefore recognises the variant and returns `kUnsupported`
with a message naming the marker, rather than guessing.

`AGENTS.md` records the general rule: a format conclusion that only holds
arithmetically is not a format conclusion.

### Implementation

`mu/assets/ozt.cpp`. `OztProbe` validates without copying (22 bytes touched —
this is what makes a 29,000-file inventory finish in 1.7 s). `OztRead` copies
the payload. `OztToRgbaTopDown` converts BGRA bottom-up to RGBA top-down, which
is what `glTexImage2D` with `GL_RGBA`/`GL_UNSIGNED_BYTE` wants. `OztWriteRgba`
writes valid OZT files and is what the test suite uses to generate fixtures.

---

## BMD — model container

10,090 files, 585.5 MB.

### Container layout

```
offset 0 : 'B' 'M' 'D'      ASCII magic
offset 3 : version byte
```

Two layouts, distinguished by the version byte:

**Version 0x0C** — 8,741 files

```
offset 4 : uint32 little endian, payload length
offset 8 : payload
```

File size equals `8 + payload length`. Holds for the great majority; the loader
checks it and reports `length_consistent = false` when it does not, so a
truncated download degrades instead of overflowing a buffer.

**Version 0x0A** — 1,194 files

Legacy layout with **no length field**. Reading offset 4 as a length yields
implausible values (1,635,017,028 on a 21,141-byte file), so the module exposes
everything from offset 4 as payload and does not interpret it.

### Three separate metrics — and why one number was wrong

An earlier report said "BMD: 10,066 of 10,090 read (99.76%)". That figure counted
only whether the **container header** could be classified. It said nothing about
whether a vertex could be extracted, and the difference between those two
answers is 99.76% versus 0.00%. The cycle-1 audit caught this correctly.

`mutool bmdreport <dir>` now reports all three levels, so the number cannot be
misread again. Measured over the whole tree (full output in
[`evidence/ciclo1_bmdreport.log`](evidence/ciclo1_bmdreport.log)):

| Level | Measures | Files | % |
|---|---|---|---|
| **1. Structure valid** | magic + version recognised, and for v0x0C the declared payload length matches the real file length | **10,023** | **99.34%** |
| **2. Payload in the clear** | bytes obtainable without an unknown cipher | **1,496** | **14.83%** |
| ↳ plaintext | already clear | 1,396 | 13.84% |
| ↳ bux-decryptable | XOR-3 `{0xFC,0xCF,0xAB}` | 100 | 0.99% |
| ↳ **OPAQUE** | **cipher not recovered** | **8,594** | **85.17%** |
| **3. Geometry extracted** | vertex positions and normals actually parsed | **1,191** | **11.80%** |

Level 3 was zero for most of cycle 1 because no body parser existed. It is now
**measured rather than structural**: a v0x0A body parser was written against the
verified layout and extracts vertex positions and normals from 1,191 of the
1,194 v0x0A files.

Two qualifications that must travel with the number:

- It counts **vertex positions and normals only**. Face indices, texture
  coordinates and bones are not decoded, so a file counted here is **not a
  renderable model**.
- v0x0C is excluded entirely, because its payload is encrypted and parsing
  ciphertext would produce noise rather than vertices. That is why the ceiling
  for level 3 is 11.83%, not 100%.

The column is still reported explicitly rather than omitted, because omitting it
is what made the earlier single percentage misleading.

Container kinds, for completeness:

```
BMD_V0C            8,741  (86.63%)
BMD_V0A            1,194  (11.83%)
BMD_OTHER_VERSION     31  ( 0.31%)
BUX_CONTAINER        100  ( 0.99%)
unrecognised          24  ( 0.24%)

payload entropy: max 7.9980 bits/byte, 8,570 above 7.5
```

**Consequence.** Usable BMD geometry is `BLOCKED` for 85.17% of files, and
`NOT IMPLEMENTED` for the remaining 14.83%. This is the project's number-one
blocker; see [`COMPATIBILITY.md`](COMPATIBILITY.md).

One useful consequence of the split: the 1,396 plaintext-payload files are
enough to **write and test a BMD body parser today**, without waiting for the
cipher. That is a concrete unblocked path.

### What has been established about the body, and what has not

Task 1a was started. Full raw output in
[`evidence/ciclo1_bmd_body_analysis.log`](evidence/ciclo1_bmd_body_analysis.log),
over the list of clear files emitted by
`mutool bmdreport <dir> --list-clear` (1,465 entries) so that classification
comes from the tool rather than from a re-derived heuristic.

**Established, measured:**

- The clear set is 1,365 `PLAINTEXT` plus 100 `BUX_DECRYPTABLE`. The 100 bux
  files do **not** begin with `BMD` once decrypted, confirming they are bux
  containers and not models.
- Among the clear files: 1,194 are v0x0A and 171 are v0x0C.
- **433 of them open with a 32-byte ASCII field holding the source model's
  name**, and all 433 end in `.SMD`. 351 of those contain a path separator, so
  the field holds an authoring path such as `Data2\Item\Bow\...`, not just a
  bare filename. Examples: `Sword40.bmd` opens with `Sword40.SMD`,
  `PrickleGlove_left.bmd` with `PrickleGlove_left.SMD`.

That last point is a real structural finding and it is worth stating precisely:
**433 of 1,465 clear files, not all of them.** The remaining 932 do not begin
with printable ASCII, so the name field is not universal to the format and any
parser must not assume it.

**Not established — layout NOT determined:**

- A search for an (offset, width, record size) triple that divides the payload
  remainder exactly found **no** candidate across all 433 named files.
- A vertex-stride test — longest run of consecutive records whose first three
  floats are plausible coordinates — is **ambiguous**. For `PrickleGlove_left`
  a 12-byte stride wins (1,407 records); for `Sword40` a 48-byte stride wins
  (654). No stride wins on both. Recording either number here would be a guess,
  and a guessed vertex layout produces silently wrong geometry, which is worse
  than no geometry.

**Partially established — a normals array, with evidence, but not a layout:**

Scanning whole files for runs of float32 triples whose Euclidean norm is 1
(tolerance 1e-3) at a 20-byte stride finds, in **16 of 41 named v0x0A files
(39.0%)**, a region where the property holds essentially always — 10 files at
**250/250 = 100.0%**. Mesh normals are unit-length by construction, so this is a
decisive and falsifiable test rather than a plausibility argument.

The test's false-positive rate was measured first, because without it "250 out
of 250" means nothing: over 2,000,000 uniform samples in the accepted range
`[-1.5, 1.5]`, the rate is **4.480e-04**, making 250 consecutive hits a
probability of effectively zero.

What this does **not** establish, and these gaps are the reason no parser
follows:

- The absolute offset ranges from 4 to 358,808 bytes between files and the
  alignment phase varies over 0, 4, 8, 12, 16. That is consistent with a
  variable-length sectioned format but does not say where the normals section
  begins in an arbitrary file. Reaching it requires the header counters, which
  are not decoded.
- **25 of 41 files (61.0%) show no such region above 90%.** Whether that means a
  different layout, quantised normals, or an absent section is unresolved.
- The stride is 20 bytes (5 floats) but only the first three are covered by the
  test. The remaining two floats are **not identified**. Coordinate pair (u, v)
  is the natural hypothesis; it is unverified and must not be assumed.

Classification of that scan alone: **strongly evidenced hypothesis for 39.0% of
named files, not a determined layout.** Raw output in
[`evidence/ciclo1_bmd_normals.log`](evidence/ciclo1_bmd_normals.log).

**Established — a verified header field and record size:**

The scan was then used as an *independent* measurement to test a header field,
which is what turns a hypothesis into a finding. The normals count was **not**
read from the header; it was derived geometrically from the longest run of
unit-norm triples. The two were then compared:

- **The u16 little-endian at body offset 40 is the count of normal records.**
  It equals the geometrically derived count in **145 of 154** testable files
  (**94.2%**).
- **The normals region is exactly `count * 20` bytes** — 145 of 145 of the
  agreeing files, zero exceptions.
- **The record is 20 bytes (5 floats): the first three are the unit normal
  (x, y, z).** The trailing two floats remain **unidentified**.

Because the count comes from a geometric property rather than from the field
being interpreted, the agreement is evidence and not circularity. Raw output in
[`evidence/ciclo1_bmd_normals_field.log`](evidence/ciclo1_bmd_normals_field.log).

So the v0x0A body layout is now known up to this point:

```
offset  size   field
0       32     source model name, ASCII, NUL-padded   (433 of 1,465 files)
32       8     header words, partly undecoded
40       2     u16 normal-record count                (verified, 94.2%)
42      ...    undecoded: spans up to 357,308 bytes before the normals region
?       n*20   normal records, 5 floats each, first 3 unit-length
```

**Still not established, and these are what block a parser:**

- 9 of 154 files disagree (e.g. `Monster290`: derived 599 against u16@40 of
  93). Whether the scan caught a different section or those files use another
  layout is unresolved.
- 46 of 200 files show no detectable run at all; cause not determined.
- The bytes between offset 42 and the start of the normals region are
  undecoded, and that gap ranges from 852 to 357,308 bytes. Reaching the
  normals in an arbitrary file therefore still requires fields this project has
  not read.
- The trailing 2 floats of each record are unidentified. The natural guess is a
  texture coordinate pair, and it was **tested and not supported**: 95.5% of
  those floats fall in `[0, 1]`, but so do **87.2%** of the floats in an
  adjacent equal-sized region where no texture coordinates have any reason to
  be — a ratio of only 1.10x. Arbitrary floats in these files simply land in
  `[0, 1]` often, so the test cannot discriminate. Without that control the
  95.5% would have been reported as confirmation.

### The v0x0A body layout, partially decoded and verified

Two sections are now verified with exact arithmetic across **145 of 145**
testable files, zero exceptions. Full output in
[`evidence/ciclo1_bmd_layout.log`](evidence/ciclo1_bmd_layout.log).

```
offset    size    field                                status
0         32      source model name, ASCII, NUL-padded verified (433/1465)
32         2      u16, undecoded
34         2      u16, undecoded
36         2      u16, undecoded
38         2      u16 VERTEX COUNT                     VERIFIED
40         2      u16 NORMAL COUNT                     VERIFIED
42        ...     undecoded
52        vc*16   vertex array, 16 B each              VERIFIED
                  [f32 x][f32 y][f32 z][u16][u16]
52+vc*16  nc*20   normal array, 20 B each              VERIFIED
                  [f32 nx][f32 ny][f32 nz][f32][f32]
...               remainder NOT decoded
```

The two predictions that were tested, both falsifiable:

- `52 + u16@38 * 16 == start of the normals region` — **145 of 145 (100.0%)**
- `normals region size == u16@40 * 20` — **145 of 145 (100.0%)**

Both hold because the normals offset and count were derived geometrically first
(unit-norm float triples) and only then compared against the header, so the
agreement is evidence rather than circularity.

Worked example, `Sword40.bmd`: name `Sword40.SMD`, `u16@38 = 88`, `u16@40 = 129`,
vertices at 52..1460 (88*16), normals at 1460..4040 (129*20).

**Still not established:**

- The 2 trailing u16 of each vertex record are unidentified. In `Sword40` the
  first is always 0 and the second varies (670, 681, 0, 512).
- The 2 trailing f32 of each normal record are 0.0000 in `Sword40`. The (u,v)
  reading was tested and **rejected** (see above).
- The section after the normals is **not decoded**. A `nc*8` UV array was
  tested: 92.65% of its floats fall in `[0,1]`, but that is barely above the
  ~87% baseline already measured, and only 28 of 145 files have the whole
  region inside `[0,1]`. Record sizes of 8/12/16/20 bytes were then tested
  against a following face-index array with indices `< max(nc, vc)`: only 12 to
  18 of 99 files passed, so **no size holds**.
- 9 of 154 files disagree on the normal count; 46 of 200 show no detectable
  region.

**Classification: two sections verified (vertex positions and normals) with
exact offsets, counts and record sizes. A parser that extracts vertex positions
and normals is now possible for the 145 confirmed files. Face indices, texture
coordinates and bones are not.** Layout as a whole: **PARTIAL**.

The one paired `.smd`/`.bmd` in the tree, `Object40` (3,847,871 B of plain-text
SMD beside a 1,041,808 B **opaque** BMD), does not currently close the
arithmetic either: 15,390 triangles and 2,234 unique positions leave a remainder
of 58–65 bytes per triangle for every plausible record size. It remains the
best known-plaintext candidate for the cipher itself, and that pairing was
checked explicitly — it is 1 of 10 `.smd` files, not 10 of 10.

### The payload cipher is not recovered — BLOCKED

8,570 of the 10,090 files have a payload with entropy above 7.5 bits/byte.
Sampled in detail at 7.99. What was tested and ruled out:

| Hypothesis | Test | Result |
|---|---|---|
| Repeating XOR, any period 1–32 | Key derived per position class assuming zeros dominate | Best score 0.416 against a random baseline of 0.375 — no signal |
| Repeating XOR, any period up to 600 | Shift-coincidence scan | Peak 0.0056 against a baseline of 0.0039, no periodic spike |
| CFB chaining `p[i] = c[i] ^ c[i-1]` | Decrypt and score | 0.399, entropy 7.83 |
| Rolling subtraction | Decrypt and score | 0.348, entropy 7.81 |
| Index-derived XOR keys (`i`, `7i`, `i^0x5A`, `i%32`) | Decrypt and score | 0.369–0.374, baseline |
| zlib stream | Check for a `78 xx` prefix | No |

The payload is either a stream cipher or a compressed block. Recovering it
requires the loader, which is inside the VMProtect-protected `Main.exe`. Per
`AGENTS.md`, that is out of scope. Status: **BLOCKED**.

What is *not* blocked: the container is parsed, classified, validated and
inventoryed, and the payload is available as opaque bytes.

### Bux-encrypted `.bmd` files that are not models

100 files with a `.bmd` extension are bux-encrypted but contain no `BMD` magic
once decrypted. `Data/gate.bmd` is one: it opens with four repetitions of the
bux key, meaning twelve plaintext zero bytes, and decrypts to a data blob with
no model header anywhere in it.

Detection therefore cannot key on the magic. `mu::BuxDecryptGain()` measures how
much decrypting raises the zero-byte fraction; a real bux file shows a large
positive gain and unrelated data shows none. The threshold is 0.05, and both
directions are tested in `tests/test_assets.cpp`.

---

## The bux cipher — solved

A repeating 3-byte XOR with the key `{ 0xFC, 0xCF, 0xAB }`.

### Evidence

The reference client ships both an encrypted and a plaintext terrain attribute
file, which makes the key directly recoverable.

`Data/World1/Terrain1.att` (encrypted) opens:

```
fc 30 54 f8 cb af f8 cb af f8 cb af f8 cb af f8 cb af f8 cb af f8 cb af f8 cb
```

XOR with `fc cf ab` repeating:

```
00 ff ff 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04
```

`Data/World1/Lorencia.att` (plaintext) opens:

```
00 ff ff 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04
```

Identical. Two independent observations agree: the first three bytes decrypt to
the plaintext file's own header, and the repeating `f8 cb af` decrypts to the
plaintext file's repeating `04`.

A useful property follows: plaintext zero runs encrypt to runs of the key
itself, which is what makes `BuxDecryptGain` work as a detector.

### Scope — an important correction

An early conclusion was that the bux key covered all encrypted terrain files. It
does not. Decrypting 60 `EncTerrain*` files with the bux key and measuring the
density of bytes below `0x40` improved exactly **1 of 60**. The `Enc*` family
uses a different scheme. Measured: `EncTerrain1.att` has entropy 7.9926 and
small-byte density 0.255, which is what uniform random data looks like.

`TerrainTryBuxDecrypt` therefore requires a decisive improvement (gain > 0.10)
before it accepts a decryption, so the `Enc*` family is left untouched rather
than silently corrupted.

Status of `Enc*` terrain: **BLOCKED**, not recovered.

---

## Terrain — `.att`, `.map`, `.obj`

265 files under `Data/World*`. All 265 parse.

### Size families measured

```
.map  196610 bytes   77 files   = 2 + 256*256*3
.att  131076 bytes   68 files   = 4 + 256*256*2
.att   65540 bytes   27 files   = 4 + 256*256
.att   65539 bytes    9 files   = 3 + 256*256
.map  196609 bytes    2 files   = 1 + 256*256*3
```

The constant term is always `256*256`: **every map is a 256x256 tile lattice**,
with 1 or 2 bytes per tile for attributes and 3 bytes per tile for maps. The
small prefix (1–4 bytes) differs per family and is treated as an opaque header
rather than guessed at.

Encryption split over the whole tree:

```
.att  97 encrypted,   8 plaintext
.map  77 encrypted,   2 plaintext
.obj  79 encrypted,   2 plaintext
```

Plaintext detection uses the density of bytes below `0x40`, which measures
0.70–1.00 on real plaintext terrain and about 0.25 on ciphertext. The threshold
is 0.55, with margin on both sides.

`Data/World1/Lorencia.att` measures density 1.000 (plaintext);
`Data/World1/Terrain1.att` measures 0.000 (fully bux-encrypted);
`Data/World1/EncTerrain1.att` measures 0.255 (unknown cipher).

### `.obj` is not a Wavefront object file

Despite the extension, these are the terrain object lists — variable-length
records, no fixed grid. They are classified separately and never forced into the
256x256 model.

---

## TGA

15 files, 7.85 MB. All 15 parse.

Standard 18-byte header. `mu/assets/tga.cpp` reads uncompressed 24/32-bit
images and **detects but does not decode** RLE (image type 10). None of the 15
files in the reference client use RLE, and shipping an untested RLE decoder
would be worse than reporting the gap. The status code makes the gap explicit.

---

## Formats present but not yet implemented

| Format | Files | Status |
|---|---|---|
| `.ozb` | 100 | Binary height/light maps. Header not resolved — sampled files open with long zero runs, so the structure needs more work. NOT STARTED |
| `.smd` | 10 | Valve StudioModelData, **plain text** (`version 1`, `nodes`, `skeleton`, `triangles`). Trivial to parse; not needed yet |
| `.cg` | 6 | Cg shaders, plain text. Read and analysed in [`RENDERING.md`](RENDERING.md) |
| `.lua` | 22 | Plain text Lua |
| `.ozp` | 1 | Unknown |
| `.cws` | 1 | 92 bytes, unknown |
| `.bmp` | 2 | Standard bitmap |
| `.dat` | 2 | 108 bytes total. `Dec2.dat` is 54 bytes of what appear to be encrypted little-endian floats |

---

## Junk in the asset tree

Not a format, but found by the same scan and worth recording:

```
Thumbs.db        12 files    677,376 bytes
Data/**/*.rar     1 file     1.2 MB
Data/**/*.zip     1 file    13.1 MB
Data/**/*.att1    1 file   131,076 bytes   (typo for .att)
```

`Thumbs.db` is a Windows Explorer thumbnail cache. It leaks the directory
listing of whoever packaged the client and serves no purpose in a distribution.
`mutool scan` counts these separately so they can be excluded from a build.

---

# Anexo do ciclo 2 — a cifra v0x0C e o layout do corpo, ambos resolvidos

Este anexo **substitui** a seção "BLOQUEIO nº 1" acima e a seção que descrevia o
layout do corpo como verificado só nas duas primeiras seções.

## Cifra do payload v0x0C — RESOLVIDA

```
plain[i] = (cipher[i] ^ key[i & 15]) - running
running  = cipher[i-1] + 0x3D        running inicial = 0x5E

key = D1 73 52 F6 D2 9A CB 27 3E AF 59 31 37 B3 E7 A2
```

XOR rolante sobre uma tabela de 16 bytes combinado com uma chave corrente
derivada do byte de **texto cifrado** anterior. Por isso **não é auto-inversa**
como o XOR-3 do bux. Determinística, passada única, O(n).

**Proveniência: projeto de terceiros.** Algoritmo e tabela vêm de
`xulek/muonline-bmd-viewer` (licença ISC), reimplementados aqui. Ver
`docs/LICENSING.md` e `docs/DECISIONS.md` (DEC-007).

**Validação deste projeto:** a decifragem só é aceita se o campo de nome de 32
bytes for ASCII imprimível. Sem isso uma chave errada reclassificaria lixo como
decifrado. Resultado: **7.226 de 8.741** v0x0C decifrados; 1.519 rejeitados.

## Layout do corpo — PARCIAL, com uma fonte externa que NÃO se confirmou

### Cabeçalho do corpo (deslocamentos de *payload*, não de arquivo)

O payload começa depois do cabeçalho de 4 bytes `"BMD"` + versão. **Todo
deslocamento abaixo é relativo ao início do payload.** Confundir as duas bases
desloca cada leitura em 4 bytes — foi exatamente esse erro que produziu uma
revisão inteira de resultados falsos neste ciclo, registrada mais adiante.

```
32  u16 meshCount           mediana 1, max 18, zero em 3 arquivos
34  u16 boneCount
36  u16 actionCount
38  u16 numVertices         mediana 68,  max 5.082
40  u16 numNormals          mediana 69,  max 5.376
42  u16 numTexCoords
44  u16 numTriangles
46  u16 textureIndex
48   2  u16, provável contagem — 0 em 57,4%, depois 1, 2, 4, 3 (2.500 arquivos)
50   2  u16, parcialmente padding — 0 em 60,0%; 0xCDCD em 62 arquivos
52  vc*16  array de vértices
```

As contagens em 38 e 40 foram verificadas por este projeto em 145/145 arquivos
testáveis. Os nomes dos campos em 32..46 vêm do projeto ISC e são **coerentes**
com as distribuições medidas, mas não foram verificados um a um.

### Registros — medidos, e DIFERENTES da fonte externa

| Registro | Tamanho | Conteúdo |
|---|---|---|
| Vértice | 16 B | `f32 x` + `f32 y` + `f32 z` + `i16 node` + `i16 bindVertex` |
| Normal | 20 B | `f32 nx` + `f32 ny` + `f32 nz` + `f32 aux0` + `f32 aux1` |
| Triângulo | 64 B | 3 referências de 8 B em +0/+8/+16: `u16 vértice` + `u16 normal` + `u16 opcional` + `u8 flag` + `u8 pad` |
| Osso | não verificado neste projeto | — |

**As coordenadas vêm primeiro nos dois registros.** Evidência, medida sobre os
1.194 arquivos v0x0A:

- Normais lidas no deslocamento interno **0**: 260.030 de 260.268 com norma 1 =
  **99,91%**. No deslocamento 4: 84.611 = **32,51%**.
- No registro de vértice, o `u16` no offset **12** é menor que 200 em **99,94%**
  dos registros e o do offset **14** em **95,53%**. Nos offsets 0 e 2, as cifras
  são 2,63% e 0,82%. Os inteiros pequenos ficam no fim.

O projeto `xulek/muonline-bmd-viewer` inicia cada registro com índice de osso e
padding, colocando as coordenadas no deslocamento 4. **Isso não se aplica a
estes arquivos.** A cifra daquele projeto foi confirmada de forma independente;
as tabelas de layout, não.

### Duas correções de afirmações deste projeto

1. **Os dois `f32` finais da normal NÃO estão confirmados como (u,v).** Uma
   versão anterior deste documento afirmou que eram, citando a fonte externa.
   Como o layout de registro daquela fonte não corresponde aos dados, essa
   citação perdeu a base. O que se sabe: 95,5% dos valores estão em [0,1] contra
   87,2% num controle uniforme — razão 1,10×, insuficiente para concluir.
   Seguem como `aux0`/`aux1` sem nome, deliberadamente.

2. **A varredura de UV que deu razão 0,146 a 0,824 continua inconclusiva.** A
   hipótese de corpo organizado por malha permanece plausível, mas não foi
   demonstrada; foi inferida da mesma fonte cujo layout de registro falhou.

### Topologia — localização CONFIRMADA, conteúdo interno NÃO DETERMINADO

Avanço do ciclo 2 sobre o item que estava bloqueado. O array de triângulos foi
localizado com controle estatístico, o que este projeto não havia conseguido nas
tentativas anteriores.

**Localização, para arquivos de malha única:**

```
tstart = 52 + numVertices*16 + numNormals*20 + numTexCoords*8
stride = 64 bytes por triângulo
índices de vértice em +2, +6 e +10 dentro de cada registro
```

**Evidência** (reproduzível por `python3 tools/bmd_topology_probe.py <Data>`,
saída em [`evidence/ciclo2_bmd_topology.log`](evidence/ciclo2_bmd_topology.log)):

| Medida | Resultado |
|---|---|
| Hipótese — índices em +2/+6/+10 | 134.524/137.205 = **98,05%** |
| Controle limpo — região `f32` de vértices | 5.961/254.025 = **2,35%** |
| Acaso — `vc/65536` médio | **0,71%** |
| Razão hipótese/acaso | **138,3×** |
| Razão hipótese/controle | **41,78×** |
| Nome de textura no ponto previsto | 515/586 = **87,9%** |

A última linha é uma confirmação **independente**: se o stride de 64 estiver
errado, o nome da textura não cai logo depois do último triângulo. Ela não
depende de interpretar índice nenhum.

**Por que as tentativas anteriores falharam.** Elas buscavam o array de faces
ancorando em `uv_end` derivado da hipótese `nc*8` sem confirmá-la, e testavam
stride 8/10/12/14/16 bytes. O stride é 64, muito maior que qualquer um testado.

### Registro de triângulo — DECODIFICADO em sua parte essencial

Avanço posterior à localização. A varredura dos 64 bytes por deslocamento, com o
mesmo controle limpo, mostrou que os campos `u16` reais estão em deslocamentos
**pares** — os ímpares (~67%) são leituras sobrepostas dos mesmos valores. Isso
revelou a estrutura: **três referências de 8 bytes** no início do registro.

```
+0   u16 índice de vértice      < numVertices  em 97,66% dos casos
+2   u16 índice de normal       < numNormals   em 97,20%
+4   u16 campo opcional         mediana 0, máximo 65535 — NÃO é índice de UV
+6   u8  flag
+7   u8  padding
     (repete em +8 e +16: são os três vértices do triângulo)
+60  u16 não decodificado       valor 3 em 53,3% dos registros
+62  u16 não decodificado       mediana 24
```

**Isto é topologia utilizável.** Um triângulo é lido como três pares
(vértice, normal), que é o suficiente para montar a malha.

**O que continua não decodificado, sem eufemismo:**

- O campo em +4 — **resolvido como padding, não como índice**. A distribuição
  medida em 40.902 amostras é 64,95% zero e **18,18% o valor 0xCDCD**, que é o
  padrão de preenchimento de memória não inicializada do MSVC. Excluindo essas
  sentinelas, o campo é menor que `numTexCoords` em apenas **5,33%** dos casos.
  Não é índice de coisa nenhuma: é padding que o exportador não zerou.

  A leitura anterior ("campo opcional com sentinela −1, mediana 0, máximo
  65535") era uma inferência que os dados não sustentam. O máximo 65535 vinha de
  leituras desalinhadas, não de uma sentinela real.
- **O índice de UV por vértice do triângulo continua não localizado.** O campo
  em +4 era o candidato natural e foi descartado. O *array* de UV, porém, está
  confirmado — ver abaixo.
- Os bytes 24..59. Todos zero em apenas 40,9% dos registros, então há conteúdo
  ali que não foi identificado.
- Os `u16` em +60 e +62.
- O primeiro byte **não** é tamanho do polígono: valores 3 e 4 somam 3,01%,
  indistinguível de aleatório. As tabelas do projeto ISC não se aplicam.

**Restrição de confiança, e uma discrepância que precisa constar.** Os 97,66% e
97,20% foram medidos sobre 58.836 referências em 319 arquivos, **limitando a 80
triângulos por arquivo**. Ao rodar o parser C++ sobre a população inteira
(1.221.030 triângulos em 4.149 arquivos), as referências fora de faixa somam
**8,357%**, não ~2,4%.

A amostra truncada subestimava o problema: os primeiros 80 triângulos de um
arquivo são mais bem comportados que o conjunto. **A cifra correta é 8,357%.**
Causa não determinada. Renderizar sem validar limites leria fora dos arrays em
cerca de uma em cada doze referências, o que é inaceitável — por isso o parser
conta e reporta esses casos em vez de aceitá-los silenciosamente.

**Restrição do levantamento.** Tudo acima vale para **arquivos de malha única**
(586 dos 1.500 examinados). Para malhas múltiplas o corpo se repete por malha e
a aritmética de um bloco só não se aplica; isso não foi medido.

### Array de coordenadas de textura — CONFIRMADO

```
uvstart = 52 + numVertices*16 + numNormals*20
ntc entradas de 8 bytes, cada uma [f32 u][f32 v]
```

A posição já era conhecida indiretamente — é ela que faz o array de triângulos
cair no lugar certo, confirmado pelo nome de textura em 87,9% dos arquivos. O
que faltava era verificar o **conteúdo**, e isso agora está feito.

**Evidência** (250.450 leituras em 319 arquivos de malha única):

| Medida | Resultado |
|---|---|
| Leituras `f32` não-finitas | **0** de 250.450 |
| Esperado por acaso (expoente todo-1 = NaN/Inf, ~0,39%) | **~976** |
| Componentes em [0,1] | 245.773/250.450 = **98,13%** |
| Distribuição | mín −0,4266 · p1 0,0083 · mediana 0,5058 · p99 1,5518 · máx 1,9980 |

**Zero não-finitos em 250.450 leituras é o argumento forte.** Bytes arbitrários
interpretados como `f32` produzem NaN ou Inf em ~0,39% dos casos; em 250.450
leituras isso seriam ~976 ocorrências. Observar zero não é compatível com dados
que não sejam float.

A distribuição fecha o caso como coordenadas de textura e não como outro tipo de
float: concentrada em [0,1] com cauda até ~2,0, que é o padrão de *tiling* de
textura em modelos reais.

**Um controle que falhou e precisa constar.** Deslocar a leitura 4 bytes deu
97,91% em [0,1] contra 98,13% da hipótese — razão 1,00×, sem poder
discriminatório. O controle estava contaminado: deslocar dentro de um array de
pares `f32` ainda lê floats válidos do par vizinho. É a mesma armadilha do
controle dentro do array de triângulos. **Controle válido precisa sair da região,
não se deslocar dentro dela.**

**O que continua bloqueado.** O array existe e está legível, mas **o índice que
liga cada vértice de um triângulo a uma entrada desse array não foi localizado**.
Sem esse índice não há como texturizar a malha: sabe-se quais coordenadas
existem, não qual pertence a cada canto.

Três hipóteses para essa ligação foram testadas e **todas descartadas**:

1. **Campo em +4 do registro de triângulo.** É padding: 64,95% zero e 18,18%
   0xCDCD (preenchimento de memória não inicializada do MSVC). Excluindo
   sentinelas, menor que `numTexCoords` em só 5,33%.
2. **Um quarto canto em +24.** Não existe: offsets +24 a +56 dão 50–81% de
   validade de índice, o nível de leitura contaminada, contra 94–100% em
   +0/+8/+16.
3. **Relação implícita por posição.** Descartada: `numTexCoords` é igual a
   `numVertices` em apenas 5,74% dos arquivos de malha única e a `numNormals` em
   3,87% — medido em 802 arquivos. Em 93% dos casos difere de ambos, então o
   índice de UV não pode ser o de vértice nem o de normal.

**Quarta hipótese testada e descartada: os bytes 24..59.** Varridos os 39
deslocamentos de 24 a 62 medindo a fração de `u16` abaixo de `numTexCoords`, com
controle limpo (mesma leitura sobre o array `f32` de UV, 5,50%; acaso 0,15%).

O resultado **não é um pico, é um platô**: todos os deslocamentos de 24 a 62
ficam entre 40% e 94%, com razão de 7,3× a 17,1× sobre o controle, variando de
forma gradual. Não há nenhum deslocamento que se destaque.

Isso é assinatura de **teste sem poder discriminativo**, não de um campo real. A
comparação é direta: quando o índice de vértice foi procurado no mesmo registro,
os deslocamentos +0/+8/+16 deram 94–100% e os vizinhos caíram — um pico nítido.
Aqui não há pico, então não há campo.

A explicação mais provável para o platô é que esses bytes contenham **dados em
ponto flutuante**, não índices: interpretar `f32` como `u16` produz valores
pequenos com frequência, porque o expoente ocupa os bits altos. Isso é coerente
com o platô gradual e incompatível com um array de índices.

**Estado do índice de UV: NÃO RESOLVIDO.** Quatro caminhos fechados:

| Hipótese | Resultado |
|---|---|
| Campo em +4 do registro | padding (64,95% zero, 18,18% 0xCDCD) |
| Quarto canto em +24 | não existe (nível de ruído) |
| Relação implícita por posição | refutada (93% dos arquivos diferem) |
| Bytes 24..59 | platô sem pico; provavelmente `f32`, não índices |

**A explicação foi confirmada: os bytes 24..59 são ponto flutuante.** Lidos como
`f32`, dão **0,055%** de não-finitos (68 em 122.706 leituras) contra **0,39%**
esperado por acaso — seriam ~478. Bytes arbitrários não produzem esse resultado.
O controle (bytes 0..23, sabidamente índices inteiros) dá 0,027%, também abaixo
do acaso, porque inteiro pequeno interpretado como `f32` é finito.

Isso fecha a questão: **não há índice de UV nos bytes 24..59 porque ali não há
índices.** O que são esses floats — vetor de luz, coordenada de lightmap,
atributo por face — não foi determinado.

**O segundo caminho também foi examinado.** Os 4 bytes do cabeçalho em payload
48..51, medidos em 2.500 arquivos:

| Campo | Distribuição |
|---|---|
| `u16@48` | zero em 57,4%; depois 1 (177×), 2 (96×), 4 (82×), 3 (63×); máximo 57.856 |
| `u16@50` | zero em 60,0%; 0xCDCD em 62 arquivos; máximo 52.685 |

`u16@48` é dominado por inteiros pequenos — assinatura de **contagem**, não de
índice. `u16@50` contém 0xCDCD, o preenchimento de memória não inicializada do
MSVC, então é ao menos parcialmente padding. **Nenhum dos dois é índice de UV.**

### Índice de UV por canto — NÃO RESOLVIDO, investigação encerrada

**Sétima e última hipótese testada: array paralelo de `ntri` entradas fora do
registro.** Varrido o corpo a partir do fim do array de triângulos, em passos de
2 bytes, procurando um bloco de `ntri` valores `u16` todos menores que
`numTexCoords`. Resultado em 161 arquivos: encontrado em **19 = 11,8%**, com
distâncias dispersas (84, 66, 40, 636, 3252, 2650, 42, 10) e **nenhum offset
consistente**.

Um array paralelo real apareceria na mesma distância em quase todos os arquivos.
A dispersão mostra que os 19 acertos são coincidência — blocos de valores
pequenos que existem por acaso, não uma estrutura.

**Investigação encerrada por critério, não por falta de tempo.** Sete hipóteses
descartadas em sequência:

| # | Hipótese | Resultado |
|---|---|---|
| 1 | Campo em +4 do registro | padding — 64,95% zero, 18,18% 0xCDCD |
| 2 | Quarto canto em +24 | não existe — nível de ruído |
| 3 | Relação implícita por posição | refutada — 93% dos arquivos diferem |
| 4 | Bytes 24..59 do registro | platô sem pico; confirmado serem `f32` |
| 5 | `u16@48` do cabeçalho | contagem, não índice |
| 6 | `u16@50` do cabeçalho | parcialmente padding (0xCDCD) |
| 7 | Array paralelo no corpo | 11,8% com distâncias dispersas — coincidência |

Sete falhas seguidas são evidência de que **a pergunta pode estar mal formulada**,
não de que falta tentar mais uma vez. As possibilidades que este projeto não
conseguiu formular incluem: o índice de UV ser derivado aritmeticamente dos
outros índices; não existir UV por canto neste formato, com texturização feita
por outro mecanismo; ou `numTexCoords` não significar o que se assumiu.

**Consequência prática, declarada:** os 8.406 arquivos com vértice e normal e os
4.149 com topologia **não podem ser texturizados**. Malha sem textura pode servir
para colisão, caixa delimitadora e análise, não para renderização fiel.

### Ossos — localização PARCIAL, 73,7% dos arquivos

Primeiro avanço sobre ossos, que estavam inteiramente não examinados. Após o array
de triângulos vem o nome da textura (32 B) e depois os nomes de osso, também em
blocos de 32 bytes:

```
fim_dos_triângulos + 0    32 B   nome da textura
fim_dos_triângulos + 32   32 B   nome do osso 0
fim_dos_triângulos + 64   32 B   nome do osso 1
...                              (boneCount entradas)
```

`boneCount` é o `u16` no offset de payload **34**, que este projeto havia
registrado como "não decodificado".

**Evidência** (468 arquivos de malha única): **345 = 73,7%** têm `boneCount`
nomes ASCII válidos a partir de +32. Os nomes recuperados são nomes reais de nó
do 3ds Max, o que é forte indício de que a interpretação está certa:

```
Box01 (70×) · Bip01 (42×) · Object03 (26×) · Sphere01 (20×) · Footsteps (20×)
```

**O que impede chamar isso de resolvido.** Em **26,3%** dos arquivos a leitura não
bate: ou há menos nomes válidos que `boneCount`, ou os blocos contêm bytes não
imprimíveis. Causa não determinada — a restrição a malha única já estava aplicada,
então não é isso.

### Registro de osso — intercalado, 59 bytes, confirmado em 98,0%

**Correção de uma suposição deste projeto.** A leitura inicial assumiu que todos
os nomes de osso vinham primeiro, em blocos de 32 bytes, e as transformações
depois. Um dump mostrou o contrário: logo após o primeiro nome aparece outro nome
(`t04`), não dados numéricos. **Os registros são intercalados.**

```
fim_dos_triângulos + 32          início dos ossos
cada osso: 32 B nome ASCII NUL-pad + 27 B de transformação
total: boneCount × 59 − 1 bytes
```

**Evidência:** 919 arquivos têm tamanho exatamente `boneCount × 59 − 1`, e
**901 = 98,0%** deles apresentam nomes ASCII válidos na posição intercalada
prevista. É uma confirmação dupla — o tamanho fecha e o conteúdo confere.

O `− 1` do modelo permanece sem explicação.

**Estado: nomes e tamanho do registro confirmados; o conteúdo dos 27 bytes de
transformação NÃO foi decodificado.** Uma amostra mostra `u16 = 65535` seguido de
floats quase todos zero, o que é compatível com "sem osso pai" e transformação
identidade, mas uma amostra não é evidência. Skinning continua **BLOQUEADO**.

**O que não foi determinado:**

- O conteúdo dos 27 bytes de transformação, campo a campo.
- Por que o modelo tem `− 1` byte.
- A seção de animação, que vem depois dos ossos nos arquivos com sobra maior que
  a prevista.

  | Sobra | Arquivos |
  |---|---|
  | 26 bytes | 627 |
  | 53 bytes | 60 |
  | 368 bytes | 23 |
  | 80 bytes | 9 |

  **A conta fecha com o modelo `27 × boneCount − 1`**, equivalente a 26 bytes
  para o primeiro osso e 27 para cada osso seguinte. Verificado agrupando a sobra
  por `boneCount` em 1.525 arquivos:

  | `boneCount` | Sobra modal | Arquivos | Previsto `27·bc−1` | Bate? |
  |---|---|---|---|---|
  | 1 | 26 | 1.222 de 1.328 | 26 | **sim** |
  | 2 | 53 | 82 de 93 | 53 | **sim** |
  | 3 | 368 | 23 de 50 | 80 | não |
  | 4 | 107 | 3 de 13 | 107 | **sim** |
  | 5 | 134 | 5 de 7 | 134 | **sim** |
  | 6 | 1.745 | 8 de 13 | 161 | não |
  | 8 | 215 | 3 de 7 | 215 | **sim** |
  | 9 | 242 | 2 de 2 | 242 | **sim** |
  | 11 | 296 | 2 de 4 | 296 | **sim** |

  **1.319 de 1.525 = 86,5%** dos arquivos seguem o modelo na moda de cada
  contagem.

  Os que divergem têm sobra muito maior que o previsto (bc=3 → 368 em vez de 80;
  bc=6 → 1.745 em vez de 161). O padrão é coerente com **presença de dados de
  animação** nesses arquivos, que vêm depois das transformações. Isso é hipótese,
  não resultado.

  **27 bytes por osso não se decompõe em floats de forma inteira**: 6×4 = 24,
  sobram 3. A decomposição mais plausível é `u16` índice do osso pai + 6 floats
  (24 B) + 1 byte não identificado. **Hipótese, não resultado** — o byte extra e
  o `−1` do modelo não foram explicados.
- A hierarquia pai/filho.
- A seção de animação, que vem depois dos ossos.
- Por que 26,3% falham.

**Estado: PARCIAL.** Nomes de osso localizados na maioria dos arquivos;
transformações, hierarquia e animação não determinados. Skinning continua
**BLOQUEADO** — exige transformações, não nomes.

### Verificação independente da decifragem v0x0C

A decifragem foi confirmada por uma via que não depende do campo de nome: sobre
400 arquivos v0x0C decifrados, o corpo parseia completamente sob o layout
v0x0A e as normais têm norma 1 em **99,98%** dos casos (37.988 de 37.997),
contra 31,46% no deslocamento 4. Uma chave errada não produziria vetores
unitários. Isso também descarta a hipótese de que v0x0C usaria half-floats nos
arrays — usa `f32` como v0x0A.

**Atenção à base do cabeçalho, que difere por versão:**

| Versão | Cabeçalho | Início do payload |
|---|---|---|
| v0x0A | `"BMD"` + versão = 4 B | offset 4 |
| v0x0C | `"BMD"` + versão + `u32 payloadSize` = 8 B | offset 8 |

Usar 4 para v0x0C desalinha tanto o corpo quanto o fluxo da cifra, e o resultado
parece uma falha do layout quando é uma falha da medição. Foi o que aconteceu
numa primeira tentativa, que reportou "54 de 8.741 com corpo completo" e
"normais unitárias 0,02%" — ambos artefatos da base errada.

### Registro de um erro de método deste ciclo

Uma revisão deste ciclo trocou a ordem dos campos nos registros com base numa
medição que reportava "offset 4 vence em 1.050 arquivos, offset 0 em nenhum". A
medição estava errada: os scripts liam deslocamentos de **arquivo** tratando-os
como de **payload**, defasando tudo em 4 bytes. A reversão está no commit
`bf1668c`.

Regra que fica: **toda medição sobre o payload precisa declarar a base.** Um
resultado "decisivo" obtido com a base errada é mais perigoso que um resultado
inconclusivo, porque encerra a investigação.

## Estado de implementação

| Etapa | Status |
|---|---|
| Cifra v0x0C em C++ (`mu::BmdV0CDecrypt`) | **CONCLUÍDO** |
| Vértices + normais (v0x0A) | **CONCLUÍDO** |
| Vértices + normais (v0x0C) | **CONCLUÍDO** — verificado em 400/400 amostrados: corpo completo sob o mesmo layout, normais unitárias 99,98% no deslocamento 0. Arrays são `f32`, não `f16` |
| Triângulos, UV, nomes de textura | **NÃO DETERMINADO** — a cifra está quebrada, mas o layout destas seções não foi estabelecido contra os dados |
| Ossos e animações | **NÃO INICIADO** |

---

# Limites reais do formato BMD — conhecidos vs desconhecidos

Levantamento do ciclo 2. A regra aqui é separar o que foi **medido nos arquivos**
do que só poderia ser conhecido decodificando seções ainda bloqueadas. Omitir a
segunda coluna seria esconder o tamanho do que falta.

## Limites CONHECIDOS — medidos nos 10.090 arquivos

| Limite | Valor | Origem da medição |
|---|---|---|
| Precisão de coordenadas | **`f32`** (não `f16`) | Confirmado no ciclo 2: 400/400 arquivos v0x0C com corpo completo sob layout `f32` e normais de norma 1 em 99,98% |
| Contagem de vértices por malha | campo `u16` → **teto de 65.535** | `u16` no payload offset 38; máximo observado 5.082 |
| Contagem de normais por malha | campo `u16` → **teto de 65.535** | payload offset 40; máximo observado 5.376 |
| Contagem de UV por malha | campo `u16` → **teto de 65.535** | payload offset 42 |
| Contagem de triângulos por malha | campo `u16` → **teto de 65.535** | payload offset 44 |
| Registro de vértice | **16 bytes** | Verificado em 260 mil+ registros |
| Registro de normal | **20 bytes** | Verificado em 260.268 registros (99,91% de norma 1) |
| Registro de triângulo | **64 bytes** | Confirmado pela posição do nome de textura em 87,9% dos arquivos de malha única |
| Contagem de malhas | campo `u16`; mediana 1, **máximo observado 18** | payload offset 32; nomes dos campos não verificados um a um |
| Nome do modelo de origem | **32 bytes**, ASCII NUL-pad | Campo inicial do corpo |
| Nome de textura | campo de texto após o array de triângulos | Observado em 87,9% dos arquivos de malha única |

**Consequência prática do teto `u16`.** Uma malha não pode exceder 65.535
vértices nem 65.535 triângulos. Para conteúdo pesado isso é restritivo e é um
argumento concreto a favor do caminho B (assets customizados em formato moderno)
para modelos novos — ver `docs/ARCHITECTURE.md`.

## Limites DESCONHECIDOS — bloqueados por parsing não concluído

| Limite | Por que não se sabe |
|---|---|
| **Número de materiais/texturas por modelo** | Exige percorrer a tabela de nomes de textura, que só foi localizada em arquivos de malha única. Não foi contada nem validada |
| **Máximo de ossos por modelo** | Nenhum registro de osso foi localizado nem validado. O campo `boneCount` existe no cabeçalho, mas seu significado não foi confirmado um a um |
| **Máximo de frames de animação** | Depende de ossos e da seção de ações, ambas bloqueadas |
| **Bytes 24..59 do registro de triângulo** | Não zero em 59,1% dos registros. Sem quarto canto (offsets +24..+56 dão 50–81%, nível de ruído). Conteúdo não identificado |
| **UV por vértice do triângulo** | O candidato em +4 foi descartado. UV segue sem localização |
| **Bytes 24..59 do registro de triângulo** | Todos zero em só 40,9% dos registros; conteúdo não identificado |
| **Limite de malhas por modelo** | Máximo observado 18, mas sem confirmação de que o campo é contagem de malhas, 18 é observação, não limite do formato |
| **Semântica dos `u16` em 32/34/36** | Coerentes com contagem de malha/osso/ação, não verificados |

**Registro explícito, como exigido:** a ausência de índices de face decodificados
impede levantar limites que dependem de topologia. Isso é limitação do
levantamento, não propriedade conhecida do formato.

## Restrição que se aplica a tudo acima

As medições de estrutura interna valem para **arquivos de malha única** (586 dos
1.500 examinados no levantamento de topologia). Para malhas múltiplas o corpo se
repete por malha e a aritmética de bloco único não se aplica — não foi medido.
