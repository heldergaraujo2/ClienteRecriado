# Next task

Ordered by the directive that the BMD v0x0C cipher be investigated **before**
CAND-001, because it blocks ~85% of the models and is a prerequisite for world
and entity rendering. CAND-001 is a texture-path optimisation; it makes loading
of already-decodable textures faster, and no amount of it produces a single
renderable model. The ordering below follows that reasoning.

---

## Primary: resolve the BMD v0x0C payload, by any legitimate route

**Blocker #1 of the project.** Measured state, `mutool bmdreport` over 10,090
files ([`evidence/ciclo1_bmdreport.log`](evidence/ciclo1_bmdreport.log)):

```
structure valid        10,023  (99.34%)
payload in the clear    1,496  (14.83%)
payload OPAQUE          8,594  (85.17%)
geometry extracted          0  ( 0.00%)
```

Two independent sub-tasks. The second is **not blocked** and should start
immediately.

### 1a. Write and test a BMD body parser against the 1,396 clear-payload files

**This is the concrete unblocked path and it is the highest-value work
available.** The cipher blocks 85.17% of files, but 1,396 files have payloads
that are already in the clear and 100 more are bux-decryptable. That is enough
real data to derive, implement and test the body format — vertex layout, face
indices, texture references, bone hierarchy — without waiting for anything.

Value: when the cipher is eventually recovered, the remaining 8,594 files become
usable immediately instead of requiring a second project. It also converts level
3 of the report from a structural zero into a measured number for 14.83% of the
set, which is a real compatibility result rather than a placeholder.

Method:

1. Dump the clear payloads with `mutool` and find structural regularity across
   files of the same kind — `Custom/Item/*` share a layout, `Custom/Monster/*`
   share another. Entropy of these files is 5.18–7.02, i.e. real structure with
   some variation, not compressed.
2. Look for a leading count field followed by a fixed-size record array. Cross-
   validate: if the declared vertex count times a candidate record size equals
   the payload length across many files, the record size is confirmed by
   arithmetic rather than by guessing.
3. Correlate with `.smd` files (10 of them, plain text) if the same models exist
   in both forms — that would give a ground-truth reference for vertex and bone
   layout. **Check this first; it is the cheapest possible confirmation.**
4. Never accept a layout that only fits one file.

**Acceptance:** a parser that extracts vertex positions, face indices and bone
names from at least 1,000 of the 1,396 clear files, with a unit test built on
synthetic fixtures and a validation run over the real set whose per-file results
are recorded in `docs/evidence/`. Level 3 of `bmdreport` must report a non-zero
count that matches the parser's own tally.

**Rejection rule:** if the arithmetic only closes for a subset, report the
subset and mark the rest `BLOCKED`. Do not generalise a layout from a
coincidence.

### Progress recorded on 1a in cycle 1 — read this before continuing

Verified, with evidence in `docs/evidence/`:

- The v0x0A body opens with a **32-byte ASCII field holding the source model
  name**, in 433 of 1,465 clear files. The other 932 do not begin with printable
  ASCII, so the field is not universal and a parser must not assume it.
- **The u16 at body offset 40 is the count of normal records.** It matches a
  count derived independently from geometry in **145 of 154** testable files
  (94.2%), and the normals region measures **exactly `count * 20` bytes** in
  145 of 145.
- **The record is 20 bytes (5 floats); the first three are the unit normal.**

Rejected, with the measurement that rejected it — do not retry these:

- The trailing 2 floats are **not** established as texture coordinates. 95.5%
  fall in `[0,1]`, but so do 87.2% of floats in an adjacent control region of
  equal size — ratio 1.10x, so the test cannot discriminate. **Do not assume
  (u,v).**
- A uniform vertex stride across the whole file does not exist: a 12-byte stride
  wins for `PrickleGlove_left` and a 48-byte one for `Sword40`.

### What cycle 1 settled, and the corrected resume point

The gap before the normals is no longer the open question — it was decoded. The
verified layout is:

```
0        32      source model name, ASCII, NUL-padded   (433 of 1,465 files)
38        2      u16 VERTEX COUNT                       VERIFIED
40        2      u16 NORMAL COUNT                       VERIFIED
52       vc*16   vertex array, [f32 x][f32 y][f32 z][u16][u16]   VERIFIED
52+vc*16 nc*20   normal array, [f32 nx][f32 ny][f32 nz][f32][f32] VERIFIED
```

Both offsets hold on **145 of 145** testable files. `mu/assets/bmd_body` parses
them, and `bmdreport` level 3 went from a structural 0 to a measured **1,191
(11.80%)**.

**Where to resume: the section after the normals, which is not one array.**
Three attempts were made and all three are recorded as negative results in
[`evidence/ciclo1_bmd_layout.log`](evidence/ciclo1_bmd_layout.log) — read them
before retrying, because two of them look like they should work:

1. A `nc*8` UV array followed by a face-index array. Rejected: record sizes of
   8/10/12/14/16 bytes were tested and 8 wins for 4 files, 16 for 4, 12 for 2.
2. Counting consecutive float32 pairs in `[0,1]` from the end of the normals.
   Rejected: the `pairs/nc` ratio ranges 0.146–0.824 (median 0.566), so it is
   not an array of `nc` UV pairs, and the scan stops early in every file.
3. The methodological trap, worth naming: both searches started from an offset
   derived from the unconfirmed `nc*8` hypothesis. **Do not anchor a search on a
   hypothesis you have not verified** — the result is inconclusive even when it
   looks like it closes.

**The record-size line of attack is closed.** Four separate attempts to
determine the face section by searching for a record size have failed, and all
four are recorded with their numbers in
[`evidence/ciclo1_bmd_layout.log`](evidence/ciclo1_bmd_layout.log):

1. Uniform vertex stride across the file — ambiguous (12 B wins one file, 48 B
   another).
2. `nc*8` UV array followed by faces — ambiguous (8 B wins 4 files, 16 B wins 4,
   12 B wins 2).
3. UV region by consecutive float pairs in `[0,1]` — `pairs/nc` ranges
   0.146–0.824, so it is not an array of `nc` pairs.
4. Dual vertex+normal constraint `[v0 v1 v2 n0 n1 n2]` — inconsistent: 629
   records in `Monster263` against 5 in `Sword40`.

Attempting a fifth record size is not a reasonable next step. What is missing is
not a better guess but the meaning of the fields, and search does not produce
that. Two notes for whoever resumes:

- The weak tests stop discriminating exactly where their numbers look best. In
  `Monster263`, `nc = 1872`, so a random u16 has a high probability of being
  below `nc` and long runs of "triplets < nc" arise by chance in a large file.
- Do not anchor a search on an offset derived from an unconfirmed hypothesis.
  Attempts 2 and 4 both started from `nc*8`, which was never verified.

**What would actually unblock this**, in order:

1. An open-source implementation of the format under a compatible licence. This
   is the normal route and it is **blocked by the development environment having
   no outbound access to a code host** — not by the difficulty of the format.
2. Documentation of the format from its author or community.
3. A reference `.smd`/`.bmd` pair where the BMD is in the clear. Only 1 of the
   10 `.smd` files in the tree has a same-name `.bmd`, and that one
   (`Object40`) is encrypted.

**The remaining hypothesis, untested:** the remainder is organised in per-mesh
blocks rather than flat sections. Two things point there:

- Texture names are NUL-terminated strings, not fixed-width fields. In
  `Monster263.bmd`, `RsylvesterB.jpg` and `RsylvesterA.jpg` sit at 200000 and
  200016, separated by a single `0x00`. A fixed-stride name table is ruled out.
- Texture names appear at widely separated offsets — 63,744 and 222,464 in
  `Monster277.bmd` — which is what scattered per-mesh blocks look like, and not
  what one contiguous section looks like.

So the concrete next step is to find the **mesh block header**: a per-mesh record
holding its own vertex/face/UV counts and its texture name. The header words at
offsets 32, 34, 36, 42 and 44 are still undecoded and are the natural place for
a mesh count. `Sword40.bmd` reads `[2, 2, 1, 88, 129, 69, 142, 0]` and
`Monster263.bmd` reads `[4, 128, 10, 1183, 1872, 638, 2164, 0]`, where the fifth
word is the verified normal count in both — so at least two of the others are
counts too.

Test mesh count candidates by checking whether they partition the remainder into
that many blocks, each containing exactly one texture name. That is falsifiable
and uses the texture names as anchors, which are the one hard landmark found so
far in that region.

### 1b. Recover the v0x0C cipher itself — currently `BLOCKED`

Six hypotheses were tested and ruled out, recorded in
[`ASSET_FORMATS.md`](ASSET_FORMATS.md): periodic XOR at periods 1–32, CFB,
CBC-like chaining, rolling subtraction, key derivation from the header, and
shift-coincidence analysis. Entropy is 7.99 bits/byte. The bux 3-byte key does
**not** apply (1 of 60 files improved).

Legitimate routes, in order of promise:

1. **Open-source MU client implementations.** Several Season 6-era clients are
   publicly available under permissive licences. If one contains the BMD loader,
   the algorithm is available without touching a protected binary. **This is the
   route to pursue.** Requires outbound access to a code host, which the
   development sandbox restricts — that is the actual blocker, not the crypto.
2. **Author or community documentation** of the format.
3. **Known-plaintext analysis using 1a.** If the body format is understood from
   the clear files, the first bytes of an encrypted payload are predictable, and
   that is a real cryptanalytic advantage that does not exist today. This is why
   1a precedes 1b: 1a is not a consolation prize, it is the input to 1b.

**Prohibited route.** The loader lives inside `Main.exe`, which is VMProtect
protected. Unpacking it is prohibited by project policy and will not be done.
Any attempt to "recover" the cipher by guessing a transform and accepting it
because output "looks plausible" is also prohibited — that produces silently
wrong geometry, which is worse than no geometry.

**Unblocking condition:** outbound access to a code host, or a user supplying
the algorithm or a licensed reference implementation.

---

## Secondary: CAND-001 — upload BGRA directly, removing the texture conversion

Demoted from primary by the directive above, not because it lost value. It
remains the highest-value *performance* task that is unblocked, and it should be
taken as soon as 1a is underway or if 1a stalls on data access.

### Problem

`OztToRgbaTopDown` is the most expensive step in the measured texture loading
path:

```
OztToRgbaTopDown 256x256   med 62.4 us   p95 97.2   p99 138.6   mean 68.7   4003.6 MB/s
```

The p99 is 2.2x the median. For a 1024x1024 texture the equivalent work is
roughly 1.1 ms of blocking CPU time. The step exists only to turn OZT's native
BGRA bottom-up layout into RGBA top-down.

### Hypothesis

Uploading with `GL_BGRA` / `GL_UNSIGNED_BYTE` removes the conversion entirely.
`GL_EXT_bgra` is universally available on desktop OpenGL. OZT stores rows
bottom-up, which matches GL's default unpack orientation, so the vertical flip
may not be needed either — **to be verified, not assumed**.

### Acceptance criteria

1. A `mu/gfx` texture upload path accepting an OZT payload without an
   intermediate RGBA copy.
2. `OztToRgbaTopDown` remains available but off the default path.
3. A test proving channel order is correct. **Swapped red and blue is the
   specific failure mode** and is easy to miss in a screenshot; compare numeric
   pixel values.
4. A test proving row order is correct, using an asymmetric image so a vertical
   flip is visible in the data.
5. A benchmark recorded in [`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md) with
   before and after, buffer size, iteration count and standard deviation.
6. Documentation updated.

### Non-goals

Do not build the renderer as part of this task. The upload path is a small
testable module with a null backend so it can be exercised headlessly.

---

## Queue after that

### 3. CAND-002 — vectorise the bux cipher

880.5 MB/s is the slowest per-byte operation measured. A 131 KB terrain file
costs about 150 µs; across 265 terrain files that is roughly 40 ms of serial
work at map load.

**Mandatory test:** byte-for-byte equality against the scalar implementation
across a range of lengths **and phase offsets**. The cipher is its own inverse,
so a wrong implementation still round-trips and a self-inverse test would still
pass. Only the equality test catches it.

### 4. CAND-003 — word-at-a-time search in the JPEG scan skip

`OzjProbe` costs 179.2 µs median on a 512x512 JPEG with a 200 KB scan segment,
scaling linearly with texture size. The two escape rules — `FF 00` is a stuffed
literal, `FF D0..D7` is a restart marker — must survive vectorisation exactly.

### 5. Windows build verification

`docs/BUILD.md` documents an MSVC and a MinGW path. **Neither has been run.**
Needs a person with a Windows machine. High value: the target platform has never
been compiled for and every portability assumption is unverified.

Acceptance: `mutool` and `test_assets` build and the tests pass on Windows,
compiler version recorded in `TEST_RESULTS.md`.

### 6. Runtime baseline

Run `scripts/benchmark_windows.ps1` and record real FPS, frametime, CPU, RAM and
VRAM. Until then every runtime performance statement in this repository is
`NOT EXECUTED` — including the 2,893.80 MB figure, which is a derived footprint
estimate and not a measurement.

### 7. `Settings.ini` / `CESettings.ini` reader

Both files are documented in `REFERENCE_AUDIT.md` section 5. A reader with the
same key names and defaults is a prerequisite for drop-in compatibility and for
testing the `[AntiLag]` and `MaxAnisotropy` trade-offs. Low risk, no blocker.

**Security constraint:** this reader must never copy the `[AutoLogin]`
`User`/`Password` values into a log, a test fixture, a commit message or a
document. See [`SECURITY.md`](SECURITY.md).

### 8. `.ozb` format

100 files, height and light maps. Header not resolved — sampled files open with
long zero runs. Needs the same byte-level investigation the other formats got.

---

## Blocked — do not start without unblocking evidence

| Task | Blocker |
|---|---|
| The 8,594 opaque BMD payloads | Cipher not recovered. Needs code-host access or a supplied reference. **Task 1a is not blocked and does not wait for this.** |
| Most terrain rendering | `Enc*` encryption not recovered. |
| Network compatibility | No authorised test server. |
| FPS / frametime / VRAM measurement | No Windows machine, no GPU. |
| Visual comparison against the original | Needs both a working renderer and a Windows machine. |

If the primary task is already done, take the next unblocked item. Do not start
a blocked one and guess past the blocker — that is how a cipher gets "recovered"
into something that produces silently wrong data.
