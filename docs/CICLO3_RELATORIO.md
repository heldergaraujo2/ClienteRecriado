# Ciclo 3 — relatório de fim de ciclo

Branch `arena/519f8fc9-clienterecriado`. Base do ciclo: `c03264b` (fechamento do
ciclo 2). Lista canônica: `git log --oneline c03264b..HEAD`.

Este documento não copia hash de ponta nem enumera commits. A causa é estrutural:
o commit que escreve o documento é posterior ao que ele descreve, e nesta seção a
defasagem ocorreu quatro vezes no ciclo 2 antes de a causa ser identificada.

---

## 0. ESCLARECIMENTO PRIORITÁRIO — a contradição sobre topologia

A auditoria apontou contradição entre "4.149 arquivos com topologia" e o registro
anterior de topologia como não resolvida. **A contradição existia, e a auditoria
estava certa em exigi-la. Mas a direção do erro é o inverso do que a pergunta
supunha.**

**A topologia está resolvida e implementada em C++. Eram os documentos que
continuavam dizendo que não.**

Evidência, verificada neste ciclo:

**1. O código existe.** `src/mu/assets/bmd_body.cpp`:

```
linha 117:  const std::size_t triangles_offset = required + texcoord_count * 8;
linha 135:        out->triangles.push_back(t);
```

**2. Os testes existem e passam.** `TestBmdTriangles`, 12 asserções sobre o
conteúdo lido:

```
CHECK_EQ(1u, parsed.triangles.size());
CHECK_EQ(0, parsed.triangles[0].corners[0].vertex);
CHECK_EQ(1, parsed.triangles[0].corners[1].vertex);
CHECK_EQ(2, parsed.triangles[0].corners[2].vertex);
CHECK_EQ(2, parsed.triangles[0].corners[2].normal);
CHECK_EQ(0xFFFFu, parsed.triangles[0].corners[0].optional);
CHECK_EQ(3u, parsed.triangles[0].corners[1].flag);
```

Mais `TestMeshFromBmd`, que testa a conversão para a malha interna incluindo o
descarte de índice fora de faixa. `make clean && make test`: **344 checks, 0
failures**.

**3. O número sai do parser, não de script.** `mutool bmdreport`, nível 4:

```
with triangles                            4149  ( 41.12%)
triangles read                         1221030
corner refs out of range                306109  ( 8.357% of refs)
```

**Por que os documentos estavam errados.** `COMPATIBILITY.md` foi escrito quando a
topologia era apenas prova de conceito em Python. Os commits `03052f0` (registro
decodificado) e `bee2a5e` (parser C++) vieram depois e **não atualizaram o
documento**. A seção continuou afirmando "não está implementada em C++" por
dezenas de commits.

**Por que não foi destacado como conquista.** Porque o documento de estado dizia
que não existia. O relatório citava "4.149 arquivos com topologia" — número certo
— mas `COMPATIBILITY.md` dizia na mesma página que a topologia não estava
implementada. Os dois não podem ser verdade, e o relatório não reconciliou.

**Definição precisa, para eliminar a ambiguidade do termo:**

| Termo | Significado exato |
|---|---|
| "arquivo com topologia" | Parser C++ leu o array de triângulos e produziu `BmdTriangle` com três cantos, cada um com índice de vértice e de normal |
| "4.149" | Arquivos de **malha única** em que isso ocorreu, de 10.090 no total |
| O que **não** inclui | Coordenadas de textura por canto. `has_texcoords` fica `false` |

Topologia (índices de face) e índice de UV por canto são **coisas diferentes**. A
confusão entre elas é o que gerou a contradição.

---

## 1. O que foi pesquisado

Este ciclo foi de **auditoria e verificação**, não de descoberta nova. Cada
número herdado foi re-medido contra a árvore completa.

- Topologia: confirmada em código, testes e ferramenta
- Hierarquia de ossos: full-scan sem teto, substituindo medição amostral
- Animação: confirmado que a população é total, não corte
- BMP de altura: população completa verificada (76 arquivos, 47 + 29)
- Nona hipótese de UV testada: bytes 24..59 como coordenadas por canto

## 2. O que foi implementado

| Item | Tipo |
|---|---|
| `tools/bmd_bones_probe.py` | Full-scan de ossos, sem teto, imprime denominador |
| `DEC-010` | Registro retroativo do culling antes da decisão de pipeline |
| `docs/ARCHITECTURE.md` §10, §11 | Limites reais do BMD; escalabilidade de renderização |
| Correções em 5 documentos | Topologia, ossos, BMP, numeração de ciclo |

**Nenhum código de backend gráfico foi escrito.** `DEC-010` lista o que continua
bloqueado até `DEC-008` estar completa.

## 3. Problemas resolvidos

**Quatro afirmações falsas corrigidas, todas por medição:**

| Afirmação anterior | Corrigida para |
|---|---|
| Topologia não implementada em C++ | **Implementada, com testes, desde `bee2a5e`** |
| Ossos 98,0% | **98,68% de 3.552** = 85,09% dos 4.119 elegíveis = 34,74% da árvore |
| BMP 44 de 47 | **42** (41 no offset 4 + 1 no offset 0) |
| Referências fora de faixa pendentes | **Já tratadas** — descartadas e contadas em `bad_indices` |

**Um falso-positivo evitado.** A nona hipótese de UV deu 79,42% em [0,1]. Contra
bytes aleatórios (24,82%) pareceria forte, razão 3,2×. **Esse controle é
inválido**: a pergunta não é "dados ou ruído", é "estes `f32` são UV". O controle
correto — array de normais, `f32` reais que não são UV — dá **70,34%**, razão de
apenas **1,13×**. Hipótese rejeitada.

**Regra nova que saiu disso:** o controle tem que ser da mesma classe do que se
testa. Está registrada em `ASSET_FORMATS.md`, não só na mensagem de commit.

## 4. Testes executados

| Comando | Resultado |
|---|---|
| `make clean && make test` | **344 checks, 0 failures** — **PASS** |
| Compilação com 12 flags `-W` | **0 warnings** — **PASS** |
| `mutool bmdreport` (árvore completa) | exit 0 — **PASS** |
| `mutool meshimport` (árvore completa) | exit 0, 8.408 malhas — **PASS** |
| `mutool terrain` (76 arquivos) | exit 0 — **PASS** |
| `mutool bmdanim` (árvore completa) | exit 0, 500 arquivos — **PASS** |
| `python3 tools/bmd_bones_probe.py` | exit 0, 10.090 arquivos — **PASS** |
| `grep glGenBuffers\|wglCreateContext\|GL/gl.h src/` | 1 comentário, 0 código — **PASS** |
| Linkar OpenGL | **BLOCKED** — sem headers de GL |
| Executar no Windows | **NOT EXECUTED** — sem acesso |
| Medir FPS/frametime/VRAM | **NOT EXECUTED** — sem GPU |

## 5. Métricas medidas

**Tabela final de níveis de parsing BMD** — `docs/evidence/ciclo3_scan_final.log`,
execução do zero com o código atual:

| Nível | Arquivos | % de 10.090 |
|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% |
| 2 — decifrado (payload em claro) | 8.571 | 84,95% |
| 3 — vértice + normal parseáveis | 8.406 | 83,31% |
| 4 — **faces parseáveis** | **4.149** | **41,12%** |
| 5 — **ossos parseáveis** | **3.505** | **34,74%** |
| 6 — **UV parseável** | **0** | **0,00%** |

Nível 2 = plaintext 1.245 + bux 100 + v0x0C 7.226. OPAQUE 1.519 (15,05%).

**Os níveis não são cumulativos na mesma população, e isso importa.** O nível 4
conta apenas arquivos de **malha única**; malhas múltiplas não são parseadas porque
o layout não foi confirmado. O nível 5 conta arquivos cuja hierarquia de ossos foi
lida — 3.505 de 4.119 elegíveis (85,09%), o que sobre a árvore inteira é 34,74%.
Citar "98,68%" sem o denominador foi exatamente o erro corrigido neste ciclo.

**Nenhum nível é um modelo renderizável.** O nível 6 é zero: sem índice de UV por
canto não há textura, e sem ordem confirmada dos floats de transformação não há
skinning.

**Faces:** 1.221.030 triângulos, 306.109 referências fora de faixa (8,357%).

**Malha interna** (`meshimport`): 8.408 convertidas, 1.792.108 vértices, 4.071 com
triângulos, 978.853 triângulos, 242.177 referências descartadas.

**Ossos** (`bmd_bones_probe.py`, full-scan sem teto):

| População | Arquivos | Taxa |
|---|---|---|
| `.bmd` vistos | 10.090 | — |
| Elegíveis (malha única com ossos) | 4.119 | 40,82% dos vistos |
| Tamanho bate com `bc×59−1` | 3.552 | 86,23% dos elegíveis |
| Nomes ASCII válidos | 3.505 | **98,68% dos que fecham** · 85,09% dos elegíveis · 34,74% da árvore |

**Animação** (`bmdanim`, população total, sem teto): 500 arquivos com bloco após
os ossos, 385 (77,00%) divisível por `12 × boneCount`.

**Altura de terreno** (100% da população): 76 arquivos = 47 com `"BM"` + 29 sem.
Dos 47: 41 com cabeçalho no offset 4, 1 no offset 0, 5 sem. Dos 29: prefixo todo
zero em 29 de 29.

**Testes:** 344 checks, 0 failures (208 no fim do ciclo 1).

## 6. Comparação com a linha de base

**Não há linha de base de desempenho.** Nenhum número de FPS, frametime, latência,
VRAM ou carregamento foi medido — sem GPU nem Windows. A única otimização com
antes/depois medido continua sendo **OPT-001** (CRC32, 391,6 → 2.553,5 MB/s,
5,03×), do ciclo 1.

O que mudou neste ciclo foi **cobertura de parsing e correção de documentação**,
não desempenho.

## 7. Arquivos alterados

Lista exata: `git diff --stat c03264b..HEAD`. Cinco documentos corrigidos, uma
ferramenta nova, uma decisão nova, duas seções de arquitetura novas, cinco
evidências novas.

## 8. Commits e push

Todos com push confirmado no momento do commit. Para ver o estado real:

```
git rev-parse HEAD
git ls-remote origin arena/519f8fc9-clienterecriado
git status --short
git log --oneline c03264b..HEAD
```

**Incidente de Git neste ciclo, registrado porque afetou o trabalho:** o ambiente
foi restaurado de snapshot antigo e o branch local divergiu da ponta remota —
o clone de referência `Cliente_ref` também desapareceu. O trabalho foi salvo em
patch, o branch realinhado a `origin`, o patch reaplicado, e `Cliente_ref`
re-clonado com hash conferido (`88edfc73`). **Nenhum commit do remoto foi perdido
e não houve force-push.**

## 9. Problemas restantes

**Bloqueio nº 1 — índice de UV por canto. NÃO RESOLVIDO.** Nove hipóteses
descartadas e documentadas. **É o único bloqueio entre a malha montada e a malha
texturizada.** Sem ele o modelo desenha sem textura, não desenha errado.

**Bloqueio nº 2 — nenhuma linha de código OpenGL.** `DEC-008` incompleta;
`DEC-010` lista o que está bloqueado até lá.

**Bloqueio nº 3 — 1.519 arquivos v0x0C permanecem OPAQUE.**

**Não determinados:** ordem dos 6 floats de transformação de osso (inferida —
aplicar posição e rotação trocadas daria esqueleto silenciosamente errado);
conteúdo dos 3 floats de keyframe; escala vertical da altura (três caminhos
fechados); significado dos 9 floats em bytes 24..59 do triângulo; malhas múltiplas.

**Limitação de ambiente:** sem Windows, GPU, headers de GL/zlib/png/jpeg/SDL2;
`apt-get install` inutilizável.

## 10. Próxima tarefa e por quê

**Completar `DEC-008` com a comparação de custo/risco exigida.**

É o que está na frente de tudo. Enquanto estiver incompleta, nenhum código de
backend pode ser escrito (`DEC-010`), e sem backend não há como medir desempenho
— que é o objetivo declarado do projeto. Precisa de: custo de cada alternativa,
risco de cada uma incluindo o risco de não conseguir medir neste ambiente, o que
será medido e em que hardware, e o que acontece com a compatibilidade visual em
cada caso.

**Alternativas, se houver acesso a GPU:** iniciar o pipeline VBO/VAO, que não está
bloqueado por nenhuma descoberta pendente — apenas pela ausência de hardware e
pela decisão incompleta.

**O que não adianta retomar:** índice de UV. Nove hipóteses descartadas. Sem uma
hipótese genuinamente nova — não uma variação das nove — reabrir é desperdício.

## 11. Padrão de erro deste projeto, e o que foi feito a respeito

Seis ocorrências, duas classes:

**Medição em subconjunto publicada como geral** (cinco vezes): cabeçalho BMD,
array de UV, registro de triângulo, suavidade de altura (4,4 de um arquivo contra
8,038 reais), registro de animação (86,4% de 88 contra 77,00% de 500).

**Código que avança sem atualizar o documento** (uma vez, e é a deste ciclo):
topologia implementada em `bee2a5e`, documentação dizendo "não implementada" por
dezenas de commits depois.

**Correções estruturais aplicadas:**

1. Toda medição virou comando de `mutool` ou script commitado — um script com teto
   não declara o teto na saída
2. `bmd_bones_probe.py` imprime o denominador ao lado de cada taxa
3. Regra nova: **o controle tem que ser da mesma classe do que se testa**
4. Documentos não copiam hash de ponta nem enumeram commits

**O que ainda não tem correção estrutural:** nada obriga o commit que muda o código
a mudar o documento. A topologia ficou defasada por dezenas de commits justamente
porque os testes passavam e a ferramenta funcionava. Isso é o próximo problema a
resolver, e não foi resolvido neste ciclo.

---

## ESTADO DO GIT NESTE CICLO

```
commits locais        : git log --oneline c03264b..HEAD
push realizado        : SIM, em todos os commits
hash local × remoto   : git rev-parse HEAD  ×  git ls-remote origin arena/519f8fc9-clienterecriado
                        coincidiam no momento da escrita
git status --short    : vazio — árvore limpa
referência Cliente_ref: 88edfc73 — re-clonada neste ciclo após desaparecer do
                        ambiente, hash conferido, não modificada
nada ficou sem commit
incidente             : restauração de snapshot divergiu o branch local; resolvido
                        com patch + realinhamento, sem force-push e sem perda
```
