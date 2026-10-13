# Performance baseline

This document records what was actually measured, and — equally important —
what could not be measured and why. No number here is an estimate presented as
a measurement.

## Cycle-17 audit note

No Windows/GPU client run was available. FPS, frametime, runtime memory, input
latency, and GPU performance remain **NOT EXECUTED**. The current asset-size
sum is a header/payload-derived estimate, not decoded/allocated memory. CPU
microbenchmarks are not a client-performance baseline; see the cycle-17 result
in `BENCHMARKS.md` and `evidence/ciclo17_bench_final.log`.

## 1. What could not be measured

**There is no frame rate baseline.** The development environment is a Linux
container with no GPU, no window system and no Windows. The reference client is
a 32-bit Windows executable whose render loop is inside a VMProtect-protected
module. It cannot be run here, and running it is not something a claim should
rest on without a real execution.

Therefore, as of this writing:

| Metric | Status |
|---|---|
| FPS average / minimum / percentiles | `NOT EXECUTED` — no Windows, no GPU |
| Frametime median / P95 / P99 | `NOT EXECUTED` — same |
| Frametime spike frequency and duration | `NOT EXECUTED` — same |
| Startup time | `NOT EXECUTED` — same |
| Map load time | `NOT EXECUTED` — same |
| CPU usage and peaks | `NOT EXECUTED` — same |
| GPU usage and peaks | `NOT EXECUTED` — same |
| RAM and VRAM usage | `NOT EXECUTED` — same |
| Draw call and state change counts | `NOT EXECUTED` — requires runtime instrumentation of protected code |
| Input latency | `NOT EXECUTED` — requires dedicated hardware |

Fabricating any of these would be worse than leaving them blank. Instead this
project did two things: it measured everything that *can* be measured without a
GPU, and it built the harness that will produce the real numbers on a Windows
machine. See `scripts/` and [`BENCHMARKS.md`](BENCHMARKS.md).

## 2. What was measured

### 2.1 Environment

```
Host      Linux container, x86_64
Compiler  g++ 12.2.0 (Debian 12.2.0-14+deb12u14)
Build     -std=c++17 -O2
Clock     std::chrono::steady_clock
Asset tree /tmp/cliente_ref/Data (reference HEAD 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9) — 29,060 files, 2,372.04 MiB
```

The scan and benchmark use **MiB** (1,048,576 bytes), not decimal MB.

No GPU. No Windows. CPU readings below are machine- and run-specific. Only the
same-harness before/after pairs documented in [`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md)
support comparative claims; the cycle-17 snapshot alone does not establish a
speedup.

### 2.1a Submission-path measurement (DEC-008) — sample size

> **Amostra pequena (N=5), indicativa, não definitiva — sujeita a nova medição
> quando ambiente gráfico real estiver disponível.**

The DEC-008 submission-path numbers are the median of **5 complete runs of
`build/bench_assets`**. Five is enough to show that the spread between runs
(4.95×–6.18× on the load-time ratio) is not negligible, and not enough to call
any single figure settled. The amplitude between runs is reported alongside
every median in [`DECISIONS.md`](DECISIONS.md) DEC-008-A §3.2 and in
[`evidence/ciclo4_dec008_submission_cost.log`](evidence/ciclo4_dec008_submission_cost.log).

Two limits that matter more than the sample size:

- **Load-time cost is what was measured on CPU.** The per-frame cost is *not*
  measured here at all: it is driver marshalling, bus transfer and per-vertex
  call overhead, all of which need a GL context. What is stated per frame is a
  **volume** (an exact quantity derived from the absent imports), not a cost.
  See DEC-008-A §3.1 for why an earlier revision got this wrong.
- **Buffers are resident between repetitions**, which is the case *favourable*
  to the expanded path. Non-resident memory would widen the gap.

Re-measure with a larger N, on the target hardware, with the real visible-set
vertex count of a scene rather than the whole asset tree.

### 2.2 Asset-tree image-byte estimate — planning context

From `build/mutool scan`:

```
files: 29060   total: 2372.04 MiB
junk (Thumbs.db): 12 files, 0.65 MiB
header-derived image-byte total: 2893.80 MiB
```

The scan's image-byte total is a sum of unlike **header/payload-derived
estimates**: OZJ uses an RGBA8-equivalent based on probed dimensions, while OZT
and TGA use declared/calculated pixel payload bytes. The scan does not decode
OZJ pixels.

| Format | Probe-accepted files | Header/payload-derived estimate bytes |
|---|---|---|
| OZJ | 13,832 | 2,602,283,596 |
| OZT | 3,404 | 424,237,332 |
| TGA | 15 | 7,847,772 |
| **Total** | | **3,034,368,700 (2,893.80 MiB)** |

**Classification: theoretical image-byte estimate — NOT measured memory or a
measured bottleneck.** This is not a sum of decoded/allocated runtime buffers.
No process was observed and no GPU memory was measured; there is no Windows
machine or GPU in the development environment. The arithmetic total is a **header/payload-derived image-byte estimate** under
those format assumptions. It is neither a guaranteed upper nor lower bound on
decoded allocation: OZJ is estimated as RGBA8, while OZT/TGA use their declared
or calculated payload bytes, and runtime conversion/caching can change
residency. It is **not** a measurement of RAM, VRAM, or runtime residency and
must never be quoted as one. Runtime measurement requires a real run with a
memory counter; see [`BENCHMARKS.md`](BENCHMARKS.md) section 4.

The estimate is a **planning scale**, not a measured constraint or proof that
every texture is resident. Under the scan's assumptions, 3,034,368,700 bytes is
about 2.83 GiB (3.03 GB decimal). It cannot be compared with a GPU-memory budget
until actual decode formats, simultaneous residency, and eviction behavior are
measured. Texture residency deserves measurement and an explicit budget, but
the estimate does **not** demonstrate that residency is the dominant runtime
memory bottleneck. A resource cache/streaming policy remains an architectural
requirement to evaluate after a Windows/GPU baseline exists.

Two supporting measurements:

- 561 OZJ textures are larger than 256x256.
- The 400-file OZT sample measured 32 bits per pixel in all 400 files. This is
  sample evidence, not an exhaustive pixel decode. The current standard reader
  also supports uncompressed 24-bit BGR input; the scan probe does not decode
  every OZT pixel payload.

Neither figure is a claim that the original client is slow, and neither is a
runtime measurement. They are asset-tree size/sample observations, not bounds
on decoded memory; runtime allocation depends on conversion and residency.
Every runtime metric remains `NOT EXECUTED`.

### 2.3 CPU microbenchmark snapshot

One execution of `build/bench_assets`; each case uses the repetition count in
the harness, but this is **one process/run**, not a run-to-run variability
study. The raw record is [`evidence/ciclo17_bench_final.log`](evidence/ciclo17_bench_final.log).

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
LOAD-TIME expand indexed -> flat    19860.2  25045.4 25045.4  21299.2      67.21    3384.3
LOAD-TIME vertex buffer preparation   3399.9   3869.2   3905.9   3567.2      41.02   12064.6
```

The `OztProbe` time rounds to 0.0 µs, so its displayed throughput is not an
interpretable performance claim. The `OztToRgbaTopDown` case uses a synthetic
256×256 image; it does not measure a map load or game hitch. The BGRA case
preserves BGRA channels and reverses rows; it does not measure a GPU upload or
prove that a driver supports the intended format. The `LOAD-TIME` cases measure
CPU expansion/preparation on synthetic arrays only. The 67.21 MiB expanded and
52.22 MiB hypothetical indexed-buffer figures are data-volume calculations;
per-frame driver/GPU cost is **BLOCKED / NOT MEASURED**. No FPS or runtime
latency is inferred.

### 2.4 Tool throughput — single-run observations

| Operation | Wall time | Observed exit/status |
|---|---:|---|
| `mutool scan` — 29,060 files | 3.308 s | exit 0; warm filesystem cache |
| `mutool bmdreport` — 10,090 BMD files | 0.744 s | exit 2: 1,519 payloads remain OPAQUE |
| `mutool meshimport` — 10,090 BMD files | 0.760 s | exit 0 |
| `mutool verify` — 188 manifest entries | 0.036 s | exit 2: 116 match, 8 size mismatches, 11 CRC mismatches, 53 missing |

Recorded in [`evidence/ciclo17_tool_timings_final.log`](evidence/ciclo17_tool_timings_final.log). These are single-run observations from one environment, timed with Python `perf_counter` after prior full-tree reads; they are not repeatable throughput benchmarks. Filesystem cache state affects wall time, so no stable CI performance bound is claimed. The scan and CPU microbenchmark report counts/CPU work, not product performance.

## 3. Hypotheses ranked by expected impact

These are hypotheses, explicitly not findings. Each has a stated test.

| # | Hypothesis | Evidence so far | Test that would confirm |
|---|---|---|---|
| 1 | Texture residency is the dominant memory constraint | 2,893.80 MiB header/payload-derived image-byte estimate; not decoded allocation or a measured bound | Measure RAM/VRAM and residency on Windows with a GPU/process counter while moving between maps |
| 2 | Immediate-mode submission caps the frame rate in crowded scenes | `glBegin`/`glEnd` in the render loop's import table | Count draw calls per frame on Windows, then compare against a batched renderer |
| 3 | `glReadPixels` causes a per-frame stall | import-table presence alone does not prove the path is active | Identify the call site and compare timed frames with/without it on an authorized build |
| 4 | `MaxAnisotropy = 32` costs measurable frame time | configuration value, present and enabled | Toggle to 1/4/16 in `Settings.ini`, measure frametime |
| 5 | Display lists are emulated on modern drivers and cost more than they save | `glGenLists`/`glCallLists` imported | Compare driver behaviour on a modern GPU against a VBO path |
| 6 | Texture conversion is a load-time hitch source | latest single CPU microbenchmark run: synthetic 256×256 OZT RGBA conversion p99 80.4 µs; not an in-game load measurement | Measure map loading and frametime spikes on the reconstructed Windows/GPU path |

Hypotheses 4 and 6 are the two that can be tested without touching protected
code, and they are the ones queued next.

## 4. What is needed for a real baseline

A Windows machine with a GPU and an authorised test server. The procedure is
written and ready: [`scripts/benchmark_windows.ps1`](../scripts/benchmark_windows.ps1).
It captures FPS, frametime percentiles, CPU, RAM, VRAM and load times for a
defined set of scenarios, and writes results in a format that can be diffed
between runs.

Until that is executed, every FPS claim in this repository is `NOT EXECUTED`.
