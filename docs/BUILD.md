# Build

## 1. Prerequisites

| Requirement | Verified here | Notes |
|---|---|---|
| C++17 compiler | g++ 12.2.0 | clang++ or MSVC 2019+ should also work |
| GNU make | 4.3 | Any recent version |
| `ar` | binutils 2.40 | For the static library |
| Third-party libraries | **none** | No OpenGL, no zlib, no package manager |
| Network access | **not needed** | Nothing is downloaded |

Verified in a container with no `GL/gl.h`, no `zlib.h`, no `libpng`, no
`libjpeg`, no SDL and no access to a package registry. The build does not
notice.

## 2. Linux / macOS

```sh
make            # build/libmuassets.a, build/mutool, build/test_assets
make test       # run the unit tests (expect 0 failures)
make bench      # build/bench_assets
make clean
```

Override variables as needed:

```sh
make CXX=clang++ OPT=-O3
make CXX=g++-13 WARN="-Wall -Wextra"
```

## 3. Windows — MSVC

**Status: `NOT EXECUTED`.** There is no Windows machine in the development
environment, so this path has never been compiled. It is written to be correct
and reviewed, but unverified. Treat it as a starting point and record what
actually happens in [`TEST_RESULTS.md`](TEST_RESULTS.md).

From a *x86 Native Tools Command Prompt* (32-bit, to match the reference
client's target):

```bat
cl /nologo /std:c++17 /O2 /EHsc /W4 /permissive- /Isrc ^
   src\mu\assets\format.cpp src\mu\assets\ozt.cpp src\mu\assets\ozj.cpp ^
   src\mu\assets\tga.cpp src\mu\assets\bmd.cpp src\mu\assets\terrain.cpp ^
   src\mu\assets\manifest.cpp src\mu\fs.cpp tools\mutool.cpp ^
   /Fe:build\mutool.exe
```

Tests:

```bat
cl /nologo /std:c++17 /O2 /EHsc /W4 /permissive- /Isrc ^
   src\mu\assets\*.cpp src\mu\fs.cpp tests\test_assets.cpp ^
   /Fe:build\test_assets.exe
build\test_assets.exe
```

Portability notes for whoever runs this first:

- `src/mu/fs.cpp` has a `#if defined(_WIN32)` branch using `FindFirstFileA` /
  `FindNextFileA`. It has never been compiled.
- `tests/test_assets.cpp` guards `mkdtemp` and `rmdir` behind
  `#if !defined(_WIN32)`; the temp-directory test falls back to a fixed path on
  Windows and may need adjusting.
- No POSIX-only API is used anywhere else.

## 4. Windows — MinGW

**Status: `NOT EXECUTED`.** Also, no `mingw-w64` cross-compiler is available in
the development environment (`apt-get` requires root, which is not available),
so this could not even be smoke-tested.

```sh
x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -Wextra -Isrc \
  src/mu/assets/*.cpp src/mu/fs.cpp tools/mutool.cpp -o build/mutool.exe
```

Use `i686-w64-mingw32-g++` for a 32-bit build matching the reference client.

## 5. Verifying a build

```sh
make test
```

Expected: `171 checks, 0 failures`, and **no compiler warnings**. The warning
set is `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
-Wold-style-cast -Wuseless-cast -Wnon-virtual-dtor -Woverloaded-virtual
-Wcast-align -Wdouble-promotion -Wformat=2`. Do not narrow it to silence a
warning.

Then, if the reference client is available:

```sh
build/mutool scan   ../Cliente_ref/Data
build/mutool verify ../Cliente_ref/ListaPatch.txt ../Cliente_ref
```

Expected counts are in [`AGENTS.md`](../AGENTS.md). A drop in any `ok` count is
a regression.

## 6. What is not built

There is no client executable, no installer, no distribution package and no
artefact. There is nothing to distribute yet.

When a package is eventually produced it must contain **only** code from this
repository and open-licensed dependencies. No file from the reference client's
`Data/` tree, no client binary, no licensing or anti-cheat component. See
[`LICENSING.md`](LICENSING.md) section 7.

## 7. Known build issues

| Issue | Impact | Status |
|---|---|---|
| Makefile header dependencies | Editing a header once produced an ODR-violating segfault | **Fixed** with `-MMD -MP` and `-include` of generated `.d` files. Do not remove. |
| No CI configuration | Regressions depend on someone running `make test` | Open |
| Windows path unverified | Target platform never compiled | Open, high priority |
