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
   produces a hitch, and a mean-only report would hide it. The latest cycle-17 single run reports OZT RGBA conversion median 55.5 µs, p99 80.4 µs; this is one machine/run snapshot, not a cross-version speedup claim.
3. **Report the dispersion.** Where multiple runs exist, the spread is stated
   rather than the best result being selected. The 1,674 ms and 1,641 ms scan
   timings are a historical cycle-1 two-run range, not a cycle-17 comparison.
4. **Use repetitions appropriate to each case.** Counts are chosen per
   benchmark in `bench/bench_assets.cpp`; the current snapshot reports each
   harness case's internal repetitions where available.
5. **Same machine, same build flags, consecutive runs.** Comparing across
   machines or optimisation levels is meaningless.

Environment: g++ 12.2.0, `-std=c++17 -O2`, Linux x86_64, no GPU.

## 3. Results — CPU-side

**Units:** current outputs use MiB (1,048,576 bytes). Several historical logs
label the same binary unit as `MB`; those labels are preserved in the historical
records and do not mean decimal megabytes.

### 3.1 Current cycle-17 CPU microbenchmark snapshot

One run of `build/bench_assets` on the environment above. Each row includes the
harness's internal repetitions; this does **not** measure run-to-run variability
and must not be compared to another machine/build as a speedup. Raw output:
[`evidence/ciclo17_bench_final.log`](evidence/ciclo17_bench_final.log).

```
benchmark                            med us   p95 us   p99 us  mean us        MiB     MiB/s
---------                            ------   ------   ------  -------        ---     -----
OztProbe 256x256                        0.0      0.0      0.0      0.0       0.25 7354287.5
OztRead 256x256 (copy pixels)           5.9      6.0     10.4      6.1       0.25   42626.3
OztToRgbaTopDown 256x256               55.5     70.4     80.4     56.9       0.25    4502.5
OztToBgraTopDown 256x256 (GL_BGRA)      6.5      6.7     12.3      6.7       0.25   38261.4
OztRead 512x512                        41.1     76.4     91.5     45.8       1.00   24305.4
OztRead 1024x1024                     332.5    396.8    428.4    344.4       4.00   12030.6
BuxCrypt 1 MiB                        295.5    319.8    334.7    300.1       1.00    3384.0
ByteEntropy 1 MiB                     488.3    528.0    547.1    491.6       1.00    2047.9
Crc32 64 KiB (in cache)                30.4     35.8     48.8     31.3       0.06    2058.8
Crc32 1024 KiB (in cache)             503.1    534.9    556.1    506.1       1.00    1987.5
Crc32 16384 KiB (in cache)           8716.2   8900.2   8900.2   8761.8      16.00    1835.7
ManifestParse 5000 entries             958.1   1014.0   1107.0    964.0       0.19     199.1
OzjProbe 512x512 + 200KB scan          14.4     19.7     40.0     15.6       0.19   13286.7
LOAD-TIME expand indexed -> flat    19860.2  25045.4  25045.4  21299.2      67.21    3384.3
LOAD-TIME vertex buffer preparation   3399.9   3869.2   3905.9   3567.2      41.02   12064.6
```

The same log explicitly marks per-frame GL/driver cost as BLOCKED. The 67.21 MiB
expanded volume and 52.22 MiB hypothetical indexed-buffer volume are data-volume
calculations in the latest snapshot, not measured frame costs. No FPS or runtime
latency is inferred.

### 3.2 Historical cycle-11 baseline (superseded as the latest CPU snapshot)

The older table below is retained for before/after history only; it is not the
current cycle-17 reading.

```
OztProbe 256x256                        0.0      0.0      0.0      0.0       0.25  8930206.3
OztRead 256x256 (copy pixels)           6.0      6.9     10.0      6.4       0.25    41820.7
OztToRgbaTopDown 256x256               62.4     97.2    138.6     68.7       0.25     4003.6
OztRead 512x512                        61.6     95.0    163.4     70.0       1.00    16226.1
OztRead 1024x1024                     274.8    321.7    351.8    290.7       4.00    14558.8
BuxCrypt 1 MiB                       1135.7   1189.3   1200.2   1146.5       1.00      880.5
ByteEntropy 1 MiB                     472.2    487.7    506.5    469.4       1.00     2117.9
Crc32 1 MiB                           507.2   527.5    537.9    503.6       1.00     1971.6
ManifestParse 5000 entries           1002.1  1060.8  1083.3  1012.4       0.19      190.3
OzjProbe 512x512 + 200KB scan         179.2   237.0    272.7   187.1       0.19     1064.9
```

### 3.3 Tool-level timings (historical cycle-1 runs)

| Operation | Time | Spread |
|---|---|---|
| `mutool scan` — 29,060 files, 2,372.04 MiB | 1,641–1,674 ms | 2% over 2 runs |
| `mutool verify` — 188 manifest entries | 42–49 ms | 17% over 3 runs |

The verify spread is larger because the working set is small enough that page
cache state dominates. That is stated rather than hidden.

### 3.4 Historical CRC-32 regression/fix record

This historical cycle-1 CRC-32 record documents a slice-by-1 to slice-by-8
before/after pair measured in the same harness at that time:

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

## 6. Historical optimization snapshots

The cycle-13/14 blocks below preserve the original measurements and their units
as recorded at the time. Their legacy `MB` / `MB/s` labels use 1,048,576 bytes;
read them as MiB / MiB/s. New benchmark output uses the correct `MiB` labels.

### Cycle-13 optimizations — OPT-003/004/005

Methodology same as §2, g++ 12.2.0 -O2, Linux x86_64, 2000 reps OZT 200 reps BuxCrypt 200 reps OzjProbe, buffer sizes 0.25 MB 256x256 1 MiB BuxCrypt 0.19 MB OzjProbe, evidence `docs/evidence/ciclo13_bench_after.log` full output with median p95 p99 mean MB MB/s.

```
Before (ciclo11 baseline):
OztToRgbaTopDown 256x256  med 62.4 us  p95 97.2 us  p99 138.6 us  mean 68.7 us  0.25 MB  4003.6 MB/s
BuxCrypt 1 MiB            med 1135.7 us  p95 1189.3 us  p99 1200.2 us  mean 1146.5 us  1.00 MB  880.5 MB/s
OzjProbe 512x512+200KB     med 179.2 us  p95 237.0 us  p99 272.7 us  mean 187.1 us  0.19 MB  1064.9 MB/s

After (ciclo13 item10, after OPT-003/004/005):
OztToRgbaTopDown 256x256 (optimized 2px bit ops)  med 63.0 us  p95 77.9 us  p99 89.7 us  mean 65.1 us  0.25 MB  3969.0 MB/s  ~1x median p99 1.54× tail reduction
OztToBgraTopDown 256x256 (GL_BGRA direct CAND-001)  med 6.6 us  p95 7.2 us  p99 11.3 us  mean 6.8 us  0.25 MB  38057.5 MB/s  9.45× median 12.26× p99 vs old 62.4/138.6, throughput 4003→38057 9.5×
BuxCrypt 1 MiB  med 384.8 us  p95 417.3 us  p99 435.9 us  mean 393.1 us  1.00 MB  2598.9 MB/s  2.96× vs original 1135.7/880.5, 3.9× peak OPT-003 290.9us 3437 MB/s
OzjProbe 512x512+200KB  med 22.9 us  p95 33.5 us  p99 36.4 us  mean 24.2 us  0.19 MB  8342.2 MB/s  7.8× vs original 179.2/1064.9, 11.79× median 8.47× p99 peak OPT-004 15.2us
```

Full log: `docs/evidence/ciclo13_bench_after.log` 474 checks 0 failures test-san full after `ciclo13_test_san_full_after.log`.

Decision: Keep OPT-003 BuxCrypt word-at-a-time 8B key64[3] phase, OPT-004 OzjProbe word-at-a-time 8B has_zero bitmask, OPT-005 OztToBgraTopDown GL_BGRA direct memcpy flip only 10.2×.


### Cycle-14 — BuxCrypt N=10 peak-vs-median reconciliation

**Context:** an earlier cycle-10 report highlighted the single-run 3.9× peak (290.9 µs, 3,437 MB/s) as if it were representative. Cycle 14 reran the benchmark N=10 to establish a median and dispersion; the older figure remains marked as an outlier below.

**Metodologia N=10:** `make bench && for i in 1..10; do ./build/bench_assets; done` extrai BuxCrypt 1 MiB med us e MB/s, g++ 12.2.0 -O2 Linux x86_64, 200 reps por execução, buffer 1 MiB, warm-up antes timing, evidência `docs/evidence/ciclo14_buxcrypt_10runs.log`.

**Resultados N=10:**

```
run 1: med 388.6 us p95 419.6 p99 453.7 mean 393.4 MB/s 2573.2
run 2: med 397.6 us p95 428.6 p99 495.9 mean 395.7 MB/s 2515.0
run 3: med 389.8 us p95 412.7 p99 417.3 mean 391.3 MB/s 2565.5
run 4: med 397.9 us p95 435.9 p99 457.9 mean 398.0 MB/s 2513.2
run 5: med 398.3 us p95 425.7 p99 441.0 mean 394.5 MB/s 2510.7
run 6: med 391.0 us p95 420.4 p99 430.9 mean 394.1 MB/s 2557.8
run 7: med 393.5 us p95 411.2 p99 423.6 mean 391.2 MB/s 2541.2
run 8: med 390.4 us p95 413.8 p99 432.0 mean 392.7 MB/s 2561.7
run 9: med 391.6 us p95 419.2 p99 438.2 mean 393.2 MB/s 2553.8
run 10: med 383.7 us p95 413.7 p99 429.4 mean 391.7 MB/s 2606.0

median MB/s: 2555.8
mean MB/s: 2549.81
stdev MB/s: 30.42
min MB/s: 2510.7
max MB/s: 2606.0
median med_us: 391.3
mean med_us: 392.24
stdev med_us: 4.67
min med_us: 383.7
max med_us: 398.3
```

**Interpretação:**
- Antes ciclo11 baseline: 1135.7 us 880.5 MB/s
- Depois ciclo14 N=10 mediana: 391.3 us 2555.8 MB/s → speedup 2.90× (1135.7/391.3) ou 2.90× MB/s (2555.8/880.5)
- Pico anterior 3437 MB/s (290.9us) citado em ciclo10 `ciclo10_opt002_bux_benchmark.log` foi **outlier** medido em máquina diferente / condição cache quente / compilação anterior, não reproduzível neste ambiente: max observado N=10 é 2606.0 MB/s, 24% abaixo de 3437. Portanto 3437 MB/s **não é representativo**, é outlier e não deve ser usado como headline.
- Mediana 2555.8 MB/s com desvio padrão 30.42 MB/s (1.2%) é número de referência correto para documentação futura.
- Ainda >2× vs original, mantém decisão Keep OPT-003.

**Decisão:** Usar mediana N=10 2555.8 MB/s como referência futura, mencionar 3437 MB/s explicitamente como outlier não reproduzível neste ambiente, não como ganho típico. Atualizar toda documentação futura para mediana, não pico.

