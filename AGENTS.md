# AGENTS.md — permanent instructions for agents working in this repository

Read this file before changing anything. It is the contract between you and
every agent that comes after you.

## 1. The two repositories and their roles

| Repository | Role | Rule |
|---|---|---|
| `heldergaraujo2/Cliente` | Reference / research. The original MU Online client. | **Read only. Never commit to it.** |
| `heldergaraujo2/ClienteRecriado` | This repository. All new code, tools, tests, docs. | All work goes here. |

A local clone of the reference client is expected at `../Cliente_ref`
(a sibling of this repository, **outside** it). It is created with
`git clone --no-checkout https://github.com/heldergaraujo2/Cliente.git`.
Never add it to this repository, never copy asset bytes out of it into a
commit, and never push to it.

## 2. Non-negotiable rules

1. **Do not bypass protection.** `Main.exe` in the reference client is packed
   with VMProtect (sections `.vmp0` / `.vmp1`). `License.hinet`,
   `main.premium`, `3D.Premium`, `Checksum.premium`, `MHPVerify.dll` and
   `3DGuard.dll` are commercial licensing / anti-cheat components. Do not
   unpack, patch, keygen, or defeat any of them. If a task seems to require it,
   the task is out of scope — record that in `docs/TEST_RESULTS.md` as
   `BLOCKED` and pick another task.
2. **Do not commit original game assets.** Every texture, model, map, sound and
   script in the reference client is third-party intellectual property. This
   repository contains **zero** original asset bytes. Test fixtures are
   generated at run time by the code in `tests/`. Keep it that way.
3. **Do not commit secrets.** The reference client's `Settings.ini` contains an
   `[AutoLogin]` section with a plaintext username and password. Never copy
   those values into this repository, into a commit message, or into a log.
   Refer to the keys only. See `docs/SECURITY.md`.
4. **Never claim a measurement you did not take.** There is no Windows machine
   and no GPU in the development sandbox. FPS, frametime, VRAM and latency
   numbers must come from a run whose command and output are recorded in
   `docs/BENCHMARKS.md`. If you did not run it, write `NOT EXECUTED` and say
   what is needed to run it.
5. **Verify against real data, not samples.** Sampling hid a defect that a full
   scan exposed: 120 sampled OZJ files looked like plain JPEG, but 13,927 of
   13,928 real files failed to parse. Run `build/mutool scan` over the whole
   tree before believing a format conclusion.

## 3. Working cycle

1. Read `docs/CURRENT_STATE.md`, `docs/NEXT_TASK.md` and `docs/DECISIONS.md`.
2. Check `git status` and `git branch`. Confirm you are on the session branch.
   Preserve work that is already there.
3. Pick the highest-value task you can complete safely.
4. Implement in small, testable steps.
5. Run the checks in section 4. Fix what you broke.
6. Update `docs/CURRENT_STATE.md`, `docs/CHANGELOG.md` and `docs/NEXT_TASK.md`.
7. Review `git diff` before committing. Commit narrowly, one concern per commit.

## 4. Checks you must run before saying "done"

```sh
make            # build library, mutool and the test binary
make test       # unit tests; must be 0 failures
make bench      # optional, builds the micro-benchmark
```

`make` must produce **no warnings**. The build uses `-Wall -Wextra -Wpedantic
-Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wuseless-cast
-Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wdouble-promotion
-Wformat=2`. Do not silence a warning by narrowing the flags; fix the code.

When you have the reference client available, also run:

```sh
build/mutool scan   ../Cliente_ref/Data
build/mutool verify ../Cliente_ref/ListaPatch.txt ../Cliente_ref
```

Expected as of this writing (record the numbers you actually get):

```
files: 29060   total: 2372.04 MB
.ozj  13928 files, 13832 ok,    96 fail
.ozt   3470 files,  3404 ok,    66 fail
.bmd  10090 files, 10066 ok,    24 fail
.tga     15 files,    15 ok,     0 fail
.att/.map/.obj 265 files, 265 ok, 0 fail
verify: 188 entries -> 116 match, 9 size mismatch, 11 crc mismatch, 52 missing
```

A drop in any `ok` count is a regression. Investigate it rather than updating
the expectation.

## 5. Code conventions

- C++17. No third-party dependencies. The library must build with nothing but a
  compiler, because the Windows build path cannot assume a package manager.
- Readers never throw and never abort. They return `mu::Status` with a message
  specific enough to diagnose a file from the message alone.
- Never index a buffer before checking its length. Every reader is fed
  untrusted, possibly truncated files from the internet.
- The `Makefile` generates header dependencies (`-MMD -MP`). Do not remove
  that. Without it, editing a header leaves stale objects linked against a
  changed struct layout, which fails as a segfault far from the cause. This has
  already happened once in this repository.
- Little-endian accessors for every format except JPEG, which is big-endian end
  to end. Using `ReadU16LE` for a JPEG segment length reads `00 43` as 17152
  instead of 67 and reports a healthy file as truncated. That bug existed here
  and was caught only by a test.

## 6. Language

Code, comments, identifiers and commit messages are in English. Player-facing
documentation may be in Portuguese, since the target audience is Brazilian.
This file and `docs/*` are in English so any agent can read them.

## 7. When you are blocked

Say so precisely and move on. The format used throughout the documentation is
one of `PASS`, `FAIL`, `BLOCKED`, `NOT EXECUTED`, `NOT APPLICABLE`, always with
either evidence or the reason. Two items are currently blocked and must not be
reported as solved:

- The BMD v0x0C payload cipher. Entropy 7.99 bits/byte, no repeating-XOR
  period. The loader lives inside the VMProtect-protected module.
- The `Enc*` terrain encryption. Not the bux cipher: decrypting 60 of those
  files with the bux key improved the low-byte density of exactly 1.
