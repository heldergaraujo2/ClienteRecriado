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
está** nos arquivos `.att`, que são atributos. Está em `TerrainHeight.OZB`. Os 76
arquivos são **um formato só**: BMP de 8 bits com prefixo duplicado de 4 bytes,
sendo que em 29 deles o cabeçalho está zerado. O cabeçalho real fica no offset 4,
confirmado em 44 dos 47 que o têm legível.

**Animação BMD.** O bloco que segue os ossos tem tamanho
`boneCount × keyframes × 12`, com 12 bytes = 3 floats. Confirmado em 385 de 500
arquivos (77,00%) sobre a árvore completa. O conteúdo dos três floats **não foi
determinado**.

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
| `mutool bmdanim` | Mede o tamanho do registro de animação | **executado** |

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
claramente suaves. 42 lidas pelo cabeçalho BMP, 5 em fallback, 29 sem cabeçalho.

**Animação** (`docs/evidence/ciclo2_bmdanim.log`): 500 arquivos com bloco após os
ossos, 385 (77,00%) com tamanho divisível por `12 × boneCount`; keyframes por osso
concentrados em 8, 12 e 14.

**Testes:** 344 checks, 0 failures (eram 208 no fim do ciclo 1).

**Um padrão que este ciclo produziu cinco vezes, e que vale mais que qualquer
número individual:** medir em subconjunto e publicar como se fosse o todo.
Ocorreu no cabeçalho BMD, no array de UV, no registro de triângulo, na suavidade
de altura (4,4 de um arquivo contra 8,038 reais) e no registro de animação (86,4%
de 88 contra 77,00% de 500). Todos foram corrigidos. A correção estrutural foi
transformar cada medição em comando de `mutool`, porque um script com teto não
declara o teto na saída.

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
errado); conteúdo dos 3 floats de keyframe; escala vertical da altura de terreno
(três caminhos examinados e fechados: prefixo todo zero, `pixelsPerMeter` é o
default de 72 DPI, paleta é identidade); malhas múltiplas.

**Limitação de ambiente registrada:** sem Windows, sem GPU, sem headers de dev de
GL/zlib/png/jpeg/SDL2, `apt-get install` inutilizável. Nada que exija esses
recursos pode ser declarado aprovado.

## 10. Próxima tarefa e por quê

**A tarefa registrada foi executada durante o ciclo.** O prefixo duplicado dos
arquivos de altura foi decodificado: o cabeçalho BMP real está no offset 4, e 42
arquivos passaram a ser lidos pela estrutura em vez de por fallback.

**O que está aberto agora, em ordem de impacto:**

1. **Conteúdo dos 3 floats de keyframe.** O tamanho do registro é conhecido
   (`boneCount × keyframes × 12`), o significado não. Sem isso não há animação.
   Exige o modelo de origem, que este projeto não tem.
2. **Escala vertical da altura.** Três caminhos fechados dentro do arquivo. Resta
   procurar em outro arquivo do mundo ou tratar como parâmetro de renderização.
3. **Unificar os dois caminhos do parser de terreno.** Os 29 arquivos sem
   cabeçalho são provavelmente o mesmo formato BMP. É simplificação, não correção
   de bug — a saída já é a mesma.

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
