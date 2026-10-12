# Ciclo 2 — relatório de fim de ciclo

Branch `arena/519f8fc9-clienterecriado`. Commit de referência `12aa5d1` (fim do
ciclo 1). Lista canônica de commits: `git log --oneline 12aa5d1..HEAD`.

Este documento não enumera commits nem copia hash de ponta. Ambos defasam no
momento em que são escritos, e neste ciclo a seção de Git ficou defasada quatro
vezes por esse motivo antes de a causa ser identificada como estrutural.

---

## 1. O que foi pesquisado

**Formato BMD, campo a campo, por medição sobre os 10.090 arquivos reais** — não
por leitura de especificação externa. Cada conclusão tem um controle ao lado.

- Cifra v0x0C: validada de forma independente por nomes ASCII e normais unitárias
  (99,98% de magnitude 1,00, contra ~32% esperado por acaso)
- Registro de triângulo de 64 bytes: três referências de 8 B em +0/+8/+16, cada
  uma `[u16 vértice][u16 normal][u16 padding][u8 flag][u8 pad]`
- Array de coordenadas UV: zero não-finitos em 250.450 leituras, onde o acaso
  daria ~976
- Registro de osso: intercalado, 59 bytes (32 B de nome + 27 B de transformação)
- Transformação de osso: `u16` pai (65535 = raiz em 75,9%) + 6 floats 100%
  finitos + 1 byte de padding
- Seção de animação: localizada após os ossos, não decodificada

**Altura de terreno.** Descoberta que corrigiu uma suposição: a altura **não
está** nos arquivos `.att`, que são atributos. Está em `TerrainHeight.OZB`, e 47
dos 76 desses arquivos são imagens BMP de 8 bits cujo dado de pixel é a grade.

**Índice de UV por canto.** Investigação encerrada com **oito hipóteses
descartadas e documentadas**. Nenhuma se sustentou.

## 2. O que foi implementado

| Módulo | O que faz | Estado |
|---|---|---|
| `src/mu/assets/mesh.hpp/.cpp` | Representação interna de malha + importador BMD (caminho A) | **testado** |
| `src/mu/assets/terrain.cpp` | Leitor de altura de terreno, com BMP e fallback marcado | **testado** |
| `src/mu/render/cull.cpp` | Frustum culling por caixa delimitadora | **testado** |
| `mutool meshimport` | Pipeline BMD completo sobre a árvore real | **executado** |
| `mutool culltest` | Mede culling sobre malhas reais | **executado** |
| `mutool terrain` | Lê as 76 grades de altura e mede suavidade | **executado** |

**Nada disso é renderização.** Não há nenhuma linha de código OpenGL, porque não
há GPU nem headers de GL neste ambiente. O que existe é a camada de dados e a
matemática de culling, ambas testáveis em CPU.

## 3. Problemas resolvidos

1. **Cifra v0x0C** — 7.226 arquivos que eram ilegíveis passaram a payload em
   claro. Nível 2 foi de 14,83% para 84,95%
2. **Topologia** — 1.221.030 triângulos lidos em 4.149 arquivos
3. **Hierarquia de ossos** — de "inteiramente não examinada" para decodificada em
   98,0% dos arquivos cujo tamanho bate
4. **Altura de terreno** — de "não localizada" para legível em 76 arquivos
5. **Quatro identificações falsas que este projeto havia feito**, todas corrigidas
   por medição: campos 12/14 do vértice não são índice de osso; ossos não são
   bloco de nomes seguido de transformações; altura não está nos `.att`; o prefixo
   de altura não é constante

## 4. Testes executados

| Comando | Resultado |
|---|---|
| `make test` | **329 checks, 0 failures** — **PASS** |
| `make test-san` (ASan+UBSan) | **329 checks, 0 failures** — **PASS** |
| Compilação com 12 flags `-W` | **0 warnings** — **PASS** |
| `mutool meshimport` sobre 10.090 arquivos | exit 0, 8.408 malhas — **PASS** |
| `mutool terrain` sobre 76 grades | exit 0, 72 de 76 suaves — **PASS** |
| `mutool culltest` | exit 0 — **PASS**, com ressalva registrada |
| Linkar OpenGL | **BLOCKED** — sem headers de GL neste ambiente |
| Executar no Windows | **NOT EXECUTED** — sem acesso a Windows |
| Medir FPS/frametime/VRAM | **NOT EXECUTED** — sem GPU |

## 5. Métricas medidas

**Cobertura de formato BMD:**

| Nível | Ciclo 1 | Fim do ciclo 2 |
|---|---|---|
| 1 — estrutura válida | 10.023 (99,34%) | 10.023 (99,34%) |
| 2 — payload em claro | 1.496 (14,83%) | **8.571 (84,95%)** |
| 3 — geometria parseada | 1.191 (11,80%) | **8.406 (83,31%)** |
| 4 — topologia | — | **4.149 (41,12%)** |
| 5 — hierarquia de ossos | — | **901 de 919 (98,0%)** |
| OPAQUE | 8.594 (85,17%) | **1.519 (15,05%)** |

**Pipeline BMD → malha interna** (`docs/evidence/ciclo2_meshimport.log`):
8.408 malhas (83,33%), 1.792.108 vértices, 4.071 com triângulos, 978.853
triângulos, 242.177 referências de canto descartadas por estarem fora de faixa.

**Altura de terreno** (`docs/evidence/ciclo2_terrain.log`): 76 grades, diferença
média entre vizinhos **8,038** contra ~85,3 do aleatório; 72 de 76 (94,74%)
claramente suaves.

**Testes:** 329 checks, 0 failures (eram 208 no fim do ciclo 1).

## 6. Comparação com a linha de base

**Não há linha de base de desempenho para comparar.** Nenhum número de FPS,
frametime, latência de entrada, VRAM ou tempo de carregamento foi medido, porque
este ambiente não tem GPU nem Windows. Afirmar melhoria de desempenho aqui seria
inventar.

O que pode ser comparado é cobertura de formato e contagem de testes, e ambas
estão na tabela acima. A única otimização com antes/depois medido continua sendo
**OPT-001** (CRC32, 391,6 → 2.553,5 MB/s, 5,03×), do ciclo 1.

## 7. Arquivos alterados

11 arquivos `.cpp` em `src/mu/`, 20 documentos em `docs/`, 24 evidências em
`docs/evidence/`. Lista exata: `git diff --stat 12aa5d1..HEAD`.

## 8. Commits e push

Todos os commits do ciclo tiveram push confirmado no momento do commit.

Este documento **não copia hash de ponta nem enumera commits**, porque o commit
que escreve o documento é posterior ao que ele descreve. Nesta seção de Git a
defasagem ocorreu quatro vezes antes de a causa ser identificada como estrutural.
Para ver o estado real:

```
git rev-parse HEAD
git ls-remote origin arena/519f8fc9-clienterecriado
git status --short
git log --oneline 12aa5d1..HEAD
```

No momento da escrita, local e remoto coincidiam e a árvore estava limpa.

## 9. Problemas restantes

**Bloqueio nº 1 — índice de UV por canto. NÃO RESOLVIDO, investigação encerrada.**
Oito hipóteses descartadas e documentadas em `ASSET_FORMATS.md`. Sem ele, os 4.149
arquivos com topologia **não podem ser texturizados**. Malha sem textura serve para
colisão, caixa delimitadora e análise, não para renderização fiel.

**Bloqueio nº 2 — 1.519 arquivos v0x0C permanecem OPAQUE.** Cifra não recuperada.

**Bloqueio nº 3 — nenhuma linha de código OpenGL.** O pipeline VBO/VAO (DEC-008)
não tem implementação. Exige GPU e headers de GL.

**Não determinados:** ordem dos 6 floats de transformação de osso (inferida, não
confirmada — aplicar posição e rotação trocadas daria esqueleto silenciosamente
errado); seção de animação; escala vertical da altura de terreno; prefixo
duplicado dos 46 arquivos BMP; malhas múltiplas.

**Limitação de ambiente registrada:** sem Windows, sem GPU, sem headers de dev de
GL/zlib/png/jpeg/SDL2, `apt-get install` inutilizável. Nada que exija esses
recursos pode ser declarado aprovado.

## 10. Próxima tarefa e por quê

**Decodificar o prefixo duplicado dos arquivos de altura de terreno.**

É a única descoberta recente com caminho concreto aberto. Os 47 arquivos
`TerrainHeight.OZB` que abrem com `"BM"` têm cabeçalho ilegível (`pixoff = 0`,
`bpp = 0`, valor 40 onde deveria estar a largura) e os primeiros quatro bytes
aparecem **duas vezes** — o mesmo padrão de prefixo duplicado que o formato OZJ
deste cliente usa. A hipótese é que o cabeçalho real comece após o prefixo.

**Por que essa e não outra:** hoje 46 arquivos são lidos por fallback, o que
funciona em 94,74% dos casos mas não é decodificação — o campo `header_ignored`
marca isso no código.

**Por que não é urgente, e isso está registrado para o próximo ciclo não
superestimar:** o fallback já produz campo suave na maioria dos casos, e a escala
vertical continua desconhecida. Sem ela a altura é relativa, não métrica, e
terreno não fica renderizável de qualquer forma.

**Alternativa de maior impacto, se houver acesso a GPU:** iniciar o pipeline
VBO/VAO (DEC-008). É pré-requisito de todo o requisito de desempenho e não está
bloqueado por nenhuma descoberta pendente — apenas pela ausência de hardware.

---

## ESTADO DO GIT NESTE CICLO

```
commits locais        : git log --oneline 12aa5d1..HEAD
push realizado        : SIM, em todos os commits
hash local × remoto   : git rev-parse HEAD  ×  git ls-remote origin arena/519f8fc9-clienterecriado
                        coincidiam no momento da escrita
git status --short    : vazio — árvore limpa
referência Cliente_ref: 88edfc73 no início e no fim — INTACTA, não modificada
nada ficou sem commit
```
