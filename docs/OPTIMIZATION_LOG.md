# Optimization log

One entry per change, in the required format: problem, evidence, hypothesis,
change, test, before/after, side effects, decision.

An entry is only added when there is a measurement. Changes made for
correctness go in [`CHANGELOG.md`](CHANGELOG.md) instead.

---

## OPT-001 — CRC-32 from slice-by-1 to slice-by-8

**Status: KEPT.**

### Problem observed

`mutool verify` recomputes a CRC-32 over every file listed in a patch manifest.
On a full asset tree this is a whole-tree read plus checksum, so CRC throughput
directly bounds how fast integrity checking can run — and integrity checking is
something a distribution pipeline wants to run often.

### Evidence

`build/bench_assets`, slice-by-1 implementation:

```
Crc32 1 MiB    med 2553.5 us   p95 2581.9   p99 2613.3   mean 2555.1    391.6 MB/s
```

391.6 MB/s. For reference, the same harness measured `ByteEntropy` over the same
1 MiB buffer at 656.3 µs — a histogram pass was **faster** than the checksum.
That ordering is the tell: a table-driven CRC should not be slower than counting
byte frequencies.

### Hypothesis

The byte-at-a-time loop does one table lookup and one shift per input byte, so
it is limited by load-use latency on the table rather than by memory bandwidth.
Processing eight bytes per iteration from eight tables exposes enough
independent work for the out-of-order engine to hide that latency.

### Change

`src/mu/assets/format.cpp`: replaced the single 256-entry table with eight, and
rewrote `Crc32Update` to consume eight bytes per iteration with a byte-at-a-time
tail. Both 32-bit words are read with `memcpy` rather than a cast, so there is
no unaligned access and no strict-aliasing violation.

### Test

Correctness first, performance second — a fast wrong checksum is worthless.

Added to `tests/test_assets.cpp`, with expected values computed independently
using Python's `zlib.crc32` rather than taken from the implementation:

```
1024 bytes, 0..255 repeating      -> 0xB70B4C26
4096 zero bytes                   -> 0xC71C0011
37 bytes (odd length, tail path)  -> 0x39A397B4
```

Plus the canonical `CRC-32/ISO-HDLC` check value for `"123456789"` =
`0xCBF43926`, and a loop asserting that for every length from 0 to 40 the
one-shot result equals the result of feeding one byte at a time. That last check
is what catches an off-by-one in the tail loop.

### Result

Same machine, same flags, same harness:

```
before:  med 2553.5 us   391.6 MB/s
after:   med  507.2 us  1971.6 MB/s
speedup: 5.03x
```

Test count went from 131 to 171 checks, all passing.

### Side effects

- Table memory grows from 1 KB to 8 KB, allocated once on first use. Negligible.
- First call pays for building the tables. Irrelevant at 8 KB, and it is inside
  a function-local static so it is thread-safe under C++11 rules.
- No behavioural change. `mutool verify` produces identical results.

### Decision

**Keep.** A 5x improvement on a correctness-critical path, verified against
independently computed values, with no behavioural change.

---

## OPT-002 — Header-only probing for inventories

**Status: KEPT.** This was a design choice validated by measurement rather than
a fix.

### Problem

A naive inventory tool would decode every texture to learn its dimensions. On
2,372 MB of assets that means reading and converting roughly 2.83 GB of pixel
data to produce a table of counts.

### Evidence

`OztProbe` touches 22 bytes; `OztRead` copies the whole payload.

```
OztProbe 256x256              med 0.0 us
OztRead 256x256 (copy)        med 6.0 us
OztToRgbaTopDown 256x256      med 62.4 us
```

The ratio between probing and full decode-plus-convert is roughly 1:60 in time,
before accounting for the memory traffic.

### Change

`mutool scan` calls the `*Probe` functions only. Pixel data is never copied
during an inventory.

### Result

```
mutool scan over 29,060 files / 2,372.04 MB : 1,641–1,674 ms
```

Fast enough to run in CI on every change, which is what makes the regression
detection in `AGENTS.md` practical.

### Decision

**Keep**, and extend the pattern: every format gets a `Probe` that validates
without copying, and a `Read` that does. Any future resource manager should
validate headers on the loading thread and only touch pixels when the texture is
actually needed.

---

## Candidates identified, not yet implemented

Each has evidence and a stated expected effect. None has been measured, so none
is claimed.

### CAND-001 — Upload BGRA directly instead of converting

**Evidence.** `OztToRgbaTopDown` costs 62.4 µs median and 138.6 µs at p99 for a
256x256 texture — the most expensive step in the texture loading path measured
so far, with a 2.2x tail. OZT payloads are natively BGRA; the conversion exists
only to reach an RGBA layout.

**Hypothesis.** Uploading with `GL_BGRA` / `GL_UNSIGNED_BYTE` removes the step
entirely. `GL_EXT_bgra` is universally available on desktop OpenGL.

**Test.** Time texture load with and without the conversion, then compare
rendered output pixel-for-pixel to confirm channel order is correct. A swapped
red and blue channel is a bug that is easy to introduce and easy to miss in a
screenshot.

**Risk.** Low, but the vertical flip must be handled separately — `GL_BGRA` does
not remove the need for it. Note that OZT stores rows bottom-up, which happens
to match GL's default `GL_UNPACK` orientation, so the flip may not be needed at
all for direct upload. That needs verifying, not assuming.

### CAND-002 — Vectorise the bux cipher

**Evidence.** 880.5 MB/s, the slowest per-byte operation measured. A 131,076
byte terrain attribute file costs about 150 µs; across 265 terrain files that is
roughly 40 ms of serial work during map load.

**Hypothesis.** XOR against a repeating 3-byte key can be done 16 or 32 bytes at
a time by pre-expanding the key to a multiple of the vector width, or simply by
unrolling to a 3-byte-period loop over machine words.

**Test.** `BuxCrypt` benchmark before and after, plus the existing
self-inverse test and a new test asserting byte-for-byte equality with the
scalar implementation on a range of lengths and phase offsets.

**Risk.** Low. The phase offset must be handled correctly or decryption silently
produces garbage — and because the cipher is its own inverse, a wrong
implementation still round-trips, so the equality test against the scalar
version is mandatory rather than optional.

### CAND-003 — Word-at-a-time search in the JPEG scan skip

**Evidence.** `OzjProbe` on a 512x512 JPEG with a 200 KB entropy-coded segment
costs 179.2 µs median and 272.7 µs at p99. The dominant cost is scanning for the
terminating marker one byte at a time. Real JPEG scan data is larger than 200 KB
for high-resolution textures, so this scales linearly with texture size.

**Hypothesis.** Searching for a `0xFF` byte using word-sized loads and a
bitmask, then validating candidate positions, reduces the byte loop by 4–8x.

**Test.** Benchmark before and after; the existing OZJ tests plus the duplicated
prefix test must still pass.

**Risk.** Moderate. The two escape rules — `FF 00` is a stuffed literal and
`FF D0..D7` is a restart marker — must survive the vectorisation exactly. A
subtle error here means a texture silently reports the wrong dimensions.
