# Próxima tarefa

> **Ciclo 6 fechado** com auditoria ciclo 5 (itens 1-5). Base `56777bd` → HEAD atual.
> Ciclo 5 tinha regressão 8.406→8.401 (5 arquivos) devido a limite fixo 10k; corrigida.
> Nível 3 agora 8.406 (83,31%), nível 4 8.405 (83,30%), triângulos 3.681.664.

## O que mudou neste ciclo (ciclo 6)

**Regressão corrigida, contabilidade, validação, licenciamento, segurança:**

- Item 1: 5 arquivos (`Monster865.bmd`, `Ozarugold.bmd`, `Ozaruplata.bmd`, `FreddyKrueger.bmd`, `bann.bmd`) com `ntri` 11k–17k rejeitados por limite 10k. Fix: overflow check + `meshCount` max 64. Nível 3 volta a 8.406, nível 4 8.405 (+5). Evidência `ciclo6_level3_regression.log`.
- Item 2: contabilidade nível 4 falhando 1.685 = 1.483 OPAQUE + 67 inválidos + 134 geometria não parseada + 1 zero faces. Multi-mesh 0. Evidência `ciclo6_level4_breakdown.log`.
- Item 3: validação multi-mesh full-scan 4.256 arquivos, 12.590 malhas, parsing 100%, nome estrito 14,27% vs controle 0,0376% (379×). Evidência `ciclo6_multimesh_validation.log`.
- Item 4: licenciamento `xulek/muonline-bmd-viewer` commit `3f11bf89f3702261174d8efa1436d56a15c7d97a`, `bmd-loader.ts:348-422`, ISC. Loop inspirado, header 14 B vs 10 B e triângulo diferente são deste projeto. Atribuição em `bmd_body.cpp`, `LICENSING.md`, `DEPENDENCIES.md` §6.
- Item 5: segurança multi-mesh, `meshCount` limitado, overflow checado, `test-san` 384/0, testes extremos. Evidência `ciclo6_test_san_final.log`.

## Próxima tarefa: parsing completo de ossos

**Por que esta, e não outra.** Nível 4 agora é 83,25%, praticamente igual ao
nível 3 (83,26%). O gargalo para malha montável foi removido. Próximo bloqueio
para renderização com skinning é nível 5 (ossos): 3.505 arquivos (34,74%) hoje.

Já temos:

- Nomes de osso em 73,7% dos arquivos de malha única (345/468), nomes reais do
  3ds Max (Box01, Bip01 etc)
- Registro de osso intercalado 59 B, confirmado em 98,0% (901/919) com tamanho
  `boneCount*59-1`
- Transformação: u16 pai (65535=raiz 75,9%) + 6 f32 100% finitos + 1 B zero,
  hierarquia decodificada
- Animação: 385/500 =77,00% com `boneCount*keyframes*12` bytes

O que falta:

- Ordem dos 6 floats: posição (x,y,z) + rotação (x,y,z) é inferência a partir de
  frequência de não-zero (66% vs 30%), não confirmação
- Sistema de coordenadas, graus vs radianos, convenção Euler
- Por que modelo tem -1 byte
- Seção de animação: 3 floats por keyframe, 2 constantes +1 variando, não
  determinado o que representam

É testável sem GPU: parsing puro CPU, evidência automática como em ciclo 5.

**Critério de aceite:**

1. `mutool bmdreport` nível 5 acima de 3.505, com evidência bruta versionada
2. `make test` e `make test-san` limpos (384 checks)
3. `check_docs_consistency.py` 0 falhas (36→40 verificações após ciclo 6)
4. Documentação de ordem dos floats com evidência, não inferência
5. Zero regressão em nível 4 (8.405) e nível 3 (8.406)

**O que isso NÃO destrava:** nível 6 (UV) continua zero. Modelo com esqueleto
pode ser animado sem textura; texturizado não.

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
python3 tools/check_docs_consistency.py   # 36 verificações, 0 falhas
grep -rn "glGenBuffers\|wglCreateContext\|glDrawElements\|GL/gl.h" src/
```

Deve retornar apenas comentários. Verificado no ciclo 5: 0 includes gráficos.
