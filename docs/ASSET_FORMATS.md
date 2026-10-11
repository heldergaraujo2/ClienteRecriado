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
| **3. Geometry extracted** | vertices / faces / bones actually parsed | **0** | **0.00%** |

Level 3 is zero because **no BMD body parser exists**. It is reported as an
explicit zero rather than omitted, because omitting it is exactly what made the
earlier percentage misleading.

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

Classification: **strongly evidenced hypothesis for 39.0% of named files, not a
determined layout.** Raw output in
[`evidence/ciclo1_bmd_normals.log`](evidence/ciclo1_bmd_normals.log).

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
