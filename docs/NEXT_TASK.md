# Próxima tarefa

> **Ciclo 9 fechado** com correções críticas, validação estatística ossos 3.800, UV reinvestigado, DEC-012. Base `aa617ab` → HEAD atual. 7 commits atômicos itens 1-7 + 3 itens 8-10, checker 56 verificações 0 falhas, build limpo 389/0 ASan 0.

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

## Próxima tarefa: redução OPAQUE 15,05% e backend VBO/VAO com skinning (após UV sem progresso)

**Por que esta, e não outra.** Nível 4 83,30% (8.405) igual nível3 83,31% (8.406), nível5 37,66% (3.800) loose marginal 5,43x strict 19,05% 4803x confirmado com segurança 389 checks, nível6 UV 0% com 11 hipóteses descartadas mesmo com offset awareness (v0A4 vs v0C8, delta +-4, âncora 59B) — investigação encerrada com critério parada. DEC-012 escolheu UV por fidelidade visual, UV re-testado sem progresso.

Próximos bloqueios com maior valor agora:
- OPAQUE 1.519 (15,05%) — cifra não recuperada, bloqueia 15% da árvore, pré-requisito para aumentar nível3/4/5
- Animação 500 arquivos 385 divisível 77% — 3 f32 por keyframe não decodificados, bloqueia skinning
- Backend VBO/VAO com skinning — DEC-008 registrada piso mínimo, mas BLOCKED sem GPU/contexto

Já temos:
- Vértices, normais, faces, ossos hierarquia com ciclo detection em 3.800 arquivos
- Texturas OZT/OZJ/TGA 98-100%, altura terreno decodificada
- Validação estatística ossos 5,43x marginal 4803x strict, segurança 389 checks ASan0
- UV 11 hipóteses descartadas com controle mesma classe

O que falta:
- Reduzir OPAQUE: investigar se 1.519 usam chave diferente, compressão, ou versão 0x0B etc
- Decodificar animação: o que 3 floats representam, ordem keyframes, actionCount
- Backend VBO/VAO: BLOCKED GPU, mas parsing completo é pré-requisito

É testável sem GPU: parsing puro CPU, evidência automática.

**Critério de aceite:**
1. OPAQUE <15,05% ou animação decodificada com evidência bruta versionada, OU relatório conclusivo com evidência negativa
2. make test e make test-san limpos (389 checks)
3. check_docs_consistency.py 0 falhas (56 verificações)
4. Documentação com evidência em ASSET_FORMATS.md, sem inferência
5. Zero regressão nível3 8.406 nível4 8.405 nível5 3.800

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
