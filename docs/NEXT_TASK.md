# Próxima tarefa — ciclo14 A1

> **Ciclo 13 fechado** — 10 itens A1-E3 com auto-auditoria obrigatória e revisão STATUS_FINAL.md linha a linha.
> HEAD atual `550593c` + 772d0a4 item9 + commits pendentes push. Base ciclo12 `a97b605` (460 checks) → ciclo13 474 checks. Referência `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` intacto. Branch `arena/519f8fc9-clienterecriado`.

## O que foi concluído ciclo13 (10 itens)

**A1-A3:**
- A1: `docs/REFERENCE_AUDIT.md` e `docs/ARCHITECTURE.md` auditados, bones loose 3693 ratio per file 1846,28× definitivo, strict 1186 headline conservador, max_bc 63 limite prático 64 teto 63 sem contradição, fix divergência 389 vs 469
- A2: Makefile fix ciclo13 item1 — test-san agora cobre 100% checks 474, antes 389 vs 469 discrepância corrigida, regra permanente divergência test vs test-san = bloqueio crítico
- A3: `docs/evidence/ciclo13_git_incident.log` e `ciclo13_bones_ratio_final_calc.log` com cálculo passo a passo ratio per file LOOSE 43,9329% / 0,023795% = 1846,28× mesma unidade, per trial 43,9329% / 0,007932% = 5538× unidades diferentes INVÁLIDO

**B1-B3:**
- B1: `docs/STATUS_FINAL.md` revisado — antes otimista só identity/self-loop/zero, depois não-trivial rotação 90° Z
- B2: D2 corrigido de otimista para CONCLUÍDO E VALIDADO com rotação 90° Z não-trivial (1,0,0)->(0,1,0) fabs<1e-5, 15 checks
- B3: D3 corrigido para CONCLUÍDO E VALIDADO com 3 casos unitários a/b/c + escala real 8406 bounds reais culled 2,77% (frustum 0..10 X) e 7,88% (identity VP), evidência ciclo13_cull_scale.log

**C1-E3:**
- C1: `src/mu/assets/mesh.hpp` 23 checks, D1: `buffers` 21 checks, E1: `main_loop` 9 checks — revisados com código, teste, saída [ RUN ]/[ OK ], classificação final sem otimismo, evidência ciclo13_c1_e3_detailed.log, ciclo13_e1_e2_e3_content.log
- C2: DEC-013 glTF 2.0 Caminho B não substitui Caminho A
- C3: Limites reais consolidados max_vc 22987 max_mc 30 max_bc 63
- D4: `docs/RENDERING.md` §12.6 plano otimização consistente com dados medidos LOD/streaming/draw calls
- D5: `src/mu/platform/win32_window` esqueleto BLOCKED 4 checks
- E2-E3: Áudio/rede/interface avaliados, dependências §7

**D/E item9:** STATUS_FINAL.md revisado linha a linha corrigindo classificação otimista 772d0a4 105 insertions

**E item10:** Otimização BuxCrypt e OztToRgbaTopDown implementada
- OztToRgbaTopDown otimizado 2 pixels 8B via uint64_t swap BGRA->RGBA via (p & 0xFF00FF00) | ((p & 0xFF)<<16) | ((p>>16)&0xFF) memcpy unaligned safe
- OztToBgraTopDown novo CAND-001/OPT-005 GL_BGRA direct — apenas flip vertical memcpy row, sem swap, 10,2× teórico 66,4us→6,5us
- bench_assets OztToBgraTopDown 256x256 (GL_BGRA) adicionado
- make clean && make test-san → 474 checks 0 failures 100% testes, evidence ciclo13_test_san_full_after.log
- bench_assets → ciclo13_bench_after.log: OztToRgba 63,0us med 3969 MB/s, OztToBgra 6,6us med 38057,5 MB/s 9,55× speedup, BuxCrypt 384,8us med 2598,9 MB/s vs baseline 1135,7us 880,5 MB/s 2,96× speedup
- OPTIMIZATION_LOG.md OPT-005 documentado problema→evidência→hipótese→alteração→teste→antes/depois→efeitos→decisão manter

## Próxima tarefa ciclo14 A1 — por quê

**A1: Investigar cifra BMD v0x0C OPAQUE 1519 arquivos 15,05% — tentativa via análise Data/**

Prioridade técnica item 8: investigar cifra BMD v0x0C antes de CAND-001 pois bloqueia ~85% modelos e é pré-requisito para renderização mundo e entidades — já parcialmente resolvida com V0C decifra 7226 arquivos 71,61% mas 1519 15,05% ainda OPAQUE. Próximo passo: mapear Data/ BuxCrypt terrain vs BMD V0C keyspace, tentar full-scan 1503 variantes PlausiblePath 0 resultado anterior, documentar bloqueio #1 se falhar.

Critérios aceite ciclo14:
- build limpo: make clean && make test 474 checks 0 failures, make clean && make test-san 474 checks 0 failures 0 ASan/UBSan 100% cobertura
- números batendo com ferramenta: bench_assets com buffer size iterações desvio padrão cache/in cache
- sem contradição entre documentos: BENCHMARKS.md, PERFORMANCE_BASELINE.md, RENDERING.md §12.6, STATUS_FINAL.md, CURRENT_STATE.md, NEXT_TASK.md consistentes
- comparação real-vs-controle com unidades idênticas: ratio per file 1846,28× mesma unidade, não misturar per trial
- seção obrigatória ao final de CADA ciclo com dados reais: ESTADO DO GIT NESTE CICLO — commits locais hashes+mensagens push realizado sim/não confirmação hash remoto==local comando+saída git status final motivo explícito se algo não commitado/publicado

## Comandos ciclo13 finais

```
git log --oneline -10:
772d0a4 docs(ciclo13-item9): revisa STATUS_FINAL.md linha a linha corrigindo classificação otimista
550593c perf(ciclo13-item10): otimiza OztToRgbaTopDown 2px + OztToBgraTopDown GL_BGRA 9,45× + BuxCrypt já 2,96×
2a44d10 docs(ciclo13-item4-8): evidências detalhadas C1-E3 + correção classificação otimista D2/D3
bf1b7f0 docs(ciclo13-item2-3): evidencia git incident + ratio per file definitivo 1846,28×
65bb1c4 fix(ciclo13-item1): corrige Makefile test-san 389 vs 469 -> 474 checks 100% + regra permanente divergência = bloqueio crítico
...
make clean && make test => 474 checks 0 failures
make clean && make test-san => 474 checks 0 failures 0 ASan/UBSan
bench_assets 2000 reps OztToBgra 6,6us vs old 62,4us 9,45× median 12,26× p99
```

## ESTADO DO GIT NESTE CICLO — até item10

- Branch: arena/519f8fc9-clienterecriado
- Base: 972fabcea1b0d4e80345f0f5abd287b46d819455 main
- HEAD atual: 550593c perf(ciclo13-item10): otimiza OztToRgbaTopDown 2px + OztToBgraTopDown GL_BGRA 9,45× + BuxCrypt já 2,96× + 772d0a4 local pendente antes
- Commits locais ciclo13: 65bb1c4 fix item1 469 vs 389, bf1b7f0 evidence item2-3 git incident + ratio calc, 2a44d10 evidence item4-8 C1-E3 detalhada, 772d0a4 docs item9 STATUS_FINAL revisado, 550593c perf item10 otimização Ozt+BuxCrypt
- Push realizado: pendente, será feito agora com tentativa push após cada commit conforme regra permanente item8
- git status final: limpo após commit item10, exceto docs/CURRENT_STATE.md e docs/NEXT_TASK.md atualizados agora
