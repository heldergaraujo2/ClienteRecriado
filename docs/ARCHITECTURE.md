# Architecture

## 1. Design constraints

Four constraints determined this architecture. They come from the audit, not
from preference.

1. **The engine cannot be modified.** `Main.exe` is VMProtect-protected and the
   render loop is inside it. A patch-based optimisation strategy is impossible.
2. **The assets can be read.** No asset format in the tree is protected. This is
   the only route to visual parity with the original.
3. **Windows is the target, but development happens on Linux** with no GPU and
   no Windows machine. Everything must be testable headlessly, and the Windows
   build path must be verified by someone who has the machine.
4. **No original asset bytes may be committed.** Test fixtures must be generated
   by the code itself.

## 2. Layering

```
+-------------------------------------------------------------+
|  tools/mutool          command line inspection and reporting |
+-------------------------------------------------------------+
|  bench/bench_assets    micro-benchmarks                      |
|  tests/test_assets     unit tests, self-generating fixtures  |
+-------------------------------------------------------------+
|  src/mu/assets         format readers (OZT, OZJ, TGA, BMD,   |
|                        terrain, manifest) + CRC-32           |
|  src/mu/fs             portable directory walking            |
+-------------------------------------------------------------+
             (future) renderer, resource manager, engine
+-------------------------------------------------------------+
```

The dependency direction is strictly downward. `src/mu/assets` knows nothing
about `tools/`. Nothing in the library depends on OpenGL, Windows or any
third-party library, which is what makes it testable here and portable there.

## 3. Core conventions

**`mu::Status` instead of exceptions or booleans.**

```cpp
struct Status {
  enum Code { kOk, kNotThisFormat, kTruncated, kCorrupt, kUnsupported, kIoError };
  Code code;
  std::string message;
};
```

A boolean cannot distinguish "this is not an OZT" from "this is a truncated
OZT", and that distinction is the whole difference between a correct inventory
and a wrong one. Every message is specific enough to diagnose a file on its
own — `"OZT variant with a 00 4B 6D 4B marker: layout not resolved"` rather
than `"parse failed"`. `mutool info` prints all four readers' messages for an
unrecognised file, which is what makes triage fast.

**`mu::ByteView` instead of owned buffers.**

```cpp
struct ByteView { const uint8_t* data; std::size_t size; };
```

Non-owning, so a caller can memory-map a file instead of copying it. On a
2,372 MB asset tree the difference between copying and mapping is the
difference between a usable tool and an unusable one. `sub()` clamps instead of
trusting its arguments.

**Explicit endianness accessors.** `ReadU16LE`, `ReadU32LE`, `ReadU16BE`,
`ReadU32BE`. Every format in this client is little-endian except JPEG, which is
big-endian throughout. Mixing them is quiet and catastrophic: reading a JPEG
segment length as little-endian turns `00 43` into 17152 and reports a valid
file as truncated. That bug existed here and was caught only by a test.

**Readers never throw and never abort.** Every input is an untrusted file that
may be truncated, corrupt, or hostile. No reader indexes a buffer before
checking its length, and no reader trusts a size field read from the file
without comparing it against the bytes actually available.

## 4. Why no third-party dependencies

The library builds with a C++17 compiler and nothing else. That decision costs
a JPEG pixel decoder, which is the main thing a dependency would have provided.
It buys:

- A Windows build that needs no package manager, no vcpkg, no vendored tarball
  and no network access. This matters because the Windows path is the one that
  cannot be tested here, so it must be as simple as possible.
- No licence obligations to track for the code itself. See
  [`LICENSING.md`](LICENSING.md).
- Reproducible builds. `make` on any machine with a C++17 compiler produces the
  same result.

The JPEG decoder is a deliberate gap rather than an oversight. When it is added
it should be a pluggable backend so the choice of decoder stays reversible — see
[`DEPENDENCIES.md`](DEPENDENCIES.md).

## 5. Build system

Plain `make`, 60 lines, with generated header dependencies:

```make
DEPFLAGS := -MMD -MP
-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)
```

The header dependency tracking is not optional decoration. Without it, editing
`ozt.hpp` leaves `test_assets.o` compiled against the old `OztImage` layout
linked against a `libmuassets.a` built with the new one. That is an ODR
violation, and it presents as a segfault with no connection to the edit that
caused it. **This has already happened once in this repository.** The `-include`
is prefixed with `-` so a fresh checkout with no `.d` files still builds.

Warnings are errors in practice: `-Wall -Wextra -Wpedantic -Wshadow
-Wconversion -Wsign-conversion -Wold-style-cast -Wuseless-cast
-Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wdouble-promotion
-Wformat=2`, and the build is kept clean. Do not narrow the flags to silence a
warning.

## 6. Test architecture

A ~40-line harness inside `tests/test_assets.cpp`. No framework is downloaded,
which keeps the "compiler and nothing else" property intact.

The important property is **self-generating fixtures**. `OztWriteRgba` writes
valid OZT files, so the OZT reader is tested by round-tripping through the
writer. `MakeFakeJpeg` builds a structurally valid JPEG marker chain. Terrain
and bux fixtures are synthesised from the known key.

This means the test suite is complete without a single original asset byte in
the repository, and it stays complete when the reference client is not
available.

Tests are written against **behaviour that real data exposed**, not against
idealised inputs. Three examples:

- `TestOzjDuplicatedHeaderPrefix` exists because 13,927 real files failed.
- `TestOztVariantWithExtraPrefix` exists because 43 real files carry a marker
  the standard layout does not describe.
- `TestBmdProbe` covers a bux-encrypted non-model because `Data/gate.bmd` is
  one.

## 7. Modules not yet built

The layering anticipates these. None exists yet, and this list is a plan, not a
claim.

| Module | Purpose | Status |
|---|---|---|
| `mu/gfx` | Renderer backend abstraction | NOT STARTED |
| `mu/gfx/gl` | OpenGL backend | NOT STARTED |
| `mu/gfx/null` | Headless backend for render tests | NOT STARTED |
| `mu/res` | Resource cache, lifetime, budgeting | NOT STARTED |
| `mu/world` | Terrain, maps, objects | NOT STARTED |
| `mu/anim` | Skeletal animation | BLOCKED on the BMD payload cipher |
| `mu/net` | Protocol | NOT STARTED |
| `mu/audio` | Ogg Vorbis playback | NOT STARTED |
| `mu/app` | Window, input, main loop | NOT STARTED |

`mu/anim` is blocked because BMD payloads cannot be decrypted, so there is no
vertex or bone data to animate. That is a hard dependency, recorded in
[`TEST_RESULTS.md`](TEST_RESULTS.md).

## 8. Decisions that shaped this

Recorded in full, with alternatives considered, in
[`DECISIONS.md`](DECISIONS.md).
