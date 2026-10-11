# Relatório de fim de ciclo — Ciclo 1: resolução da auditoria

**Escopo:** resolver os 8 itens bloqueantes apontados pela auditoria do usuário e,
em seguida, avançar na prioridade técnica determinada (cifra/corpo BMD antes de
CAND-001).

**Classificações usadas:** `CONCLUÍDO`, `PARCIAL`, `BLOQUEADO`, `NÃO EXECUTADO`,
`NÃO APLICÁVEL`. Resultado × estimativa × hipótese são distinguidos em cada item.

---

## 1. O que foi pesquisado

- Estado real do repositório de referência `Cliente_ref` no início do ciclo:
  `git rev-parse HEAD` = `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`, branch
  `main`, `git status --short` vazio (0 linhas).
- Conteúdo real do commit inicial `972fabc` deste repositório, via
  `git show --stat` e `git show 972fabc:README.md`: **1 arquivo, 1 linha,
  `# ClienteRecriado`**.
- Corpo do formato BMD nos 1.465 arquivos com payload em claro.
- Pareamento `.smd` × `.bmd` na árvore completa.

## 2. O que foi implementado

| Item | Estado |
|---|---|
| `src/mu/assets/bmd_report.{hpp,cpp}` — relatório de usabilidade em 3 níveis | **CONCLUÍDO** |
| Subcomando `mutool bmdreport` (exit 2) e `--list-clear` | **CONCLUÍDO** |
| 4 testes novos fixando as 3 métricas como contadores separados | **CONCLUÍDO** |
| `bench_assets`: desvio padrão, min/max, 3 tamanhos de buffer, leitura de disco por repetição | **CONCLUÍDO** |
| Target `make test-san` (ASan+UBSan) | **CONCLUÍDO** |
| Parser do corpo BMD v0x0A (vértices + normais) | **CONCLUÍDO** — 1.191 arquivos reais |
| Faces, UV e ossos BMD | **NÃO DETERMINADO** — ver §9 |

## 3. Problemas resolvidos

Todos os 8 itens da auditoria:

| # | Item | Estado | Evidência |
|---|---|---|---|
| 1 | Separar "estrutura válida" de "payload utilizável" | **CONCLUÍDO** | 3 níveis medidos, §5 |
| 2 | Métricas comprovadas, não afirmadas | **CONCLUÍDO** | 11 arquivos em `docs/evidence/` |
| 3 | 2.893,80 MB não é gargalo medido | **CONCLUÍDO** | reclassificado como estimativa teórica |
| 4 | Hash do clone de referência | **CONCLUÍDO** | `88edfc73…`, idêntico no início e no fim |
| 5 | Declarar o estado trivial do commit inicial | **CONCLUÍDO** | `D-000` em `docs/DECISIONS.md` |
| 6 | Evidência bruta, não resumos percentuais | **CONCLUÍDO** | `docs/evidence/` |
| 7 | Credencial: campo, origem, rotação, sem reproduzir valor | **CONCLUÍDO** | `docs/SECURITY.md` |
| 8 | Regra permanente de commit/push + seção de estado do Git | **CONCLUÍDO** | `AGENTS.md` §6b, §6c |

Três erros meus adicionais foram encontrados e corrigidos durante o ciclo,
todos por verificação e não por relato:

- Afirmava limpeza sob ASan/UBSan **sem existir target** no `Makefile` — a
  alegação não era reproduzível. Target criado e o teste realmente executado.
- Os contadores por teste em `TEST_RESULTS.md` somavam **163** contra um total
  declarado de **171**; nenhum dos dois batia com o que o binário executa.
  Tabela refeita a partir de saída instrumentada.
- Uma filtragem por "primeiro byte imprimível" escrita em script casou com
  9.529 arquivos, incluindo opacos como `Rank01.bmd` (byte `0x73` = `s` por
  coincidência). Substituída por `--list-clear` na própria ferramenta.

## 4. Testes executados

| Teste | Comando | Resultado |
|---|---|---|
| Unitários | `make test` | **231 checks, 0 failures** — `PASS` ([`ciclo1_test.log`](ciclo1_test.log)) |
| Build limpo | `make clean && make` | **0** `warning:`/`error:` sob 12 flags `-W` — `PASS` |
| Sanitizadores | `make test-san` | 231 checks, 0 failures, **0 reports**, exit 0 — `PASS` ([`ciclo1_test_san.log`](ciclo1_test_san.log)) |
| Scan da árvore real | `mutool scan` (29.060 arquivos) | exit 0, 1,65 s — `PASS` |
| Relatório BMD | `mutool bmdreport` | exit **2** (esperado; sem geometria) |
| Build/execução no Windows | — | **BLOQUEADO** — sem Windows, GPU ou mingw |
| FPS / frametime / VRAM | — | **NÃO EXECUTADO** — sem hardware |

## 5. Métricas medidas

BMD, três níveis, sobre os 10.090 arquivos (não amostra):

| Nível | Mede | Arquivos | % |
|---|---|---|---|
| 1. Estrutura válida | magic + versão e, em v0x0C, tamanho declarado = real | 10.023 | 99,34% |
| 2. Payload em claro | obtível sem cifra desconhecida | 1.496 | 14,83% |
| ↳ plaintext | já claro | 1.396 | 13,84% |
| ↳ bux-decifrável | XOR-3 `{0xFC,0xCF,0xAB}` | 100 | 0,99% |
| ↳ **OPAQUE** | cifra não recuperada | **8.594** | **85,17%** |
| 3. Geometria extraída | posições de vértice e normais lidas | **1.191** | **11,80%** |

CRC-32 (mediana ± desvio padrão):

| Caso | Mediana | Desvio padrão | MB/s |
|---|---|---|---|
| 64 KiB em cache, 300 reps | 30,4 µs | 4,5 (14,53%) | 2.056,3 |
| 1024 KiB em cache, 300 reps | 511,2 µs | 14,7 (2,91%) | 1.956,0 |
| 16384 KiB em cache, 20 reps | 8.306,8 µs | 109,7 (1,32%) | 1.926,1 |
| **CRC + leitura de disco por rep**, 20 reps, 17,43 MB | 12.823,9 µs | 439,8 | **1.358,9** |

Os dois regimes **não são comparáveis**. O que `mutool verify` faz de fato é o
último, I/O-bound.

## 6. Comparação com a baseline

Não há baseline de runtime — nenhum número de FPS, frametime ou VRAM existe,
porque não há Windows nem GPU no ambiente. **Não há comparação possível e
nenhuma meta arbitrária de FPS foi inventada.**

A única otimização com antes/depois medido continua sendo OPT-001 (CRC-32
slice-by-1 → slice-by-8): 391,6 → 1.956,0 MB/s em cache, **5,03×**. Este ciclo
não adicionou otimizações; corrigiu a honestidade das métricas existentes.

A mudança de baseline real deste ciclo é de classificação, não de desempenho:
geometria BMD utilizável passou de "99,76% lidos" (contêiner) para **0,00%
medido**, e o bloqueio foi promovido a **#1 do projeto** em destaque.

## 7. Arquivos alterados

Novos: `src/mu/assets/bmd_report.hpp`, `src/mu/assets/bmd_report.cpp`,
`docs/evidence/` (11 arquivos além deste relatório).

**Regeneração registrada.** `ciclo1_test.log` foi gerado antes da adição dos 4
testes de `bmd_report` e ainda registrava 171 checks, enquanto este relatório
alegava 203 citando-o como evidência — exatamente a categoria de erro que a
auditoria apontou. O log foi regenerado e agora registra 203. `ciclo1_build.log`
foi regenerado pelo mesmo motivo, e `ciclo1_test_san.log` foi criado porque a
limpeza sob sanitizadores era citada sem log.

Modificados: `tools/mutool.cpp`, `tests/test_assets.cpp`, `bench/bench_assets.cpp`,
`Makefile`, `.gitignore`, `AGENTS.md`, e 13 documentos em `docs/`.

## 8. Commits e push

Dez commits, um por unidade coerente. Push realizado. Hash remoto confirmado
igual ao local — ver seção final.

## 9. Problemas restantes

1. **Corpo BMD: duas seções decodificadas, o resto não (medido).** O parser
   v0x0A foi implementado e extrai posições de vértice e normais de **1.191 dos
   1.194** arquivos v0x0A — o nível 3 do `bmdreport` saiu de 0 (estrutural) para
   **1.191 (11,80%) medido**. O teto é 11,83% porque v0x0C é cifrado e nunca é
   parseado. **Índices de face, coordenadas de textura e ossos continuam não
   decodificados, então nenhum desses 1.191 arquivos é um modelo renderizável.** Entre os 1.465 claros: os 100
   bux não começam com `BMD` após decifrar (são contêineres, não modelos); 433
   abrem com um campo ASCII de 32 bytes contendo o nome do modelo de origem,
   todos terminando em `.SMD`, 351 com separador de caminho (`Data2\Item\Bow\`).
   **433 de 1.465, não todos** — os outros 932 não começam com ASCII imprimível,
   então o campo não é universal.
   Nenhum triplo (offset, largura, tamanho de registro) divide o resto do
   payload exatamente nos 433 nomeados, e o teste de passo de vértice é
   **ambíguo**: 12 B vence em `PrickleGlove_left` (1.407 registros) enquanto
   48 B vence em `Sword40` (654). Nenhum vence nos dois. **Registrar 12 ou 16
   bytes seria palpite, e palpite em layout de vértice produz geometria
   silenciosamente errada.**
2. **Avanço parcial no corpo BMD (novo, medido).** Varredura por norma
   euclidiana 1 localiza um array de normais em **16 de 41** arquivos v0x0A
   nomeados (39,0%), sendo 10 deles com **250/250 = 100,0%**. A taxa de falso
   positivo do teste foi medida primeiro — **4,480×10⁻⁴** em 2.000.000 amostras
   — o que torna 250 acertos consecutivos probabilidade ~0. Isso é evidência,
   não plausibilidade.
   **Avanço subsequente, verificado:** usada como medida *independente*, a
   varredura confirmou um campo de cabeçalho — o **u16 no offset 40 do corpo é a
   contagem de registros de normais**, igual à contagem derivada geometricamente
   em **145 de 154** (94,2%), com a região medindo **exatamente `contagem × 20`
   bytes em 145 de 145**. O registro tem 20 bytes (5 floats), os 3 primeiros
   sendo a normal unitária. Como a contagem vem de uma propriedade geométrica e
   não do próprio campo, a concordância é evidência e não circularidade.
   Evidência em `ciclo1_bmd_normals_field.log`.
   **Ainda não é layout determinado:** O offset absoluto varia de 4 a 358.808 bytes e
   a fase entre 0/4/8/12/16, então não se sabe onde a seção começa sem os
   contadores do cabeçalho, que seguem não decodificados. **25 de 41 (61,0%)**
   não apresentam a região. E o passo de 20 bytes tem 2 floats finais **não
   identificados** — a hipótese (u,v) não foi verificada e não deve ser
   assumida. Classificação: **hipótese com evidência forte para 39,0%, não
   habilita parser.** Evidência em `ciclo1_bmd_normals.log`.
3. **Cifra v0x0C (8.594 arquivos, 85,17%) — BLOQUEADA.** O bloqueio real é
   acesso de saída a um host de código para consultar implementações
   licenciadas permissivamente, não a criptoanálise. O par
   `Object40.smd` (3.847.871 B, texto puro) × `Object40.bmd` (1.041.808 B,
   opaco) é o melhor candidato known-plaintext, mas **1 de 10** `.smd` tem par
   — não 10 de 10 como assumi no início desta investigação.
4. `Enc*` terreno: 219 de 264 arquivos. **BLOQUEADO.**
5. Sem build Windows; a plataforma-alvo nunca foi compilada.
6. Sem decodificação de pixels JPEG: 13.832 OZJ têm geometria validada, 0 têm
   pixels.

## 10. Próxima tarefa e por quê

**Continuar a tarefa 1a — decodificar os contadores do cabeçalho BMD.** A
varredura por norma 1 já localiza a seção de normais em 39,0% dos arquivos, mas
só o offset absoluto não basta: é preciso chegar lá a partir do cabeçalho. Os
bytes logo após o campo de nome de 32 bytes são a pista concreta — em
`Sword40.bmd` são `02 00 02 00 01 00 58 00 81 00 45 00 8e 00` e em
`PrickleGlove_left.bmd` `04 00 33 00 01 00 4c 01 c4 01 24 01 26 02`, ou seja,
u16 pequenos e plausíveis como contagens. Testar se algum deles, ou sua soma,
desloca exatamente até o início da região de normais já detectada é um teste
fechável e não exige adivinhar o resto do formato. É o trabalho de maior valor disponível: não é bloqueado, produz
resultado verificável hoje, e gera o known-plaintext que torna a cifra (1b)
tratável. A abordagem muda em relação à que falhou: em vez de buscar um passo
uniforme no arquivo inteiro, percorrer as seções em ordem — o campo de nome de
32 bytes dá um ponto de partida conhecido, e a partir dele ler contagens e
testar se cada uma divide o segmento seguinte. A hipótese a testar primeiro é a
de um layout em seções (nome → texturas → vértices → faces), não a de um array
único.

Se 1a não fechar com evidência, o próximo item não bloqueado é CAND-001
(upload BGRA direto), que mantém evidência medida e escopo testável.

---

## ESTADO DO GIT NESTE CICLO

**Commits locais** (25, em ordem cronológica):

```
e8ae494  Add three-level BMD usability reporting
accff87  Test the three BMD metrics as separate counters
bff23d8  Report standard deviation and separate cached from I/O-bound CRC
1832776  Add raw evidence logs for every claim made in this cycle
c77d5a0  Classify usable BMD geometry as BLOCKED, as the project's number-one blocker
eeb503e  Qualify derived figures as estimates and correct the test counts
069279e  Record the trivial initial state and the credential scan result
6b163dc  Make commit discipline and metric discipline permanent rules
4e5eb66  Update continuity documents and reorder the queue behind the BMD blocker
d660078  Start BMD body analysis: name field found, layout not determined
0592f17  Add the cycle-1 report and regenerate two stale evidence logs
3211b9f  Locate a normals array in 39% of clear BMD files by unit-norm scan
5efdb2b  Verify the BMD v0x0A normal-count header field and 20-byte record
8e8b844  Test the trailing record floats and reject the (u,v) hypothesis
554ba74  Record cycle-1 progress on the BMD body in NEXT_TASK.md
79fc9c4  Bring the cycle report's git state section up to date
a3030b9  Decode and verify two sections of the BMD v0x0A body layout
11c80af  Implement the v0x0A body parser; level 3 is now measured, not structural
f24ad55  Update the cycle report for the parser landing
9285756  Record the face-section attempt as a negative result
62a3be4  Record texture-name anchors and the UV region as not determined
de7adab  Point the next task at per-mesh blocks, with the dead ends marked
02427c8  Record the mesh-count test as inconclusive, with the measurement defect
d980216  Close the record-size line of attack on the BMD face section
50a5ceb  Close the record-size line of attack in NEXT_TASK.md
```

**Push realizado:** sim, `git push origin arena/519f8fc9-clienterecriado`,
exit 0, executado após cada commit. Nenhuma falha de push ocorreu neste ciclo.

**Confirmação hash remoto == hash local:**

```
$ git rev-parse HEAD
50a5cebad89b9906d9dfa7103bbb4d223ae721df

$ git ls-remote origin arena/519f8fc9-clienterecriado
50a5cebad89b9906d9dfa7103bbb4d223ae721df	refs/heads/arena/519f8fc9-clienterecriado
```

Idênticos.

**`git status --short` final:** vazio — working tree limpa.

**Repositório de referência (somente leitura):**

```
$ cd Cliente_ref && git rev-parse HEAD
88edfc73f5e1d63f806a51f21a118f84ba6c2bb9
```

Idêntico ao registrado no início do ciclo, `git status --short` vazio. Nenhuma
modificação no repositório de pesquisa.

**Nota sobre esta seção:** ela foi atualizada três vezes ao longo do ciclo,
porque a lista de commits e o hash ficaram defasados a cada rodada de trabalho
subsequente (10 → 15 → 25 commits). Um relatório cujo estado de Git não bate com
o repositório é a mesma categoria de erro que a auditoria apontou, então a seção
foi corrigida em vez de deixada como estava.

**Algo não commitado/publicado:** nada.

**Correção registrada durante o ciclo:** a mensagem original de `bff23d8`
afirmava incluir o target `make test-san`, que na verdade entrou em `e8ae494`
junto com a mudança de `LIB_SRCS` no `Makefile`. Corrigida por `--amend`
**antes** de qualquer push, e a própria correção está registrada na mensagem.
Nenhum histórico publicado foi reescrito.
