# Next task

## Primary: CAND-001 — upload BGRA directly, removing the texture conversion step

### Why this one

It is the highest-value task that is **not blocked**. The two larger items —
BMD model loading and `Enc*` terrain — are both gated on ciphers that have not
been recovered, and neither can be advanced without the algorithm arriving from
a legitimate source. CAND-001 has measured evidence, a clear implementation and
a testable outcome.

### Problem

`OztToRgbaTopDown` is the most expensive step in the measured texture loading
path:

```
OztToRgbaTopDown 256x256   med 62.4 us   p95 97.2   p99 138.6   mean 68.7   4003.6 MB/s
```

The p99 is 2.2x the median. For a 1024x1024 texture the equivalent work is
roughly 1.1 ms of blocking CPU time on whichever thread asked for the texture.
The step exists only to turn OZT's native BGRA bottom-up layout into RGBA
top-down.

### Hypothesis

Uploading with `GL_BGRA` / `GL_UNSIGNED_BYTE` removes the conversion entirely.
`GL_EXT_bgra` is universally available on desktop OpenGL. OZT stores rows
bottom-up, which matches GL's default unpack orientation, so the vertical flip
may not be needed either — but that must be **verified, not assumed**.

### Acceptance criteria

1. A `mu/gfx` texture upload path that accepts an OZT payload without an
   intermediate RGBA copy.
2. `OztToRgbaTopDown` remains available — some paths will still need RGBA — but
   is no longer on the default upload path.
3. A test proving channel order is correct. **A swapped red and blue channel is
   the specific failure mode here**, and it is easy to miss in a screenshot.
   The test must compare numeric pixel values, not images.
4. A test proving row order is correct, with an asymmetric test image so a
   vertical flip is visible in the data.
5. A benchmark showing the removal, recorded in
   [`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md) with before and after.
6. Documentation updated: `CURRENT_STATE.md`, `CHANGELOG.md`, and this file.

### Explicit non-goals

Do not build the whole renderer as part of this task. The upload path should be
a small, testable module with a null backend so it can be exercised headlessly.

---

## Queue after that

Ordered by value-per-risk, with the blocker stated for each.

### 2. CAND-002 — vectorise the bux cipher

880.5 MB/s is the slowest per-byte operation measured. A 131 KB terrain file
costs about 150 µs; across 265 terrain files that is roughly 40 ms of serial
work at map load.

**Mandatory test:** byte-for-byte equality against the scalar implementation
across a range of lengths **and phase offsets**. The cipher is its own inverse,
so a wrong implementation still round-trips and the self-inverse test would
still pass. Only the equality test catches it.

### 3. CAND-003 — word-at-a-time search in the JPEG scan skip

`OzjProbe` costs 179.2 µs median on a 512x512 JPEG with a 200 KB scan segment,
scaling linearly with texture size. The two escape rules — `FF 00` is a stuffed
literal, `FF D0..D7` is a restart marker — must survive vectorisation exactly.

### 4. Windows build verification

`docs/BUILD.md` documents an MSVC and a MinGW path. **Neither has been run.**
This needs a person with a Windows machine. It is high value because the target
platform has never been compiled for, and every portability assumption in the
codebase is currently unverified.

Acceptance: `mutool` and `test_assets` build and the tests pass on Windows, with
the compiler version recorded in `TEST_RESULTS.md`.

### 5. Runtime baseline

Run `scripts/benchmark_windows.ps1` and record real FPS, frametime, CPU, RAM and
VRAM numbers. Until this is done, every runtime performance statement in this
repository is `NOT EXECUTED`.

### 6. `Settings.ini` / `CESettings.ini` reader

Both configuration files are documented in `REFERENCE_AUDIT.md` section 5. A
reader with the same key names and defaults is a prerequisite for drop-in
compatibility and for testing the `[AntiLag]` and `MaxAnisotropy` trade-offs.
Low risk, no blocker.

### 7. `.ozb` format

100 files, height and light maps. Header not resolved — sampled files open with
long zero runs. Needs the same byte-level investigation the other formats got.

---

## Blocked — do not start without unblocking evidence

| Task | Blocker |
|---|---|
| Model loading, geometry, skeletal animation | BMD v0x0C payload cipher not recovered. Requires the algorithm from a legitimate source. |
| Most terrain rendering | `Enc*` encryption not recovered. |
| Network compatibility | No authorised test server. |
| FPS / frametime / VRAM measurement | No Windows machine, no GPU. |
| Visual comparison against the original | Needs both a working renderer and a Windows machine. |

If you are an agent picking up this project and the primary task is already
done, take the next unblocked item. Do not start a blocked one and then guess
your way past the blocker — that is how a cipher gets "recovered" into something
that produces silently wrong data.
