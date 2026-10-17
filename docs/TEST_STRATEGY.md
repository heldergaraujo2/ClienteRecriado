# Test strategy

## 1. Principles

**Test against real data, not idealised data.** Three of the most valuable tests
in the suite exist because a real asset file did something the format
description did not predict. See [`TEST_RESULTS.md`](TEST_RESULTS.md) section 6.

**Whole-tree validation for format conclusions.** A 120-file sample supported the
conclusion "OZJ is a plain JPEG". The full tree showed 13,927 of 13,928 files
failing to parse. Any format claim must survive `mutool scan` over everything.

**No original asset bytes in the repository.** Every fixture is generated at run
time. This is a licensing requirement and it also means the suite runs anywhere,
with no setup.

**Correctness before performance.** A faster wrong answer is a defect. The
CRC-32 optimisation shipped with three independently computed expected values
(computed with Python's `zlib`, not read back from the implementation) before
its speedup was recorded.

**Classify honestly.** `PASS`, `FAIL`, `BLOCKED`, `NOT EXECUTED`,
`NOT APPLICABLE`. An absent line reads as "not relevant" when the truth is often
"not yet possible".

## 2. Layers

### 2.1 Unit tests — `tests/test_assets.cpp`

A ~40-line harness, no framework. `make test` must report 0 failures.

Coverage targets, in priority order:

1. **Malformed and truncated input.** Every reader is fed files from the
   internet. Each reader has tests for truncation at multiple points, bad
   signatures, out-of-range dimensions and impossible length fields. A reader
   that can be made to read out of bounds is a security defect, not a test gap.
2. **Round-tripping.** Where a writer exists, write then read and assert
   byte-for-byte equality. This is how OZT is tested: `OztWriteRgba` →
   `OztProbe` → `OztRead` → `OztToRgbaTopDown` → compare to the original RGBA.
3. **Known-answer tests.** Values computed independently. The CRC-32 expected
   values come from Python's `zlib.crc32`, never from the implementation under
   test.
4. **Real-world shapes.** The duplicated OZJ header prefix, the OZT variant
   marker, bux-encrypted non-model files, terrain families with different header
   lengths.
5. **Self-inverse and idempotence properties.** The bux cipher is its own
   inverse; that property is asserted, along with the derived property that zero
   plaintext encrypts to the key itself.

Tests deliberately **do not** cover: rendering output (no backend yet), network
(no server), audio (not implemented). Those are recorded as `NOT EXECUTED`
rather than skipped silently.

### 2.2 Sanitiser runs

The suite is also built with `-fsanitize=address,undefined` and run. No reports
so far. This should be repeated after any change to buffer arithmetic, which is
most changes in this codebase.

### 2.3 Whole-tree validation — `mutool scan` / `mutool verify`

Requires the reference client at `../Cliente_ref`. The expected counts are
recorded in `AGENTS.md`; a drop in any `ok` count is a regression to investigate,
not a number to update.

### 2.4 Micro-benchmarks — `bench/bench_assets.cpp`

Guard performance changes. Median plus P95 and P99, warm-up excluded, dispersion
reported. See [`BENCHMARKS.md`](BENCHMARKS.md).

### 2.5 Runtime tests — `scripts/`

Windows only, and **not yet executed**. Covers FPS, frametime percentiles, CPU,
RAM, VRAM, load times, long-session stability and map changes.

### 2.6 Platform-independent frame-loop tests

`tests/test_main_loop_standalone.cpp` runs as part of `make test` and
`make test-san`. It uses synthetic callbacks to verify update/render/present
ordering, result propagation, monotonic callback timing, invalid-rate handling,
history bounds, and prediction gating. The timing assertions test only the
orchestration logic; they are not FPS, full-frame, or GPU benchmarks.

## 3. Regression detection

The cheap, always-on layer:

```sh
make && make test        # must be 0 failures, zero warnings
```

The whole-tree layer, when the reference client is available:

```sh
build/mutool scan   ../Cliente_ref/Data
build/mutool verify ../Cliente_ref/ListaPatch.txt ../Cliente_ref
```

Watch:

- Any `fail` count increasing.
- Any failure category appearing that is not already accounted for in
  [`TEST_RESULTS.md`](TEST_RESULTS.md) section 2.1.
- The texture memory estimate changing, which would mean a header is being
  parsed differently.

## 4. Testing rules for optimisations

Every optimisation must be recorded in [`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md)
with all eight fields: problem, evidence, hypothesis, change, test,
before/after, side effects, decision.

Two hard rules:

**A change that raises mean FPS but worsens crashes, frametime spikes, visual
correctness or stability is a regression and gets reverted.** This is stated
explicitly because it is the most common way a performance project goes wrong.

**A change whose benefit cannot be measured gets reverted.** If before and after
are within noise, say so and back it out. Do not keep it on the grounds that it
"should" be faster.

## 5. Test gaps

Recorded rather than hidden.

| Gap | Why it is open | Risk |
|---|---|---|
| No rendering tests | No backend exists | High — visual correctness is currently untestable |
| No JPEG pixel decode tests | No decoder in the library | Medium — geometry is verified, pixels are not |
| No BMD payload tests | Cipher not recovered | High — blocks geometry and animation entirely |
| No network tests | No authorised server | Medium — protocol compatibility unverified |
| No Windows build test | No Windows toolchain | High — the actual target platform is unbuilt |
| No concurrency tests | Nothing is threaded yet | Low now, high once loading moves off the main thread |

The Windows build gap deserves emphasis: the target platform has never been
compiled for. `docs/BUILD.md` documents the intended path, but it is unverified.
