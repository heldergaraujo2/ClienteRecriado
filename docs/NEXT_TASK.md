# Próxima tarefa

> **Ciclo 7 fechado** com auditoria ciclo 6 (itens 0-6, 7 commits atômicos). Base `677a2cb` → HEAD `c29f473`.
> Todos os bloqueios críticos resolvidos, checker 36→53 verificações 0 falhas, build limpo 384/0 ASan 0.

## O que mudou neste ciclo (ciclo 7)

**Auditoria ciclo 6 — 7 itens, todos resolvidos em commits atômicos:**

- Item 0 (bloqueio crítico): evidência bruta `git -C /tmp/bmdref log -1 --format=fuller 3f11bf89` e `git show --format="%an <%ae>%n%ad%n%cd" --date=iso` → Author `xulek121@gmail.com` (não `example.com`), data 2026-07-24, branch `main`, repo 2025-07-02 a 2026-07-24 com clock skew. `package.json` author `xulek@example.com` placeholder RFC2606, sem LICENSE file. Marcado NÃO CONFIRMADA. Evidência `ciclo6_bmdref_commit_date.log`.
- Item 1: população multi-mesh 4257/4251/4256 reconciliada. Ferramenta `mutool bmdreport` confirma 4256 verdadeiro multi-mesh (`>1`), 4257 total `!=1` (4256+1 zero), 4251 histórico snapshot 8401 com regressão. Erro anterior: contar 4257 como multi-mesh puro sem separar zero-faces. Evidência `ciclo6_item1_bmdreport.log`.
- Item 2: violação commits atômicos `1fc1024` (14 arquivos, 3 itens agrupados) documentada DEC-011 e CHANGELOG, processo corrigido 1 commit por item.
- Item 3: build limpo `make clean && make test` 384/0 e `make clean && make test-san` 384/0 ASan 0, logs `ciclo6_test_clean.log` e `ciclo6_test_san_clean.log`.
- Item 4: checker expandido 36→53 verificações, cobre breakdown 1685=1483+67+134+1 e taxas 14,27%/0,0376%/379×/52,20%/77,51%/0,67%/38,79%/100%. 0 falhas.
- Item 5: controle estatístico com precisão UV descrito — o que buscado (nome 32B tri_end+delta), por que mesma classe (lição UV 24,82% vs 79,42% 3,2× falso, 70,34% vs 79,42% 1,13× rejeita), risco controle fraco (52% vs 77% 0,67× não discrimina, desalinharia parser).
- Item 6: confirmação segurança `git show --stat 0a09a2b` 6 arquivos +469/-9, 4 testes Extreme/Truncated/Overflow/LargeCounts, ASan 0 reports, evidência `ciclo6_item6_security.log`.

## Próxima tarefa: parsing completo de ossos

**Por que esta, e não outra.** Nível 4 agora é 83,30% (8.405 arquivos), praticamente igual ao nível 3 (83,31%). Multi-mesh 100% parseado, não há mais ganho fácil em nível 4. Próximo bloqueio para renderização com skinning é nível 5 (ossos): 3.505 arquivos (34,74%) hoje.

Já temos:

- Nomes de osso em 73,7% dos arquivos de malha única (345/468), nomes reais do 3ds Max (Box01, Bip01 etc)
- Registro de osso intercalado 59 B, confirmado em 98,0% (901/919) com tamanho `boneCount*59-1`
- Transformação: u16 pai (65535=raiz 75,9%) + 6 f32 100% finitos + 1 B zero, hierarquia decodificada
- Animação: 385/500 =77,00% com `boneCount*keyframes*12` bytes

O que falta:

- Ordem dos 6 floats: posição (x,y,z) + rotação (x,y,z) é inferência a partir de frequência de não-zero (66% vs 30%), não confirmação
- Sistema de coordenadas, graus vs radianos, convenção Euler
- Por que modelo tem -1 byte
- Seção de animação: 3 floats por keyframe, 2 constantes +1 variando, não determinado o que representam

É testável sem GPU: parsing puro CPU, evidência automática como em ciclo 6-7.

**Critério de aceite:**

1. `mutool bmdreport` nível 5 acima de 3.505, com evidência bruta versionada
2. `make test` e `make test-san` limpos (384 checks)
3. `check_docs_consistency.py` 0 falhas (53 verificações após ciclo 7)
4. Documentação de ordem dos floats com evidência, não inferência
5. Zero regressão em nível 4 (8.405) e nível 3 (8.406)

**O que isso NÃO destrava:** nível 6 (UV) continua zero. Modelo com esqueleto pode ser animado sem textura; texturizado não.

## Depois disso

| Tarefa | Por quê | Bloqueio |
|---|---|---|
| Índice de UV por canto | 9 hipóteses descartadas, critério de parada | investigação |
| Escala vertical altura | 3 caminhos fechados | testável aqui |
| Backend VBO/VAO | decisão registrada | GPU/contexto BLOCKED |
| Parte B DEC-008 | benchmark real | GPU/contexto BLOCKED |
| Baseline FPS/VRAM | objetivo do projeto | GPU BLOCKED |

## Verificação de conformidade, permanente

```
python3 tools/check_docs_consistency.py   # 53 verificações, 0 falhas (era 36)
make clean && make test                    # 384/0 + logs ciclo6_test_clean.log
make clean && make test-san                # 384/0 ASan 0 + logs ciclo6_test_san_clean.log
grep -rn "glGenBuffers\|wglCreateContext\|glDrawElements\|GL/gl.h" src/
```

Deve retornar apenas comentários. Verificado ciclo 7: 0 includes gráficos.
