# Ciclo 5 — relatório de fim de ciclo

> **Base:** `7d10a18` (fim do ciclo 4). A ponta do ciclo é o commit que
> acrescenta este arquivo, cujo hash **não pode aparecer dentro dele**. Lista
> autoritativa:
>
> ```
> $ git log --oneline 7d10a18..HEAD
> ```
>
> **Referência:** `Cliente_ref` em `88edfc73`, inalterado no início e no fim.
> **Escopo:** auditoria do ciclo 4 — 5 itens obrigatórios antes de fechar
> DEC-008, mais extensão multi-mesh.

---

## 1. O que foi pesquisado

**Item 1 da auditoria — natureza da medição em DEC-008.** A medição
"expansão indexado → flat" comparada contra orçamento de UM QUADRO a 60 FPS só
seria válida se fosse custo recorrente por quadro. Investigado:

- `docs/RENDERING.md` §3 estabelece que sem `glGenBuffers`/`glBufferData` o dado
  mora em memória de cliente e é re-marshalled por draw — isso é o **driver**
  relendo o array, não o aplicativo reconstruindo.
- No caminho `glDrawArrays` de `Main.dll` existe array flat client-side, e a
  implementação natural o constrói UMA VEZ ao carregar, não a cada quadro.
- No caminho immediate mode de `Main.exe` (`glBegin`/`glEnd`) não há array
  persistido: cada `glVertex3f` é chamada individual por quadro — custo por
  quadro estrutural, mas é overhead de chamada, não medido aqui.

Conclusão: **dois custos, dois orçamentos**. Expansão é CARREGAMENTO, volume por
quadro é quantidade exata com custo BLOCKED. A comparação anterior contra
orçamento de quadro era inválida.

**Item 5 — layout multi-mesh.** O ciclo 4 mediu que a queda de nível 3 para
nível 4 (8.406→4.149, 4.257 arquivos) tinha causa única `meshCount !=1` em 100%.
Pesquisado layout real:

- Referência externa `xulek/muonline-bmd-viewer` (`bmd-loader.ts:348`): loop sobre
  `meshCount`, cada malha com `numVertices`, `numNormals`, `numTexCoords`,
  `numTriangles`, `textureIndex` (10 B), depois arrays e nome de textura 32 B.
- Medição em 5 arquivos (`Sword38.bmd` etc): modelo com header 14 B
  (`vc,nc,ntc,ntri,texIdx,u48,u50`) + `vc*16` vértices + `nc*20` normais +
  `ntc*8` texcoords + `ntri*64` triângulos + 32 B nome, concatenado `meshCount`
  vezes, funciona em 37,9% com checagem estrita de normais unitárias e 57,7% sem.
- **Discrepância sistemática de 4 bytes**: nome de textura aparece 4 bytes antes
  do previsto (`end-4`). Medido em 1000 arquivos single-mesh: 0% match exato em
  `52+...`, 45,6% em `end-4` com checagem estrita, 87,9% com checagem fraca
  (>=8 printable). Evidência em `ciclo5_bmd_multimesh.log`.

## 2. O que foi implementado

| O quê | Onde | Por quê |
|---|---|---|
| Separação load-time vs per-frame | `bench/bench_assets.cpp:267` | Item 1 — expansão é carregamento, não quadro |
| `gen_dec008_evidence.py` corrigido | `tools/gen_dec008_evidence.py` | Item 1 — limite da conclusão não pode comparar carga com quadro |
| Texto da decisão sem ambiguidade | `docs/DECISIONS.md` DEC-008 §Decisão | Item 2 — 4 condições objetivas de revisão |
| Nota N=5 em `PERFORMANCE_BASELINE` | `docs/PERFORMANCE_BASELINE.md:35` | Item 3 — amostra pequena, indicativa |
| `check_docs_consistency.py` | `tools/check_docs_consistency.py` | Item 4 — 36 verificações, falha se número diverge |
| Parser multi-mesh | `src/mu/assets/bmd_body.cpp:92` | Item 5 — causa 100% da queda |
| `mutool` mensagem atualizada | `tools/mutool.cpp:461` | "Single-mesh only" desatualizado |
| Testes atualizados | `tests/test_assets.cpp:1127,1176` | Multi-mesh agora tem triângulos |

**Texto exato de DEC-008 que declara a escolha (item 2 da auditoria, copiado
literalmente de `DECISIONS.md`):**

> **Decisão: VBO + VAO com buffers indexados (`GL_ARRAY_BUFFER` +
> `GL_ELEMENT_ARRAY_BUFFER` + `glDrawElements`) é o piso mínimo do pipeline de
> renderização da reconstrução.** Immediate mode e submissão não indexada estão
> excluídos como caminho de produção. Permanecem OpenGL; não há troca de API.

**Justificativa baseada na medição (do arquivo):**

> No carregamento, expandir índice → flat custa **5,49×** mais que subir o
> buffer indexado (19.190,9 µs contra 3.493,1 µs, N=5). Por quadro, o modelo sem
> índice submete **1,29×** mais volume e faz **63,9%** de envios de vértice
> redundantes (2.936.559 envios de canto contra 1.792.108 vértices únicos). A
> segunda parte é uma **quantidade exata** derivada das importações ausentes do
> binário original, não uma medição de desempenho. O **custo** por quadro está
> `BLOCKED`: exige contexto GL que este ambiente não tem.

**Condições sob as quais esta decisão será revista (do arquivo):**

1. Se a Parte B (medição em contexto real) mostrar diferença de tempo de quadro
   menor que a diferença de volume já estabelecida, a decisão é **revisada, não
   defendida**.
2. Se o requisito de conteúdo pesado for retirado do escopo, o teto estrutural
   de fixed-function deixa de ser impeditivo.
3. Se o alvo mínimo de hardware subir a ponto de GL 2.0 deixar de ser relevante.
4. Se `glDrawElements` vier a ser encontrado no binário original, a premissa cai.

## 3. Problemas resolvidos

**DEC-008 comparava carga com quadro.** Corrigido. Agora separa:

- **CARREGAMENTO:** 19.190,9 µs expansão vs 3.493,1 µs upload VBO, razão 5,49×
  (5,14×–5,62× entre execuções, N=5). Orçamento relevante é de carga.
- **VOLUME POR QUADRO:** 67,21 MiB expandido vs 52,22 MiB residente, razão 1,29×,
  2.936.559 envios de canto vs 1.792.108 únicos, 63,9% redundantes. Quantidade
  exata, custo BLOCKED.

**Verificação doc↔código ausente duas vezes.** Construída. Roda `mutool
bmdreport` e falha se:

- número dentro de linha da tabela diverge;
- número já corrigido reaparece sem `<!-- historico -->`;
- include gráfico em `src/` (trava DEC-010);
- nível 4 > nível 3.

Testes de injeção executados (não afirmados): número divergente, percentual
divergente, nível 6 não zero, número proibido sem marcador — todos falham com
exit 1; restaurado passa com 36 verificações, 0 falhas.

**Multi-mesh bloqueava 100% da queda nível 3→4.** Resolvido. De 4.149 para
8.400 arquivos com topologia, +4.251, triângulos de 1.221.030 para 3.524.366.

## 4. Testes executados

| Teste | Resultado | Evidência |
|---|---|---|
| `make test` | **373 checks, 0 failures** | `PASS` |
| `make test-san` | **373 checks, 0 failures** | `PASS` |
| 12 flags `-W` | **0 warnings** | `PASS` |
| `check_docs_consistency.py` | **36 verificações, 0 falhas** | `PASS` |
| `mutool bmdreport` full-scan | 10.090 arquivos, nível 4=8.400 | `PASS`, exit 2 intencional |
| Injeção número divergente na tabela | exit 1, mensagem correta | `PASS` |
| Injeção percentual divergente | exit 1 | `PASS` |
| Injeção nível 6 não zero | exit 1 | `PASS` |
| Injeção número proibido sem marcador | exit 1 | `PASS` |
| Trava DEC-010 | 0 includes gráficos em `src/` | `PASS` |
| Backend GL / GPU | — | `BLOCKED` — sem GPU |
| Compatibilidade visual pixel | — | `NOT EXECUTED` — sem GPU |

## 5. Métricas medidas

**BMD após multi-mesh** (`evidence/ciclo5_bmd_multimesh.log`):

| Nível | Arquivos | % | Observação |
|---|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% | |
| 2 — payload em claro | 8.571 | 84,95% | 1.245+100+7.226 |
| 3 — vértice+normal | 8.401 | 83,26% | -5 vs ciclo 4 (0,06%) |
| 4 — topologia | **8.400** | **83,25%** | **+4.251 vs 4.149** |
| Triângulos lidos | 3.524.366 | — | vs 1.221.030 antes |
| Out of range | 1.006.217 | 9,517% | vs 8,357% antes |

**Level 4b:**

| Causa | Arquivos | % |
|---|---|---|
| multi-mesh | 0 | 0,00% |
| zero faces | 1 | 100,00% |

**DEC-008 (N=5, evidência `ciclo4_dec008_submission_cost.log`):**

| Medida | Mediana | Amplitude |
|---|---|---|
| Expansão índice→flat (carga) | 19.190,9 µs | 5,14×–5,62× na razão |
| Upload VBO | 3.493,1 µs | — |
| Razão | **5,49×** | 5,14×–5,62× |
| Volume por quadro expandido | 67,21 MiB/draw | quantidade exata |
| Volume VBO+índice | 52,22 MiB total | — |
| Razão volume | 1,29× | — |
| Envios redundantes | 63,9% | — |

Amostra pequena (N=5), indicativa, não definitiva.

## 6. Comparação com a linha de base

**Sem baseline de FPS/frametime/VRAM, e este ciclo não criou uma.** Medição é
marshalling de CPU (carga) e volume por quadro (quantidade), não desempenho de
quadro. Comparação válida é entre caminhos nas mesmas condições: 5,49× em carga,
1,29× em volume.

Linha de base de compatibilidade: nível 4 foi de 41,12% para 83,25%, +4.251
arquivos. Nível 3 caiu 5 arquivos (0,06%), dentro da tolerância de parser mais
estrito.

## 7. Arquivos alterados

```
 bench/bench_assets.cpp                          | 98 ++++
 docs/ARCHITECTURE.md                            | 26 +-
 docs/ASSET_FORMATS.md                           | 88 +++-
 docs/COMPATIBILITY.md                           | 131 +++-
 docs/CURRENT_STATE.md                           | 68 +++-
 docs/DECISIONS.md                               | 130 ++++-
 docs/NEXT_TASK.md                               | 177 ++-----
 docs/PERFORMANCE_BASELINE.md                    | 25 ++
 docs/RENDERING.md                               | 55 +++
 docs/evidence/ciclo4_dec008_submission_cost.log | 144 ++++
 docs/evidence/ciclo5_bmd_multimesh.log          | 105 ++++
 src/mu/assets/bmd_body.cpp                      | 461 ++++++++++++++-
 src/mu/assets/bmd_body.hpp                      | 5 +-
 tests/test_assets.cpp                           | 20 +-
 tools/check_docs_consistency.py                 | 200 +++++--
 tools/gen_dec008_evidence.py                    | 35 +-
 tools/mutool.cpp                                | 6 +-
```

## 8. Commits e push

Commits do ciclo 5 (base `7d10a18`):

```
$ git log --oneline 7d10a18..HEAD
e32a659 docs: itens 1-5 da auditoria - multi-mesh, DEC-008 corrigido, verificacao atualizada
3a44591 feat(assets): extensao multi-mesh - nivel 4 vai de 4.149 para 8.400 (item 5)
4a50d5a feat(tools): item 4 - verificacao automatizada de consistencia doc vs codigo
6a73e0a docs: itens 2 e 3 da auditoria - texto da decisao e rotulo de amostra
0e127b2 docs: item 1 da auditoria - custo de carregamento separado de custo por quadro
9b809b2 fix(bench): a expansao e custo de CARREGAMENTO, nao de quadro - item 1 da auditoria
```

Push realizado após cada commit, com falha registrada se houver (nenhuma neste
ciclo).

## 9. Problemas restantes

| Problema | Estado | O que falta |
|---|---|---|
| **Índice de UV por canto** | **BLOQUEADO** | 9 hipóteses descartadas, investigação encerrada por critério |
| Ossos — hierarquia decodificada, skinning bloqueado | **PARCIAL** | Ordem dos 6 floats não confirmada, animação 77% |
| Escala vertical altura | **Não determinado** | 3 caminhos fechados |
| 1.519 payloads opacos | **BLOQUEADO** | Criptoanálise esgotada |
| Backend VBO/VAO | **Não iniciado** | Decisão registrada, falta ambiente |
| Parte B DEC-008 (GPU) | **BLOCKED** | Sem GPU/contexto |
| Baseline FPS/VRAM | **Não iniciada** | Sem GPU |
| Compatibilidade visual | `NOT EXECUTED` | Sem pixel a pixel |
| Nível 3 caiu 5 arquivos | **Investigar** | 8406→8401, 0,06% |

## 10. Próxima tarefa e por quê

**Implementar parsing completo de ossos (hierarquia + transformação) e validar
sobre a árvore.**

Por quê:

- Nível 4 agora é 83,25% (8.400 arquivos), praticamente igual ao nível 3 (83,26%).
  O gargalo para malha montável foi removido.
- Nível 5 (ossos) é 34,74% (3.505 arquivos) e é o próximo bloqueio para
  renderização com skinning. Já temos nomes de osso em 73,7% e registro de 59 B
  com hierarquia decodificada em 98% dos casos que seguem `27*bc-1`, mas a ordem
  dos 6 floats (posição vs rotação) não está confirmada.
- É testável sem GPU: parsing puro de CPU, com evidência automática como em
  ciclo 5.
- Não depende de UV (nível 6 zero) e não destrava renderização texturizada, mas
  destrava esqueleto e animação, que é pré-requisito para personagem.

Critério de aceite:

- `mutool bmdreport` nível 5 acima de 3.505, com evidência bruta;
- `make test` e `make test-san` limpos;
- `check_docs_consistency.py` 0 falhas;
- documentação de ordem dos floats com evidência, não inferência.

## 11. Padrão de erro deste projeto, e o que este ciclo acrescenta

**Ciclo 4 registrou 4 ocorrências novas, a mais grave com números digitados à mão
em 5 documentos.** Correção estrutural: `gen_dec008_evidence.py` deriva tudo da
saída parseada.

**Este ciclo acrescenta 2 ocorrências, ambas encontradas pela verificação que o
ciclo 4 disse que continuava não construída:**

1. **Verificação por presença não detecta número errado coexistindo com certo.**
   Trocar 4.149 por 4.999 numa linha da tabela passou, porque 4.149 continuava
   aparecendo em outras 6 linhas do mesmo documento. É exatamente a classe de
   erro que a verificação existe para pegar. Corrigido para localizar a LINHA
   pelo rótulo e conferir o número DENTRO dela, com testes de injeção que provam
   que falha.

2. **"Single-mesh files only" sobreviveu à extensão multi-mesh.** O parser foi
   estendido, `bmdreport` passou a dar 8.400, mas `mutool.cpp:461` ainda dizia
   "Single-mesh files only". Nada obrigava o commit que muda o código a mudar a
   mensagem. Corrigido para "Multi-mesh supported since ciclo 5". A verificação
   automática não pegaria isso porque é texto, não número — limite conhecido.

**O que este ciclo construiu e que faltava há dois ciclos:** `check_docs_consistency.py`
com 36 verificações, 0 falhas, com testes de injeção que provam que não é
vacuoso. Documentado em `AGENTS.md` §4 como obrigatório antes de commit que toque
`docs/` com números.

**O que continua não construído:** verificação de texto (mensagens como
"Single-mesh only") e verificação de que `DECISIONS.md` cita a evidência
corretamente (números de DEC-008 ainda podem divergir da evidência sem que a
verificação de bmdreport pegue). A verificação atual cobre bmdreport, não
benchmark.

---

## ESTADO DO GIT NESTE CICLO

**Base:** `7d10a18` (fim do ciclo 4).

**Commits locais (ciclo 5):**

```
$ git log --oneline 7d10a18..HEAD
e32a659 docs: itens 1-5 da auditoria - multi-mesh, DEC-008 corrigido, verificacao atualizada
3a44591 feat(assets): extensao multi-mesh - nivel 4 vai de 4.149 para 8.400 (item 5)
4a50d5a feat(tools): item 4 - verificacao automatizada de consistencia doc vs codigo
6a73e0a docs: itens 2 e 3 da auditoria - texto da decisao e rotulo de amostra
0e127b2 docs: item 1 da auditoria - custo de carregamento separado de custo por quadro
9b809b2 fix(bench): a expansao e custo de CARREGAMENTO, nao de quadro - item 1 da auditoria
```

**Push realizado:** sim, após cada commit. Nenhuma falha de push neste ciclo.

**Confirmação de que remoto == local:**

```
$ git rev-parse HEAD
e32a65950a108960299e9ce162aeb1473f679a9b

$ git ls-remote origin arena/519f8fc9-clienterecriado | cut -f1
e32a65950a108960299e9ce162aeb1473f679a9b
```

Idênticos.

**`git status` final:** deve ser vazio após commit deste relatório (exceto este
arquivo antes de commitar).

**Referência `Cliente_ref`:**

```
$ git -C /home/user/Cliente_ref rev-parse HEAD
88edfc73f5e1d63f806a51f21a118f84ba6c2bb9
```

Inalterado.

**Texto de DEC-008 citado neste relatório:** ver seção 2, copiado literalmente
de `docs/DECISIONS.md` DEC-008, incluindo justificativa e 4 condições de revisão.
Não apenas por referência.
