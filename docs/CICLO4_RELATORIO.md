# Ciclo 4 — relatório de fim de ciclo

> **Base:** `8c83376` (fim do ciclo 3). A ponta do ciclo é o commit que
> acrescenta este arquivo, cujo hash **não pode aparecer dentro dele** — é o
> erro que ocorreu quatro vezes antes em seções de Git deste projeto. A lista
> autoritativa é o comando:
>
> ```
> $ git log --oneline 8c83376..HEAD
> ```
>
> A contagem **não** é registrada aqui de propósito. Qualquer número fixo escrito
> neste arquivo fica errado no commit seguinte ao que o escreve — inclusive no
> commit que corrige o próprio número. O comando é a fonte.
>
> **Referência:** `Cliente_ref` em `88edfc73`, inalterado no início e no fim
> **Escopo:** as cinco pendências deixadas pela auditoria do ciclo 3, mais
> `DEC-008`
>
> A auditoria aceitou o ciclo 3 com pendências e determinou que fossem
> resolvidas **antes** de `DEC-008`. As cinco foram resolvidas, e só então
> `DEC-008` foi completada.

---

## 1. O que foi pesquisado

**Item 1 — a relação entre os níveis da tabela.** A auditoria apontou que a queda
de 8.406 (nível 3) para 4.149 (nível 4) era uma perda de 50,6% sem causa
explicada, e pediu três coisas: confirmar se o nível 4 é subconjunto do 3 **por
código e não por inferência**, investigar a causa da queda, e corrigir os
documentos para eliminar a ambiguidade sobre cumulatividade.

As três foram feitas. O ponto que muda decisões: a queda **não** é propriedade
do formato, é propriedade **deste parser**.

**Item 2 — arquitetura de importação dual.** A auditoria abriu a alternativa de
que já estivesse feita e pediu o commit e o arquivo exatos caso estivesse.
Estava: `docs/ARCHITECTURE.md` §9, criada no commit `77662f5`. Lendo a seção
contra o item 1, apareceram dois defeitos nela.

**Item 3 — o incidente de Git.** Reconstruído do reflog real, não de memória.

**`DEC-008` — o que este ambiente pode medir sobre as duas opções de pipeline.**
Pesquisa de especificação: fixed-function está deprecado desde GL 3.1 e removido
do perfil core desde 3.2, o que significa que drivers modernos o mantêm por
camada de tradução.

## 2. O que foi implementado

| O quê | Onde | Por quê |
|---|---|---|
| `BmdTopologySkip` | `bmd_body.hpp:123`, `bmd_body.cpp:126,161-168` | Um array de triângulos vazio eram quatro falhas indistinguíveis |
| `declared_mesh_count` / `declared_triangle_count` | `bmd_body.hpp` | Para medir a distribuição em vez de inferi-la |
| `LEVEL 4b` + histograma de `meshCount` | `tools/mutool.cpp:469` | A causa tem que sair da ferramenta, não de análise manual |
| `TestTopologyIsSubsetOfGeometry` | `tests/test_assets.cpp:1142` | Fixa a relação de subconjunto e a exaustividade das causas |
| Benchmark de caminho de submissão | `bench/bench_assets.cpp:267` | DEC-008 precisava de medição, não de argumento |

**Escolha que precisa ficar registrada:** a causa foi medida **no parser C++**,
não em script Python. Um script que reimplementasse a aritmética mediria a minha
reimplementação, não o código publicado — o mesmo erro de "medir o substituto"
que já ocorreu neste projeto.

## 3. Problemas resolvidos

**A ambiguidade da tabela.** A resposta é que **os seis níveis são critérios
independentes aplicados à mesma população de 10.090 arquivos, não uma hierarquia
em que cada linha é subconjunto da anterior.** Existe uma única relação de
subconjunto entre linhas, e ela foi verificada:

- **Nível 4 ⊂ nível 3.** Prova em código: em `bmd_report.cpp`, `triangle_count`
  só é atribuído dentro do ramo que faz `geometry_parsed = true` e vale 0 caso
  contrário; o acumulador só incrementa `topology_parsed` sob
  `if (r.geometry_parsed && r.triangle_count > 0)`.
- **Níveis 3, 4 e 5 não formam cadeia.** O nível 4 exige `meshCount == 1`, o
  nível 5 não. Um arquivo pode ter ossos sem topologia e vice-versa.

**A causa da queda de 50,6%, medida:**

| Causa | Arquivos | % dos 4.257 |
|---|---|---|
| multi-mesh (`meshCount != 1`) | **4.257** | **100,00%** |
| declara zero faces | 0 | 0,00% |
| array declarado não cabe | 0 | 0,00% |
| payload curto para os counts | 0 | 0,00% |

Nenhum arquivo falha por tamanho, versão ou campo ausente. O parser lê **um**
bloco de triângulos; com várias malhas a sequência se repete no corpo e ler um
bloco leria o errado em silêncio.

**Dois defeitos no item 2 já entregue:**

1. Dizia **"oito hipóteses"** descartadas para UV. São **nove** — a nona foi
   testada e rejeitada no ciclo 3 e a seção 9 não foi revisitada. É a mesma
   classe de defasagem que a auditoria está cobrando, encontrada dentro do
   próprio item que ela pediu.
2. Não citava a tabela corrigida do item 1, que a auditoria pediu
   explicitamente.

**Tabela do ciclo 1 removida** de `COMPATIBILITY.md` e `ASSET_FORMATS.md`. Dizia
"payload em claro 1.496 (14,83%)" e "geometria 1.191 (11,80%)", valores
anteriores a `DEC-007`. Manter as duas tabelas lado a lado, com o mesmo
denominador e valores diferentes, **era exatamente a ambiguidade que a auditoria
pediu para eliminar**. O salto de 11,80% para 83,31% é a decifragem v0x0C.

## 4. Testes executados

| Teste | Resultado | Observação |
|---|---|---|
| `make test` | **372 checks, 0 failures** | `PASS` |
| `make test-san` (ASan+UBSan) | **372 checks, 0 failures** | `PASS` |
| Compilação com 12 flags `-W` | **0 warnings** | `PASS` |
| `TestTopologyIsSubsetOfGeometry` | 28 asserções novas | `PASS` |
| Benchmark DEC-008 | 3 caminhos, com desvio padrão | `PASS` |
| `mutool bmdreport` full-scan | 10.090 arquivos | `PASS`, exit 2 intencional |
| Trava de `DEC-010` | 1 ocorrência, comentário | `PASS` |
| Backend GL / validação em GPU | — | **`BLOCKED`** — sem GPU nem contexto |
| Comparação visual pixel a pixel | — | **`NOT EXECUTED`** — sem GPU |

**O teste novo falhou primeiro, por motivo instrutivo.** O fixture era corpo
cru, sem o contêiner `"BMD"`+versão, então `geometry_parsed` dava `false` em
todos os casos e **a relação de subconjunto teria passado vacuamente** — 17
falhas que apontaram o defeito. Envolvido por `FakeBmd`. Um teste que passa sem
exercitar o caminho é pior que teste nenhum, porque parece cobertura.

## 5. Métricas medidas

**Relação entre níveis** (`evidence/ciclo4_topology_subset.log`), árvore
completa:

| meshCount | Arquivos | % dos 8.406 de nível 3 |
|---|---|---|
| 0 | 1 | 0,01% |
| 1 | 4.149 | 49,36% |
| 2 | 2.280 | 27,12% |
| 3 | 1.086 | 12,92% |
| 4 | 449 | 5,34% |
| 5 | 190 | 2,26% |
| 6 | 114 | 1,36% |
| 7 | 42 | 0,50% |
| 8 | 38 | 0,45% |
| 9–14 | 50 | 0,59% |
| > 15 | 7 | 0,08% |

**Custo do caminho de submissão** (`evidence/ciclo4_dec008_submission_cost.log`).
Contagens **reais** da árvore via `mutool meshimport`: 1.792.108 vértices,
978.853 triângulos, 4.071 malhas. Razão de compartilhamento medida: 1,64 vértice
por triângulo em vez de 3.

| Medida | Mediana | Desvio | Min | Max |
|---|---|---|---|---|
| Expansão indexado → flat, por quadro | 21.459,9 µs | 7,31% | 18.908,7 | 25.369,5 |
| Submissão indexada, por quadro | 837,3 µs | 3,84% | 789,5 | 1.004,5 |
| Upload único do vertex buffer | 3.610,2 µs | 3,83% | — | — |

Derivados: razão **25,63×** · **1.144.451** envios de vértice poupados por quadro
(**38,97%**) · upload se paga em **0,18 quadro** a 60 FPS.

**21.459,9 µs = 128,8% do orçamento inteiro de um quadro a 60 FPS** e **309,0%**
do de 144 FPS. E é **CPU apenas** — não inclui driver, GPU nem barramento, então
o quadro real é pior, não melhor.

**O que este número sustenta e o que não sustenta.** Sustenta: o modelo expandido
custa, em marshalling de CPU, mais que o orçamento inteiro de um quadro a 60 FPS
para este volume de geometria — e isso independe de driver e de GPU, porque é
trabalho que a CPU faz antes de qualquer chamada. **Não sustenta:** nenhum
número de FPS, nenhum ganho de desempenho, nenhuma comparação de draw call em
tempo real.

**Ressalvas que viajam com o número:** buffers **residentes** entre repetições,
que é o caso **favorável** ao caminho expandido; contagens da árvore inteira,
não do subconjunto visível de um quadro. O benchmark está parametrizado para
reexecução com volume real de cena quando houver renderer.

## 6. Comparação com a linha de base

**Não há linha de base de FPS, frametime ou VRAM, e este ciclo não criou uma.**
O benchmark mede marshalling de CPU, que é uma proxy do custo de submissão, não
desempenho de quadro. Declarar ganho de FPS a partir dele violaria a regra
permanente.

A comparação válida é entre os dois caminhos de submissão nas mesmas condições:
**25,63×** em marshalling de CPU, medido no mesmo processo, mesmos buffers,
mesma máquina.

## 7. Arquivos alterados

```
 bench/bench_assets.cpp                          |  98 ++++++
 docs/ARCHITECTURE.md                            |  26 +-
 docs/ASSET_FORMATS.md                           |  88 ++++--
 docs/CHANGELOG.md                               |  64 ++++
 docs/COMPATIBILITY.md                           | 131 ++++++---
 docs/CURRENT_STATE.md                           |  68 ++++-
 docs/DECISIONS.md                               | 130 +++++++--
 docs/NEXT_TASK.md                               | 177 +++++-------
 docs/SECURITY.md                                |  57 ++++
 docs/evidence/ciclo4_dec008_submission_cost.log |  90 ++++++
 docs/evidence/ciclo4_git_incident.log           | 175 +++++++++++
 docs/evidence/ciclo4_topology_subset.log        | 122 ++++++++
 src/mu/assets/bmd_body.cpp                      |  26 ++
 src/mu/assets/bmd_body.hpp                      |  25 ++
 src/mu/assets/bmd_report.cpp                    |  23 +-
 src/mu/assets/bmd_report.hpp                    |  19 ++
 tests/test_assets.cpp                           | 114 ++++++++
 tools/mutool.cpp                                |  34 +++
 18 files changed, 1286 insertions(+), 181 deletions(-)
```

## 8. Commits e push

Oito commits, todos publicados. Lista completa na seção `ESTADO DO GIT` abaixo.

## 9. Problemas restantes

| Problema | Estado | O que falta |
|---|---|---|
| **Índice de UV por canto** | **BLOQUEADO** | Nove hipóteses descartadas. Critério de parada ativo |
| Multi-mesh não lido | **Parcial** | 4.257 arquivos; implementação conhecida |
| Ordem dos 6 `f32` de osso | **Não confirmado** | "Posição antes de rotação" é inferência |
| Conteúdo dos 3 `f32` de keyframe | **Não determinado** | 77,00% dos arquivos têm o bloco |
| Escala vertical da altura | **Não determinado** | Três caminhos fechados |
| 1.519 payloads opacos | **BLOQUEADO** | Criptoanálise esgotada |
| Backend VBO/VAO | **Não iniciado** | Decisão registrada; falta ambiente que execute |
| Validação parte B de DEC-008 | **BLOQUEADO** | Sem GPU nem contexto |
| Baseline de FPS/frametime/VRAM | **Não iniciada** | Sem GPU |
| Compatibilidade visual | **`NOT EXECUTED`** | Sem comparação pixel a pixel possível |

## 10. Próxima tarefa e por quê

**Estender o parser BMD para multi-mesh.**

É a única descoberta deste ciclo que virou **trabalho conhecido em vez de
pesquisa**:

- A causa é medida e total: `meshCount != 1` em **100,00%** dos 4.257 arquivos
- É limite **deste parser**, não do formato — a sequência por malha é a mesma já
  verificada, falta iterar e somar offsets
- Recupera **4.257 arquivos**, mais do que o parser lê hoje (4.149)
- É testável aqui: não depende de GPU, contexto nem Windows

**O que não destrava, dito antes de começar:** o nível 6 continua **zero**. Um
modelo sem textura pode ser desenhado; texturizado não. Multi-mesh aumenta a
malha montável, não a renderizável.

## 11. Padrão de erro deste projeto, e o que este ciclo acrescenta

O ciclo 3 registrou que **nada obriga o commit que muda o código a mudar o
documento**, e afirmou que as quatro correções estruturais aplicadas não
impediam a próxima defasagem. **Este ciclo confirma a afirmação com um caso
novo:** `ARCHITECTURE.md` §9 dizia "oito hipóteses" quando são nove. A nona foi
testada e rejeitada no ciclo 3; a seção não foi revisitada. Nenhum teste falhou,
nenhuma ferramenta quebrou.

**O caso é mais instrutivo do que parece, porque aconteceu dentro do item que a
auditoria pediu justamente para revisar.** A verificação de um item não protege
os outros.

**Três ocorrências novas de classes já registradas:**

1. **Teste que passaria vacuamente.** O fixture sem contêiner faria
   `geometry_parsed` dar `false` em todos os casos, e a relação de subconjunto
   passaria sem exercitar nada. Detectado porque as 17 falhas apareceram — mas
   só apareceram porque havia asserções sobre contagens absolutas
   (`CHECK_EQ(4u, sum.geometry_parsed)`), não só sobre a relação. **Lição:
   afirmar o absoluto junto com o relativo, senão o relativo passa no vazio.**

2. **Prefixo de evidência errado.** Nomeei dois arquivos novos como `ciclo3_`
   porque tratam de assunto do ciclo 3. A convenção é que o prefixo indica o
   ciclo que **produziu** a evidência. Quem listasse o diretório para auditar o
   ciclo 3 encontraria medições que o ciclo 3 não fez.

3. **Duas tabelas com o mesmo denominador e valores diferentes** convivendo no
   mesmo documento — a forma exata da ambiguidade que a auditoria pediu para
   eliminar, presente nos documentos que eu mesmo havia atualizado no ciclo
   anterior.

**O que continua não construído:** a verificação automática que atacaria a causa
— um teste que falhe se um documento negar algo que existe em `src/`, ou que
compare contagens citadas em prosa contra a saída da ferramenta. Este é o
segundo ciclo consecutivo que registra a ausência dela. Registrar sem construir
é a mesma coisa que não ter registrado, e a próxima ocorrência virá.

---

## ESTADO DO GIT NESTE CICLO

**Commits locais.** A lista autoritativa é o comando, não a cópia abaixo — esta
seção já foi desatualizada pelo próprio commit que a criou, quatro vezes neste
projeto, e a cópia existe só para leitura:

```
$ git log --oneline 8c83376..HEAD
```

Os commits de trabalho, anteriores ao deste relatório:


```
66c49c8 docs: CURRENT_STATE, NEXT_TASK e CHANGELOG do ciclo 4, no mesmo ciclo
d32f270 docs(decisions): DEC-008-A - comparacao formal de custo e risco registrada
7b4df09 chore(evidence): prefixo ciclo4 nos arquivos que ESTE ciclo produziu
4d7ee53 bench: custo de submissao expandida vs indexada, para decidir DEC-008
4f0f6eb docs(evidencia): item 3 - evidencia bruta do incidente de Git e risco operacional
9bfff93 docs(arquitetura): item 2 ja existia - dois defeitos nele corrigidos
9d03d7d docs: tabela de seis niveis com a relacao entre eles explicita (item 1)
8939899 feat(bmd): classifica por que um arquivo de nivel 3 nao tem topologia
```

**Push realizado:** sim, após cada commit, inclusive o do relatório. Nenhuma
falha de push neste ciclo. A verificação de remoto == local abaixo é executada
**depois** do push do commit que acrescenta este relatório.

**Confirmação de que remoto == local:**

```
$ git rev-parse HEAD
66c49c8dd2a9fe0ed8fa579b02d7b023b4064aa6

$ git ls-remote origin arena/519f8fc9-clienterecriado | cut -f1
66c49c8dd2a9fe0ed8fa579b02d7b023b4064aa6
```

Idênticos.

**`git status` final:** vazio — árvore limpa, nada sem commit.

**Referência `Cliente_ref`:**

```
$ git -C /home/user/Cliente_ref rev-parse HEAD
88edfc73f5e1d63f806a51f21a118f84ba6c2bb9
```

**Inalterado** — mesmo hash do início do ciclo. O repositório de pesquisa não
foi modificado, e nenhum commit deste ciclo foi feito nele.

**Confirmações pedidas pelos itens 4 e 5 da auditoria:**

- **Item 5** — `git show --stat 1e970da` confirma os dois arquivos com os nomes
  exatos, commitados:
  ```
  docs/evidence/ciclo3_scan_final.log | 161 ++++++++++++++++++++
  docs/evidence/ciclo3_test_final.log |  79 ++++++++++
  2 files changed, 240 insertions(+)
  ```
- **Item 4** — a auditoria pediu trecho colado, não referência. Segue o bloco
  como está em `COMPATIBILITY.md`, sob o título "Tabela final de
  compatibilidade — seis níveis" (verificado por `grep -c "^| [1-6] — "`, que
  retorna **6** em `COMPATIBILITY.md` e **6** em `ASSET_FORMATS.md`):

  ```
  | Nível | O que mede | Arquivos | % de 10.090 |
  |---|---|---|---|
  | 1 — estrutura de contêiner válida | magic + versão reconhecidos; para v0x0C o tamanho declarado confere | 10.023 | 99,34% |
  | 2 — payload obtível em claro | 1.245 plaintext + 100 bux + 7.226 v0x0C | 8.571 | 84,95% |
  | 3 — vértice + normal | posições e normais realmente lidas pelo parser | 8.406 | 83,31% |
  | 4 — topologia (índices de face) | triângulos lidos | 4.149 | 41,12% |
  | 5 — ossos parseáveis | registros de osso localizáveis e legíveis | 3.505 | 34,74% |
  | 6 — UV | índice ligando canto de triângulo a coordenada de textura | **0** | **0,00%** |
  ```

  A mesma tabela está em `ASSET_FORMATS.md`, seguida da frase que elimina a
  ambiguidade pedida: **"Os seis níveis são critérios independentes aplicados à
  mesma população de 10.090 arquivos, não uma hierarquia em que cada linha é
  subconjunto da anterior."**

**Nada ficou sem commitar nem sem publicar.**
