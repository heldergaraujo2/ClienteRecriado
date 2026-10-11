# Benchmarks

Methodology first, then results. A number without a method is not a benchmark.

## 1. Two different things are benchmarked here

**A. CPU-side asset work** — runnable anywhere, measured and recorded below.
**B. Runtime game performance** — FPS, frametime, VRAM. Requires Windows and a
GPU. Not yet executed. The harness exists.

Do not quote an A number as if it were a B number. Micro-benchmark throughput
says nothing about a frame rate.

## 2. Methodology for A (CPU-side)

```sh
make bench && ./build/bench_assets
```

Rules the harness follows:

1. **Warm-up before timing.** The first invocation pays for page faults and
   cold caches. Excluding it is not cherry-picking; including it would measure
   the allocator rather than the code.
2. **Median plus P95 and P99, not just the mean.** For load-time work the tail
   is the user-visible quantity. A loader with a 2 ms mean and a 40 ms p99
   produces a hitch, and a mean-only report would hide it. The measured OZT
   conversion shows exactly this: median 62.4 µs, p99 138.6 µs.
3. **Report the dispersion.** Where runs varied, the spread is stated rather
   than the best result being selected. Scan timings across two runs: 1,674 ms
   and 1,641 ms — a 2% spread, reported as a range.
4. **Repeat enough times to clear timer noise.** 20,000 repetitions for
   sub-microsecond probes, 200 for multi-millisecond ones.
5. **Same machine, same build flags, consecutive runs.** Comparing across
   machines or optimisation levels is meaningless.

Environment: g++ 12.2.0, `-std=c++17 -O2`, Linux x86_64, no GPU.

## 3. Results — CPU-side

### 3.1 Current

```
benchmark                            med us   p95 us   p99 us  mean us         MB      MB/s
---------                            ------   ------   ------  -------         --      ----
OztProbe 256x256                        0.0      0.0      0.0      0.0       0.25  8930206.3
OztRead 256x256 (copy pixels)           6.0      6.9     10.0      6.4       0.25    41820.7
OztToRgbaTopDown 256x256               62.4     97.2    138.6     68.7       0.25     4003.6
OztRead 512x512                        61.6     95.0    163.4     70.0       1.00    16226.1
OztRead 1024x1024                     274.8    321.7    351.8    290.7       4.00    14558.8
BuxCrypt 1 MiB                       1135.7   1189.3   1200.2   1146.5       1.00      880.5
ByteEntropy 1 MiB                     472.2    487.7    506.5    469.4       1.00     2117.9
Crc32 1 MiB                           507.2    527.5    537.9    503.6       1.00     1971.6
ManifestParse 5000 entries           1002.1   1060.8   1083.3   1012.4       0.19      190.3
OzjProbe 512x512 + 200KB scan         179.2    237.0    272.7    187.1       0.19     1064.9
```

### 3.2 Tool-level timings

| Operation | Time | Spread |
|---|---|---|
| `mutool scan` — 29,060 files, 2,372.04 MB | 1,641–1,674 ms | 2% over 2 runs |
| `mutool verify` — 188 manifest entries | 42–49 ms | 17% over 3 runs |

The verify spread is larger because the working set is small enough that page
cache state dominates. That is stated rather than hidden.

### 3.3 Regression recorded

The CRC-32 change from slice-by-1 to slice-by-8 is the one optimisation so far
with a before/after pair measured in the same harness:

```
Crc32 1 MiB   before:  2553.5 us median    391.6 MB/s
Crc32 1 MiB   after:    507.2 us median   1971.6 MB/s
speedup: 5.03x
```

Full write-up in [`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md).

## 4. Methodology for B (runtime) — not yet executed

`scripts/benchmark_windows.ps1` implements this. It is written and reviewed but
**has never been run**: there is no Windows machine in the development
environment. Running it and recording real output is a queued task.

### 4.1 Scenarios

Each scenario is run three times, results reported as median and spread.

| ID | Scenario | What it isolates |
|---|---|---|
| S1 | Cold process start to main menu | Startup and initial load |
| S2 | Login to first map playable | Network handshake plus map load |
| S3 | Idle in an empty area, 60 s | Baseline frame cost |
| S4 | Walking in a sparsely populated area | Terrain streaming |
| S5 | Walking in a densely populated area | Draw calls and entity updates |
| S6 | 20+ characters on screen | Entity and animation cost |
| S7 | Combat with heavy skill effects | Particles and effects |
| S8 | Inventory and UI interaction | UI path cost |
| S9 | Map change, 10 consecutive changes | Load spikes and residency churn |
| S10 | 30-minute idle session | Leaks and fragmentation |

### 4.2 Metrics per scenario

- FPS: mean, minimum, 1st and 5th percentile
- Frametime: median, P95, P99, and the count and duration of spikes over 33 ms
- CPU: mean and peak, per process
- GPU: mean and peak utilisation, plus VRAM
- Working set RAM, peak
- Load times: startup, each map change
- Errors, crashes, freezes, with timestamps

### 4.3 Reporting rules

- Hardware, OS build, resolution, refresh rate, driver version and every
  graphics setting must be recorded alongside the numbers. A result without
  them cannot be compared to anything.
- Never compare results from different machines, drivers or settings.
- Report dispersion. If three runs differ by more than 10%, say so and
  investigate before drawing a conclusion.
- Never report the best of several runs as the result.
- An optimisation is only accepted if it improves its target metric **without**
  regressing crashes, frametime spikes or visual correctness. A higher mean FPS
  with a worse p99 is a regression, not an improvement.

## 5. How to add a benchmark

1. Add the case to `bench/bench_assets.cpp` following the existing pattern:
   warm-up, N repetitions, `Summarise`, `PrintRow`.
2. Choose the repetition count so the median is well above timer noise — at
   least 100x the clock resolution.
3. Record the baseline output in this file *before* making the change.
4. Make the change, re-run, and record both numbers in
   [`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md) with the evidence, hypothesis
   and decision.
5. If the result is within noise, say so and revert. Do not keep a change whose
   benefit cannot be measured.
