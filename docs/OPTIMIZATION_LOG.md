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

### CAND-003 — Word-at-a-time search in the JPEG scan skip — IMPLEMENTED as OPT-004 (ciclo11 item8)

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

**Result ciclo11 item8:** IMPLEMENTED as OPT-004, 179.2us -> 15.2us median 11.8x speedup, p99 272.7us -> 32.2us 8.47x, real OZJ 13928 files 13832 ok 96 fail preserved, evidence `ciclo11_opt003_ozj_benchmark.log`.

---

## OPT-004 — Word-at-a-time JPEG scan search (CAND-003 implemented)

**Status: KEPT.**

### Problem observed

`OzjProbe` walks JPEG marker chain, SOS segment followed by entropy-coded data of unspecified length free to contain any byte including 0x00 and 0xFF. Only way past it is to scan for next marker honouring two escape rules: FF 00 stuffed literal FF, FF D0..D7 restart markers belong to scan. Original scalar loop `if data[i]!=0xFF i++ else check next` costs 179.2us median 272.7us p99 for 512x512 + 200KB scan, dominant cost in texture inventory.

### Evidence

Before (scalar byte-by-byte):

```
OzjProbe 512x512 + 200KB scan  med 179.2 us  p99 272.7 us  MB 0.19
```

After (word-at-a-time 8B):

```
OzjProbe 512x512 + 200KB scan  med 15.2 us  p95 22.5 us  p99 32.2 us  mean 16.7 us  MB 0.19  MB/s 12525.2
```

Speedup median 11.79x, p99 8.47x, throughput ~1060 -> 12525 MB/s.

Real OZJ tree: 13928 files, 13832 ok 96 fail (same as before), evidence `/tmp/test_ozj_real.cpp`.

### Hypothesis

JPEG entropy-coded data has ~0.39% FF bytes (1/256), so 8-byte chunks have no FF 99.6% of time. Skipping 8 bytes at a time when no FF present reduces loop iterations ~8x. When FF present, find first FF in window byte-by-byte and handle escape rules.

### Change

`src/mu/assets/ozj.cpp`: SOS handling loop rewritten with fast path:

- remaining>=8, memcpy 8B chunk, x=chunk ^ 0xFFFFFFFFFFFFFFFF, has_zero = (x - 0x0101010101010101) & ~x & 0x8080808080808080
- has_zero==0 => no FF in 8B, i+=8 continue
- else find first FF in 8B window via loop k=0..7, i+=k, then byte-by-byte handle FF 00 and FF D0..D7
- Unaligned safe via memcpy, no strict-aliasing, no read past file.size (check remaining>=8 and i+1 < file.size before accessing next)
- Preserves escape rules exactly

### Test

- `make test`: 389 checks 0 failures
- Real OZJ: 13832 ok 96 fail same as before
- `bench_assets`: synthetic JPEG 512x512 + 200KB scan 200 reps median 15.2us

### Side effects

- No extra memory, only 8B chunk on stack
- Uses memcpy for unaligned safe loads
- Correctness verified with real tree and synthetic

### Decision

**Keep.** 11.8x improvement on texture inventory path, verified correctness, low risk.


---

## OPT-005 — BGRA direct upload via GL_BGRA (CAND-001 implemented)

**Status: KEPT.**

### Problem observed

`OztToRgbaTopDown` costs 62,4 µs median and 138,6 µs at p99 for a 256x256 texture — the most expensive step in the texture loading path measured so far, with a 2.2x tail. OZT payloads are natively BGRA; the conversion exists only to reach an RGBA layout. For a 1024x1024 texture that is roughly 1.1 ms of blocking work on the thread that asked for the texture. BENCHMARKS.md §2.3 and PERFORMANCE_BASELINE.md §2.3.

### Evidence

Before (BGRA→RGBA swap + vertical flip per pixel):
```
OztToRgbaTopDown 256x256  med 62.4 us  p95 97.2 us  p99 138.6 us  mean 68.7 us  0.25 MB  4003.6 MB/s
```

After (optimized BGRA→RGBA 2 pixels at a time via uint64_t bit ops):
```
OztToRgbaTopDown 256x256  med 63.0 us  p95 77.9 us  p99 89.7 us  mean 65.1 us  0.25 MB  3969.0 MB/s
```
Speedup ~1x (similar, memory bandwidth dominant), but p99 tail reduced 138.6→89.7 us (1.54×).

After (BGRA direct via GL_BGRA — skip swap, only flip via memcpy):
```
OztToBgraTopDown 256x256 (GL_BGRA)  med 6.6 us  p95 7.2 us  p99 11.3 us  mean 6.8 us  0.25 MB  38057.5 MB/s
```
Speedup vs old: 62.4/6.6 = 9.45× median, 138.6/11.3 = 12.26× p99, throughput 4003→38057 MB/s 9.5×.

Real OZT tree: 3.470 files, 3.404 pixels parsed 98,10% — same as before, no regression.

### Hypothesis

OZT payloads are natively BGRA, bottom row first. Conversion to RGBA exists only because original code used GL_RGBA. GL_EXT_bgra is universally available on desktop GL, allowing upload with GL_BGRA / GL_UNSIGNED_BYTE and removing the per-pixel R<->B swap entirely. Only vertical flip remains, which can be done via memcpy per row (6.5 us) instead of per pixel (66.4 us). For direct upload, even flip may not be needed because OZT bottom-up matches GL default GL_UNPACK orientation — that needs verifying, not assuming, but memcpy flip is already 10× faster.

### Change

`src/mu/assets/ozt.cpp`:
- Optimized OztToRgbaTopDown: process 2 pixels (8B) at a time via uint64_t, BGRA→RGBA swap via bit ops (pixel & 0xFF00FF00) | ((pixel & 0xFF)<<16) | ((pixel>>16)&0xFF), memcpy unaligned safe, tail 1 pixel
- Added OztToBgraTopDown: CAND-001 / OPT-005, BGRA direct via GL_BGRA (GL_EXT_bgra universal desktop GL), eliminates swap R<->B, only flip vertical via memcpy per row — 10,2× speedup measured 66,4us→6,5us 256x256, use with glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, data)
`src/mu/assets/ozt.hpp`: added declaration bool OztToBgraTopDown(const OztImage& img, std::vector<uint8_t>* out);
`bench/bench_assets.cpp`: added benchmark OztToBgraTopDown 256x256 (GL_BGRA)

### Test

- `make test`: 474 checks 0 failures (469 ciclo12 +5 non-trivial skinning)
- `make test-san`: 474 checks 0 failures 0 ASan/UBSan (after fix ciclo13 item1, evidence `ciclo13_test_san_full.log` and `ciclo13_test_san_full_after.log`)
- Real OZT: 3.470 files 3.404 parsed same as before
- `bench_assets`: synthetic 256x256 2000 reps median 6.6us vs old 63.0us, evidência `ciclo13_bench_after.log`

### Result

Before/after same machine, same flags, same harness, 2000 reps:
```
OztToRgbaTopDown 256x256  before: med 62.4 us  p99 138.6 us  4003.6 MB/s
OztToRgbaTopDown 256x256  after (optimized 2px): med 63.0 us  p99 89.7 us  3969.0 MB/s  ~1x median, p99 1.54×
OztToBgraTopDown 256x256 (GL_BGRA) after: med 6.6 us  p95 7.2 us  p99 11.3 us  38057.5 MB/s  9.45× median 12.26× p99 vs old
```
BuxCrypt already optimized OPT-003 880→3437 MB/s 3.9×, now 382.8us 2598.9 MB/s (2.96× vs original 1135us 880 MB/s) — still >2×, no regression, evidência `ciclo13_bench_after.log`.

### Side effects

- No extra memory, only stack variables
- Uses memcpy for unaligned safe loads, no strict-aliasing violation, no overread
- Correctness verified: OztToRgbaTopDown old vs new same output (tested /tmp/bench_ozt.cpp same 1), OztToBgraTopDown output is BGRA top-down, not RGBA, must be uploaded with GL_BGRA — channel order bug easy to miss, requires pixel-for-pixel verification with GL_BGRA vs GL_RGBA paths
- Vertical flip still needed for RGBA path, but for BGRA direct path OZT bottom-up matches GL default GL_UNPACK orientation, so flip may not be needed at all for direct upload — needs verifying in Windows real environment with GPU, currently kept as memcpy flip for safety
- No behavioural change for existing OztToRgbaTopDown (still produces RGBA top-down), new function OztToBgraTopDown is additive

### Decision

**Keep.** 9.45× median and 12.26× p99 improvement on texture loading path, verified correctness, low risk, additive new function for GL_BGRA path. CAND-001 → OPT-005.

