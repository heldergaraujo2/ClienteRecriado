# Current state

An honest snapshot. What exists, what works, what does not.

## 1. What exists and works

**An asset format library**, C++17, zero dependencies, warning-clean:

| Module | What it does | Verified against |
|---|---|---|
| `mu/assets/ozt` | OZT read, write, probe, RGBA conversion | 3,404 of 3,470 real files |
| `mu/assets/ozj` | OZJ container + JPEG marker parsing | 13,832 of 13,928 real files |
| `mu/assets/tga` | TGA probe and uncompressed read | 15 of 15 real files |
| `mu/assets/bmd` | BMD container classification, bux cipher, entropy | 10,066 of 10,090 real files |
| `mu/assets/terrain` | Terrain classification, bux auto-detect | 265 of 265 real files |
| `mu/assets/manifest` | Patch manifest parse and CRC verify | the client's own 188-entry manifest |
| `mu/assets/format` | CRC-32 (slice-by-8), status, byte views | zlib-derived known answers |
| `mu/fs` | Recursive walker, POSIX + Win32 | the 29,060-file real tree |

**A command line tool**, `build/mutool`, with `scan`, `info`, `verify`,
`dedupe`. Whole-tree scan of 2,372 MB in 1.67 s.

**A test suite**, 171 checks, 0 failures, self-generating fixtures, also clean
under ASan and UBSan.

**A micro-benchmark harness** with median and P95/P99 reporting.

**Documentation**: audit, formats, rendering analysis, architecture, baselines,
benchmarks, decisions, licensing, security, test results.

**Windows validation scripts** in `scripts/`, written but never run.

## 2. Measured numbers

All from the real reference tree — 29,060 files, 2,372.04 MB.

```
OZJ   13,928 files -> 13,832 parsed  (99.31%)
OZT    3,470 files ->  3,404 parsed  (98.10%)
BMD   10,090 files -> 10,066 classified (99.76%)
TGA       15 files ->     15 parsed  (100%)
terrain    265 files ->    265 parsed (100%)

Estimated uncompressed texture memory: 2,893.80 MB
Junk: 12 Thumbs.db, 677,376 bytes

Integrity vs ListaPatch.txt (188 entries):
  match 116   size mismatch 9   crc mismatch 11   missing 52

mutool scan  29,060 files : 1,641–1,674 ms
mutool verify 188 entries : 42–49 ms
make test                 : 171 checks, 0 failures
```

## 3. What does not exist

There is **no renderer, no window, no game loop, no engine**. Nothing can be
displayed. This project has so far produced a measurement and asset-loading
layer, not a client.

| Component | Status |
|---|---|
| Renderer (any backend) | NOT STARTED |
| Window / input / main loop | NOT STARTED |
| Resource cache and manager | NOT STARTED |
| Texture upload to GPU | NOT STARTED |
| Model loading | BLOCKED — BMD cipher |
| Skeletal animation | BLOCKED — BMD cipher |
| Terrain rendering | BLOCKED — `Enc*` cipher |
| Particles and effects | NOT STARTED |
| UI | NOT STARTED |
| Text rendering | NOT STARTED |
| Audio | NOT STARTED |
| Network | NOT STARTED, no authorised server |
| JPEG pixel decoding | NOT STARTED — deliberate, no dependency |
| Windows build | NOT EXECUTED — never compiled for the target platform |

## 4. Optimisations landed

One, with a measured before/after:

| ID | Change | Before | After |
|---|---|---|---|
| OPT-001 | CRC-32 slice-by-1 → slice-by-8 | 391.6 MB/s | 1,971.6 MB/s (5.03x) |

Three more are identified with evidence but not implemented: direct BGRA upload
(CAND-001), vectorised bux cipher (CAND-002), word-at-a-time JPEG scan search
(CAND-003). See [`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md).

## 5. Runtime performance

**No FPS, frametime, VRAM or latency numbers exist.** There is no Windows
machine and no GPU in the development environment. Every runtime metric is
`NOT EXECUTED`. The harness to produce them is written and unrun.

This is stated prominently because it is the most common thing a reader would
assume exists. It does not.

## 6. Known defects and gaps

| Item | Severity | Note |
|---|---|---|
| BMD payload cipher not recovered | High | Blocks all geometry and animation |
| `Enc*` terrain cipher not recovered | High | Blocks 219 of 264 terrain files |
| No Windows build | High | Target platform never compiled |
| No JPEG pixel decoding | Medium | Blocks texture display for the largest format |
| 96 OZJ files unread (`FF 4B 6D` variant) | Low | Distinct container, unexamined |
| 43 OZT files unread (`00 4B 6D 4B` variant) | Low | Layout inconsistent, deliberately not guessed |
| 24 BMD files unrecognised | Low | 0.24% of models |
| `.ozb` format not implemented | Medium | 100 files, height/light maps |
| `mutool` has no `extract` or `convert` subcommand | Low | Useful, not built |

## 7. Repository state

- Branch: `arena/519f8fc9-clienterecriado`
- Base: `972fabcea1b0d4e80345f0f5abd287b46d819455` on `main`
- The repository contained a single `README.md` before this work started.
- No original asset bytes are committed. No secrets are committed.

## 8. Honest summary

The measurement and asset layer is real, tested and verified against a 2.4 GB
asset tree. The reconstruction of the client itself has not begun, and the
biggest blocker — the BMD model cipher — is a hard dependency for anything
visual. Anyone reading this should understand that "99% of assets parse" and
"you can play the game" are very far apart, and the second is not close.
