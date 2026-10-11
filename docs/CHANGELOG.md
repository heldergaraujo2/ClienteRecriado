# Changelog

Performance changes with measurements are in
[`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md). This file covers everything else.

## Unreleased — initial reconstruction work

### Added

**Asset format library** (`src/mu/assets/`), C++17, no third-party dependencies:

- `format` — `mu::Status`, `mu::ByteView`, little- and big-endian accessors,
  CRC-32, file reading.
- `ozt` — OZT probe, read, write and BGRA→RGBA conversion.
- `ozj` — OZJ container handling and JPEG marker parsing.
- `tga` — TGA probe and uncompressed read.
- `bmd` — BMD container classification, the bux cipher, byte entropy.
- `terrain` — terrain file classification and bux auto-detection.
- `manifest` — patch manifest parsing and CRC verification.

**Support** — `src/mu/fs`, a recursive directory walker with POSIX and Win32
implementations.

**Tooling** — `tools/mutool` with `scan`, `info`, `verify` and `dedupe`.
Exit status distinguishes clean (0), usage error (1) and problems found (2), so
CI can tell "you invoked me wrong" from "verification failed".

**Tests** — `tests/test_assets.cpp`, 231 checks, self-generating fixtures, no
framework dependency.

**Benchmarks** — `bench/bench_assets.cpp` with median and P95/P99 reporting.

**Build** — `Makefile` with generated header dependencies and a strict warning
set.

**Windows validation** — `scripts/validate_windows.ps1` and
`scripts/benchmark_windows.ps1`. Written and reviewed, **never executed**.

**Documentation** — `AGENTS.md`, `README.md`, and 18 documents under `docs/`.

### Formats resolved

- **OZJ** — a 24-byte duplicate of the JPEG's own leading bytes, then the full
  JPEG. Verified on 200 of 200 sampled files.
- **OZT** — 4-byte signature, 18-byte TGA header, `w*h*4` BGRA payload, variable
  trailer. Verified on 400 sampled files and 3,404 of 3,470 real files.
- **Bux cipher** — repeating 3-byte XOR `{0xFC, 0xCF, 0xAB}`, confirmed by
  decrypting an encrypted terrain file into a byte-for-byte match with a
  plaintext one shipped alongside it.
- **BMD container** — `"BMD"` plus version; v0x0C carries a `u32` payload length
  at offset 4, v0x0A does not.
- **Terrain** — every map is a 256x256 lattice; 1 or 2 bytes per tile for
  attributes, 3 for maps, with a 1–4 byte header.

### Fixed during development

Each of these was a real defect found by a test or by real data, not a
refinement.

- **JPEG segment lengths were read little-endian.** JPEG is big-endian
  throughout, so a DQT length of `00 43` (67) was read as 17152 and valid files
  were reported as truncated. Added `ReadU16BE` / `ReadU32BE`.
- **The JPEG parser could not cross entropy-coded data.** Scan segments contain
  arbitrary bytes including `0x00` and `0xFF`. Added marker scanning that
  honours the `FF 00` stuffing rule and `FF D0..D7` restart markers.
- **OZJ's duplicated header prefix was not handled.** Caused 13,927 of 13,928
  real files to fail. Added `OzjFindImageStart`.
- **Strict OZT trailer validation rejected about 3% of the asset base.**
  Trailers of 0, 26, 30 and 34 bytes all occur. Now accepted up to 4096 bytes
  and reported via `trailer_size`.
- **Bux detection keyed on the `"BMD"` magic**, which missed 100 bux-encrypted
  files that are not models. Now based on `BuxDecryptGain`.
- **`.tga` files were routed to the terrain parser** and misclassified as object
  lists. Added a real TGA reader.
- **The Makefile did not track header dependencies.** Adding a field to
  `OztImage` left `test_assets.o` compiled against the old layout linked against
  a library built with the new one — an ODR violation presenting as a segfault
  far from its cause. Fixed with `-MMD -MP`.
- **`mutool info` printed only "unrecognised"**, giving no way to diagnose a
  file. It now reports why each reader declined.

### Measured findings about the reference client

- Renderer is fixed-function OpenGL 1.1: immediate mode in `Main.exe`,
  client-side vertex arrays and display lists in `Main.dll`. No VBOs, no FBOs,
  no `glDrawElements`, no explicit vsync control.
- `Main.exe` is VMProtect-protected; the render loop is inside it.
- Asset set requires 2,893.80 MB of uncompressed texture memory.
- The client does not match its own patch manifest: 116 of 188 entries match,
  with 9 size mismatches, 11 CRC mismatches and 52 missing files.
- Plaintext credentials are committed in `Settings.ini`. See
  [`SECURITY.md`](SECURITY.md).
- 12 `Thumbs.db` files (677,376 bytes) are shipped in the asset tree.

## Ciclo 2 (em andamento)

### Quebrado
- **Cifra BMD v0x0C** — XOR rolante sobre tabela de 16 B com chave corrente
  derivada do texto cifrado anterior. Nível 2 (payload em claro) de 1.496
  (14,83%) para **8.571 (84,95%)**; nível 3 (geometria parseada) de 1.191
  (11,80%) para **8.406 (83,31%)**. OPAQUE de 8.594 para 1.519. Proveniência:
  `xulek/muonline-bmd-viewer` (ISC), reimplementado; ver DEC-007.
- **Layout do corpo BMD** — campos `meshCount/boneCount/actionCount` em
  32/34/36 e estrutura por malha (`numVertices/numNormals/numTexCoords/`
  `numTriangles/textureIndex`). Registros de vértice (16 B), normal (20 B),
  triângulo (64 B) e osso (24+64n B) conhecidos.

### Corrigido
- A hipótese "(u,v) rejeitada" estava errada: o controle usado subestimava a
  linha de base porque coordenadas de textura reais não são uniformes.
- A rota "implementação open-source do formato" foi registrada em
  `NEXT_TASK.md` e em dois commits como bloqueada por falta de acesso de saída a
  um host de código. **Isso era falso**: `github.com`, `codeload.github.com` e
  `api.github.com` são acessíveis. A afirmação foi corrigida.

### Testes
- 236 checks, 0 failures (eram 231). ASan+UBSan: 236/0.
