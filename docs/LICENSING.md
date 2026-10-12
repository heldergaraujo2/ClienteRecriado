# Licensing

What is new code, what is authorised, what is an external dependency, and what
still needs rights confirmation. The categories are kept separate because
conflating them is how a redistribution ends up infringing.

## 1. New code — MIT

Everything written for this project:

```
src/mu/assets/format.{hpp,cpp}     CRC-32, status, byte views, file I/O
src/mu/assets/ozt.{hpp,cpp}        OZT reader and writer
src/mu/assets/ozj.{hpp,cpp}        OZJ / JPEG marker parser
src/mu/assets/tga.{hpp,cpp}        TGA reader
src/mu/assets/bmd.{hpp,cpp}        BMD container, bux cipher
src/mu/assets/terrain.{hpp,cpp}    terrain file classification
src/mu/assets/manifest.{hpp,cpp}   patch manifest parse and verify
src/mu/fs.{hpp,cpp}                directory walker
tools/mutool.cpp                   command line tool
tests/test_assets.cpp              test suite and harness
bench/bench_assets.cpp             micro-benchmarks
Makefile, scripts/, docs/
```

Licensed MIT. Clean-room: written from observed byte layouts and from PE import
tables, not by copying or disassembling the original client's code.

**Important limit.** MIT here covers only the code above. It does not and cannot
grant rights to the original client's assets, binaries or any third-party
component. Saying "this repository is MIT" must never be read as "the client is
now free".

## 2. Format knowledge vs. code

What was derived from the reference client is **format knowledge**: field
offsets, sizes, magic values, cipher keys. That knowledge is documented in
[`ASSET_FORMATS.md`](ASSET_FORMATS.md) and implemented independently.

What was **not** done: copying source code, disassembling protected modules,
transcribing functions, or reusing binaries. No original code is in this
repository.

## 3. Original assets — rights unconfirmed, not redistributed

Every file under `Data/` in the reference client:

| Category | Approximate count | Status |
|---|---|---|
| OZJ textures | 13,928 | Rights unconfirmed. Not redistributed. |
| BMD models | 10,090 | Rights unconfirmed. Not redistributed. |
| OZT textures | 3,470 | Rights unconfirmed. Not redistributed. |
| WAV sound effects | 1,020 | Rights unconfirmed. Not redistributed. |
| MP3/OGG music | 64 | Rights unconfirmed. Not redistributed. |
| Terrain `.att`/`.map`/`.obj` | 265 | Rights unconfirmed. Not redistributed. |
| OZB height/light maps | 100 | Rights unconfirmed. Not redistributed. |
| TGA, BMP, SMD, CG, Lua, TXT | ~130 | Rights unconfirmed. Not redistributed. |

**This repository contains zero original asset bytes.** Test fixtures are
generated at run time by `OztWriteRgba`, `MakeFakeJpeg` and the bux cipher tests.
That is a deliberate design property, not an accident of how the tests happened
to be written, and `AGENTS.md` makes it a rule.

MU Online is a commercial product. Its assets are the property of their rights
holders. Writing an independent reader for a file format is interoperability
work; redistributing the files themselves is a different matter and is not done
here.

## 4. Third-party components in the reference client — not redistributable

| Component | Rights |
|---|---|
| `3DGuard.dll`, `3DPlugin.dll`, `hinetworks.dll`, `License.hinet` | Commercial (Hinet Networks). Not redistributable. |
| `MHPClient.dll`, `MHPVerify.dll` | Commercial, vendor unconfirmed. Not redistributable. |
| `main.premium`, `3D.Premium`, `Checksum.premium` | Commercial licensing data. Not redistributable. |
| `PS.dll`, `PS/License.ps` | Separate licence file present. Rights unconfirmed. |
| `AutoPotion.dll`, `AutoPotion.pcd` | Third-party helper. Rights unconfirmed. |
| `arcadia.dll`, `Camera.dll`, `CEffect.dll`, `Audio.dll`, `wzAudio.dll` | Rights unconfirmed. |
| `Launcher.exe` | Rights unconfirmed. |
| `Main.exe`, `Main.dll` | Commercial, VMProtect-protected. Not redistributable, not modifiable. |

**None of these are copied into this repository.**

Two components have clear open licences and are noted for completeness:
`vorbisfile.dll` and `ogg.dll` are Xiph (BSD-style), and the Visual Studio
runtime DLLs are redistributable under Microsoft's terms. Neither is used or
redistributed here.

## 5. The Cg shaders

The six `Data/Effect/glow_*.cg` files are analysed in
[`RENDERING.md`](RENDERING.md) — their structure, the `samplerRECT` usage and the
blend weights 0.105 / 0.86 / 0.085 / 0.86.

The **analysis** is documented. The shader **source is not copied** into this
repository. A GLSL reimplementation would need to be written independently; the
weight constants are observable behaviour of the effect, and reproducing an
effect's visual result from its documented parameters is a different act from
copying its source. If a reimplementation happens, it should be written from the
description in `RENDERING.md`, not transcribed.

The Cg runtime itself is an NVIDIA product and is not used here.

## 6. The patch manifest

`ListaPatch.txt` is a functional data file — paths, checksums, sizes. This
repository implements a **reader** for that format (`mu/assets/manifest.cpp`)
and does not copy the file.

## 7. What a redistributable package may contain

If a distribution is ever built from this project, it may contain:

- Code from this repository (MIT).
- Documentation from this repository.
- Open-licensed dependencies added later, with their licences included.

It may **not** contain, without separate written authorisation from the rights
holders:

- Any file from the reference client's `Data/` tree.
- `Main.exe`, `Main.dll`, `Launcher.exe` or any client binary.
- Any licensing, anti-cheat or `.premium` component.
- The `PS` tool or its licence.

A user would supply their own legally obtained copy of the game assets. The
reconstruction reads them; it does not ship them.

## 8. Open questions

| Question | Impact | Needed before |
|---|---|---|
| Who holds rights to the custom assets under `Data/Custom/`? | Some appear to be community-made additions rather than original game content | Any redistribution |
| Is the server this client targets authorised for use? | Determines whether network compatibility work is legitimate at all | Network implementation |
| Are the `Data/Local/{Eng,Por,Spn}` translation files derived from official localisations? | Affects whether they can be reused | Localisation work |

These are recorded as open rather than assumed resolved.

---

## Código e dados de terceiros incorporados

### Auditoria do ciclo 3 — confirmações executadas

Evidência completa em [`evidence/ciclo3_licensing_audit.log`](evidence/ciclo3_licensing_audit.log).

| Verificação | Comando | Resultado |
|---|---|---|
| Arquivo de terceiros commitado | `git ls-files \| grep -ciE "bmdref\|muonline-bmd-viewer\|xulek"` | **0** |
| Clone externo dentro do repo | `ls -d .git/modules` | sem submódulos |
| Arquivos rastreados | `git ls-files \| wc -l` | 86, todos do projeto |
| Cópia em bloco | `git show --stat 6821d94` | 8 arquivos, todos `src/`, `tests/`, `tools/`, `docs/evidence/` |

O commit que introduziu a cifra (`6821d94`) tocou 8 arquivos, +267/−10 linhas, e
nenhum deles é arquivo de terceiros. O algoritmo foi **reimplementado**, não
copiado; a tabela de chave é reproduzida literalmente porque é dado, não expressão.

**Atribuição presente no código-fonte** (`src/mu/assets/bmd.hpp`, linhas 46–51):

```
/// Provenance, stated because this project derived every other key itself and
/// this one was not derived: the algorithm and its key table were taken from
/// the ISC-licensed project xulek/muonline-bmd-viewer
/// (src/crypto/file-cryptor.ts). ISC is permissive and compatible with this
/// repository's MIT licence. It is re-implemented here rather than copied, and
/// the key table is reproduced verbatim because it is data, not expression.
```

Também atribuído em `tools/bmd_topology_probe.py` (linha 43) e
`src/mu/assets/bmd_body.hpp` (linha 27, que registra que o layout **não** confere
com o daquela fonte).

**O clone de consulta não foi commitado.** `/tmp/bmdref` ficou fora do
repositório e não persiste neste ambiente; `git ls-files` não lista nada dele.

### Cifra BMD v0x0C — xulek/muonline-bmd-viewer (ISC)

| Campo | Valor |
|---|---|
| Repositório | `xulek/muonline-bmd-viewer` |
| URL | https://github.com/xulek/muonline-bmd-viewer |
| Autor | `xulek <xulek@example.com>` (campo `author` do `package.json`) |
| Licença declarada | **ISC** (campo `license` do `package.json`) |
| Commit consultado | `3f11bf89f3702261174d8efa1436d56a15c7d97a` |
| Data do commit | 24 de julho de 2026 |
| Arquivo usado | `src/crypto/file-cryptor.ts` |

**Ressalva registrada por honestidade, não por cautela jurídica.** O repositório
**não contém um arquivo `LICENSE` nem `COPYING`**. A licença ISC consta apenas do
campo `"license": "ISC"` do `package.json`. O texto ISC reproduzido mais abaixo é
o texto **padrão** da licença ISC, não uma transcrição de um arquivo daquele
repositório. O endereço de e-mail do autor é `xulek@example.com`, um domínio de
exemplo, o que sugere que o campo não foi preenchido com dados reais.

Consequência prática: a atribuição está feita e a licença declarada é permissiva,
mas **este projeto não pode garantir a cadeia de licenciamento de um
repositório que não traz arquivo de licença**. Se este cliente vier a ser
distribuído, a cifra v0x0C deve ser revista por quem responder pela distribuição.
O que este projeto pode afirmar é o que fez: reimplementou a lógica, não copiou
código, e atribuiu a fonte.

O algoritmo e a tabela de chave da cifra do payload BMD v0x0C foram obtidos
desse projeto.

A ISC é permissiva e compatível com a licença MIT deste repositório. O que foi
feito aqui:

- O **algoritmo foi reimplementado** em C++ em `src/mu/assets/bmd.cpp`
  (`mu::BmdV0CDecrypt`), não copiado. Não há código TypeScript no repositório.
- A **tabela de 16 bytes foi reproduzida literalmente**, porque é dado e não
  expressão criativa. Está em `mu::kBmdV0CKey`.
- A **atribuição está registrada** no código-fonte, aqui e em
  `docs/DECISIONS.md`.
- **Nenhum arquivo do repositório de terceiros foi copiado.** Verificado por
  `git ls-files | grep -iE "\.ts$|bmd-viewer|file-cryptor"`, que não retorna
  nada, e por `git show --stat 6821d94`, que lista apenas arquivos deste
  projeto. O clone de consulta foi feito em `/tmp`, fora do repositório, e não
  foi commitado.

Declaração de proveniência nos relatórios: quando o projeto informar que a cifra
v0x0C está quebrada, isso é **resultado de terceiros validado por este projeto**,
não resultado derivado por este projeto. As demais chaves (XOR-3 do bux, layout
do corpo v0x0A) foram derivadas de forma independente neste repositório.

Texto **padrão** da licença ISC, transcrito aqui para referência — não copiado
do repositório, que não o traz:

> Permission to use, copy, modify, and/or distribute this software for any
> purpose with or without fee is hereby granted, provided that the above
> copyright notice and this permission notice appear in all copies.
>
> THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
> REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
> AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
> INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
> LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
> OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
> PERFORMANCE OF THIS SOFTWARE.
