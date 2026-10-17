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

From an **x64 Native Tools PowerShell** prompt (candidate initial target; both
x64 and x86 remain unverified, and the original client's x86 architecture does
not by itself determine the reconstruction's ABI):

```powershell
New-Item -ItemType Directory -Force build | Out-Null
$src = Get-ChildItem src\mu -Recurse -Filter *.cpp | ForEach-Object FullName
cl /nologo /std:c++17 /O2 /EHsc /W4 /permissive- /Isrc $src tools\mutool.cpp /Fe:build\mutool.exe
```

Tests:

```powershell
cl /nologo /std:c++17 /O2 /EHsc /W4 /permissive- /Isrc $src tests\test_assets.cpp /Fe:build\test_assets.exe
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
  src/mu/assets/*.cpp src/mu/render/*.cpp src/mu/platform/*.cpp \
  src/mu/fs.cpp tools/mutool.cpp -o build/mutool.exe
```

An `i686-w64-mingw32-g++` build may be tested as a separate architecture
comparison; neither architecture has been compiled here.

## 5. Verifying a build

```sh
make test
```

Expected at the cycle-17 snapshot: `539 checks, 0 failures`, and **no compiler warnings**. The warning
set is `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
-Wold-style-cast -Wuseless-cast -Wnon-virtual-dtor -Woverloaded-virtual
-Wcast-align -Wdouble-promotion -Wformat=2`. Do not narrow it to silence a
warning.

### Sanitised run

```
make test-san        # separate instrumented binary in build-san/
```

Expected at the cycle-17 snapshot: `539 checks, 0 failures` with no
AddressSanitizer, UndefinedBehaviorSanitizer, or LeakSanitizer reports, and exit code 0.
It is a separate target rather than a flag, so `make test` never silently runs
an instrumented build.

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
