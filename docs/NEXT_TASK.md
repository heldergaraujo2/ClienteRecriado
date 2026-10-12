# Próxima tarefa

> **Ciclo 5 fechado** com `docs/CICLO5_RELATORIO.md`. Base `7d10a18` → HEAD
> `e32a659` + relatório (7 commits). Auditoria do ciclo 4 resolvida.

## O que mudou neste ciclo

**Multi-mesh foi implementado.** Nível 4 foi de 4.149 (41,12%) para **8.400
(83,25%)**, +4.251 arquivos, triângulos de 1.221.030 para 3.524.366. Evidência em
`evidence/ciclo5_bmd_multimesh.log`. A causa de 100% da queda (meshCount !=1)
agora é 0% — só 1 arquivo sem topologia, que declara zero faces.

DEC-008 foi corrigida: expansão é custo de CARREGAMENTO (19.190,9 µs, 5,49×,
5,14×–5,62× entre execuções, N=5), não por quadro. Volume por quadro é quantidade
exata (1,29×, 63,9% redundantes) com custo BLOCKED.

Verificação automatizada `check_docs_consistency.py` construída (36 verificações,
0 falhas) com testes de injeção que provam não ser vacuosa.

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
2. `make test` e `make test-san` limpos (373 checks)
3. `check_docs_consistency.py` 0 falhas
4. Documentação de ordem dos floats com evidência, não inferência
5. Zero regressão em nível 4 (8.400)

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
