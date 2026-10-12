# Próxima tarefa

> **Ciclo 10 Fase A fechada** com correção ratio per file 1846×, headline conservador strict 1186, strict ⊂ loose provado, UV ratio-por-offset platô 9-26×. Base `84fee45` → HEAD atual. Fase A itens A1-A5, checker 56 verificações 0 falhas, build limpo 389/0 ASan 0. Métricas corrigidas: total geom 8406, loose 3693 (43,93% geom, 36,60% total, 7879 bones) ratio per file 1846× altamente discriminativo (antes 5,43× marginal devido falta ciclo detection + unidades diferentes), strict 1186 (14,11% geom, 11,75% total) taxa falso-positivo 0/25218, limite superior ~0,004% (1/25218), ratio >3558× headline conservador, strict ⊂ loose provado 0 falhas.

> **Ciclo 9 fechado** anteriormente com correções críticas, validação estatística ossos 3.800 (agora corrigido para 3693 após ciclo detection), UV reinvestigado, DEC-012.

## O que mudou neste ciclo (ciclo 9)

**Itens 1-10 do comando único abrangente:**

- Item1: ajuste critério 3505→3800 explicado honestamente — old mc==1 bc1..8 remain==bc*59-1, new bc1..64 mc1..64 multi extra ac remain>=bc*59-1, exemplos multi PrickleGlove mc4 bc1, bc>8 sword48 bc10 Statue_Blue bc15 newdoor bc24, anim extra Rank01 bc3 remain464 exp176 extra288=3*8*12
- Item2: validação estatística 59B full-scan 8406 geom real loose 3800 45,21% strict 1601 19,05% control 25218 trials loose 8,3234% strict 0,0040% ratio loose 5,43x marginal strict 4803x altamente discriminativo
- Item3: segurança parser ossos ciclo detection A->B->A, bc extremo 0xFFFF OOM-safe, truncado, overflow bc*59, 389 checks ASan0
- Item4: check_docs_consistency 56 verificações 0 falhas após último commit conteúdo, 384→389 checks
- Item5: atribuição xulek corrigida bmd.hpp de ISC confirmada para NAO CONFIRMADA com commit 3f11bf89 placeholder xulek@example.com sem LICENSE file clock skew
- Item6: 14 categorias multi-mesh soma 4256 vem de saída bruta mutool bmdreport meshCount 0:1 1:4149 2:2280 3:1086 4:449 5:190 6:114 7:42 8:38 9:12 10:10 11:6 12:7 13:2 14:13 >15:7
- Item7: SECURITY.md 9.4 relógio sistema outubro/2026 durante verificação commit externo 3f11bf89 AuthorDate 2026-07-24 desordem 2025-11-09 clock skew, mitigação validar por meio independente
- Item8: DEC-012 prioridade formal UV vs ossos/animação — UV pré-requisito QUALQUER renderização fiel reconhecível, ossos pré-requisito animação mas sem UV não comparável mesmo estático, estado UV 0% 9 hipóteses ossos 37,66% marginal 19,05% strict 4803x, decisão prioridade restante ciclo retomar UV (não por facilidade/recência)
- Item9: reinvestigação UV com consciência offset v0A4 vs v0C8 base correta payload após bf1668c, delta multi +-4 textura tri_end-4 sem espaço triângulos-ossos 0 bytes, âncora ossos 59B, re-teste full-scan 4149 offset 0 rate61,80% control6,27% ratio9,86x platô 24..59 rate84% ratio14x, float [0,1] 79,42% vs 70,34% ratio1,13x rejeita, 10a espaço 0 bytes 11a derivado não igual, lista agora 11 tentativas UV permanece NAO LOCALIZADO
- Item10: continuidade autônoma animação 500 arquivos 385 divisível 77% bc*keyframes*12 dump Rank01 com valores enormes -1.7e38 -1e10 não plausíveis, animação permanece NAO DECODIFICADA, autoauditoria 6 itens todas SIM

## Próxima tarefa: Fase C-D-E-F — backend VBO/VAO, otimizações, segurança, roteiro Windows

**Fase B fechada:** animação parcialmente decodificada 62 arquivos plausíveis (rotação [-pi,pi]), OPAQUE investigado com brute force init/step (1 arquivo IceGolem step 0x33 encontrado, maioria não com mesma key array), métricas mantidas 3693 loose 1186 strict OPAQUE 1519.

**Por que esta, e não outra.** Nível 4 83,30% (8.405) igual nível3 83,31% (8.406), nível5 loose 36,60% (3693) ratio 1846×, strict 11,75% (1186) headline conservador, animação 62 plausíveis com rotação [-pi,pi] (Object01 kf18, Object04 kf98 etc) mas sem parser formal actionCount→kf, OPAQUE 15,05% com 1 variação de chave encontrada (step 0x33) mas não generalizável, UV 0% 11 hipóteses + tabela platô 9-26× explicada.

Fase B resultados:
- Animação: 764 com extra, 231 divisível bc*12, 62 plausíveis finite <1e6 <1e10 com valores rotação [-pi,pi] (0.628,1.256,1.884,2.513,3.141 etc), antes valores enormes -1.7e38 eram artefato parsing falho Rank01 com bone1 vazio
- OPAQUE: brute force 20 primeiros, 1 encontrado IceGolem step 0x33 vs padrão 0x3D, 65536 combos timeout para 5 primeiros sem sucesso para CustomNpc02, indica key array diferente ou compressão, OPAQUE permanece 1519
- Backend VBO/VAO ainda BLOCKED GPU, mas parsing ossos+animação parcial é pré-requisito

Já temos:
- Vértices, normais, faces, ossos 3693/1186 com ciclo detection, animação 62 plausíveis rotação
- Texturas 98-100%, terreno decodificado
- Validação estatística 1846×, strict ⊂ loose, UV platô explicado, OPAQUE brute force evidência
- 389 checks 0 falhas, 56 verificações 0 falhas

O que falta (Fase C-D-E-F):
- Fase C: tentar otimizações CAND-001 (BGRA upload), CAND-002 (bux vectorizado), CAND-003 (JPEG scan) com medição antes/depois, mas sem GPU para VBO/VAO real
- Fase D: segurança/dependências, revisar SECURITY.md, DEPENDENCIES.md, LICENSING.md com novas evidências
- Fase E: roteiro testes reais Windows, scripts em scripts/ já existem mas nunca executados, preparar roteiro reproduzível
- Fase F: build e distribuição, verificar Makefile, README, AGENTS

É testável sem GPU: parsing CPU, benchmarks de carregamento, evidência automática.

**Critério de aceite Fase C-F:**
1. Otimizações medidas com evidência (mesmo que BLOCKED para GPU)
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
make clean && make test                    # 389/0 + logs ciclo9_item3_security_test.log
make clean && make test-san                # 389/0 ASan 0
grep -rn "glGenBuffers\|wglCreateContext\|glDrawElements\|GL/gl.h" src/
```

Deve retornar apenas comentários. Verificado ciclo 9: 0 includes gráficos.
