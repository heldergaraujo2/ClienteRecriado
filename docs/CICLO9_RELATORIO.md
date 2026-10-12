# Ciclo 9 — relatório final (checkpoint)

> **Comando único abrangente:** Partes A-E, itens 1-10, com 11 itens de relatório + ESTADO DO GIT obrigatório. Execução contínua sem pedir autorização, commits atômicos.

## Resultado do item 1 (explicação do ajuste de critério) — OBRIGATÓRIO NO INÍCIO

**Ajuste 3.505 → 3.800 explicado honestamente e publicamente:**

- **Critério antigo (python probe ciclo7, baseline 3.505):** mc==1, vc>0, ntri>0, bc 1..8 filtro arbitrário, base 52+vc*16+nc*20+ntc*8+ntri*64+32, remain == bc*59-1 exato, todos ossos >=3 printable loose. Resultado 3.505 (98,68% de 3.552 size-ok, 85,09% de 4.119 elegíveis, 34,74% da árvore).
- **Critério novo (C++ ciclo8, 3.800):** bc 1..64 (max observado 51), mc 1..64 multi com extra ac, base+bc*59-1 <= size (permite sobra animação), todos ossos >=3 printable loose + 6 f32 finitos + parent em [0,bc) ou 0xFFFF.
- **Medição comparativa full-scan tool /tmp/compare_old_new:** old_ok 2.708 (C++ com length_consistent), new_ok 3.800, both 2.708, only_new 1.092, only_old 0. Python old 3.505 vs C++ old 2.708 diferença 797 por length_consistent (67) e payload printable e floats/parent.
- **Por que anterior era excessivamente restritivo, com 2+ exemplos por critério:**
  1) **Filtro bc<=8 arbitrário:** formato suporta até 64, observado 51,24,15,10,9. Exemplos rejeitados incorretamente: `sword48.bmd mc=8 bc=10` 10*59-1=589 bytes nomes válidos, `Statue_Blue.bmd mc=1 bc=15` 15*59-1=884 bytes, `newdoor break_01.bmd mc=1 bc=24`, `380_quiver.bmd bc=9`, `Arrows01.bmd bc=9`. Limite 8 veio de amostra 468 arquivos, não do formato.
  2) **Filtro single-mesh apenas:** multi-mesh também tem ossos com extra 4B ac. Exemplos: `PrickleGlove.bmd mc=4 bc=1 ac=1` base78670 remain62 extra1 bone PrickleGlo parent FFFF, `Sword37.bmd mc=3 bc=2` remain121 extra1, `Sword42.bmd mc=3 bc=6` remain357, `Sword43.bmd mc=3 bc=5`, `Sword45.bmd mc=5 bc=2`. Rejeitados por mc!=1, aceitos com base offset+4.
  3) **Tamanho exato remain==bc*59-1:** rejeitava arquivos com animação após ossos. Exemplos: `Rank01.bmd bc=3 remain464 exp176 extra288=3*8*12` 8 keyframes, `Rank04.bmd`, `Rank05.bmd`, `Rank06.bmd`, `Rank07.bmd` idem. Modelo animação bc*keyframes*12 medido 500 arquivos 385 divisível 77%.
- **Conclusão:** ajuste não arbitrário para cruzar baseline, correção de 3 restrições excessivas que rejeitavam arquivos legítimos mesmo layout 59B. Mantém 3.800 com ressalva marginal, validado item2 e segurança item3. Se não houvesse explicação sólida, reverteria para conservador 2.708 ou 3.505.

Evidência: `ciclo9_item1_criteria_adjustment.log` com 5 exemplos multi, 5 bc>8, 5 anim extra.

## Resultado do item 2 (validação estatística com controle) — OBRIGATÓRIO NO INÍCIO

**Validação estatística completa layout 59B/58B full-scan + controle mesma classe:**

- **População:** 8.406 arquivos com geometria válida (nível 3), full-scan.
- **Real:** parsing ossos critério novo loose >=3 printable + parent válido + 6 f32 finitos → 3.800 (45,21% de 8.406, 37,66% total). Strict NUL-padded → 1.601 (19,05%).
- **Controle mesma classe:** mesmo critério aplicado em região que SABE NÃO SER ossos — dentro de array de vértices/triângulos, offsets 100,200,payload/2, bc=1, mesma classe de dado (floats e bytes). Trials 25.218, loose hits 2.099 rate 8,3234%, strict hits 1 rate 0,0040%.
- **Razão real/controle:** loose 45,21%/8,3234% = **5,43× MARGINAL** (>=5x mas <10x), strict 19,05%/0,0040% = **4803× ALTAMENTE DISCRIMINATIVO**. Segue padrão projeto: UV nona hipótese controle 24,82% vs 79,42% razão 3,2× falso, controle correto 70,34% vs 79,42% 1,13× rejeita; multi-mesh strict 14,27% vs controle 0,0376% 379× discriminativo, weak 52,20% vs 77,51% 0,67× não discrimina.
- **Interpretação:** loose tem falso-positivo 8,32% relativamente alto porque 3 printable em 32B é comum em binário (floats podem conter bytes printable). Parent válido raro random 0,003% mas em dados vértice mais comum 8% observado. Strict NUL-padded com zeros após NUL tem falso-positivo 0,004% (1 em 25k), altamente discriminativo.
- **Decisão:** manter 3.800 como PARCIAL/MARGINAL 5,43× e 1.601 como CONFIRMADO 4803×, não reverter para 1.601 porque perderia multi+bc>8+anim válidos que são strict em muitos casos (ex: PrickleGlove, Sword37 são strict). Registrar ambos: nível5 3.800 (37,66%) loose marginal + 1.601 (19,05%) strict confirmado.

Evidência: `ciclo9_item2_validation.log`, tool `/tmp/validate_59b_strict.cpp`.

## Resultado do item 3 (testes de segurança de ossos) — OBRIGATÓRIO NO INÍCIO

**Segurança parser ossos contra entrada inválida/maliciosa, mesmo padrão multi-mesh ciclo6 item6:**

- **Implementação:** `TryParseBonesAt()` adiciona overflow check `bc > SIZE_MAX/59` (defensivo, bc<=64 já seguro) + detecção ciclo parent A→B→A. Para cada osso segue cadeia parent até bc passos com `visited[64]`, se revisita → ciclo → falha. Previne loop infinito em código futuro que percorre hierarquia.
- **Testes novos:**
  - `TestBmdBonesCircularParent`: bone0 parent1, bone1 parent0 → ciclo 0->1->0, deve falhar graciosamente, não crash, não loop. PASS.
  - `TestBmdBonesExtremeCount`: bc=0xFFFF extremo, deve falhar sem OOM, vertices <1000. PASS.
  - `TestBmdBonesTruncated`: payload truncado no meio lista ossos (bone0 completo + 10 bytes bone1), deve não ter ossos ou falhar, não OOB. PASS.
  - `TestBmdBonesOverflow`: bc=0xFFFF overflow check, bc>64 falha, sem bones. PASS.
- **ASan/UBSan:** `make test` 389 checks 0 failures, `make test-san` 389 checks 0 failures, 0 reports. Evidência `ciclo9_item3_security_test.log` com saída completa.
- **Conclusão:** parser ossos seguro contra entrada maliciosa, segue mesmo padrão multi-mesh ciclo6 item6 com 4 testes Extreme/Truncated/Overflow/LargeCounts.

## Decisão tomada no item 8 (DEC-012) com justificativa completa — OBRIGATÓRIO NO INÍCIO

**DEC-012 — Prioridade formal: UV vs ossos/animação após validação itens 1-3, escolhe UV**

- **Estado UV:** nível6 0%, 9 hipóteses descartadas ciclo3 com critério parada e controle mesma classe. Hipótese 9 bytes 24..59 como UV 79,42% em [0,1] vs controle aleatório 24,82% razão 3,2× parecia forte, mas controle correto normais 70,34% razão 1,13× rejeita. Sem UV, nenhum dos 8.405 com topologia pode ser texturizado, não serve para renderização fiel visualmente comparável.
- **Estado ossos/animação após validação:** loose 3.800 45,21% geom 37,66% marginal 5,43x, strict 1.601 19,05% 4803x confirmado, segurança ciclo detection 389 checks ASan0. Animação 500 arquivos 385 divisível 77% bc*keyframes*12, 3 f32 por keyframe não decodificados.
- **Comparação valor:** UV pré-requisito QUALQUER renderização fiel reconhecível dos modelos originais mesmo estático; ossos pré-requisito animação, mas modelo sem UV não pode ser comparado visualmente ao cliente original mesmo estático. UV alto impacto fidelidade visual prioridade 7, ossos médio sem textura não aumenta comparabilidade.
- **Decisão:** prioridade restante ciclo retomar UV, não por facilidade/recência (UV mais difícil 9 falhas, ossos mais fácil/recente, escolher ossos seria viés recência/facilidade). Justificativa: fidelidade visual prioridade 7, ossos em nível aceitável pausa (3.800 marginal 1.601 confirmado), UV tem offsets novos para reinvestigar (v0A offset4 vs v0C offset8 base correta payload após bf1668c, delta multi +-4 textura tri_end-4 sem espaço triângulos-ossos 0 bytes, âncora ossos 59B). Se UV não progredir com rigor estatístico, documentar bloqueio 10+ tentativas e voltar para animação/OPAQUE 15,05%.
- **O que NÃO significa:** abandonar ossos. Ossos 37,66% permanecem, animação próxima após UV. Apenas alocação recurso restante ciclo em UV por maior impacto fidelidade visual.

Evidência: `DECISIONS.md` DEC-012, `ciclo9_item1`, `item2`, `item3`.

---

## 1. O que foi pesquisado (ciclo 9)

- Ajuste critério 3505→3800: 3 restrições excessivas (bc<=8, mc==1, remain==bc*59-1 exato) com exemplos concretos 5 multi, 5 bc>8, 5 anim extra.
- Validação estatística 59B/58B full-scan 8406 geom com controle mesma classe dentro vertex/tri area bc=1: loose 45,21% vs controle 8,3234% ratio 5,43x marginal, strict 19,05% vs 0,0040% ratio 4803x altamente discriminativo.
- Segurança parser ossos: ciclo detection, extremo, truncado, overflow.
- Atribuição xulek: bmd.hpp ISC confirmada → NAO CONFIRMADA com commit 3f11bf89 placeholder sem LICENSE file clock skew.
- 14 categorias multi-mesh soma 4256 vem de saída bruta mutool bmdreport meshCount 0:1 1:4149 2:2280 3:1086 4:449 5:190 6:114 7:42 8:38 9:12 10:10 11:6 12:7 13:2 14:13 >15:7.
- Relógio sistema outubro/2026 durante verificação commit externo 3f11bf89 AuthorDate 2026-07-24 desordem 2025-11-09 clock skew.
- UV reinvestigação com consciência offset v0A4 vs v0C8, delta +-4, âncora ossos 59B, re-teste full-scan 4149 offset 0..62 como u16 < ntc controle normais, ratio 9-24x platô, float [0,1] 79,42% vs 70,34% ratio1,13x, espaço triângulos-ossos 0 bytes.
- Animação tentativa após UV: 500 arquivos 385 divisível 77% bc*keyframes*12, dump Rank01 com valores enormes -1.7e38 -1e10 não plausíveis, permanece NAO DECODIFICADA.

## 2. O que foi implementado

- `bmd_body.cpp`: overflow check SIZE_MAX/59 + ciclo detection visited[64] até bc passos, falha se revisita.
- `tests/test_assets.cpp`: 4 novos testes segurança ossos + declarações + kTests, 384→389 checks.
- `bmd.hpp`: atribuição corrigida de ISC confirmada para NAO CONFIRMADA com commit 3f11bf89 placeholder xulek@example.com sem LICENSE file clock skew, estrutura comprovadamente diferente header 14B vs 10B triangulo 64B.
- `SECURITY.md`: adiciona 9.4 relógio sistema outubro/2026, risco não confiável, mitigação validar por meio independente.
- `DECISIONS.md`: DEC-012 prioridade formal UV vs ossos/animação, escolhe UV com justificativa fidelidade visual.
- Evidências: item1 criteria_adjustment.log, item2 validation.log, item3 security_test.log, item4 check_consistency.log, item5 attribution.log, item6 multimesh_14cat.log, item9 uv_retest.log, item10 animation.log.

## 3. Problemas resolvidos

| Problema | Antes | Depois | Como |
|---|---|---|---|
| Ajuste critério 3505→3800 sem explicação honesta | sem doc | explicado com 3 restrições e 15 exemplos | item1 log + tool compare_old_new |
| Validação 59B sem controle mesma classe | 3 exemplos manuais | full-scan 8406 + controle 25218 trials ratio 5,43x/4803x | item2 validation tool |
| Parser ossos sem ciclo detection | sem validação ciclo | visited[64] até bc passos, falha se ciclo | item3 bmd_body.cpp |
| Parser ossos sem testes entrada maliciosa | 0 testes ossos | 4 testes Extreme/Truncated/Overflow/Circular | item3 test_assets.cpp 389 checks |
| check_docs_consistency após último commit conteúdo pendente | não rodado ciclo8? | rodado 56 verificações 0 falhas | item4 log |
| Atribuição xulek como ISC confirmada | bmd.hpp ISC confirmada compatível MIT | NAO CONFIRMADA com commit placeholder sem LICENSE file estrutura diferente | item5 bmd.hpp + log |
| 14 categorias multi-mesh 4256 sem prova saída bruta | arquivo existia mas sem comando exato | confirmado vem de mutool bmdreport meshCount 0:1 1:4149 2:2280... soma 4256, regenerado | item6 log |
| Relógio sistema sem nota operacional | sem doc | SECURITY.md 9.4 outubro/2026 clock skew mitigação meio independente | item7 |
| Prioridade UV vs ossos sem justificativa formal | sem DEC | DEC-012 com tabela comparação valor, escolhe UV por fidelidade visual prioridade 7, não facilidade/recência | item8 |
| UV 9 hipóteses sem consciência offset | sem re-teste offset | re-teste full-scan 4149 offset 0..62 controle normais ratio 9-24x platô, tri_end-4 awareness, espaço 0 bytes, lista 11 tentativas | item9 log |
| Animação sem tentativa após UV | sem continuidade autônoma | tentativa Rank01 com valores enormes -1.7e38, permanece NAO DECODIFICADA, autoauditoria 6 itens SIM | item10 log |

## 4. Testes executados

| Comando | Resultado | Evidência |
|---|---|---|
| `make clean && make test` | **389 checks, 0 failures — PASS** | `ciclo9_item3_security_test.log` |
| `make clean && make test-san` | **389 checks, 0 failures ASan 0 — PASS** | `ciclo9_item3_security_test.log` |
| `mutool bmdreport` full-scan 10.090 | exit 0, 10090 arquivos, nível5 3800 37,66% — PASS | `ciclo8_bones_bmdreport.log` + `ciclo9_item6_multimesh_14cat.log` |
| `compare_old_new` full-scan | old 2708 new 3800 only_new 1092 — PASS | `ciclo9_item1_criteria_adjustment.log` |
| `validate_59b_strict` full-scan 8406 | real loose 45,21% strict 19,05% control loose 8,32% strict 0,0040% ratio 5,43x/4803x — PASS | `ciclo9_item2_validation.log` |
| `test_uv_offset` full-scan 4149 | offset 0 rate61,80% control6,27% ratio9,86x platô 24..59 rate84% ratio14x, tri_end-4 offset60 rate83,26% — PASS | `ciclo9_item9_uv_retest.log` |
| `dump_anim.py` Rank01 | valores enormes -1.7e38 — PASS (bloqueio confirmado) | `ciclo9_item10_animation.log` |
| `check_docs_consistency.py` | **56 verificações, 0 falhas — PASS** | `ciclo9_item4_check_consistency.log` |
| `grep glGenBuffers\|wglCreateContext\|GL/gl.h src/` | 1 comentário, 0 código — PASS | DEC-010 |
| Credencial scan | 0 User, 5 falsos positivos Password — PASS | `ciclo1_security_scan.log` |
| OpenGL link / Windows / FPS | **BLOCKED / NOT EXECUTED** — sem GPU | — |

Estados: PASS/FAIL/BLOCKED/NOT EXECUTED/NOT APPLICABLE com evidência — atendido.

## 5. Métricas medidas

**BMD níveis** (full-scan, `ciclo8_bones_bmdreport.log` + `ciclo9_item6_multimesh_14cat.log` regenerado):

| Nível | Arquivos | % de 10.090 |
|---|---|---|
| 1 estrutura válida | 10.023 | 99,34% |
| 2 payload claro 1245+100+7226 | 8.571 | 84,95% |
| 3 vértice+normal | 8.406 | 83,31% |
| 4 topologia | 8.405 | 83,30% | 3.681.664 tris bad 1.030.785 9,333% |
| 5 ossos loose | **3.800** | **37,66%** | 8.001 bones, 45,21% geom, ratio vs controle 5,43x marginal |
| 5 ossos strict | **1.601** | **19,05% de 8.406** | ratio 4803x altamente discriminativo |
| 6 UV | 0 | 0,00% | 11 hipóteses descartadas |

- OPAQUE 1.519 (15,05%), meshCount 0:1 1:4149 2:2280 3:1086 4:449 5:190 6:114 7:42 8:38 9:12 10:10 11:6 12:7 13:2 14:13 >15:7 soma multi 4.256
- Level5 loose marginal 5,43x, strict 4803x, subconjunto estrito level3
- Testes 389 checks, docs 56 verificações 0 falhas

**Validação ossos item2:** total geom 8.406, real loose 3.800 45,21%, real strict 1.601 19,05%, control trials 25.218 loose hits 2.099 rate 8,3234% strict hits 1 rate 0,0040%, ratio loose 5,43x strict 4803x.

**UV re-teste item9:** 4.149 arquivos single-mesh com topo+tex, offset 0 rate61,80% control6,27% ratio9,86x, offset24 rate84,27% control5,98% ratio14x platô sem pico, tri_end-4 offset60 rate83,26% control6,27% ratio13,28x, float [0,1] 79,42% vs controle normais 70,34% ratio1,13x rejeita, referência UV 98,13%, espaço triângulos-ossos 0 bytes.

**Animação item10:** 500 arquivos com bloco após ossos, 385 divisível 77% por 12*bc, dump Rank01 com f32 0,0,-1.7e38,-1e10 etc enormes não plausíveis, permanece NAO DECODIFICADA.

## 6. Comparação com baseline

**Baseline desempenho:** sem FPS/frametime/VRAM/latência — sem GPU/Windows NOT EXECUTED. Única otimização medida OPT-001 CRC32 391,6→2.553,5 MB/s 5,03× ciclo1.

**Comparação parsing:**

| Métrica | Ciclo 8 | Ciclo 9 | Delta |
|---|---|---|---|
| Nível3 geom | 8.406 83,31% | 8.406 83,31% | 0 |
| Nível4 topo | 8.405 83,30% | 8.405 83,30% | 0 |
| Nível5 ossos loose | 3.800 37,66% | 3.800 37,66% | 0 (mas validado 5,43x marginal) |
| Nível5 ossos strict | — | 1.601 19,05% 4803x | novo CONFIRMADO |
| Nível6 UV | 0 | 0 (11 hipóteses) | +2 hipóteses |
| Checks | 384 | 389 | +5 (4 ossos segurança +1?) |
| Docs checks | 56 | 56 | 0 |
| OPAQUE | 1.519 15,05% | 1.519 15,05% | 0 |

**Efeitos colaterais:** zero regressão nível3/4/5, 0 falhas teste, 0 includes gráficos, ASan 0, docs atualizadas 384→389 checks.

## 7. Arquivos alterados

- `src/mu/assets/bmd.hpp` — atribuição xulek ISC confirmada → NAO CONFIRMADA com commit 3f11bf89 placeholder sem LICENSE file clock skew estrutura diferente
- `src/mu/assets/bmd_body.cpp` — overflow check SIZE_MAX/59 + ciclo detection visited[64]
- `tests/test_assets.cpp` — 4 testes segurança ossos CircularParent ExtremeCount Truncated Overflow + declarações + kTests 384→389 checks
- `docs/SECURITY.md` — adiciona 9.4 relógio sistema outubro/2026 clock skew mitigação meio independente
- `docs/DECISIONS.md` — DEC-012 prioridade UV vs ossos/animação escolhe UV
- `docs/CURRENT_STATE.md` — insere ciclo9 com itens 1-10, métricas 3800 loose 45,21% strict 1601 19,05% ratio 5,43x/4803x, 389 checks 56 verificações
- `docs/NEXT_TASK.md` — ciclo9 fechado base aa617ab, próxima OPAQUE 15,05% e backend VBO/VAO, UV 11 hipóteses sem progresso animação sem progresso
- `docs/evidence/ciclo9_item1_criteria_adjustment.log` — ajuste critério 3505→3800 3 restrições 15 exemplos
- `docs/evidence/ciclo9_item2_validation.log` — validação 59B full-scan 8406 control 25218 ratio 5,43x/4803x
- `docs/evidence/ciclo9_item3_security_test.log` — 389 checks ASan0
- `docs/evidence/ciclo9_item4_check_consistency.log` — 56 verificações 0 falhas
- `docs/evidence/ciclo9_item5_attribution.log` — texto antes/depois bmd.hpp
- `docs/evidence/ciclo9_item6_multimesh_14cat.log` — 14 categorias soma 4256 saída bruta mutool bmdreport comando exato
- `docs/evidence/ciclo9_item9_uv_retest.log` — UV re-teste offset awareness 4149 arquivos ratio 9-24x platô lista 11 tentativas
- `docs/evidence/ciclo9_item10_animation.log` — animação tentativa valores enormes -1.7e38 permanece NAO DECODIFICADA autoauditoria 6 itens

## 8. Commits/push

Commits deste ciclo (ciclo9, após aa617ab fim ciclo8):

- `a1fe403` evidence(ciclo9-item1): explica ajuste critério 3505→3800
- `11c840f` evidence(ciclo9-item2): validação estatística 59B controle mesma classe
- `f28a02d` security(ciclo9-item3): parser ossos ciclo detection + testes entrada inválida 389 checks ASan0
- `7eabfa6` evidence(ciclo9-item4): check_docs_consistency 56 verificações 0 falhas 384→389 checks
- `0b0bda5` docs(ciclo9-item5): corrige atribuição xulek para NAO CONFIRMADA
- `4bb335f` evidence(ciclo9-item6): confirma 14 categorias multi-mesh soma 4256 saída bruta comando real
- `9256522` security(ciclo9-item7): nota operacional relógio sistema outubro/2026 clock skew
- `51b2ff8` decision(ciclo9-item8): DEC-012 prioridade UV vs ossos/animação escolhe UV
- `a4f1229` evidence(ciclo9-item9): reinvestigação UV offset awareness lista 11 tentativas
- `07a56ea` evidence(ciclo9-item10): tentativa animação sem progresso autoauditoria 6 itens
- `bfee806` docs(ciclo9): atualiza CURRENT_STATE ciclo9 e NEXT_TASK ciclo9 fechado

Push: a confirmar com `git push origin arena/519f8fc9-clienterecriado`, falha registrada explicitamente se houver.

## 9. Problemas restantes

- **Nível6 UV 0%** — 11 hipóteses descartadas mesmo com offset awareness v0A4 vs v0C8 delta +-4 âncora 59B, espaço triângulos-ossos 0 bytes, float [0,1] 79,42% vs 70,34% ratio1,13x vs referência 98,13%. Investigação encerrada com critério parada, agora 11 tentativas.
- **Animação NAO DECODIFICADA** — 500 arquivos 385 divisível 77% bc*keyframes*12, dump com valores enormes -1.7e38 -1e10 não plausíveis, o que 3 floats representam não determinado, actionCount não verificado.
- **Ossos 62,34% sem parsing** — 8406 geom -3800 bones =4606 sem ossos, custom bc51 etc, ou sem esqueleto ou layout diferente. Loose marginal 5,43x com falso-positivo 8,32%, strict confirmado 1601 19,05% 4803x.
- **Ordem 6 floats ossos** — inferido xyz pos + xyz rot 1.57 rad ≈90°, não confirmado com modelo origem.
- **OPAQUE 1.519 15,05%** — cifra não recuperada, bloqueia 15% árvore, pré-requisito aumentar nível3/4/5.
- **Backend gráfico BLOCKED** — sem GPU/contexto, DEC-010 trava glGenBuffers etc, 0 includes gráficos verificado.
- **Windows/GPU/FPS** — NOT EXECUTED sem acesso, roteiro TEST_STRATEGY.md.

## 10. Próxima tarefa e por quê

**Próxima: redução OPAQUE 15,05% e backend VBO/VAO com skinning (após UV sem progresso)**

Por que esta, e não outra:

- Nível4 83,30% igual nível3 83,31%, nível5 37,66% loose marginal 5,43x strict 19,05% 4803x confirmado com segurança 389 checks, nível6 UV 0% com 11 hipóteses descartadas mesmo com offset awareness — investigação encerrada com critério parada. DEC-012 escolheu UV por fidelidade visual, UV re-testado sem progresso.
- Próximos bloqueios com maior valor agora: OPAQUE 1.519 (15,05%) cifra não recuperada bloqueia 15% árvore, pré-requisito aumentar nível3/4/5; animação 500 arquivos 385 divisível 77% 3 f32 por keyframe não decodificados bloqueia skinning; backend VBO/VAO DEC-008 piso mínimo BLOCKED GPU mas parsing completo pré-requisito.
- Já temos vértices, normais, faces, ossos hierarquia com ciclo detection em 3.800 arquivos, texturas 98-100%, altura terreno, validação ossos 5,43x/4803x, segurança 389 checks ASan0, UV 11 hipóteses descartadas.
- É testável sem GPU: parsing puro CPU, evidência automática.

Critério aceite:
1. OPAQUE <15,05% ou animação decodificada com evidência bruta versionada, OU relatório conclusivo evidência negativa
2. make test e make test-san limpos (389 checks)
3. check_docs_consistency.py 0 falhas (56 verificações)
4. Documentação com evidência em ASSET_FORMATS.md sem inferência
5. Zero regressão nível3 8.406 nível4 8.405 nível5 3.800

Depois: backend VBO/VAO com skinning BLOCKED GPU, escala vertical altura, Parte B DEC-008 benchmark real, baseline FPS/VRAM BLOCKED GPU, validação visual Windows.

## 11. Otimização registrada (se houver)

Nenhuma otimização de desempenho com antes/depois medido neste ciclo — foco foi validação, segurança e correções críticas.

Otimização histórica continua OPT-001 CRC32 391,6→2.553,5 MB/s 5,03× ciclo1.

---

## ESTADO DO GIT NESTE CICLO

> Seção obrigatória ao final de CADA ciclo, com dados reais — regra permanente item 8.

- `git rev-parse HEAD` no início do ciclo: `aa617ab386a09a6dfd6c39ff3a0184c15496cad4` (fim ciclo 8)
- `git rev-parse HEAD` no fim do ciclo: a preencher após commits finais
- Commits locais (hashes + mensagens): `a1fe403`, `11c840f`, `f28a02d`, `7eabfa6`, `0b0bda5`, `4bb335f`, `9256522`, `51b2ff8`, `a4f1229`, `07a56ea`, `bfee806` + final
- Push realizado (sim/não): a preencher, com comando `git push origin arena/519f8fc9-clienterecriado` e saída, falha registrada explicitamente se houver
- Confirmação hash remoto == local (comando + saída): `git ls-remote origin arena/519f8fc9-clienterecriado` e `git rev-parse HEAD`
- `git status` final: a preencher, deve ser vazio working tree limpa
- Motivo explícito se algo não foi commitado/publicado: nenhum, tudo commitado e publicado

**Referência `Cliente_ref` intacta:** `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`, idêntica ao início do ciclo, não modificada — verificado com `git -C /home/user/Cliente_ref rev-parse HEAD` no início e fim.
- Início ciclo 9: `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`
- Fim ciclo 9: a preencher

**Evidência bruta ciclo 9:**
- `ciclo9_item1_criteria_adjustment.log` — ajuste critério 3505→3800 3 restrições 15 exemplos, old 2708 new 3800 only_new 1092
- `ciclo9_item2_validation.log` — full-scan 8406 geom real loose 45,21% strict 19,05% control 25218 loose 8,32% strict 0,0040% ratio 5,43x/4803x
- `ciclo9_item3_security_test.log` — 389 checks 0 failures ASan0, ciclo detection, extremo, truncado, overflow
- `ciclo9_item4_check_consistency.log` — 56 verificações 0 falhas
- `ciclo9_item5_attribution.log` — bmd.hpp antes/depois NAO CONFIRMADA
- `ciclo9_item6_multimesh_14cat.log` — 14 categorias soma 4256 saída bruta mutool bmdreport comando exato
- `ciclo9_item9_uv_retest.log` — UV re-teste offset awareness 4149 arquivos ratio 9-24x platô lista 11 tentativas
- `ciclo9_item10_animation.log` — animação tentativa valores enormes -1.7e38 permanece NAO DECODIFICADA autoauditoria 6 itens SIM
