# Changelog

Performance changes with measurements are in
[`OPTIMIZATION_LOG.md`](OPTIMIZATION_LOG.md). This file covers everything else.

## 2026-10-13 — isolated orbital-camera foundation for BMD preview

### Added

- Added platform-independent `mu::render::OrbitCamera` state and pose
  calculation in `src/mu/render/orbit_camera.hpp/.cpp`, with 360-degree yaw
  wrapping, clamped pitch/distance, exponential wheel zoom and rejection of
  non-finite controls.
- Added `tests/test_orbit_camera_standalone.cpp`; it compiles directly to `/tmp`
  and does not use or replace the parser test executable under `build/`.
- Documented the camera-to-D3D11 integration contract and the separate geometry,
  UV/material, OZT/OZJ and GPU-render acceptance tests in
  `docs/D3D11_BMD_PREVIEW.md`.

### Validation status

- Standalone warning-as-error C++17 run: **PASS**, 57 checks, 0 failures.
- Standalone ASan/UBSan run: **PASS**, 57 checks, 0 failures.
- `make`, `make test`, `make test-san` and the D3D11 BMD preview were not run by
  this task; the active Windows validation and repository `build/` artifacts
  were left untouched. The docs consistency checker was invoked but blocked
  before data checks (exit 2; missing `Cliente_ref/Data`). The new camera is not
  yet wired into the Windows preview.
- No additional BMD parser, existing parser test, original asset, or preview
  source change was authored for this unit. The checkout was fast-forwarded to
  the published `0c21ddd` baseline before implementation. No UV/material
  semantics or model-render claim is added.

## 2026-10-13 — BMD v0x0A topology parser correction

### Changed

- Corrected the triangle-array cursor by four bytes, accounting for the parser's
  float-data cursor being four bytes after the beginning of vertex/normal records.
- Read polygon size and separate vertex, normal, and candidate UV-index tables
  from the 64-byte record; split quads into two triangles.
- Added synthetic tests for the corrected record layout and two-mesh index bases.

### Validation status

- Source changes and test fixtures are published to the fixed session branch.
- `make`, `make test`, `make test-san`, documentation consistency, and the
  Windows preview **have not yet been rerun** after the change.
- The prior cycle-17 full-tree metrics are historical baseline data until
  regenerated with the corrected parser. No full-tree compatibility or GPU model
  render is claimed.

## 2026-10-12 — cycle-17 audit and recovery reconciliation

### Changed

- Split BMD accounting into out-of-range vertex references, out-of-range normal
  references, and triangles dropped once per invalid triangle. The full-tree
  report now distinguishes 1,030,785/22,089,984 invalid reference fields
  (4.666%) from 513,304/3,681,664 triangles dropped (13.94%).
- Added standard uncompressed 24-bit BGR-to-BGRA8 OZT conversion and documented
  the 24/32-bit conversion contract. Hardened software texture sampling against
  undersized public pixel buffers.
- Made render texture selection explicit (`--texture <file.ozt>` or
  `--checkerboard`); an output filename no longer implicitly enables a
  placeholder. Planar mapping is explicitly diagnostic when BMD UVs are unknown.
- Updated probes, CLI labels, evidence logs, compatibility tables and continuity
  documents to distinguish header recognition, structural validation, decoding,
  semantics and verified rendering.

### Measured / not claimed

- `make test` and `make test-san`: 539 checks, 0 failures (cycle-17 clean build
  and sanitizer logs are recorded separately).
- The available 188-entry reference manifest check: 116 matches, 8 size
  mismatches, 11 CRC mismatches, 53 missing; this is incomplete coverage, not a
  188/188 integrity pass.
- A real OZT was converted and supplied to the CPU diagnostic renderer next to a
  different BMD. The files were not shown to be associated; no visual or GPU
  compatibility was claimed.
- No Windows/GPU build, FPS, frametime, VRAM, or authorized network test was run.
  The BMD v0x0C and `Enc*` terrain ciphers remain BLOCKED under `AGENTS.md`.

### Recovery and continuity reconciliation

- Re-verified the fixed branch and remote refs through both Git and GitHub API;
  branch 519 remained unchanged. The recovery stash and archives remain local;
  six alternate PPMs remain outside Git. The worktree and branch had no dirty or
  unpushed Git changes at the audit baseline.
- Recorded the historical test-count conflict in `TEST_RESULTS.md`: the 519
  snapshot has 389 in `TEST_RESULTS.md` but 500 in other summaries; the current
  recovered source was rerun at 539 checks in both normal and sanitizer modes.
- Added DEC-014 to preserve, rather than silently choose between, the earlier
  OpenGL VBO/VAO decision and the newer D3D12/Vulkan evaluation plan. No API was
  selected or GPU milestone claimed.

## 2026-10-12 — performance target and renderer priority

### Added

- `docs/PERFORMANCE_TARGET.md`: records the selected target of maximum practical
  performance on modern Windows PCs with dedicated GPUs, preservation of original
  assets, optional visual enhancements, heavier-scene support, architecture
  boundaries and benchmark evidence requirements.
- Replaced the stale audio-first next task with a Windows/GPU renderer vertical
  slice: real window/device, evidence-based API selection, real model rendering
  and a reproducible GPU baseline.
- Updated README and project overview to distinguish the CPU software-rendering
  proof of concept from a production GPU renderer, and to distinguish BMD
  structural recognition from usable geometry and real UV support.

### Not claimed

No production GPU renderer, FPS gain, VRAM usage, input-latency improvement or
Windows runtime validation was performed by this documentation change.

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

## Ciclo 6 — auditoria ciclo 5 e correção de bloqueios críticos (itens 0-6, em andamento)

### Corrigido (auditoria ciclo 6 item 0 — bloqueio crítico)

- **Atribuição licença externa com data futura e e-mail placeholder.** Commit `3f11bf89f3702261174d8efa1436d56a15c7d97a`
  de `xulek/muonline-bmd-viewer` reportado como 24 jul 2026 com e-mail `xulek@example.com`.
  Verificação bruta `git -C /tmp/bmdref log -1 --format=fuller` mostra Author `xulek <xulek121@gmail.com>`
  (não `example.com`), AuthorDate `Fri Jul 24 21:10:59 2026 +0200`, CommitDate idem, branch `main`.
  `package.json` author é `xulek <xulek@example.com>` (domínio reservado RFC 2606, placeholder).
  Repositório inteiro datado 2025-07-02 a 2026-07-24, com inconsistências de ordenação (2025-11 intercalado),
  indicando clock skew. Sem arquivo LICENSE, apenas campo `license: ISC`. Atribuição marcada como
  **NÃO CONFIRMADA** até verificação adicional, evidência bruta salva em `evidence/ciclo6_bmdref_commit_date.log`,
  LICENSING.md e DEPENDENCIES.md atualizados.

### Corrigido (auditoria ciclo 6 item 1 — população multi-mesh divergente)

- **Divergência 4.257/4.251/4.256 explicada e corrigida.** Ferramenta `mutool bmdreport` confirma:
  level3 8.406, meshCount1 4.149, meshCount0 1 (`player.bmd`), meshCount>1 4.256 verdadeiro multi-mesh,
  meshCount!=1 4.257 (4.256+1). Soma distribuição 2.280+1.086+449+190+114+42+38+12+10+6+7+2+13+7=4.256.
  - 4.257 usado ciclo 4 como total !=1 (4256 multi +1 zero) para queda 8.406→4.149.
  - 4.251 usado ciclo 5 como >1 no snapshot 8.401 com regressão 10k (8.401-4.149-1), histórico.
  - 4.256 correto final snapshot 8.406. Erro anterior: contar 4.257 como multi-mesh puro sem separar zero-faces,
    e reutilizar 4.251 de snapshot com regressão. Docs ARCHITECTURE, ASSET_FORMATS, COMPATIBILITY, CURRENT_STATE
    corrigidos, evidência `evidence/ciclo6_item1_bmdreport.log`.

### Documentado (auditoria ciclo 6 item 2 — violação commits atômicos)

- **Commit `1fc1024` agrupou 3 itens distintos** (contabilidade nível 4, validação estatística multi-mesh,
  licenciamento referência externa) em um único commit com mensagem `docs: itens 2-4 auditoria ciclo5`,
  violando regra permanente de commits atômicos (1 unidade coerente por commit, AGENTS.md e auditoria ciclo 1).
  Não desfeito (push já realizado, reescrever histórico proibido), mas registrado como desvio em DECISIONS.md
  (DEC-011) e aqui. A partir deste ciclo, 1 commit por item da auditoria (0,1,2...), com push após cada um.

## Ciclo 5 — auditoria do ciclo 4 (itens 1-5) e extensão multi-mesh

### Corrigido (auditoria item 1)

- **DEC-008 comparava custo de carregamento com orçamento de quadro.** A expansão
  índice→flat é custo de CARREGAMENTO, não por quadro. O que se repete por quadro
  é o **volume** de submissão (1,29×, 63,9% envios redundantes) com custo BLOCKED
  (driver/barramento). Corrigido em `bench/bench_assets.cpp`, `DECISIONS.md`
  §3.1 e `RENDERING.md` §3.1, com justificativa técnica citando `RENDERING.md`
  §3: "re-marshalled per draw" é driver relendo array, não reconstruindo.
- **Evidência regenerada:** `ciclo4_dec008_submission_cost.log` agora separa
  CARREGAMENTO (19.190,9 µs vs 3.493,1 µs, razão 5,49×, 5,14×–5,62× entre execuções,
  N=5) de VOLUME POR QUADRO (67,21 MiB vs 52,22 MiB, 1,29×). Limite da conclusão
  corrigido para não afirmar FPS.

### Adicionado (auditoria itens 2-4)

- **Texto da decisão sem ambiguidade** em `DECISIONS.md`: blockquote "VBO + VAO
  com buffers indexados é o piso mínimo" + 4 condições objetivas de revisão
  (Parte B, conteúdo pesado, hardware mínimo, glDrawElements encontrado).
- **Nota de amostra pequena** em `PERFORMANCE_BASELINE.md` §2.1a: "Amostra
  pequena (N=5), indicativa, não definitiva" + limites que importam mais que N.
- **Verificação automatizada doc↔código** `tools/check_docs_consistency.py`:
  36 verificações, 0 falhas. Roda `mutool bmdreport` e falha se número na linha
  da tabela diverge, número proibido sem `<!-- historico -->`, include gráfico
  em src/, nível 4 > nível 3. Testes de injeção: número divergente, percentual
  divergente, nível 6 não zero, número proibido — todos falham; restaurado passa.
  Comando documentado em `AGENTS.md` §4 como obrigatório.

### Adicionado (auditoria item 5 — multi-mesh)

- **Parser multi-mesh** em `bmd_body.cpp`: loop sobre `meshCount`, cada malha com
  header 14 B (vc,nc,ntc,ntri,texIdx,u48,u50) + arrays + 32 B nome textura.
  Nome sistematicamente 4 bytes antes (end-4) — busca com delta.
- **Resultado:** nível 4: 4.149 (41,12%) → **8.400 (83,25%)**, +4.251 arquivos,
  triângulos 1.221.030 → **3.524.366**. Nível 3: 8.406 → 8.401 (-5, 0,06%).
  Level 4b: multi-mesh 0 (0%), zero faces 1 (100%).
- **Evidência:** `ciclo5_bmd_multimesh.log` — full-scan sem números digitados.

### Corrigido

- `mutool.cpp:461` dizia "Single-mesh files only" após extensão multi-mesh.
  Corrigido para "Multi-mesh supported since ciclo 5".
- `COMPATIBILITY.md` e `ASSET_FORMATS.md` atualizados para 8.401/8.400 e
  3.524.366 triângulos, com causa corrigida e distribuição meshCount atualizada.

### Testes

373 checks, 0 falhas (era 372 — multi-mesh agora conta) · 36 verificações docs,
0 falhas · 0 warnings 12 flags.

---

## Ciclo 4 — pendências da auditoria do ciclo 3 e DEC-008 completa

### Adicionado

- **`BmdTopologySkip`** em `src/mu/assets/bmd_body.{hpp,cpp}` — o parser agora
  registra *por que* não leu os triângulos: multi-mesh, zero faces declarado,
  array que não cabe, ou payload curto. Um array vazio eram quatro falhas
  indistinguíveis; agora são quatro causas com consequências diferentes
- **`TestTopologyIsSubsetOfGeometry`** (28 asserções novas) — prova três coisas
  que a tabela não expressa: nível 4 é subconjunto estrito do nível 3, um array
  vazio são quatro falhas distinguíveis, e as quatro causas são **exaustivas**
  (soma comparada com `geometry_parsed - topology_parsed`, então uma quinta causa
  não aparece em silêncio)
- **`LEVEL 4b`** e histograma de `meshCount` na saída de `mutool bmdreport`
- **Benchmark de caminho de submissão** em `bench/bench_assets.cpp`, para
  decidir DEC-008 com medição em vez de argumento
- **`DEC-008-A`** em `DECISIONS.md` — a comparação formal de custo e risco que
  `DEC-010` exigia antes de qualquer código de backend
- **`SECURITY.md` §9** — risco operacional de restauração de snapshot, com a
  tabela de mitigação e o que fazer quando o branch divergir

### Medido

- **Causa da queda de 50,6% entre nível 3 e nível 4: `meshCount != 1` em
  100,00%** dos 4.257 arquivos. Zero falham por tamanho, versão ou campo
  ausente. É limite do parser, não do formato
- **Expansão indexado → flat: 20.502,7 µs por quadro** contra **935,7 µs** da <!-- historico -->
  submissão indexada — razão **21,91×** (entre execuções: 21,82× a 22,78×). A <!-- historico -->
  expansão sozinha é **123,0% do orçamento inteiro de um quadro a 60 FPS**, e é <!-- historico -->
  CPU apenas
- Upload único do vertex buffer: 3.647,9 µs, pago em **0,19 quadro**

> **Correção registrada.** A primeira versão destes números (21.459,9 / 837,3 / <!-- historico -->
> 25,63×) veio de uma execução do benchmark cujos valores foram **digitados** na <!-- historico -->
> seção de contas da evidência, enquanto a seção de resultado veio de outra
> execução. As duas discordavam dentro do mesmo arquivo. Os números acima saem
> de `tools/gen_dec008_evidence.py`, que deriva tudo da saída do benchmark e
> reporta a dispersão **entre** execuções (6,40% na expansão, 10,23% no
> indexado), não só a interna.

### Corrigido

- **`ARCHITECTURE.md` §9 dizia "oito hipóteses" descartadas para UV. São nove.**
  A nona foi testada e rejeitada no ciclo 3 e a seção não foi revisitada — a
  mesma classe de defasagem que a auditoria cobra. Corrigido também em
  `COMPATIBILITY.md`. Relatórios de ciclo fechado ficaram como estavam: eram
  verdadeiros quando escritos
- **Tabela do ciclo 1 removida de `COMPATIBILITY.md` e `ASSET_FORMATS.md`.**
  Dizia "payload em claro 1.496 (14,83%)" e "geometria 1.191 (11,80%)", <!-- historico -->
  anteriores a DEC-007. Manter as duas tabelas lado a lado era a própria
  ambiguidade que a auditoria pediu para eliminar
- Prefixo dos arquivos de evidência novos: `ciclo3_` → `ciclo4_`. A convenção é
  que o prefixo indica o ciclo que **produziu** a evidência, não o ciclo sobre o
  qual ela fala

### Estado de decisão

**`DEC-008` completa.** A trava de `DEC-010` sobre código de backend deixou de
valer por ausência de decisão. Continua valendo por ausência de ambiente —
motivo diferente, que precisa ficar separado. Compatibilidade visual da opção
adotada: **`NOT EXECUTED`**, não "preservada".

### Evidência

- `docs/evidence/ciclo4_topology_subset.log` — relação entre os níveis, com o
  histograma de `meshCount` sobre os 8.406 arquivos de nível 3
- `docs/evidence/ciclo4_git_incident.log` — reflog e diff de verificação
- `docs/evidence/ciclo4_dec008_submission_cost.log` — benchmark com hardware,
  flags, desvio padrão e estado de cache declarados

---

## Ciclo 3 — auditoria, correção de documentação e confirmação de escala

### Corrigido

- **A documentação afirmava que a topologia BMD não estava implementada em C++.
  Estava.** O parser lê triângulos desde o commit `bee2a5e`, há testes
  (`TestBmdTriangles`, `TestMeshFromBmd`) e o nível 4 do `mutool bmdreport` sai do
  parser. `COMPATIBILITY.md` e `ASSET_FORMATS.md` continuavam dizendo "não
  implementada" por dezenas de commits. Sexta ocorrência do padrão deste projeto:
  o trabalho avança e a documentação fica para trás
- **Hierarquia de ossos: o "98,0%" era de amostra.** Full-scan real: 98,68% sobre
  os 3.552 arquivos cujo tamanho fecha, o que é **85,09% dos 4.119 elegíveis** e
  **34,74% dos 10.090** da árvore. Os três números são verdadeiros; nenhum deles é
  "98% dos modelos"
- **BMP de altura: "44 dos 47" corrigido para 42.** O número anterior vinha de
  teste mais frouxo que conferia só o tamanho declarado; o critério que o parser
  de fato aplica dá 41 no offset 4 + 1 no offset 0
- Referências de canto fora de faixa (~8,4%) deixaram de ser listadas como
  pendência: o importador já as descarta e conta em `bad_indices`

### Adicionado

- **`tools/bmd_bones_probe.py`** — full-scan de hierarquia de ossos, sem teto, que
  imprime o denominador ao lado de cada taxa
- **`DEC-010`** — registro retroativo de que o frustum culling foi implementado
  antes da decisão formal de pipeline, com o teste de que o módulo é
  backend-agnóstico e a lista explícita do que continua bloqueado
- **Auditoria de licenciamento executada**, não afirmada: `git ls-files` não lista
  arquivo de terceiros, sem submódulos, 86 arquivos rastreados todos do projeto,
  atribuição transcrita do código-fonte

### Adicionado — segunda parte do ciclo

- **Nona hipótese de UV testada e rejeitada.** Genuinamente diferente das oito
  anteriores: procurava as **coordenadas** UV dentro do registro de triângulo, não
  um índice para o array. Deu 79,42% em [0,1] — contra controle de bytes aleatórios
  (24,82%) pareceria forte, razão 3,2×. **Esse controle é inválido**: a pergunta é
  "estes `f32` são UV", então o controle correto é outro `f32` que não seja UV. O
  array de normais dá **70,34%**, razão de apenas **1,13×**. Rejeitada
- **`docs/ARCHITECTURE.md` §10 e §11** — limites reais do BMD medidos (nove tetos
  com valor, método e consequência) e requisitos de escalabilidade de renderização
  com o que está e o que não está medido
- **Seção 9.1 corrigida** — dizia "a definir em código; ainda não implementado", e
  `mesh.hpp` existe com testes. A tabela nova separa o que está implementado do que
  está apenas declarado e nunca populado

### Regra nova

**O controle tem que ser da mesma classe do que se testa.** Registrada em
`ASSET_FORMATS.md`. Sem ela, a nona hipótese teria sido publicada como descoberta.

### Evidência

- `docs/evidence/ciclo3_test_final.log` — `make clean && make test`, 344/0, 0 warnings
- `docs/evidence/ciclo3_scan_final.log` — bmdreport + meshimport + terrain + bmdanim
- `docs/evidence/ciclo3_bones_fullscan.log` — hierarquia de ossos sem teto
- `docs/evidence/ciclo3_escala_medicoes.log` — população de cada medição
- `docs/evidence/ciclo3_licensing_audit.log` — confirmações de licenciamento

## Ciclo 2 (registro histórico; alegações de cifra não são status atual)

> **Aviso do ciclo 17:** esta nota preserva os termos e contagens usados naquela
> época. A cifra BMD v0x0C permanece **BLOCKED** em `AGENTS.md`; os 7.226 rótulos
> do classificador legado não provam cifra validada, autorização ou compatibilidade.
> Veja `COMPATIBILITY.md` e `TEST_RESULTS.md` para o estado atual.

### Mudanças registradas naquela época
- **Cifra BMD v0x0C** — XOR rolante sobre tabela de 16 B com chave corrente
  derivada do texto cifrado anterior. Nível 2 (payload em claro) de 1.496 <!-- historico -->
  (14,83%) para **8.571 (84,95%)**; nível 3 (geometria parseada) de 1.191 <!-- historico -->
  (11,80%) para **8.406 (83,31%)**. OPAQUE de 8.594 para 1.519. Proveniência: <!-- historico -->
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


## 2026-10-12 — DEC-014 reopened; D3D12 first GPU spike prepared

### Added
- platform/windows/d3d12_triangle.cpp: isolated Win32/D3D12 smoke-test application that requests a high-performance hardware adapter, creates a device/swap chain and draws a known triangle.
- scripts/build_d3d12_smoke.ps1: Windows SDK/MSVC build helper for the isolated smoke test.
- docs/RENDERER_API_SPIKE.md: criteria-based API comparison, run instructions, evidence checklist and official references.

### Changed
- DEC-014 records the modern-Windows/dedicated-GPU priority, formally reopens the renderer API decision and supersedes the old-hardware compatibility premise of DEC-008 for the current target.
- D3D12 is the first prototype only. D3D11 is the comparison baseline after the spike runs; Vulkan remains conditional.
- No Windows build/runtime test was possible in this environment. The new spike is NOT EXECUTED; no performance claim is made.


## 2026-10-13 — D3D12 target-PC smoke pass; D3D11 baseline added

### Added
- `platform/windows/d3d11_triangle.cpp`: isolated D3D11 hardware-device/swap-chain triangle smoke test with normal shutdown reporting.
- `scripts/build_d3d11_smoke.ps1`: MSVC build helper for the D3D11 baseline.

### Verified on target PC
- User reported AMD Radeon RX 6600, triangle visibly rendered, and `D3D12 smoke test: clean shutdown.` after closing the window normally.
- DXGI reported 8147 MiB dedicated video memory; this is adapter metadata, not measured runtime VRAM use.
- D3D11 build/runtime remain NOT EXECUTED. No benchmark or production API choice is claimed.


## 2026-10-13 — D3D11 target-PC smoke pass; both isolated APIs validated

### Verified on target PC
- D3D11 smoke executable ran on AMD Radeon RX 6600, reported feature level `0xB100`, visibly displayed the triangle, and printed `D3D11 smoke test: clean shutdown.` after normal window close.
- D3D12 smoke test also passed its triangle/clean-shutdown check on the same target PC.
- Evidence committed as `docs/evidence/d3d11_smoke_20261013.log` and `docs/evidence/d3d12_smoke_20261012.log` in commit `fe70970`.
- These are isolated smoke tests, not a playable client, original-asset render, benchmark, FPS result, or production API decision.

### Next
- Audit the existing BMD and texture paths to select one legitimate, verifiable model/texture pair without crossing the documented cipher blockers.
- Plan an integrated Windows GPU render of that asset; measure performance only after visual correctness is established.


## 2026-10-13 — source audit for first original-asset GPU slice

- Added `docs/ASSET_GPU_SLICE_AUDIT.md` documenting confirmed importer/texture limitations, the unverified Axe11/Axes06 diagnostic pairing, and the next evidence required before GPU integration.
- Linked the audit from `docs/NEXT_TASK.md` and updated `docs/CURRENT_STATE.md`.
- No C++ source, original game assets, or local files were changed. No build, unit test, full reference-tree scan, or target-PC run was performed as part of this GitHub-side audit. Existing D3D11/D3D12 smoke results remain isolated triangle tests only.


## 2026-10-13 — experimental D3D11 BMD preview source

### Added

- Added an isolated D3D11 preview for plaintext BMD v0x0A, reusing the existing
  parser/importer and uploading validated mesh indices to GPU buffers.
- Added a Windows PowerShell build script and instructions for target-PC
  validation.

### Not yet verified

- The new source has not been compiled or run on the target Windows PC in this
  session. It is untextured and does not establish UV/material association,
  animation, skinning, or playable-client status. See
  [D3D11_BMD_PREVIEW.md](D3D11_BMD_PREVIEW.md).
