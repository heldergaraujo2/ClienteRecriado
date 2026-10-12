# Performance baseline

This document records what was actually measured, and — equally important —
what could not be measured and why. No number here is an estimate presented as
a measurement.

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
Asset tree /home/user/Cliente_ref/Data — 29,060 files, 2,372.04 MB
```

No GPU. No Windows. The CPU numbers below are valid for this machine and are
useful as *relative* measurements between code versions, which is how they are
used in [`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md).

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

### 2.2 Asset tree composition — the real bottleneck candidate

From `build/mutool scan`:

```
files: 29060   total: 2372.04 MB
junk (Thumbs.db): 12 files, 0.65 MB
estimated uncompressed texture memory: 2893.80 MB
```

The texture memory figure is the sum of `width * height * 4` for every texture
whose header parses:

| Format | Files | Decoded bytes |
|---|---|---|
| OZJ | 13,832 | 2,602,283,596 |
| OZT | 3,404 | 424,237,332 |
| TGA | 15 | 7,847,772 |
| **Total** | | **3,034,368,700 (2,893.80 MB)** |

**Classification: theoretical maximum footprint estimate — NOT a measured
bottleneck.** This number is the arithmetic sum of `width * height * 4` over
every texture whose header parses. Nothing was executed, no process was
observed, and no GPU memory was measured; there is no Windows machine or GPU in
the development environment. It bounds the worst case if every texture were
resident at once. It is **not** a measurement of runtime memory use, and it must
never be quoted as one. What would turn it into a measurement is a VRAM counter
on a real run — see [`BENCHMARKS.md`](BENCHMARKS.md) section 4.

With that qualification stated, the figure is still the most consequential
*derived* number in the project, because it bounds a real constraint. The
client's texture set would require 2.83 GB of uncompressed RGBA if everything
were resident. For context:

- A GeForce 4 Ti (2002, the era of this renderer) had 64–128 MB.
- An Intel HD 620 integrated GPU typically has 512 MB to 1 GB shared.
- Even a modern mid-range card with 8 GB would be spending over a third of its
  VRAM on MU textures alone.

The client obviously does not hold everything at once — no client does — but the
figure bounds the problem: **texture residency management is the dominant memory
constraint**, not geometry, not effects, not UI. Any reconstruction must treat
texture streaming and eviction as a first-class requirement rather than an
optimisation added later.

Two supporting measurements:

- 561 OZJ textures are larger than 256x256.
- OZT textures are 32 bits per pixel in 400 of 400 sampled files. There is no
  16-bit or compressed variant anywhere in the tree, so there is no existing
  reduced-precision path to fall back on.

Neither figure is a claim that the original client is slow, and neither is a
runtime measurement. They are bounds derived from the asset files themselves,
and they hold regardless of the renderer. Every runtime metric remains
`NOT EXECUTED`.

### 2.3 Format parsing cost

From `build/bench_assets`. Median of many repetitions, with tail percentiles,
because for load-time work the tail is what a player feels as a hitch.

```
benchmark                            med us   p95 us   p99 us  mean us         MB      MB/s
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

Readings worth acting on:

- **`OztProbe` is effectively free.** It touches 22 bytes. This is why
  `mutool scan` can classify 29,060 files in 1.67 s: the inventory path never
  copies pixel data. Header-only validation should stay on the hot path of any
  resource manager.
- **`OztToRgbaTopDown` is the expensive step in texture loading**, at 4,003 MB/s
  and with a p99 of 138.6 µs against a median of 62.4 µs — a 2.2x tail. The
  per-pixel BGRA-to-RGBA byte swap plus vertical flip is pure CPU work. For a
  1024x1024 texture that is roughly 1.1 ms of blocking work, and it happens on
  the thread that asked for the texture.

  Two concrete improvements follow, both testable:
  1. Upload BGRA directly with `GL_BGRA` and skip the swap entirely. Whether the
     driver accepts `GL_BGRA` depends on `GL_EXT_bgra`, which is universal on
     desktop GL. This removes the whole step.
  2. If a flip is still needed, do it once at load time off the render thread.
- **`BuxCrypt` at 880 MB/s is the slowest per-byte operation.** Decrypting a
  131 KB terrain attribute file costs about 150 µs. Across 265 terrain files
  that is roughly 40 ms of serial work at map load. Vectorising this is a
  straightforward win.
- **`OzjProbe` at 179 µs for a 200 KB scan segment** is dominated by the
  byte-at-a-time search for the terminating marker. Real JPEG scan data is
  larger, so this scales badly. A word-at-a-time search for `0xFF` would help.

### 2.4 Tool throughput

| Operation | Time | Notes |
|---|---|---|
| `mutool scan` over 29,060 files / 2,372 MB | 1,641–1,674 ms | reads every interpretable file |
| `mutool verify` of 188 manifest entries | 42–49 ms | reads and CRC-checks each listed file |

The scan figure is the practical bound on how fast a full asset audit can run,
and it is fast enough to put in CI.

## 3. Hypotheses ranked by expected impact

These are hypotheses, explicitly not findings. Each has a stated test.

| # | Hypothesis | Evidence so far | Test that would confirm |
|---|---|---|---|
| 1 | Texture residency is the dominant memory constraint | 2,893.80 MB **theoretical** uncompressed footprint (derived from headers, not measured at runtime) | Measure VRAM on Windows with a GPU counter while moving between maps |
| 2 | Immediate-mode submission caps the frame rate in crowded scenes | `glBegin`/`glEnd` in the render loop's import table | Count draw calls per frame on Windows, then compare against a batched renderer |
| 3 | `glReadPixels` causes a per-frame stall | imported by the render loop; use unverified | Time frames with and without the code path, once it is identified |
| 4 | `MaxAnisotropy = 32` costs measurable frame time | configuration value, present and enabled | Toggle to 1/4/16 in `Settings.ini`, measure frametime |
| 5 | Display lists are emulated on modern drivers and cost more than they save | `glGenLists`/`glCallLists` imported | Compare driver behaviour on a modern GPU against a VBO path |
| 6 | Texture conversion is a load-time hitch source | p99 138.6 µs at 256x256, measured | Load a new map and record frametime spikes |

Hypotheses 4 and 6 are the two that can be tested without touching protected
code, and they are the ones queued next.

## 4. What is needed for a real baseline

A Windows machine with a GPU and an authorised test server. The procedure is
written and ready: [`scripts/benchmark_windows.ps1`](../scripts/benchmark_windows.ps1).
It captures FPS, frametime percentiles, CPU, RAM, VRAM and load times for a
defined set of scenarios, and writes results in a format that can be diffed
between runs.

Until that is executed, every FPS claim in this repository is `NOT EXECUTED`.
