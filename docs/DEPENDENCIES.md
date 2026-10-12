# Dependencies

## 1. This repository has none

The library, the tool, the tests and the benchmarks build with a C++17 compiler
and the standard library. Nothing is downloaded, vendored or linked.

```
Build requires:  g++ (or clang++, or MSVC) with C++17, GNU make, ar
Runtime requires: nothing beyond the C++ standard library
```

Verified: the build succeeds in a container with no OpenGL headers, no zlib
headers, no libpng, no libjpeg, no SDL and no network access to a package
registry. `/usr/include/GL/gl.h`, `zlib.h`, `png.h` and `jpeglib.h` are all
absent from the development environment and the build does not notice.

### Why

Three reasons, in order of weight:

1. **The Windows build cannot be tested here.** There is no Windows machine and
   no MinGW cross-compiler in the development environment. The one build path
   that cannot be verified must be the simplest possible one. A vcpkg manifest
   that has never been run on Windows is a way of shipping a broken build.
2. **No licence obligations to track.** See [`LICENSING.md`](LICENSING.md).
3. **Reproducible builds.** Any machine with a C++17 compiler produces the same
   result.

### What it costs

There is no JPEG pixel decoder. `mu/assets/ozj.cpp` parses the JPEG marker
structure — geometry, precision, component count, termination — which is
everything the inventory and VRAM tooling needs, but it does not produce pixels.
This is a deliberate, recorded gap rather than an oversight.

## 2. Dependencies identified in the reference client

Recorded because they constrain compatibility work, and because several are
third-party products whose redistribution rights are unconfirmed.

| Component | Version evidence | Role | Rights |
|---|---|---|---|
| `msvcr100.dll`, `msvcp100.dll` | Visual Studio 2010 runtime | CRT for `Main.dll` and most plugins | Microsoft, redistributable under MS terms |
| `MSVCP140.dll`, `VCRUNTIME140.dll` | VS2015+ runtime, imported by `Audio.dll` | CRT for the audio module | Microsoft, redistributable under MS terms |
| `vorbisfile.dll`, `ogg.dll` | Xiph | Ogg Vorbis playback | BSD-style (Xiph) |
| `3DGuard.dll`, `3DPlugin.dll`, `hinetworks.dll`, `License.hinet` | Hinet Networks | Licensing / anti-cheat | **Commercial. Not redistributable.** |
| `MHPClient.dll`, `MHPVerify.dll` | Unknown vendor | Verification | **Commercial. Not redistributable.** |
| `main.premium`, `3D.Premium`, `Checksum.premium` | Unknown | Licensing data | **Commercial. Not redistributable.** |
| `PS.dll`, `PS/License.ps` | Delphi build | External tool with its own licence file | **Unknown. Not redistributable.** |
| `arcadia.dll` | MSVC with control-flow guard (VS2019+) | OpenGL effects | Unknown |
| `AutoPotion.dll`, `AutoPotion.pcd` | Unknown | Third-party helper | Unknown |
| `Camera.dll`, `CEffect.dll`, `Audio.dll`, `wzAudio.dll` | Mixed | Client modules | Unknown |
| `Launcher.exe` | .NET (`mscoree.dll`) | Launcher | Unknown |
| Cg shaders under `Data/Effect/` | NVIDIA Cg toolkit | Glow post-process | Cg runtime is NVIDIA's; the shader source is the client author's |

Two Visual Studio runtime generations are present (2010 and 2015+), plus a
module built with MSVC control-flow guard. The client has been extended over
many years by different authors, which matters for any compatibility claim.

## 3. Candidates for future dependencies

Each is a decision to be made explicitly, not a default.

| Need | Candidate | Licence | Notes |
|---|---|---|---|
| JPEG pixel decoding | `stb_image.h` | Public domain / MIT | Single header, no build integration. Strongest candidate. |
| JPEG pixel decoding | libjpeg-turbo | IJG / BSD-style | Faster, but a real build dependency on every platform. |
| PNG support | `stb_image` or libpng | PD/MIT, or libpng licence | Only if PNG assets become relevant. |
| Ogg Vorbis playback | stb_vorbis, or libvorbis | PD/MIT, or BSD | Needed for the 61 MP3 and 3 OGG music files — note MP3 needs a separate decoder. |
| Windowing and input | Win32 directly, or SDL2 | —, or zlib | Win32 keeps the zero-dependency property on the actual target platform. |
| OpenGL loading | hand-written WGL loader, or GLAD | —, or MIT | The client uses only GL 1.1 entry points; a hand-written loader is ~100 lines. |
| Test framework | none — keep the built-in harness | — | Adding one buys little over 40 working lines. |

**Recommendation.** Add `stb_image.h` for JPEG decoding behind a pluggable
interface, so the choice stays reversible. Keep everything else dependency-free.
Record the decision in [`DECISIONS.md`](DECISIONS.md) when it is made.

## 4. Supply chain posture

- No package manager in the build.
- No download step.
- No vendored third-party code.
- No network access required to build.
- No transitive dependencies, therefore no transitive licence obligations.

If a dependency is added, this document must be updated with its name, version,
licence, and the reason the dependency-free property was given up.

---

## 5. Conhecimento de terceiros incorporado (ciclo 2)

Este projeto não tem dependências de build, mas **incorporou conhecimento de uma
fonte externa**. Isso não é uma dependência — nada é baixado, linkado ou
vendorizado no build — mas é uma obrigação de atribuição e pertence aqui.

| Item | Detalhe |
|---|---|
| Fonte | `xulek/muonline-bmd-viewer` — https://github.com/xulek/muonline-bmd-viewer |
| Commit consultado | `3f11bf89f3702261174d8efa1436d56a15c7d97a` — `git log -1 --format=fuller` em `docs/evidence/ciclo6_bmdref_commit_date.log` mostra `xulek <xulek121@gmail.com>` 2026-07-24, branch `main` |
| Arquivo | `src/crypto/file-cryptor.ts` |
| Licença declarada | ISC (campo `license` do `package.json` apenas — sem arquivo LICENSE, e-mail placeholder `xulek@example.com`) — **NÃO CONFIRMADA** (item 0 ciclo 6) |
| O que foi usado | Algoritmo e tabela de 16 bytes da cifra do payload BMD v0x0C |
| O que **não** foi usado | Nenhum arquivo. Não há TypeScript nem código copiado neste repositório |
| Onde está | `src/mu/assets/bmd.hpp` (`kBmdV0CKey`) e `src/mu/assets/bmd.cpp` (`BmdV0CDecrypt`), com atribuição em comentário no próprio código |

**Verificação de que nada foi copiado em bloco:**

```
$ git ls-files | grep -iE "\.ts$|bmd-viewer|file-cryptor"
(sem saída)

$ git show --stat 6821d94
 docs/evidence/ciclo2_bmdreport_v0c.log | 66 ++++++
 docs/evidence/ciclo2_test.log          | 44 ++++
 src/mu/assets/bmd.cpp                  | 16 ++
 src/mu/assets/bmd.hpp                  | 24 ++
 src/mu/assets/bmd_report.cpp           | 58 +++++-
 src/mu/assets/bmd_report.hpp           |  2 +
 tests/test_assets.cpp                  | 61 ++++++
 tools/mutool.cpp                       |  6 +-
```

O clone de consulta foi feito em `/tmp/bmdref`, fora do repositório, e **não foi
commitado**: `git ls-files | grep -ci "bmdref\|muonline-bmd-viewer"` retorna `0`.

**Re-verificado no ciclo 3.** As quatro confirmações foram rodadas de novo, não
apenas reafirmadas — evidência em
[`evidence/ciclo3_licensing_audit.log`](evidence/ciclo3_licensing_audit.log):

| Verificação | Comando | Resultado |
|---|---|---|
| Arquivo de terceiros commitado | `git ls-files \| grep -ciE "bmdref\|muonline-bmd-viewer\|xulek"` | **0** |
| Clone externo dentro do repo | `ls -d .git/modules` | sem submódulos |
| Arquivos rastreados | `git ls-files \| wc -l` | 86, todos do projeto |
| Cópia em bloco | `git show --stat 6821d94` | 8 arquivos, nenhum de terceiros |

**Ressalva sobre a licença.** O repositório de origem **não traz arquivo
`LICENSE`**; a licença ISC consta apenas do `package.json`, e o e-mail do autor é
`xulek@example.com`, um domínio de exemplo. Ver `docs/LICENSING.md` para a
consequência disso em caso de distribuição.

**O que foi rejeitado da mesma fonte.** As tabelas de layout de registro daquele
projeto (vértice e normal começando com índice de osso e padding) **não
correspondem** aos arquivos deste cliente. Foram medidas e descartadas; o layout
usado aqui foi estabelecido contra os dados. Ver `docs/ASSET_FORMATS.md`.

## 6. Conhecimento de terceiros incorporado — extensão multi-mesh (ciclo 5)

| Item | Detalhe |
|---|---|
| Fonte | `xulek/muonline-bmd-viewer` — https://github.com/xulek/muonline-bmd-viewer |
| Commit consultado | `3f11bf89f3702261174d8efa1436d56a15c7d97a` — evidência bruta em `docs/evidence/ciclo6_bmdref_commit_date.log`, autor commit `xulek121@gmail.com`, `package.json` author `xulek@example.com` placeholder, branch `main` |
| Arquivo | `src/bmd-loader.ts`, linhas 348–422 |
| Licença declarada | ISC (campo `license` do `package.json` apenas, sem LICENSE) — **NÃO CONFIRMADA** (item 0 ciclo 6) |
| O que foi usado | **Estrutura de loop** sobre `meshCount` e ordem dos campos por malha: `numVertices`, `numNormals`, `numTexCoords`, `numTriangles`, `textureIndex`, depois arrays e nome de textura 32 B |
| O que **não** foi usado | Código TypeScript não copiado; layout de triângulo daquele projeto (polygon byte em +0, vertexIndex em +2, normalIndex em +10, texCoordIndex em +18) **não corresponde** aos dados deste cliente (medido: vertex em +0/+8/+16, polygon byte não é 3/4 em 97% dos casos). Header por malha deste projeto é **14 B** (`vc,nc,ntc,ntri,texIdx,u48,u50`) vs 10 B da fonte; busca de nome com delta -4 é descoberta deste projeto |
| Onde está | `src/mu/assets/bmd_body.cpp` (parser multi-mesh, com atribuição em comentário) |
| Validação | 4.256 arquivos multi-mesh (12.590 malhas) parseados 100%, taxa de nome estrito em `tri_end-4` 14,27% vs controle 0,0376% (razão 379×) |

**Verificação de que nada foi copiado em bloco (ciclo 5):**

```
$ git show --stat 3a44591
 docs/evidence/ciclo5_bmd_multimesh.log | 105 ++++++++
 src/mu/assets/bmd_body.cpp             | 437 +++++++++++++++++++++++++--------
 tests/test_assets.cpp                  |  25 +-
 tools/mutool.cpp                       |   6 +-
 4 files changed, 461 insertions(+), 112 deletions(-)
```

Nenhum arquivo `.ts` adicionado. Clone de consulta em `/tmp/bmdref`, fora do repo, não commitado: `git ls-files | grep -ci "bmdref"` retorna `0`.

**Atribuição no código-fonte** (`src/mu/assets/bmd_body.cpp`, após correção do ciclo 6):

```cpp
// Provenance: loop structure over meshCount and per-mesh field order inspired
// by xulek/muonline-bmd-viewer (src/bmd-loader.ts:348-422, ISC). The 14-byte
// header and triangle layout are this project's own measurements, not that
// project's tables, which were refuted for these files.
```

**Ressalva idêntica à da cifra:** repositório sem arquivo LICENSE, licença apenas no package.json, e-mail de exemplo. Ver LICENSING.md.
