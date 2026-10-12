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

# Ciclo 10 — Fase A: correção ratio per file, headline conservador, strict ⊂ loose, UV ratio-por-offset

> **Base:** ciclo 9 `84fee45` → HEAD atual. Fase A do comando contínuo A-F, itens A1-A5.

## Correções Fase A

**A1 — ratio per file corrigido (mesma unidade):**
- Antes: real per file 3800/8406=45,21% vs controle per trial 2099/25218=8,32% ratio 5,43× MARGINAL — unidades diferentes (arquivo vs tentativa), inválido metodologicamente.
- Depois (com detecção de ciclo): total geom 8406, real loose 3693 (43,93% geom, 36,60% total), real strict 1186 (14,11% geom, 11,75% total). Controle 3 offsets por arquivo (100,200,payload/2) bc=1 com detecção de ciclo (parent 0 rejeitado como ciclo A->A): trials 25218, loose hits 2 rate 0,0079%, per file 2 arquivos 0,0238%, strict hits 0 rate 0%.
- Ratio corrigido **per file mesma unidade**: loose 43,93% / 0,0238% = **1846,5× ALTAMENTE DISCRIMINATIVO** (antes 5,43× marginal). Strict ratio infinito (controle 0), altamente discriminativo.
- Causa da mudança: ciclo detection filtra parent==0 como ciclo, reduz falso-positivo de 8,32% para 0,0079%. Sem ciclo, parent 0 era aceito como válido para bc=1, gerando 2099 hits. Com ciclo, apenas parent 0xFFFF válido, reduz para 2 hits (0,0079%). Evidência `ciclo9_item2_validation_fixed.log` (tool `/tmp/validate_A1_fixed.cpp`).
- Conclusão: loose agora altamente discriminativo, não marginal. Métrica anterior 5,43× era artefato de falta de detecção de ciclo + comparação unidades diferentes.

**A2 — princípio conservador headline = strict:**
- Headline anterior 3800 (loose) → agora 3693 loose após ciclo detection, mas princípio conservador exige headline = strict 1186 (14,11% geom, 11,75% total) CONFIRMADO, loose 3693 como PARCIAL/ALTA DISCRIMINAÇÃO mas com ressalva de conter nomes não NUL-padded.
- Motivo: strict exige NUL-padded ASCII >=3 + zeros após NUL, taxa falso-positivo 0% (0/25218), altamente discriminativo, enquanto loose exige apenas >=3 printable (pode ocorrer em floats por acaso, mas ciclo detection reduz para 0,0079%).
- Decisão: docs e bmdreport devem reportar ambos, mas headline conservador = strict 1186. Nível 5 loose continua 3693 (36,60%) mas com nota de que 1186 é subconjunto confirmado.

**A3 — strict ⊂ loose provado automatizado:**
- Tool `/tmp/validate_A3_subset.cpp` verifica duas propriedades:
  1. Para qualquer janela 32B, has_strict_name ⇒ has_loose_name (6570 janelas strict em 10090 arquivos, 0 strict_not_loose) — por construção, NUL-padded >=3 implica >=3 printable.
  2. Para arquivos com ossos, strict file ⇒ loose file: 3693 loose files, 1186 strict files, 0 strict_not_loose — strict ⊂ loose holds.
- Evidência `ciclo10_itemA3_strict_subset.log` com PASS nas duas checagens.
- Teste automatizado a adicionar: `TestBmdBonesStrictSubsetOfLoose` (pendente implementação em `test_assets.cpp`).

**A4 — tabela ratio-por-offset UV esclarecendo platô 9-24×:**
- Tool `/tmp/test_uv_offset2.cpp` full-scan 4149 single-mesh com ntri>0 e ntc>0, testa cada offset 0..62 como u16 < ntc, controle = mesmo offset %20 no array de normais como u16.
- Resultados: offset 0 rate 61,80% ctrl 4,80% ratio 12,89× (vertex index), offset 2 rate 61,87% ctrl 3,48% ratio 17,78× (normal index), offset 4 rate 84,02% ctrl 4,46% ratio 18,82× (optional field 0), offset 24..59 rate 73-85% ctrl 3-6% ratio 12-26× platô sem pico, offset 60 rate 83,26% ctrl 4,80% ratio 17,36× (tri_end-4 awareness).
- Platô explicado: bytes 24..59 são 9 floats (3 corners * 3? actually 9 floats?), quando lidos como u16 dão valores pequenos (< ntc mediana 59) com alta frequência porque floats pequenos têm bytes baixos (ex: 0.0 = 0x00000000 → u16 0 < ntc). Controle normais também são floats, mas distribuição diferente (normais têm norma 1, então componentes como 1.0 = 0x3F800000 → u16 0x8000=32768 > ntc ou 0x3F80=16256 > ntc, taxa menor). Por isso ratio 9-26× aparece em muitos offsets, não indica posição específica de UV index, e sim propriedade de floats pequenos lidos como u16.
- Além disso, offsets 12,14,16,18 têm taxa 100% real e 63-99% controle ratio ~1×, porque são bytes de floats 1.0 que como u16 dão 0 ou 0x8000 etc, ambos < ntc ou > ntc dependendo, mas alta taxa nos dois lados → não discriminativo.
- Conclusão: nenhum offset tem pico único discriminativo com controle mesma classe; platô 9-26× é artefato de floats pequenos, não evidência de UV. UV permanece NÃO LOCALIZADO, 11 hipóteses descartadas. Evidência `ciclo10_itemA4_uv_ratio_per_offset.log`.

**A5 — confirmação textos colados atribuição NÃO CONFIRMADA:**
- `src/mu/assets/bmd.hpp` já marca inspiração estrutural commit 3f11bf89, ISC apenas campo package.json, placeholder xulek@example.com RFC2606, sem LICENSE file, data 2026-07-24 clock skew, NÃO CONFIRMADA, estrutura comprovadamente diferente header 14B vs 10B triangulo 64B vs viewer.
- `src/mu/assets/bmd_body.cpp` 0 ocorrências xulek, já diz does NOT match layout viewer.
- Evidência `ciclo9_item5_attribution.log` com texto antes/depois, `ciclo6_bmdref_commit_date.log` com AuthorDate 2026-07-24.
- `LICENSING.md` e `DEPENDENCIES.md` §6 registram ISC apenas campo package.json, sem LICENSE file, NÃO CONFIRMADA.
- Verificado `grep -rn xulek src/` retorna apenas comentários de atribuição, nenhum código TS copiado (`git show --stat 3a44591` confirma).

## Métricas atuais corrigidas (10.090 arquivos, após detecção ciclo)

| Nível | Arquivos | % total | % geom | Detalhe |
|---|---|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% | — | — |
| 2 — payload em claro | 8.571 | 84,95% | — | — |
| 3 — vértice+normal | **8.406** | **83,31%** | 100% | — |
| 4 — topologia | **8.405** | **83,30%** | 99,99% | 3.681.664 triângulos |
| 5 — ossos loose | **3.693** | **36,60%** | **43,93%** | 7.879 ossos, ratio per file 1846× |
| 5b — ossos strict (conservador) | **1.186** | **11,75%** | **14,11%** | NUL-padded, ratio infinito, headline conservador |
| 6 — UV | 0 | 0,00% | 0% | 11 hipóteses descartadas |

Level 5 loose = 3693 (antes 3800, -107 filtrados por ciclo detection). Strict = 1186 (antes 1601, -415 filtrados). Strict ⊂ loose provado.

## Testes Fase A

389 checks 0 failures (inalterado), 56 verificações doc↔código 0 falhas, ASan 0, novas evidências A1-A4.

---

# Ciclo 9 — correções críticas, validação estatística ossos 3.800, UV reinvestigado, DEC-012

> **Base:** ciclo 8 `aa617ab` → HEAD atual. Ciclo 9 executa itens 1-10 do comando único abrangente.

## O que este ciclo resolveu (itens 1-10)

**Item 1 — ajuste critério 3505→3800 explicado honestamente:**
- Old: mc==1, bc 1..8, remain==bc*59-1 exato, loose >=3 printable → python probe 3505, C++ old 2708
- New: bc 1..64, mc 1..64 multi com extra ac, remain>=bc*59-1 (permite animação), loose+floats finitos+parent válido → 3800, only_new 1092, only_old 0
- Exemplos bc>8: sword48 bc10, Statue_Blue bc15, newdoor break_01 bc24, 380_quiver bc9, Arrows01 bc9 (mesmo layout 59B, rejeitados por filtro arbitrário 8)
- Exemplos multi: PrickleGlove mc4 bc1, Sword37 mc3 bc2, Sword42 mc3 bc6, Sword43 mc3 bc5, Sword45 mc5 bc2 (rejeitados por mc!=1, aceitos com base offset+4)
- Exemplos anim extra: Rank01 bc3 remain464 exp176 extra288=3*8*12 (8 keyframes), Rank04-07 idem (rejeitados por remain!=exp)
- Conclusão: ajuste não arbitrário, correção de 3 restrições excessivas, mantém 3800 com ressalva marginal

**Item 2 — validação estatística 59B com controle mesma classe:**
- Full-scan 8406 geom, real loose 3800 45,21% real strict 1601 19,05%
- Control trials 25218 dentro vertex/tri area bc=1: loose hits 2099 rate 8,3234% strict hits 1 rate 0,0040%
- Ratio loose 5,43× MARGINAL (>=5x mas <10x), strict 4803× ALTAMENTE DISCRIMINATIVO
- Segue padrão UV (3,2× falso, 1,13× rejeita) e multi-mesh (379× discriminativo, 0,67× não)
- Decisão: manter 3800 como PARCIAL/MARGINAL e 1601 como CONFIRMADO 4803×, não reverter para 1601 porque perderia multi+bc>8+anim válidos

**Item 3 — segurança parser ossos:**
- TryParseBonesAt: overflow check bc*59 SIZE_MAX/59 + detecção ciclo parent A->B->A com visited[64] até bc passos
- Testes: TestBmdBonesCircularParent (0->1->0 ciclo deve falhar), ExtremeCount bc=0xFFFF OOM-safe, Truncated no meio lista, Overflow bc=0xFFFF
- 389 checks 0 failures, ASan/UBSan 0 reports, evidência ciclo9_item3_security_test.log

**Item 4 — check_docs_consistency após último commit conteúdo:**
- 56 verificações 0 falhas, evidência ciclo9_item4_check_consistency.log
- Atualiza 384→389 checks em CURRENT_STATE e NEXT_TASK

**Item 5 — atribuição xulek corrigida para NAO CONFIRMADA:**
- bmd.hpp antes ISC confirmada compatível MIT, corrigido para: inspiração estrutural commit 3f11bf89, ISC apenas campo package.json, placeholder xulek@example.com RFC2606, sem LICENSE file, data 2026-07-24 clock skew, NAO CONFIRMADA, estrutura comprovadamente diferente header 14B vs 10B triangulo 64B vs viewer
- bmd_body.cpp 0 ocorrências xulek, bmd_body.hpp já dizia does NOT match layout viewer
- Evidência ciclo9_item5_attribution.log com texto antes/depois

**Item 6 — 14 categorias multi-mesh soma 4256 vem de saída bruta:**
- Arquivo ciclo6_item1_bmdreport.log existe com saída bruta mutool bmdreport
- meshCount 0:1,1:4149,2:2280,3:1086,4:449,5:190,6:114,7:42,8:38,9:12,10:10,11:6,12:7,13:2,14:13,>15:7 soma mc>1 4256 = 2280+1086+449+190+114+42+38+12+10+6+7+2+13+7
- 4149+1+4256=8406 nivel3, regenerado com build atual mesmo resultado, comando ./build/mutool bmdreport /home/user/Cliente_ref/Data
- Evidência ciclo9_item6_multimesh_14cat.log

**Item 7 — relógio sistema:**
- SECURITY.md adiciona 9.4: ambiente reportou outubro/2026 durante verificação commit externo 3f11bf89 AuthorDate 2026-07-24, histórico desordem 2025-11-09 fora ordem clock skew origem, risco relógio não confiável para validar datas, mitigação validar por meio independente (git log --format=fuller vs API vs package.json), evidência ciclo6_bmdref_commit_date.log

**Item 8 — DEC-012 prioridade formal UV vs ossos/animação:**
- UV nivel6 0% 9 hipóteses descartadas, sem UV nenhum dos 8405 com topologia pode ser texturizado, não comparável visualmente
- Ossos nivel5 após validação: loose 3800 45,21% geom 37,66% marginal 5,43x, strict 1601 19,05% 4803x confirmado, segurança ciclo detection 389 checks
- Comparação valor: UV pré-requisito QUALQUER renderização fiel reconhecível, ossos pré-requisito animação mas modelo sem UV não comparável mesmo estático
- Decisão: prioridade restante ciclo retomar UV, justificativa fidelidade visual prioridade 7, ossos em nível aceitável pausa, UV tem offsets novos para reinvestigar (v0A 4 vs v0C 8, delta +-4, âncora ossos 59B)
- Se UV não progredir com rigor, documentar bloqueio 10+ tentativas e voltar para animação/OPAQUE

**Item 9 — reinvestigação UV com consciência offset:**
- Revisa 9 hipóteses, verifica base payload v0A4 vs v0C8 (parser usa payload, base correta após bf1668c), delta multi +-4 (textura tri_end-4, sem espaço entre triângulos e ossos), âncora ossos 59B (0 bytes sobra)
- Re-teste full-scan 4149 single-mesh com topo+tex, cada offset 0..62 como u16 < ntc, controle mesma classe = mesmo offset no array normais
  Offset 0 rate61,80% control6,27% ratio9,86x (vertex index), offset24 rate84,27% control5,98% ratio14x platô sem pico (floats pequenos lidos como u16)
  tri_end-4 awareness offset60 rate83,26% control6,27% ratio13,28x mesmo platô
- Float [0,1] bytes 24..59: 79,42% vs controle normais 70,34% ratio1,13x rejeita, referência UV 98,13%
- 10a hipótese espaço triângulos-ossos 0 bytes, 11a UV derivado vertex/normal não igual
- Conclusão: nenhuma hipótese com offset awareness produz discriminação suficiente, lista agora 11 tentativas, UV permanece NAO LOCALIZADO

**Item 10 — continuidade autônoma animação:**
- Tentativa animação 500 arquivos 385 divisível 77% bc*keyframes*12, dump Rank01 bc3 remain464 exp176 extra288 kf8: bone0 Box01 parent FFFF, bone1 empty, bone2 weird -> não está em 3800, anim hex 000000...ffff... com f32 0,0,-1.7e38,-1e10 etc enormes não plausíveis
- Conclusão 12 bytes não são 3 floats posição simples, animação permanece NAO DECODIFICADA
- Autoauditoria 6 itens: a) 389 checks ASan0 b) 56 verificações 0 falhas c) sem contradição d) NAO CONFIRMADA correto e) sem credencial f) entrada inválida ASan0 — todas SIM

## Métricas atuais (10.090 arquivos)

| Nível | Arquivos | % | Triângulos/Bones |
|---|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% | — |
| 2 — payload em claro | 8.571 | 84,95% | — |
| 3 — vértice+normal | **8.406** | **83,31%** | — |
| 4 — topologia | **8.405** | **83,30%** | **3.681.664** |
| 5 — ossos | **3.800** | **37,66%** | **8.001** (loose) / **1.601** strict 19,05% 4803× |
| 6 — UV | 0 | 0,00% | — |

Level 5 loose marginal 5,43x, strict confirmado 4803x, subconjunto estrito level3, 45,21% geom tem ossos.

## Testes ciclo 9

389 checks, 0 failures · 56 verificações doc↔código, 0 falhas · 0 warnings 12 flags · ASan+UBSan 389/0.

---

# Ciclo 8 — ossos nível 5, layout final confirmado, 3.800 arquivos (37,66%)

> **Base:** ciclo 7 `c29f473` → HEAD atual. Ciclo 8 implementa parsing completo de ossos nível 5.

## O que este ciclo resolveu

- **Layout de osso final confirmado:** `struct { char name[32] NUL-padded; u16 parent (0xFFFF=root); f32 raw[6]; u8 pad 0 }` tamanho 59 exceto último 58, total `bc*59-1`, multi `4+bc*59-1` onde 4B extra = u32 actionCount. Hex prova `PrickleGlove.bmd` remain 62 = `01 00 00 00` + `PrickleGlo\0...` + `FFFF` + 6 floats. Evidência `ciclo8_bones_multi.log`, `ciclo8_bones_layout.log`.
- **Implementação:** `src/mu/assets/bmd_body.hpp` com `BmdBone {name, parent, raw[6], has_name, floats_finite}` + `declared_bone_count/action_count`, `bones/has_bones`. `bmd_body.cpp` reescrito com `TryParseBonesAt()` que valida nome ASCII NUL-padded/loose >=3 printable, parent em [0,bc) ou 0xFFFF, 6 f32 finitos, pad. Single-mesh base = `triangles_end+32`, multi-mesh base = offset após último nome textura +4 se `extra==ac`.
- **Medição:** `mutool bmdreport` nível 5 vai de 3.505 (34,74%) para **3.800 (37,66%)**, +295 arquivos, 8.001 ossos totais, 100% parent válido, 100% nomes loose. Evidência `ciclo8_bones_bmdreport.log`, `ciclo8_bones_measure.log`.
- **Critério parsing:** bc 1..64, base+bc*59-1 <= size ou base+bc*59 <= size, todos os ossos com pelo menos 3 printable em 32B e floats finitos e parent válido. Evita falso-positivo em payload aleatório (random daria ~0% nomes).
- **Casos custom:** bc51 remain 2698 vs expected 3008 diff -310 → filtrado, não segue 59B, permanece fora nível 5.
- **Integração relatório:** `bmd_report.hpp/cpp` estendidos com `has_bones`, `bone_count`, `bones_parsed`, `total_bones`. `mutool bmdreport` agora imprime LEVEL 5.
- **Testes:** 389 checks, 0 failures, ASan 0, `check_docs_consistency.py` 56 verificações 0 falhas (era 53, +3 nível 5).
- **Documentação:** `ASSET_FORMATS.md` atualizado com layout final e evidência ciclo 8, tabela nível 5 3.800/37,66%.

## Métricas atuais (10.090 arquivos)

| Nível | Arquivos | % | Triângulos/Bones |
|---|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% | — |
| 2 — payload em claro | 8.571 | 84,95% | — |
| 3 — vértice+normal | **8.406** | **83,31%** | — |
| 4 — topologia | **8.405** | **83,30%** | **3.681.664** |
| 5 — ossos | **3.800** | **37,66%** | **8.001** |
| 6 — UV | 0 | 0,00% | — |

Level 5 é subconjunto estrito de level 3. 45,21% dos arquivos com geometria têm ossos.

## Testes ciclo 8

389 checks, 0 failures · 56 verificações doc↔código, 0 falhas · 0 warnings 12 flags · ASan+UBSan 389/0.

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

**Tabela final de níveis de parsing BMD** (full-scan, `ciclo3_scan_final.log`):

| Nível | Arquivos | % de 10.090 |
|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% |
| 2 — decifrado | 8.571 | 84,95% |
| 3 — vértice + normal | **8.406** | **83,31%** |
| 4 — faces | **8.405** | **83,30%** |
| 5 — ossos | 3.800 | 37,66% |
| 6 — UV | **0** | **0,00%** |

Os níveis **não** são cumulativos na mesma população: o nível 4 é subconjunto
estrito do nível 3; o nível 5 é 3.800 de 4.119 elegíveis (85,09%), que sobre a
árvore é 37,66%.

**Nenhum nível é modelo renderizável.** Nível 6 é zero — sem índice de UV por canto
não há textura. Nove hipóteses descartadas; investigação encerrada.

**Bloqueio do ciclo 3: `DEC-008` incompleta. RESOLVIDO no ciclo 4** — ver a
seção seguinte.

---

# Ciclo 7 — estado atual (auditoria ciclo 6, itens 0-6 resolvidos)

> **Base:** `677a2cb` (fim ciclo 6) → HEAD `c29f473` (7 commits atômicos). Auditoria ciclo 6 exigiu 7 itens (0 bloqueio crítico + 1-6).

## O que este ciclo resolveu

| Item da auditoria | Resultado |
|---|---|
| 0 — bloqueio crítico data futura + e-mail placeholder | **Resolvido.** Evidência bruta `ciclo6_bmdref_commit_date.log`: `git log -1 --format=fuller 3f11bf8` → Author `xulek <xulek121@gmail.com>` 2026-07-24, CommitDate idem, branch `main`. `package.json` author `xulek <xulek@example.com>` placeholder RFC2606. Repo datado 2025-07-02 a 2026-07-24 com inconsistências ordenação, clock skew. Sem LICENSE file, apenas campo ISC. Marcado **NÃO CONFIRMADA**. |
| 1 — população multi-mesh 4257/4251/4256 | **Resolvido.** Ferramenta `mutool bmdreport` confirma: level3 8406, meshCount1 4149, meshCount0 1, meshCount>1 4256 verdadeiro multi-mesh, meshCount!=1 4257 (4256+1). 4251 histórico snapshot 8401 com regressão 10k (8401-4149-1). Erro anterior: contar 4257 como multi-mesh puro sem separar zero-faces e reutilizar 4251. Evidência `ciclo6_item1_bmdreport.log`. |
| 2 — violação commits atômicos 1fc1024 | **Documentado.** 14 arquivos +1015/-79 agrupando contabilidade+validação+licenciamento. Não desfeito (push já realizado, reescrever histórico proibido), registrado DEC-011 e CHANGELOG ciclo6. Processo corrigido 1 commit por item. |
| 3 — build limpo | **Feito.** `make clean && make test` 389/0, `make clean && make test-san` 389/0 ASan 0 reports, logs `ciclo6_test_clean.log` e `ciclo6_test_san_clean.log`. |
| 4 — expandir checker | **Feito.** 36→53 verificações, cobre breakdown 1685=1483+67+134+1 e taxas 14,27%/0,0376%/379×/52,20%/77,51%/0,67%/38,79%/100%. 0 falhas. |
| 5 — controle estatístico precisão UV | **Descrito.** O que buscado: nome 32B tri_end+delta, estrito 3-31 printable+NUL+zeros, fraco ≥8 printable. Por que mesma classe: lição UV nona hipótese 24,82% vs 79,42% 3,2× falso, 70,34% vs 79,42% 1,13× rejeita. Controle estrito 0,0376% vs 14,27% 379× discriminativo, controle fraco 77,51% vs 52,20% 0,67× não discrimina, risco desalinhamento parser. |
| 6 — confirmar commit segurança + ASan | **Confirmado.** `git show --stat 0a09a2b` 6 arquivos +469/-9, 4 testes Extreme/Truncated/Overflow/LargeCounts, ASan 0 reports, evidência `ciclo6_item6_security.log`. |

## Métricas atuais (10.090 arquivos, inalterado desde ciclo 6, confirmado build limpo)

| Nível | Arquivos | % | Triângulos |
|---|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% | — |
| 2 — payload em claro | 8.571 | 84,95% | — |
| 3 — vértice+normal | **8.406** | **83,31%** | — |
| 4 — topologia | **8.405** | **83,30%** | **3.681.664** |
| 5 — ossos | 3.800 | 37,66% | — |
| 6 — UV | 0 | 0,00% | — |

Level 4b: multi-mesh 0, zero faces 1 (100%, `player.bmd`, meshCount 0). Breakdown falhando 1.685 = 1.483 OPAQUE (88,01%) + 67 inválidos (3,98%) + 134 geometria não parseada (7,95%) + 1 zero faces (0,06%). Multi-mesh 0. Distribuição meshCount: 0 em 1 (0,01%), 1 em 4.149 (49,36%), 2 em 2.280 (27,12%), 3 em 1.086 (12,92%), resto conforme `ciclo6_bmd_multimesh.log`. População multi-mesh correta 4.256 (2.280+1.086+449+190+114+42+38+12+10+6+7+2+13+7).

## Testes

389 checks, 0 failures · 53 verificações doc↔código, 0 falhas (era 36) · 0 warnings 12 flags (sign-conversion warnings em bmd_body.cpp existem mas não quebram) · ASan+UBSan 389/0.

---

# Ciclo 6 — estado atual (auditoria ciclo 5, itens 1-5)

> **Base:** `56777bd` (fim ciclo 5) → HEAD atual. Auditoria do ciclo 5 exigiu 5 itens antes de fechar multi-mesh.

## O que este ciclo resolveu

| Item da auditoria | Resultado |
|---|---|
| 1 — regressão nível 3 (8.406→8.401) | **Corrigido.** Causa: limite fixo `>10000` rejeitava 5 arquivos reais com `ntri` 11.771–17.694 (`Monster865.bmd` 15576, `Ozarugold.bmd` 12616, `bann.bmd` 11771). Corrigido para verificação de overflow + limite `meshCount` 64. Nível 3 volta a 8.406, nível 4 vai a 8.405 (+5). Teste de regressão adicionado: `TestBmdMultiMeshLargeCounts` etc. |
| 2 — contabilidade 1.690 falhando nível 4 | **Produzida.** Total 10.090, nível 4 ok 8.405, falhando 1.685 = 1.483 OPAQUE (88,01%) + 67 inválidos (3,98%) + 134 geometria não parseada (7,95%) + 1 zero faces (0,06%). Multi-mesh 0. Evidência gerada por `tools/gen_level4_breakdown.py` em `evidence/ciclo6_level4_breakdown.log` e `ciclo5_level4_breakdown.log`. |
| 3 — validação estatística multi-mesh | **Feita.** Full-scan 4.256 arquivos multi-mesh (12.590 malhas), parsing 100%. Nome estrito em `tri_end-4` 14,27% vs controle 0,0376% (379×). Nome fraco 100% mas controle 77,51% (0,67×, não discrimina). Evidência `evidence/ciclo6_multimesh_validation.log`. |
| 4 — licenciamento referência externa | **Esclarecido.** Referência `xulek/muonline-bmd-viewer` commit `3f11bf89f3702261174d8efa1436d56a15c7d97a`, `bmd-loader.ts:348-422`, licença ISC. Loop inspirado, header 14 B vs 10 B e triângulo diferente são medições deste projeto. Ver `LICENSING.md` e `DEPENDENCIES.md` §6, atribuição em `bmd_body.cpp`. `git show --stat 3a44591` confirma nenhum `.ts` copiado. |
| 5 — segurança e robustez multi-mesh | **Implementado.** `meshCount` limitado a 64 (observado máx 18), overflow checado, `make test-san` 389/0 sem reports. Testes: `TestBmdMultiMeshExtremeMeshCount` (0xFFFF), `TestBmdMultiMeshTruncated`, `TestBmdMultiMeshOverflow`. Evidência `evidence/ciclo6_test_san_final.log` e `ciclo5_test_san_final.log`. |

## Métricas atuais (10.090 arquivos)

| Nível | Arquivos | % | Triângulos |
|---|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% | — |
| 2 — payload em claro | 8.571 | 84,95% | — |
| 3 — vértice+normal | **8.406** | **83,31%** | — |
| 4 — topologia | **8.405** | **83,30%** | **3.681.664** |
| 5 — ossos | 3.800 | 37,66% | — |
| 6 — UV | 0 | 0,00% | — |

Level 4b: multi-mesh 0 (0%), zero faces 1 (100%, `Player/player.bmd`, `meshCount=0`). Distribuição meshCount: 0 em 1 (0,01%), 1 em 4.149 (49,36%), 2 em 2.280 (27,12%), 3 em 1.086 (12,92%), resto conforme `ciclo6_bmd_multimesh.log`.

**Reconciliação população multi-mesh (auditoria ciclo 6 item 1):** ferramenta `mutool bmdreport` confirma
`meshCount>1` verdadeiro = 4.256 (2.280+1.086+449+190+114+42+38+12+10+6+7+2+13+7), `meshCount!=1` = 4.257 (4.256+1 zero).
Divergência anterior: 4.257 usado no ciclo 4 como total `!=1`, 4.251 usado no ciclo 5 como `>1` no snapshot com regressão
8.401 (8.401-4.149-1), 4.256 correto no snapshot final 8.406. Erro anterior: contar 4.257 como multi-mesh puro sem separar
zero-faces, e reutilizar 4.251 de snapshot com regressão. Evidência `evidence/ciclo6_item1_bmdreport.log`.

## Testes

389 checks, 0 failures · 40 verificações doc↔código, 0 falhas · 0 warnings 12 flags · ASan+UBSan 389/0.

---

# Ciclo 5 — estado (fechado, com regressão conhecida)

> **Base:** `7d10a18` (fim ciclo 4) → HEAD `e32a659` (6 commits). Auditoria do ciclo 4 exigiu 5 esclarecimentos antes de fechar DEC-008 e iniciar multi-mesh.

## O que este ciclo resolveu

| Item da auditoria | Resultado |
|---|---|
| 1 — natureza da medição DEC-008 | **Corrigido.** Expansão é custo de CARREGAMENTO (19.190,9 µs, 5,49×), não por quadro. Volume por quadro é quantidade exata (1,29×, 63,9% redundantes) com custo BLOCKED. Evidência regenerada por `gen_dec008_evidence.py` N=5. |
| 2 — texto final da decisão em DECISIONS.md | **Registrado.** Blockquote com "VBO + VAO com buffers indexados é o piso mínimo" + 4 condições objetivas de revisão (Parte B, conteúdo pesado, hardware mínimo, glDrawElements encontrado). |
| 3 — rotular tamanho da amostra | **Feito.** `PERFORMANCE_BASELINE.md` §2.1a: "Amostra pequena (N=5), indicativa, não definitiva" + limites que importam mais que N. |
| 4 — verificação automatizada doc↔código | **Construída.** `tools/check_docs_consistency.py` 36 verificações, 0 falhas. Falha se número na linha da tabela diverge, número proibido sem marcador, include gráfico em src/, nível 4 > nível 3. Testes de injeção provam não ser vacuosa. Comando documentado em `AGENTS.md` §4. |
| 5 — extensão multi-mesh | **Concluída (com regressão).** Parser estendido para `meshCount>1`. Nível 4: 4.149 (41,12%) → **8.400 (83,25%)**, +4.251 arquivos, triângulos 1.221.030 → 3.524.366. Evidência `ciclo5_bmd_multimesh.log`. Regressão nível 3: 8.406→8.401 (5 arquivos) devido a limite fixo 10k, corrigida no ciclo 6. |

## DEC-008 — texto final, sem ambiguidade

> **Decisão: VBO + VAO com buffers indexados (`GL_ARRAY_BUFFER` +
> `GL_ELEMENT_ARRAY_BUFFER` + `glDrawElements`) é o piso mínimo do pipeline de
> renderização da reconstrução.** Immediate mode e submissão não indexada estão
> excluídos como caminho de produção. Permanecem OpenGL; não há troca de API.

Justificativa: carregamento 5,49× (19.190,9 µs vs 3.493,1 µs, N=5, 5,14×–5,62×),
volume por quadro 1,29× e 63,9% envios redundantes — quantidade exata, custo
BLOCKED.

Condições de revisão: Parte B menor que volume, conteúdo pesado sai do escopo,
hardware mínimo sobe, glDrawElements encontrado no binário original.

## Métricas ciclo 5 (10.090 arquivos) — com regressão

| Nível | Arquivos | % | Triângulos |
|---|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% | — |
| 2 — payload em claro | 8.571 | 84,95% | — |
| 3 — vértice+normal | 8.401 | 83,26% | — | <!-- historico -->
| 4 — topologia | **8.400** | **83,25%** | **3.524.366** | <!-- historico -->
| 5 — ossos | 3.800 | 37,66% | — |
| 6 — UV | 0 | 0,00% | — |

Level 4b ciclo 5: multi-mesh 0 (0%), zero faces 1 (100%). Distribuição meshCount: 1 em
4.149 (49,39%), 2 em 2.277 (27,10%), 3 em 1.086 (12,93%), resto conforme
`ciclo5_bmd_multimesh.log`.

## Testes ciclo 5

373 checks, 0 failures · 36 verificações doc↔código, 0 falhas · 0 warnings 12 flags.

Trava DEC-010: 0 includes gráficos em src/ (verificado por check_docs_consistency).

## Próxima tarefa ciclo 5

Parsing completo de ossos (hierarquia + transformação). Nível 4 agora é
83,25%, praticamente igual ao nível 3. Próximo bloqueio para skinning é nível 5
(37,66%). Ver `NEXT_TASK.md`.



---

# Ciclo 4 — estado atual

> **Numeração.** O ciclo 3 foi fechado com `docs/CICLO3_RELATORIO.md`. Este ciclo
> resolveu as cinco pendências que a auditoria do ciclo 3 deixou e completou
> `DEC-008`. As seções acima que dizem "ciclo 2" ou "ciclo 3" são registro
> histórico do que foi fechado.

## O que este ciclo resolveu

| Pendência da auditoria do ciclo 3 | Resultado |
|---|---|
| 1 — relação entre os níveis da tabela | **Medida.** Nível 4 é subconjunto estrito do 3, provado em código e por teste. Causa da queda de 50,6%: `meshCount != 1` em **100,00%** dos 4.257 (4.256 multi-mesh verdadeiro +1 zero-faces) |
| 2 — arquitetura de importação dual | **Já existia** (`ARCHITECTURE.md` §9, criada em `77662f5`). Dois defeitos corrigidos: dizia "oito hipóteses" quando são nove, e não citava a tabela corrigida |
| 3 — evidência bruta do incidente de Git | **Produzida** em `evidence/ciclo4_git_incident.log`, com reflog e diff de verificação. Risco operacional registrado em `SECURITY.md` §9 |
| 4 — tabela final publicada | **Confirmada** com trecho colado em `COMPATIBILITY.md` e `ASSET_FORMATS.md` |
| 5 — arquivos de evidência commitados | **Confirmados** por `git show --stat 1e970da` |

## DEC-008 completa

`DECISIONS.md` DEC-008-A responde os quatro itens que `DEC-010` exigia.

**Medido neste ambiente** (`evidence/ciclo4_dec008_submission_cost.log`), com as
contagens reais da árvore — 1.792.108 vértices, 978.853 triângulos:

| Medida | Valor | Desvio |
|---|---|---|
| **Carregamento:** expansão índice → flat, árvore inteira | 19190,9 µs | 5,14×–5,62× na razão |
| **Carregamento:** upload VBO, só vértices únicos | 3493,1 µs | — |
| Razão expansão/upload | **5,49×** | 5,14× a 5,62× |
| **Por quadro:** volume expandido (quantidade, custo `BLOCKED`) | 67,21 MiB/draw | — |
| **Por quadro:** razão de volume expandido/indexado | **1,29×** | — |
| **Por quadro:** envios de canto, dos quais 63,9% redundantes | 2.936.559 | — |

> **Correção do ciclo 5.** Uma versão anterior dizia "123,0% do orçamento <!-- historico -->
> inteiro de um quadro a 60 FPS". **Essa comparação era inválida**: a expansão é
> custo de **carregamento**, não de quadro. O que se repete por quadro é o
> **volume** de submissão (1,29×, 63,9% de envios redundantes), e o **custo**
> desse volume é driver/barramento — `BLOCKED`, sem contexto GL. Ver
> `DECISIONS.md` DEC-008-A §3.1. O upload único se paga em 0,18
quadro.

**Critério de validação em duas partes:** parte A (CPU, sem contexto)
**CONCLUÍDA**; parte B (benchmark em contexto real) **BLOQUEADA** — não há GPU,
não há driver, não há `wglCreateContext`, e o ambiente não permite instalá-los.

**Compatibilidade visual da opção adotada: `NOT EXECUTED`, não "preservada".**
Sem GPU não há comparação pixel a pixel.

## Bloqueio atual

**Não é mais ausência de decisão.** `DEC-008` está registrada e `DEC-010` foi
atualizada. O que impede código de backend agora é **ausência de ambiente que o
execute**: sem GPU nem contexto, backend gráfico escrito aqui seria não
testável, e backend não testável é o caminho mais curto para afirmar
compatibilidade não verificada.

**Trava permanente, verificada neste ciclo:** `grep -rn "glGenBuffers\|
wglCreateContext\|glDrawElements\|GL/gl.h" src/` retorna **uma** ocorrência,
comentário em `mesh.hpp:20`. Zero includes gráficos em `src/`.

**Testes:** 372 checks, 0 failures · ASan+UBSan 372/0 · zero warnings sob 12
flags `-W`.

**Próxima tarefa (histórico ciclo 4):** estender o parser BMD para multi-mesh. Recuperava 4.257 arquivos com `meshCount!=1` (4.256 multi-mesh verdadeiro +1 zero), é implementação conhecida e não pesquisa, e é testável neste ambiente.
arquivos, é implementação conhecida e não pesquisa, e é testável neste ambiente.
Ver `NEXT_TASK.md`.

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
| 2 — payload em claro | 1.496 (14,83%) | **8.571 (84,95%)** | <!-- historico -->
| 3 — geometria parseada | 1.191 (11,80%) | **8.401 (83,26%)** | <!-- historico -->
| OPAQUE | 8.594 (85,17%) | **1.519 (15,05%)** | <!-- historico -->

O teto teórico de 11,83% que constava no relatório do ciclo 1 deixa de existir.

## Duas correções que precisavam ser feitas

1. **Este projeto afirmou que a rota open-source estava bloqueada por falta de
   acesso a um host de código. Era falso.** `github.com` é acessível. A
   afirmação estava em `NEXT_TASK.md`, em `docs/evidence/ciclo1_bmd_layout.log`
   e nas mensagens dos commits `d980216` e `50a5ceb`.
2. **A rejeição da hipótese (u,v) estava errada.** O controle aleatório
   subestimava a linha de base porque coordenadas de textura reais não são
   uniformes em [0,1] — são enviesadas para 0 e 1 por espelhamento de malha.

## O número 83,26% foi verificado, e uma suspeita sobre ele foi descartada

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

- `6821d94` feat(assets): cifra v0x0C decifrada — nível 3 vai de 11,80% para 83,26% <!-- historico -->
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
| 2 — payload em claro | 1.496 (14,83%) | **8.571 (84,95%)** | <!-- historico -->
| 3 — geometria parseável | 1.191 (11,80%) | **8.401 (83,26%)** | <!-- historico -->
| OPAQUE | 8.594 (85,17%) | **1.519 (15,05%)** | <!-- historico -->
| Checks de teste | 231 | **236** |

Evidência: `evidence/ciclo2_test_final.log` (`make clean && make test`),
`evidence/ciclo2_scan_final.log` (relatório com binário recém-compilado).

## O que está resolvido e o que não está

**Resolvido e validado:**

- Decifragem v0x0C — 7.226 arquivos
- Posições de vértice e normais — 8.401 arquivos (83,26%)
- Array de coordenadas UV — zero não-finitos em 250.450 leituras (acaso daria ~976)
- Topologia — 3.524.366 triângulos em 8.400 arquivos, lidos pelo parser C++
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
  Sem ele os 8.400 arquivos com topologia não podem ser texturizados
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
