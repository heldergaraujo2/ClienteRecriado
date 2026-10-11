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

**Where to resume: the bytes between offset 42 and the start of the normals
region.** That gap is undecoded and ranges from 852 to 357,308 bytes. Dividing it
by each header u16 at plausible record sizes closed for only 1 file in every
case, which is noise — so it is not one flat array. The likeliest reading is a
variable-length section (texture names, then positions, then bones) whose
sub-counts are among the header words already visible at offsets 32–46, several
of which are small plausible counts: `Sword40.bmd` reads
`[2, 2, 1, 88, 129, 69, 142, 0]` and `Monster263.bmd` reads
`[4, 128, 10, 1183, 1872, 638, 2164, 0]`, where the fifth word is the verified
normal count in both.

Decode one section at a time against the normals anchor rather than guessing the
whole layout. The normals region gives a known byte offset to work backwards and
forwards from, which is the advantage that did not exist before this cycle.

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
