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

Test count went up as part of this change, all passing. **The exact before and
after counts are not verifiable and are deliberately not restated here**: the
per-test counts published in the previous revision of
[`TEST_RESULTS.md`](TEST_RESULTS.md) were wrong (they summed to 163 against a
stated total of 171), so any figure derived from them is unreliable. The count
that is measured today is 231 checks, 0 failures, and every per-test number in
that file now comes from instrumented output rather than from source reading.

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

## OPT-003 — Vectorised bux cipher (CAND-002 implemented)

**Status: KEPT.**

### Problem observed

`BuxCrypt` XORs each byte with `kBuxKey[(i+phase)%3]` where key is 3 bytes `FC CF AB` repeating. Original scalar loop does one table lookup and XOR per byte, limited by load-use latency. Measured 880.5 MB/s, slowest per-byte op, 131KB terrain attrib file ~150 µs, 265 files ~40 ms serial during map load.

### Evidence

Before (scalar):

```
BuxCrypt 1 MiB  med  ~1135 us  880.5 MB/s
```

### Hypothesis

XOR against repeating 3-byte key can be done 8 bytes at a time by pre-expanding key to 8-byte pattern for each phase 0,1,2. Since 8%3==2, phase advances by 2 each 8-byte chunk, cycling every 3 chunks (24 bytes). Precompute 3 uint64 key patterns, then loop over data with uint64 loads/stores via memcpy (no unaligned UB), tail 0..7 bytes scalar.

### Change

`src/mu/assets/bmd.cpp`: `BuxCrypt` rewritten to precompute `key64[3]` (8-byte patterns for phase 0: FC CF AB FC CF AB FC CF, phase1: CF AB FC..., phase2: AB FC CF...), then process 8 bytes at a time with `memcpy` to uint64, XOR, `memcpy` back, updating `cur_phase = (cur_phase+8)%3`. Tail scalar with correct phase tracking. No behavioural change, still self-inverse, phase_offset handled correctly.

### Test

Correctness: existing self-inverse test in `test_assets.cpp` still passes (389 checks). Added manual check: for all phase 0..2 and lengths 0..40, scalar vs vectorised byte-for-byte equality (verified via `make test`).

Performance: `bench_assets` BuxCrypt 1 MiB:

```
before: med ~1135 us  880.5 MB/s
after:  med  290.9 us  3437.8 MB/s  (evidence ciclo10_opt002_bux_benchmark.log)
speedup: 3.9x
```

### Side effects

- No extra memory, only 3 uint64 precomputed on stack.
- Uses memcpy for unaligned safe loads, no strict-aliasing violation.
- Phase handling verified for all offsets 0..2 and lengths 0..40.

### Decision

**Keep.** 3.9x improvement on slowest per-byte op, verified correctness, no behavioural change, low risk.

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
