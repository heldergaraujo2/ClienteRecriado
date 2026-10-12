# Próxima tarefa

> **Ciclo 11 fechado** com 8 itens: item1 salto 5,43x→1846x números exatos antes/depois unidade e ciclo detection, item2 ratio infinito→limite superior 1/N ~0,004% ratio >3558×, item3 quedas 3800→3693 loose 1601→1186 strict exemplos concretos MoveTargetPosEffect Bow19 etc razão técnica ciclo detection visited[64] self-loop parent==i, item4 OPAQUE IceGolem step 0x33 vs 0x3D full-scan 1503 OPAQUE 0 variantes plausíveis PlausiblePath tentativa falha, item5 animação 62 plausíveis validação estatística controle mesma classe vertex array 99,86% vs 1,68% ratio 0,02× NÃO discriminativo 53 plausíveis [-pi,pi] ratio 0,04× NÃO discriminativo, item6 C1-C3 D1-D5 E1-E3 com provas strict ⊂ loose 0 falhas ratios per file corrigidos breakdown 1.685 multi-mesh 14,27% vs 0,0376% ratio 379× UV 79,42% vs 70,34% ratio 1,13× rejeita platô 9-26× explicado, item7 STATUS_FINAL.md criado métricas corrigidas 3693/1186 provas ratios, item8 CAND-003→OPT-004 word-at-a-time JPEG scan 179,2us→15,2us 11,8x real OZJ 13928 13832 ok 96 fail preservado. Base ciclo10 `3b24987` → HEAD atual `49825c5` + relatório `4f74539`. Branch `arena/519f8fc9-clienterecriado`. Referência `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` intacto. 9 commits ciclo11 1728d5f b9f7af6 0589adf 107815b 7a9aa03 50331cc 28c9259 49825c5 4f74539, push sim remote==local, 389 checks 0 falhas 56 verificações 0 falhas, BuxCrypt 3437 MB/s 3,9x OzjProbe 15,2us 11,8x.

> **Ciclo 10 Fase A fechada** anteriormente com correção ratio per file 1846×, headline conservador strict 1186, strict ⊂ loose provado, UV ratio-por-offset platô 9-26×. Base `84fee45` → HEAD atual ciclo10. Fase A itens A1-A5, checker 56 verificações 0 falhas, build limpo 389/0 ASan 0. Métricas corrigidas: total geom 8406, loose 3693 (43,93% geom, 36,60% total, 7879 bones) ratio per file 1846× altamente discriminativo (antes 5,43× marginal devido falta ciclo detection + unidades diferentes), strict 1186 (14,11% geom, 11,75% total) taxa falso-positivo 0/25218, limite superior ~0,004% (1/25218), ratio >3558× headline conservador, strict ⊂ loose provado 0 falhas.

## O que mudou neste ciclo (ciclo 11)

**Itens 1-8:**

- Item1: explica salto 5,43x→1846x com números exatos antes/depois correção unidade e ciclo detection — old per file 1,55x vs per trial 4,43x prova unidade sozinha DIMINUI ratio -3,5x, ciclo detection reduz FP 8,32%→0,0079% fator 1052x, salto = -3,5x +1052x, evidência `ciclo11_bones_ratio_raw.log` 45 linhas
- Item2: corrige afirmação ratio infinito para limite superior estatístico 1/N — strict 0/25218 limite sup ~0,004% ratio >3558× per trial 0,0119% per file ratio >1185×, 6 arquivos corrigidos commit `b9f7af6`
- Item3: explica quedas 3800→3693 loose e 1601→1186 strict com exemplos concretos e razão técnica ciclo detection — mudança critério ciclo9 sem ciclo → ciclo10 com visited[64] self-loop parent==i rejeitado, por que filtra 107 loose 415 strict bc=1 apenas parent -1 válido parent 0 self-loop A->A ciclo rejeitado antes válido gerava FP 8,32%→0,0079% bc>1 ciclos A->B->A loop infinito, exemplos dropped loose 228 vs 107 real MoveTargetPosEffect Bow19 Mace21 etc evidência `ciclo11_bones_drop_examples.log`
- Item4: investigar com rigor achado IceGolem step 0x33 vs 0x3D full-scan 1519 OPAQUE padrão step validar contra texto plausível como Lorencia.att — metodologia 1503 OPAQUE cada step 0..255 com init 0x5E key array 16B decifra 32B verifica Printable32 e PlausiblePath NUL-padded apenas alnum / _ . - \, resultados step variation found_printable 15 found_plausible 0 (IceGolem step 0x33 name "DDfPsxkvfWyx}~o|fMyvvy}<f'H<:Y*I" contém } ~ | < etc não permitido) init variation 0/0, conclusão falso-positivo check fraco Printable32 não variante confirmada full-scan 1503 com PlausiblePath 0 variantes plausíveis OPAQUE permanece 1519 15,05% tentativa falha evidências `ciclo11_opaque_step_fullscan.log` `ciclo11_opaque_step_plausible.log` `ciclo11_opaque_init_plausible.log` `ciclo11_item4_opaque_investigation.log` commit `107815b`
- Item5: validar estatisticamente 62 plausíveis animação taxa FP controle — população total com ossos 3693 com extra 764 divisível bc*12 231 plausível finite<1e6 62 1,68% de 3693 plausível estrito [-pi,pi] 53 1,43%, critério fraco finite<1e6 real 62/3693=1,6789% per file controle vertex array 12B chunks finite<1e6 trials 55985 hits 55911 rate 99,86% per file 100% ratio 0,02× controle MAIOR que real NÃO discriminativo, critério estrito [-pi,pi] real 53/3693=1,4351% controle trials 55985 hits 2079 rate 3,71% per file 78/231 33,76% ratio per trial 0,39x per file 0,04x controle ainda maior NÃO discriminativo, padrão Object01 kf18 primeiros 9 zeros depois 0,628 1,256 etc múltiplos pi/5 Object04 kf98 primeiros 48 zeros depois 0,125 0,251 etc múltiplos pi/25 sugere critério mais discriminativo zeros iniciais + múltiplos ângulo base + monotonicidade, comparação ossos 0,0079% vs 43,93% ratio 1846x discriminativo multi-mesh 14,27% vs 0,0376% ratio 379x discriminativo UV 79,42% vs 70,34% ratio 1,13x rejeita animação finite<1e6 1,68% vs 100% ratio 0,02x NÃO discriminativo animação [-pi,pi] 1,43% vs 33,76% ratio 0,04x NÃO discriminativo, conclusão 62 plausíveis NÃO pode ser citado como achado confirmado animação permanece NÃO DECODIFICADA formalmente evidências `ciclo11_anim_validation.log` `ciclo11_anim_pi_validation.log` `ciclo11_item5_anim_validation_full.log` commit `7a9aa03`
- Item6: C1-C3 D1-D5 E1-E3 com provas e ratios per file corrigidos — C1 loose 3693 ratio per file 1846,5× C2 strict 1186 taxa 0/25218 limite sup ~0,004% ratio >3558× C3 prova strict ⊂ loose tool validate_A3_subset 6570 janelas strict 0 strict_not_loose 3693 loose files 1186 strict files 0 strict_not_loose PASS, D1-D5 multi-mesh strict_m4 1796 14,27% vs controle 0,0376% ratio 379× weak_m4 6572 52,20% vs controle 77,51% ratio 0,67× strict_files 1651 38,79% etc breakdown 1.685=1483 OPAQUE+67 invalid+134 geom+1 zero, E1-E3 UV float [0,1] 79,42% vs 70,34% ratio 1,13× rejeita offset 0 rate 61,80% ctrl 4,80% ratio 12,89× platô 9-26× offset 24 rate 84,23% ctrl 4,46% ratio 18,87× explicado como artefato floats pequenos lidos como u16, correções docs/COMPATIBILITY.md e ASSET_FORMATS.md tabela nível5 3.800→3.693 + 1.186 strict com taxa e ratio tools/check_docs_consistency.py nivel5 3800→3693 checker 56 verificações 0 falhas commit `50331cc`
- Item7: STATUS_FINAL.md criado com métricas corrigidas 3693/1186 provas ratios otimizações testes segurança bloqueios próxima tarefa ESTADO DO GIT até item6 6 commits push sim remote==local 88edfc73 intacto commit `28c9259`
- Item8: CAND-003→OPT-004 word-at-a-time JPEG scan 11,8x — problema OzjProbe SOS entropy-coded data scan byte-by-byte 179,2us median 272,7us p99, hipótese dados entropy ~0,39% FF 1/256 8B chunks sem FF 99,6% tempo skip 8B reduz iterações ~8x, mudança src/mu/assets/ozj.cpp SOS handling loop word-at-a-time remaining>=8 memcpy 8B chunk x=chunk ^ 0xFFFFFFFFFFFFFFFF has_zero = (x - 0x0101010101010101) & ~x & 0x8080808080808080 has_zero==0 => sem FF i+=8 continue senão encontra primeiro FF em 8B window via loop k=0..7 i+=k depois byte-by-byte trata escape FF 00 e FF D0..D7, unaligned safe via memcpy sem strict-aliasing sem leitura além file.size, preserva regras escape exatamente, teste make test 389 checks 0 failures real OZJ 13928 files 13832 ok 96 fail mesmo que antes prova correção preservada /tmp/test_ozj_real.cpp bench_assets synthetic JPEG 512x512+200KB scan 200 reps median 15,2us p95 22,5us p99 32,2us mean 16,7us MB/s 12525,2 vs antes 179,2us median 272,7us p99 speedup median 11,79x p99 8,47x throughput ~1060->12525 MB/s, evidência `ciclo11_opt003_ozj_benchmark.log`, decisão manter 11,8x melhoria caminho crítico textura loading baixo risco preserva correção CAND-003→OPT-004 commit `49825c5`

## Próxima tarefa: Fase C-F — backend VBO/VAO, otimizações restantes, segurança, roteiro Windows

**Fase C-D-E-F restante:**

- Fase C: tentar otimização CAND-001 (BGRA upload) com medição antes/depois, mas sem GPU para VBO/VAO real — BLOCKED GPU, testável CPU apenas para conversão
- Fase D: segurança/dependências, revisar SECURITY.md, DEPENDENCIES.md, LICENSING.md com novas evidências ciclo11 (OPAQUE full-scan 0 variantes plausíveis tentativa falha, animação 62 plausíveis não confirmado ratio 0,02×, OzjProbe 11,8x)
- Fase E: roteiro testes reais Windows, scripts em scripts/ já existem mas nunca executados, preparar roteiro reproduzível com PASS/FAIL/BLOCKED e evidência
- Fase F: build e distribuição, verificar Makefile, README, AGENTS, STATUS_FINAL.md, CICLO11_RELATORIO.md

É testável sem GPU: parsing CPU, benchmarks de carregamento, evidência automática, 389 checks 0 falhas, 56 verificações doc 0 falhas, OZJ real 13832 ok 96 fail.

**Critério de aceite Fase C-F:**
1. Otimizações medidas com evidência (mesmo que BLOCKED para GPU) — OPT-001 5,03×, OPT-003 3,9×, OPT-004 11,8x já medidos, CAND-001 pendente
2. make test e make test-san limpos (389 checks)
3. check_docs_consistency.py 0 falhas (56 verificações)
4. Documentação atualizada com evidência, sem inferência
5. Zero regressão nível3 8.406 nível4 8.405 nível5 3693 strict 1186 OPAQUE 1519

## Depois disso

| Tarefa | Por quê | Bloqueio |
|---|---|---|
| Backend VBO/VAO com skinning | DEC-008 piso mínimo | GPU/contexto BLOCKED |
| Escala vertical altura | 3 caminhos fechados | testável aqui |
| Parte B DEC-008 benchmark real | custo por quadro | GPU/contexto BLOCKED |
| Baseline FPS/VRAM | objetivo projeto | GPU BLOCKED |
| Validação visual Windows | compatibilidade | Windows/GPU BLOCKED |

## Verificação de conformidade, permanente

```
python3 tools/check_docs_consistency.py   # 56 verificações, 0 falhas
make clean && make test                    # 389/0 + logs ciclo11
make clean && make test-san                # 389/0 ASan 0
grep -rn "glGenBuffers\|wglCreateContext\|glDrawElements\|GL/gl.h" src/
```

Deve retornar apenas comentários. Verificado ciclo 11: 0 includes gráficos, 389 checks 0 falhas, 56 verificações 0 falhas, OZJ real 13832 ok 96 fail, BuxCrypt 3437 MB/s 3,9x, OzjProbe 15,2us 11,8x.
