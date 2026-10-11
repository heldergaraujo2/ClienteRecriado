# Decisions

Each entry records the decision, the alternatives considered, the evidence, and
what would make it worth revisiting.

---

## D-001 — Build an independent asset layer rather than patching the client

**Decision.** Read the original assets with new code; do not modify the client.

**Alternatives.**
- Patch or unpack the existing client. Rejected: `Main.exe` is VMProtect-protected
  and the render loop is inside it. Out of scope by policy and by practicality.
- Wrap the existing executable with a launcher. Rejected: it would not improve
  anything that matters, and presenting a wrapper as a reconstruction would be
  dishonest.
- Replace individual assets with optimised versions. Considered and deferred:
  promising, but it requires the formats to be readable first, which is what
  this decision delivers.

**Evidence.** `objdump -h Main.exe` shows `.vmp0` and `.vmp1` sections with
`.text`/`.rdata`/`.data` at zero on-disk size. Meanwhile no asset format in the
tree is protected, and 99.31% of OZJ, 98.10% of OZT, 99.76% of BMD and 100% of
terrain and TGA files now parse.

**Revisit when.** Never, unless the protection situation changes fundamentally.

---

## D-002 — C++17 with zero third-party dependencies

**Decision.** The library builds with a compiler and nothing else.

**Alternatives.**
- C. Rejected: `std::vector`, `std::string` and RAII remove a whole class of
  memory bug in code whose entire job is parsing hostile files.
- Rust. Considered seriously and rejected for this project: the Windows build
  path cannot be tested here, and adding a toolchain that has never been run on
  the target platform is a bigger risk than the language's memory guarantees
  offset.
- C++ with vcpkg/Conan. Rejected: a manifest that has never been executed on
  Windows is a way to ship a broken build.

**Evidence.** The build succeeds in a container with no `GL/gl.h`, no `zlib.h`,
no `libpng`, no `libjpeg`, no SDL and no package registry access.

**Cost.** No JPEG pixel decoder. Recorded as a gap in
[`DEPENDENCIES.md`](DEPENDENCIES.md).

**Revisit when.** A JPEG decoder is needed, at which point `stb_image.h` behind
a pluggable interface is the recommended addition — one header, public domain,
no build integration.

---

## D-003 — Plain `make` instead of CMake

**Decision.** A 60-line Makefile.

**Alternatives.** CMake, Meson, Bazel. All rejected because none is installed in
the development environment, and installing one means the build depends on a
network fetch that cannot be verified on Windows.

**Evidence.** `cmake` and `meson` are both absent from the environment. `make`
4.3 and `g++` 12.2 are present.

**Cost.** Less portable across exotic toolchains. Acceptable: the target is
Windows plus Linux development, both well served.

**Revisit when.** A Visual Studio solution or an IDE integration is needed. A
`CMakeLists.txt` can be added *alongside* the Makefile rather than replacing it.

---

## D-004 — Custom 40-line test harness instead of a framework

**Decision.** No test framework.

**Alternatives.** GoogleTest, Catch2, doctest. All require a download.

**Rationale.** The dependency-free property is worth more than parameterised
tests and matchers. The harness is `Check`, `CHECK`, `CHECK_EQ` and a table of
function pointers — 40 lines that do what is needed.

**Cost.** No test filtering, no XML output, no parameterised cases. Mitigated by
printing `[ RUN ]` / `[ OK ]` / `[ FAIL ]` per test, which is enough to see what
broke.

**Revisit when.** The suite passes a few hundred tests and filtering becomes a
genuine time cost.

---

## D-005 — Accept unusual OZT trailers rather than rejecting them

**Decision.** The OZT loader accepts any trailer up to 4096 bytes and reports
its length, rather than requiring 0 or 26.

**Evidence.** Measured over 400 files: trailer 26 bytes in 362, 0 bytes in 22,
30 bytes in 12, 34 bytes in 1. Strict rejection of anything but 0 or 26 would
fail roughly 3% of the asset base.

**Rationale.** The pixel payload is fully determined by the header, so trailing
metadata cannot corrupt the image. Compatibility with the existing asset base is
a stated project priority, and there was nothing to gain from strictness here.

**Guard.** A trailer above 4096 bytes is still rejected, so a corrupt length
field cannot pass as data.

**Revisit when.** If a trailer turns out to carry meaningful data — a mipmap
chain, say — it should be parsed rather than skipped.

---

## D-006 — Report the OZT `00 4B 6D 4B` variant as unsupported rather than guessing

**Decision.** 43 variant files are recognised and returned as `kUnsupported`
with a message naming the marker.

**Alternatives.** Pick a pixel offset that makes the file length arithmetic
close.

**Why not.** Three measurements on those files cannot all be satisfied by one
layout: image type reads 0 rather than 2, dimensions sit 4 bytes later than the
standard form, and the file is exactly 4 bytes shorter than header-plus-payload.
An offset that closes the arithmetic would overlap the bits-per-pixel field and
produce silently wrong pixels. A texture that loads and displays the wrong image
is worse than one that reports a clear error.

**Evidence.** `Rank01.OZT` is 24,358 bytes; 26 + 78*78*4 = 26,362.

**Revisit when.** The variant's layout is established from a second source —
another file that documents it, or the tool that produced these files.

---

## D-007 — Auto-detect bux encryption by measured gain, not by magic bytes

**Decision.** `BuxDecryptGain` measures how much decrypting raises the zero-byte
fraction; a gain above 0.05 means the file is bux-encrypted.

**Evidence.** `Data/gate.bmd` is bux-encrypted but contains no `BMD` magic once
decrypted — it opens with four repetitions of the key, i.e. twelve plaintext
zero bytes, and decrypts to a data blob. 100 such files exist. Keying detection
on the magic alone would have classified all of them as unknown.

**Guard.** The reverse direction is tested too: high-entropy noise shows no gain
and must not be classified as bux.

**Revisit when.** A bux-encrypted file is found whose plaintext has no zero
padding. None has been seen.

---

## D-008 — Require a decisive gain before applying bux to terrain

**Decision.** `TerrainTryBuxDecrypt` only accepts a decryption that improves
small-byte density by more than 0.10.

**Evidence.** Decrypting 60 `EncTerrain*` files with the bux key improved the
density of exactly 1. The `Enc*` family uses a different cipher, and applying
bux to it would silently corrupt data that a future loader might otherwise
handle correctly.

**Rationale.** A cipher applied to data it does not match still "succeeds" — XOR
always produces output. The only protection against silently corrupting a file
is to check that the result looks more like the expected plaintext.

---

## D-009 — `mu::Status` instead of exceptions or booleans

**Decision.** Every reader returns a status code plus a message.

**Rationale.** A boolean cannot distinguish "not an OZT" from "a truncated
OZT", and that distinction is the difference between a correct inventory and a
wrong one. Exceptions would be reasonable, but they add cost to a hot path that
runs over 29,000 files, and they make the "keep going past one bad file"
behaviour awkward.

**Consequence that matters.** Messages are written to be actionable. `mutool
info` prints all four readers' messages for an unrecognised file, which turns
triage from guesswork into reading.

---

## D-010 — Header-only probing on the inventory path

**Decision.** Separate `Probe` (validate, no copy) from `Read` (copy pixels).
Inventories use `Probe` only.

**Evidence.** `OztProbe` 0.0 µs median against `OztRead` 6.0 µs and
`OztToRgbaTopDown` 62.4 µs. `mutool scan` over 29,060 files and 2,372 MB takes
1.67 s.

**Consequence.** Whole-tree validation is cheap enough to run on every change,
which is what caught the OZJ defect that sampling missed.

---

## D-011 — No original asset bytes in the repository

**Decision.** Fixtures are generated at run time.

**Rationale.** Licensing, first: the assets are third-party intellectual
property. Practicality, second: the suite runs anywhere with no setup and no
large files in Git.

**Cost.** Real-asset validation needs the reference client present. Handled by
making it a separate, documented step (`mutool scan`) rather than part of
`make test`.

---

## D-012 — Record `NOT EXECUTED` explicitly rather than omitting

**Decision.** Every unrun test gets a line saying so, with its blocker.

**Rationale.** An absent line reads as "not relevant". In this project a great
many things are "not yet possible" — no Windows, no GPU, no server — and the
difference between those two states is the difference between an honest status
report and a misleading one.
