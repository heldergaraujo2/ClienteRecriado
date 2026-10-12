# STATUS FINAL — Ciclo 11

> **Base:** ciclo10 `3b24987` → HEAD atual `50331cc` + itens ciclo11 1-6. Branch `arena/519f8fc9-clienterecriado`. Referência `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` intacto.

## Métricas medidas (10.090 arquivos .bmd)

| Nível | Arquivos | % total | % geom | Detalhe |
|---|---|---|---|---|
| 1 estrutura válida | 10023 | 99,34% | — | magic + versão, length_consistent |
| 2 payload em claro | 8571 | 84,95% | — | 1245 plaintext +100 bux +7226 v0x0C |
| 2 OPAQUE | 1519 | 15,05% | — | cifra não recuperada |
| 3 vértice+normal | 8406 | 83,31% | 100% | posições e normais lidas |
| 4 topologia | 8405 | 83,30% | 99,99% | 3.681.664 tris |
| 5 ossos loose | **3693** | **36,60%** | **43,93%** | 7879 bones, ratio per file 1846,5× altamente discriminativo |
| 5b ossos strict (headline conservador) | **1186** | **11,75%** | **14,11%** | NUL-padded, taxa FP 0/25218 limite sup ~0,004% ratio >3558× |
| 6 UV | 0 | 0,00% | 0% | 11 hipóteses descartadas, platô 9-26× explicado |
| Anim extra | 764 | 7,57% | 9,09% | com extra após ossos |
| Anim divisível bc*12 | 231 | 2,29% | 2,75% | |
| Anim plausível finite<1e6 | 62 | 0,61% | 0,74% | rotação [-pi,pi] mas ratio 0,02× NÃO discriminativo |
| Anim plausível [-pi,pi] | 53 | 0,53% | 0,63% | ratio per file 0,04× NÃO discriminativo |

## Correções ciclo 11 itens 1-6

### Item1 — explica salto 5,43x→1846x com números exatos antes/depois unidade e ciclo
- Antes ciclo9: real loose 3800 45,21% geom vs controle per trial 2099/25218 8,32% ratio 5,43× marginal (unidades diferentes arquivo vs tentativa) + strict 1/25218 0,004% ratio 4803×
- Depois correção unidade apenas (sem ciclo): controle per file 1765/7209 24,48% vs per trial 1853/21627 8,56% ratio per file OLD 1,55× < per trial OLD 4,43×, prova que unidade sozinha DIMINUI ratio
- Depois ciclo detection: controle NEW 2/25218 0,0079% per file 2/8406 0,0238% ratio 1846,28× definitivo (tool simplificada 1271× estava errada: parser single-mesh subcontou real 2966 vs 3693 correto, denominador 7209 vs 8405), fator redução controle 8,32%→0,0079% ~1052× ou 24,48%→0,0277% 882×
- Salto 5,43x→1846x = -3,5x unidade + ~1052x ciclo detection (parent 0 self-loop rejeitado como ciclo A->A)
- Evidência `ciclo11_bones_ratio_raw.log` com 45 linhas números exatos

### Item2 — corrige "ratio infinito" → limite superior 1/N
- Antes: "ratio infinito" para strict 0 hits
- Depois: strict 0/25218 trials → taxa ≤0,003965% (~0,004%), ratio >14,11%/0,003965%≈3558× per trial, per file N=8406 limite 0,0119% ratio >1185× loose per file 2/8406 0,0238% ratio 1846×
- Arquivos corrigidos: ASSET_FORMATS.md, CURRENT_STATE.md, NEXT_TASK.md, TEST_RESULTS.md, CICLO10_RELATORIO.md, evidence/ciclo9_item2_validation_fixed.log
- Commit `b9f7af6` fix(ciclo11-item2) 6 files

### Item3 — explica quedas 3800→3693 loose e 1601→1186 strict com exemplos concretos e razão técnica
- Mudança critério ciclo9→ciclo10: ciclo9 TryParseBonesAt sem ciclo detection, ciclo10 adiciona visited[64] até bc passos, self-loop parent==i rejeitado, overflow bc>SIZE_MAX/59, bc>64
- Por que filtra 107 loose e 415 strict: bc=1 apenas parent -1 (0xFFFF) válido, parent 0 self-loop A->A ciclo rejeitado (antes válido, gerava falso-positivo 8,32%→0,0079%), bc>1 ciclos A->B->A causariam loop infinito while(parent!=-1)
- Exemplos dropped loose (old ok new fail) 228 com tool simplificada vs 107 real mutool bmdreport: MoveTargetPosEffect.bmd base 5140 bc1 parent 40117 inválido mas base+4 parent 0 self-loop, Bow19.bmd mc2 bc1 sbow06_R.jpg segundo mesh nome "/" parent 0 self-loop, Mace21.bmd, 7mastery_mixiteam.bmd, DarkHorseHorn.bmd, Shield27.bmd, Staff02.bmd, ThunderBolt.bmd, S14_quiver.bmd, ProofOfConqueror.bmd etc
- Evidência `ciclo11_bones_drop_examples.log` com 20 exemplos e detalhe primeiro dropped

### Item4 — investigar com rigor achado IceGolem step 0x33 vs 0x3D full-scan 1519 OPAQUE
- Metodologia: 1503 OPAQUE (BMD_V0C length_consistent payload>=32 mas BmdV0CDecrypt padrão init 0x5E step 0x3D falha Printable32), para cada arquivo cada step 0..255 com init 0x5E e key array 16B, decifra primeiros 32B e verifica Printable32 e PlausiblePath (NUL-padded >=3 apenas alnum / _ . - \)
- Resultados: step variation found_printable 15 arquivos found_plausible 0 arquivos (ex IceGolem step 0x33 name "DDfPsxkvfWyx}~o|fMyvvy}<f'H<:Y*I" contém } ~ | < etc não permitido path, boss_invitationticket step 0x34 similar), init variation found_printable 0 found_plausible 0, padrão IceGolem steps 0x33..0x3A todos printable mas nenhum plausível sem padrão único
- Conclusão: achado IceGolem step 0x33 é falso-positivo check fraco Printable32, não variante confirmada, full-scan 1503 com PlausiblePath resulta 0 variantes plausíveis, OPAQUE permanece 1519 15,05% tentativa documentada como FALHA
- Evidências `ciclo11_opaque_step_fullscan.log` (2 printable fraco), `ciclo11_opaque_step_plausible.log` (15 printable 0 plausível), `ciclo11_opaque_init_plausible.log` (0/0), `ciclo11_item4_opaque_investigation.log` síntese
- Commit `107815b`

### Item5 — validar estatisticamente 62 plausíveis animação com controle mesma classe
- População: total com ossos 3693, com extra 764, divisível bc*12 231, plausível finite<1e6 62 (1,68% de 3693), plausível estrito [-pi,pi] 53 (1,43%)
- Critério fraco finite<1e6: real 62/3693=1,6789% per file, controle vertex array 12B chunks finite<1e6 trials 55985 hits 55911 rate 99,86% per file 100% ratio 0,02× — controle MAIOR que real NÃO discriminativo
- Critério estrito [-pi,pi]: real 53/3693=1,4351% per file, controle vertex array trials 55985 hits 2079 rate 3,71% per file 78/231 33,76% ratio per trial 0,39x per file 0,04x — controle ainda maior NÃO discriminativo, vertex coords pequenos 0..500 caem em [-pi,pi] por acaso
- Padrão observado Object01.bmd kf18 primeiros 9 zeros depois 0,628 1,256 1,884 2,513 3,141 -2,513 etc múltiplos pi/5, Object04.bmd kf98 primeiros 48 zeros depois 0,125 0,251 0,376 até 3,141 múltiplos pi/25, sugere critério mais discriminativo zeros iniciais + múltiplos ângulo base + monotonicidade
- Conclusão: 62 plausíveis NÃO pode ser citado como achado confirmado, ratio <1x, animação permanece NÃO DECODIFICADA formalmente, citar como tentativa parcial ou evidência negativa
- Evidências `ciclo11_anim_validation.log`, `ciclo11_anim_pi_validation.log`, `ciclo11_item5_anim_validation_full.log`
- Commit `7a9aa03`

### Item6 — C1-C3 D1-D5 E1-E3 com provas e ratios per file corrigidos
- C1 loose 3693 ratio per file 1846,5×, C2 strict 1186 taxa 0/25218 limite sup ~0,004% ratio >3558×, C3 prova strict ⊂ loose tool validate_A3_subset 0 falhas
- D1-D5 multi-mesh: strict_m4 1796 14,27% vs controle 0,0376% ratio 379×, weak_m4 6572 52,20% vs controle 77,51% ratio 0,67×, strict_files 1651 38,79% etc, breakdown 1.685 =1483 OPAQUE+67 invalid+134 geom+1 zero
- E1-E3 UV: float [0,1] 79,42% vs 70,34% ratio 1,13× rejeita, offset 0 rate 61,80% ctrl 4,80% ratio 12,89×, platô 9-26× offset 24 rate 84,23% ctrl 4,46% ratio 18,87× explicado como artefato floats pequenos lidos como u16
- Correções docs/COMPATIBILITY.md e ASSET_FORMATS.md tabela nível5 3.800 37,66% → 3.693 36,60% loose + 1.186 11,75% strict com taxa e ratio, tools/check_docs_consistency.py nivel5 3800→3693, checker 56 verificações 0 falhas
- Commit `50331cc`

## Otimizações

| ID | Descrição | Antes | Depois | Speedup | Status |
|---|---|---|---|---|---|
| OPT-001 | CRC-32 slice-by-1→slice-by-8 | 391,6 MB/s | 1.971,6 MB/s | 5,03× | KEPT |
| OPT-003 | BuxCrypt vetorizado 8B key64[3] | 880,5 MB/s 1135us | 3437,8 MB/s 290,9us | 3,9× | KEPT ciclo10 Fase C |
| CAND-001 | BGRA upload direto | — | — | — | NOT IMPLEMENTED, BLOCKED GPU |
| CAND-003 | JPEG scan word-at-a-time | — | — | — | NOT IMPLEMENTED, ver item8 |

## Testes

- `make test`: 389 checks 0 failures
- `make test-san`: 389 checks 0 failures ASan 0 reports
- `check_docs_consistency.py`: 56 verificações 0 falhas (após correção 3693)
- `mutool bmdreport`: total 10090, valid 10023 99,34%, plaintext 8571 84,95% OPAQUE 1519 15,05%, parsed 8406 83,31%, with triangles 8405 83,30% 3681664 tris, with bones 3693 36,60% strict 1186 11,75% (headline conservador)
- Evidências em `docs/evidence/` com logs brutos e ferramentas em `/tmp/`

## Segurança

- Parser ossos: detecção ciclo A->B->A com visited[64], self-loop parent==i rejeitado, bc>64 limite, overflow bc>SIZE_MAX/59, bc=1 apenas parent -1 válido, reduz falso-positivo 8,32%→0,0079%, testes CircularParent, ExtremeCount, Truncated, Overflow, 389 checks ASan0 (SECURITY.md 9.5)
- BuxCrypt vetorizado: memcpy unaligned safe, phase handling testado fases 0..2 lengths 0..40, timing side-channel não risco pois obfuscação não criptografia segurança, self-inverse mantido (SECURITY.md 9.6)
- Nenhum include gráfico em src/ (trava DEC-010)

## Bloqueios

- OPAQUE 1519 (15,05%) cifra não recuperada, full-scan step/init 1503 com PlausiblePath 0 variantes plausíveis, tentativa falha documentada item4, requer key arrays alternativos ou versão 0x0B, compressão etc
- Animação 62 plausíveis mas ratio 0,02× NÃO discriminativo, 53 plausíveis [-pi,pi] ratio 0,04× NÃO discriminativo, sem parser formal actionCount→kf, ordem xyz vs zyx, convenção Euler/quaternion, falta confirmação
- UV 0% 11 hipóteses descartadas, platô 9-26× explicado como artefato floats pequenos, investigação encerrada critério parada
- Backend VBO/VAO com skinning BLOCKED sem GPU/contexto, DEC-008 piso mínimo registrada
- Baseline FPS/VRAM NOT EXECUTED sem Windows/GPU, roteiro em scripts/ nunca executado
- Validação visual Windows BLOCKED sem renderer
- CAND-001 BGRA upload e CAND-003 JPEG scan ainda não implementados (CAND-003 ver item8 ciclo11)

## Próxima tarefa e por quê

**Próxima: Fase C-F — otimizações restantes CAND-001 e CAND-003 com medição CPU + roteiro testes reais Windows + build/distribuição**

Por que esta e não outra:
- Nível3 8406 83,31% e nível4 8405 83,30% estáveis, nível5 loose 3693 36,60% ratio per file 1846× altamente discriminativo com segurança ciclo detection, strict 1186 11,75% headline conservador taxa FP 0/25218 limite sup ~0,004% ratio >3558×, strict ⊂ loose provado, animação 62 plausíveis mas não confirmado, OPAQUE 15,05% investigado full-scan 0 variantes plausíveis tentativa falha, UV 0% 11 hipóteses platô explicado, bux otimizado 3,9×, segurança documentada
- Próximos bloqueios de maior valor: backend VBO/VAO BLOCKED GPU, baseline FPS BLOCKED, validação visual BLOCKED — todos requerem Windows/GPU, não testáveis aqui
- O que é testável aqui e tem valor: finalizar documentação, roteiro Windows reproduzível, build/distribuição, otimizações restantes CAND-001 e CAND-003 com medição CPU (CAND-003 ver item8)
- Critério: preparar roteiro para pessoa com Windows/GPU executar validação completa, com PASS/FAIL/BLOCKED e evidência, e fechar ciclo 11 com relatório 10 itens + ESTADO DO GIT

## ESTADO DO GIT NESTE CICLO — até item6

- Branch: `arena/519f8fc9-clienterecriado`
- Base: `972fabcea1b0d4e80345f0f5abd287b46d819455` main
- Commits locais neste ciclo até item6 (hashes + mensagens):
  - `1728d5f` evidence(ciclo11-item1): explica salto 5,43x->1846x com números exatos antes/depois correção unidade e ciclo detection
  - `b9f7af6` fix(ciclo11-item2): corrige afirmação ratio infinito para limite superior estatístico 1/N
  - `0589adf` evidence(ciclo11-item3): explica quedas 3800->3693 loose e 1601->1186 strict com exemplos concretos e razão técnica ciclo detection
  - `107815b` evidence(ciclo11-item4): investiga rigor IceGolem step 0x33 vs 0x3D full-scan 1503 OPAQUE, 0 variantes plausíveis — tentativa falha
  - `7a9aa03` evidence(ciclo11-item5): valida estatisticamente 62 plausíveis animação com controle mesma classe — não confirmado
  - `50331cc` fix(ciclo11-item6): C1-C3 D1-D5 E1-E3 com provas e ratios per file corrigidos
- Push realizado: sim para todos até 50331cc, `git push origin arena/519f8fc9-clienterecriado` exit 0, `git ls-remote` confirma hash, `git status` limpo após commit
- Referência `Cliente_ref`: `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` intacto início e fim ciclo, verificado via `git -C /home/user/Cliente_ref rev-parse HEAD`
- Métricas finais até item6: nível3 8406 83,31%, nível4 8405 83,30% 3681664 tris, nível5 loose 3693 36,60% 43,93% geom ratio per file 1846×, strict 1186 11,75% 14,11% geom taxa FP 0/25218 limite sup ~0,004% ratio >3558×, nível6 UV 0% 11 hipóteses, OPAQUE 1519 15,05%, animação 62 plausíveis mas não confirmado ratio 0,02×, 389 checks 0 falhas, 56 verificações doc 0 falhas, BuxCrypt 3437 MB/s 3.9x
- Evidências: 7 novos logs ciclo11 até item5 + logs item6 (ciclo11_bones_ratio_raw.log, ciclo11_bones_drop_examples.log, ciclo11_opaque_step_fullscan.log, ciclo11_opaque_step_plausible.log, ciclo11_opaque_init_plausible.log, ciclo11_item4_opaque_investigation.log, ciclo11_anim_validation.log, ciclo11_anim_pi_validation.log, ciclo11_item5_anim_validation_full.log) + logs anteriores ciclo10

**Comandos de verificação executados até item6:**
```
git rev-parse HEAD -> 50331cc...
git ls-remote origin arena/519f8fc9-clienterecriado -> 50331cc == local
git status --short -> vazio
git -C /home/user/Cliente_ref rev-parse HEAD -> 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9
make test -> 389 checks 0 failures
python3 tools/check_docs_consistency.py -> 56 verificações 0 falhas
./build/mutool bmdreport -> 8406 geom, 8405 topo, 3693 bones loose, 1186 strict, 1519 OPAQUE
```
