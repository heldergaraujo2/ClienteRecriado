# Current state

An honest snapshot. What exists, what works, what does not.

## 1. What exists and works

**An asset format library**, C++17, zero dependencies, warning-clean:

| Module | What it does | Verified against |
|---|---|---|
| `mu/assets/ozt` | OZT read, write, probe, RGBA conversion | 3,404 of 3,470 real files |
| `mu/assets/ozj` | OZJ container + JPEG marker parsing | 13,832 of 13,928 real files |
| `mu/assets/tga` | TGA probe and uncompressed read | 15 of 15 real files |
| `mu/assets/bmd` | BMD container classification, bux cipher, entropy | 10,023 of 10,090 structurally valid; **0 with geometry parsed** |
| `mu/assets/bmd_report` | Three-level usability reporting (structure / cleartext / geometry) | the full 10,090-file BMD set |
| `mu/assets/bmd_body` | v0x0A body parser: vertex positions and normals | 1,191 real files, layout verified 145/145 |
| `mu/assets/terrain` | Terrain classification, bux auto-detect | 265 of 265 real files |
| `mu/assets/manifest` | Patch manifest parse and CRC verify | the client's own 188-entry manifest |
| `mu/assets/format` | CRC-32 (slice-by-8), status, byte views | zlib-derived known answers |
| `mu/fs` | Recursive walker, POSIX + Win32 | the 29,060-file real tree |

**A command line tool**, `build/mutool`, with `scan`, `info`, `verify`,
`dedupe`, `bmdreport`. Whole-tree scan of 2,372 MB in 1.67 s.
`bmdreport` exits 2, because a report that cannot deliver geometry should not
claim success.

**A test suite**, 231 checks, 0 failures, self-generating fixtures, also clean
under ASan and UBSan.

**A micro-benchmark harness** with median, standard deviation, min/max and
P95/P99 reporting, over three buffer sizes, plus an optional per-repetition
disk read (`MU_BENCH_FILE`) so cached and I/O-bound regimes are reported
separately instead of being averaged together.

**Documentation**: audit, formats, rendering analysis, architecture, baselines,
benchmarks, decisions, licensing, security, test results.

**Windows validation scripts** in `scripts/`, written but never run.

## 2. Measured numbers

All from the real reference tree — 29,060 files, 2,372.04 MB.

```
OZJ   13,928 files -> 13,832 header+geometry parsed  (99.31%)   pixels NOT decoded
OZT    3,470 files ->  3,404 pixels parsed           (98.10%)
TGA       15 files ->     15 pixels parsed           (100%)
terrain    265 files ->    265 classified            (100%)   97 .att still encrypted
```

**BMD is reported at three levels, because one percentage was misleading.**
An earlier revision of this document said "10,066 of 10,090 classified
(99.76%)". That described container headers only. The corrected breakdown,
produced by `mutool bmdreport` and stored in
[`evidence/ciclo1_bmdreport.log`](evidence/ciclo1_bmdreport.log):

```
level 1  structure valid        10,023 / 10,090  (99.34%)
level 2  payload in the clear    1,496 / 10,090  (14.83%)
           plaintext             1,396  (13.84%)
           bux-decryptable         100  ( 0.99%)
           OPAQUE                8,594  (85.17%)
level 3  geometry extracted      1,191 / 10,090  ( 11.80%)
```

**Level 3 is the number that matters for rendering.** It rose from 0 to 1,191
during cycle 1, when a v0x0A body parser was written against the verified
layout. It counts **vertex positions and normals only** — face indices, texture
coordinates and bones are still undecoded, so none of those 1,191 files is a
renderable model. v0x0C is excluded from level 3 entirely, which caps it at
11.83%.

```
Theoretical maximum texture footprint: 2,893.80 MB
  -> sum of w*h*4 over parseable headers. DERIVED, NOT MEASURED.
     No process was run and no GPU memory was observed.

Integrity vs ListaPatch.txt (188 entries, evidence/ciclo1_crc_comparison.log):
  compared 136   identical 116   different 20   not present on disk 52
  all 20 differences also differ in file size, i.e. the tree and the
  manifest disagree; the CRC computation itself is correct

mutool scan  29,060 files : 1,641–1,674 ms
mutool verify 188 entries : 42–49 ms   (I/O bound: 1,358.9 MB/s incl. disk read)
make test                 : 231 checks, 0 failures
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
| Model loading | **PARTIAL** — vertex positions and normals for 1,191 files; faces/UV/bones not decoded |
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
| **BMD payload cipher not recovered** | **Critical** | **Project blocker #1. 85.17% of 10,090 models opaque; 0% geometry parsed** |
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
visual. Anyone reading this should understand that "99% of containers classify" and
"you can play the game" are very far apart, and the second is not close. The
cycle-1 audit was right to reject the earlier single-percentage framing: the
number that describes usable geometry is 0.00%, and it is reported that way
everywhere it appears.

---

# Ciclo 3 — estado atual

> **Numeração.** O ciclo 2 foi fechado com `docs/CICLO2_RELATORIO_FINAL.md`. Todo
> trabalho posterior é **ciclo 3** e não reabre o número anterior. As seções
> abaixo que dizem "ciclo 2" são registro histórico do que foi fechado, não do
> que está em andamento.

**Correções deste ciclo, todas por medição:**

| Afirmação anterior | Corrigida para | Como |
|---|---|---|
| Topologia não implementada em C++ | **Implementada, com testes** | `bmd_body.cpp:117,135`; `TestBmdTriangles`; nível 4 do `bmdreport` |
| Ossos 98,0% | **98,68% de 3.552** = 85,09% dos 4.119 elegíveis | `tools/bmd_bones_probe.py`, full-scan |
| BMP 44 de 47 | **42** (41 no offset 4 + 1 no offset 0) | critério real do parser |
| Referências fora de faixa pendentes | **Já tratadas** — descartadas e contadas | `MeshFromBmd`, `bad_indices` |

**Testes:** 344 checks, 0 failures · ASan+UBSan 344/0 · zero warnings sob 12 flags `-W`

---

# Atualização do ciclo 2 — bloqueio nº 1 resolvido

## O que mudou

A cifra do payload BMD v0x0C foi quebrada. Proveniência: algoritmo e tabela de
chave do projeto `xulek/muonline-bmd-viewer` (licença ISC), reimplementados em
C++ neste repositório. Ver `docs/DECISIONS.md` (DEC-007) e `docs/LICENSING.md`.

**Isto é resultado de terceiros validado por este projeto, não resultado
derivado por este projeto.** As chaves derivadas aqui continuam sendo o XOR-3 do
bux e o layout v0x0A.

## Números medidos (10.090 arquivos reais)

| Nível | Ciclo 1 | Ciclo 2 |
|---|---|---|
| 1 — estrutura válida | 10.023 (99,34%) | 10.023 (99,34%) |
| 2 — payload em claro | 1.496 (14,83%) | **8.571 (84,95%)** |
| 3 — geometria parseada | 1.191 (11,80%) | **8.406 (83,31%)** |
| OPAQUE | 8.594 (85,17%) | **1.519 (15,05%)** |

O teto teórico de 11,83% que constava no relatório do ciclo 1 deixa de existir.

## Duas correções que precisavam ser feitas

1. **Este projeto afirmou que a rota open-source estava bloqueada por falta de
   acesso a um host de código. Era falso.** `github.com` é acessível. A
   afirmação estava em `NEXT_TASK.md`, em `docs/evidence/ciclo1_bmd_layout.log`
   e nas mensagens dos commits `d980216` e `50a5ceb`.
2. **A rejeição da hipótese (u,v) estava errada.** O controle aleatório
   subestimava a linha de base porque coordenadas de textura reais não são
   uniformes em [0,1] — são enviesadas para 0 e 1 por espelhamento de malha.

## O número 83,31% foi verificado, e uma suspeita sobre ele foi descartada

Uma versão anterior deste documento registrava como "defeito conhecido" a
suposição de que v0x0C usaria half-floats nos arrays, o que invalidaria as
posições lidas. **A suposição era infundada.** Medido sobre 400 arquivos v0x0C
decifrados: corpo completo em 400 de 400 sob o layout v0x0A, e normais com norma
1 em **99,98%** dos casos (37.988 de 37.997) no deslocamento interno 0, contra
31,46% no deslocamento 4. Uma chave errada não produz vetores unitários.

Isso vale como verificação independente da cifra, por uma via que não depende do
campo de nome usado na checagem de plausibilidade.

## Um erro cometido e desfeito neste ciclo

Uma revisão trocou a ordem dos campos nos registros de vértice e normal, com
base numa medição que reportava "offset 4 vence em 1.050 arquivos, offset 0 em
nenhum". A medição lia deslocamentos de arquivo como se fossem de payload,
defasando tudo em 4 bytes. Revertido em `bf1668c`, com as medições corretas
registradas em `docs/ASSET_FORMATS.md`.

O layout de registro do projeto ISC **não corresponde** a estes arquivos: ele
inicia cada registro com índice de osso e padding. Aqui as coordenadas vêm
primeiro. A cifra daquele projeto foi confirmada; as tabelas de layout, não.

## Testes

236 checks, 0 failures (eram 231). ASan+UBSan: 236/0. Compilação sem warnings
sob 12 flags `-W`. Evidência em `docs/evidence/ciclo2_test.log` e
`docs/evidence/ciclo2_bmdreport_v0c.log`.

## ESTADO DO GIT NESTE CICLO

- `6821d94` feat(assets): cifra v0x0C decifrada — nível 3 vai de 11,80% para 83,31%
- push: **sim**, `git push origin arena/519f8fc9-clienterecriado` → sucesso
- commits deste segmento: 1 código + 1 documentação
- referência `Cliente_ref` intacta: `88edfc73` (inalterado)

---

# Estado final do ciclo 2 (pós-correções)

Substitui todas as seções anteriores deste arquivo para o ciclo 2.

## Números finais — execução do zero, código consolidado

| Nível | Ciclo 1 | **Ciclo 2** |
|---|---|---|
| 1 — estrutura válida | 10.023 (99,34%) | 10.023 (99,34%) |
| 2 — payload em claro | 1.496 (14,83%) | **8.571 (84,95%)** |
| 3 — geometria parseável | 1.191 (11,80%) | **8.406 (83,31%)** |
| OPAQUE | 8.594 (85,17%) | **1.519 (15,05%)** |
| Checks de teste | 231 | **236** |

Evidência: `evidence/ciclo2_test_final.log` (`make clean && make test`),
`evidence/ciclo2_scan_final.log` (relatório com binário recém-compilado).

## O que está resolvido e o que não está

**Resolvido e validado:**

- Decifragem v0x0C — 7.226 arquivos
- Posições de vértice e normais — 8.406 arquivos (83,31%)
- Array de coordenadas UV — zero não-finitos em 250.450 leituras (acaso daria ~976)
- Topologia — 1.221.030 triângulos em 4.149 arquivos, lidos pelo parser C++
- Hierarquia de ossos — `u16` pai (65535 = raiz em 75,9%) + 6 floats 100% finitos,
  em registro intercalado de 59 B confirmado em 98,0%

**Implementado neste ciclo, em código testado:**

- `src/mu/assets/mesh.hpp/.cpp` — representação interna de malha e importador BMD
  (caminho A). Contagens de 32 bits, índices separados, caixa delimitadora na
  importação. Não produz coordenadas de textura nem ossos, e diz por quê
- `src/mu/assets/terrain.cpp` — leitor de altura de terreno (`TerrainParseHeight`,
  `TerrainHeightSmoothness`), com tratamento de BMP e fallback marcado
- `src/mu/render/cull.cpp` — frustum culling por caixa delimitadora, o primeiro
  módulo de renderização do projeto
- `mutool meshimport <dir>` — pipeline BMD completo sobre a árvore real:
  8.408 malhas internas, 1.792.108 vértices, 978.853 triângulos
- `mutool culltest <dir>` — mede culling sobre malhas reais
- `mutool terrain <dir>` — lê as 76 grades de altura reais e mede suavidade
- Testes: **338 checks, 0 failures**; ASan+UBSan 338/0; zero warnings sob 12 flags `-W`

**Bloqueado:**

- **Índice de UV por canto** — investigação ENCERRADA, sete hipóteses descartadas.
  Sem ele os 4.149 arquivos com topologia não podem ser texturizados
- Ordem dos 6 floats de transformação — posição antes de rotação é inferência, não
  confirmação. Aplicar trocado daria esqueleto silenciosamente errado
- Seção de animação — não examinada
- 1.519 arquivos v0x0C, malhas múltiplas, pipeline VBO/VAO (nenhuma linha de código)

## Correções feitas neste ciclo

1. A afirmação de que a rota open-source estava "bloqueada por falta de acesso"
   era **falsa**; o acesso existia.
2. A troca de layout de registro baseada em medição com base errada (arquivo vs
   payload, defasagem de 4 bytes) foi **revertida** em `bf1668c`.
3. O "defeito conhecido" de que v0x0C usaria `f16` era **infundado**; descartado
   por medição (400/400 em `f32`).
4. A atribuição ISC foi registrada com a ressalva de que o repositório de origem
   **não tem arquivo `LICENSE`**.

## Nova exigência arquitetural incorporada

DEC-008 (VBO/VAO como piso mínimo), DEC-009 (teste sintético não é suporte a
asset), limites do BMD conhecidos vs desconhecidos, importação dual com glTF 2.0
no caminho B, requisitos de escalabilidade. **Nada disso está implementado** — são
decisões e requisitos registrados.

## ESTADO DO GIT NESTE CICLO

- Commits: **15**, listados em `CICLO2_RELATORIO.md`. Um por unidade coerente
- Push: **realizado após cada commit**, nenhuma falha em todo o ciclo
- Hash local **==** hash remoto. O valor **não se copia deste documento**: um
  commit não contém o próprio hash, então lê-se com `git rev-parse HEAD` e
  `git ls-remote origin arena/519f8fc9-clienterecriado` no momento da consulta.
  Esta regra existe porque três versões anteriores desta seção ficaram defasadas
  pelo commit seguinte
- `git status --short`: vazio — **working tree limpa**
- Referência `Cliente_ref`: `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`, idêntica
  ao início do ciclo, **não modificada**
- Nada ficou sem commitar ou sem publicar

Relatório completo em `CICLO2_RELATORIO.md`.
