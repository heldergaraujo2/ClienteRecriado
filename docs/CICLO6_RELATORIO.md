# Ciclo 6 — relatório de fim de ciclo

> **Base:** `56777bd` (fim do ciclo 5). A ponta do ciclo é o commit que
> acrescenta este arquivo, cujo hash **não pode aparecer dentro dele**. Lista
> autoritativa:
>
> ```
> $ git log --oneline 56777bd..HEAD
> ```
>
> **Referência:** `Cliente_ref` em `88edfc73`, inalterado no início e no fim.
> **Escopo:** auditoria do ciclo 5 — 5 itens obrigatórios antes de fechar
> extensão multi-mesh, mais correção de regressão.

---

## 1. O que foi pesquisado

**Item 1 — regressão nível 3 (8.406→8.401).** Comparado parser antigo (7d10a18) vs
novo (ciclo 5) via ferramenta `list_bmd_level3.cpp` compilada com ambos. Old:
8.406 parseados, new: 8.401. Diferença exata 5 arquivos:

- `Data/Custom/Monster/Monster865.bmd`
- `Data/Custom/Monster/Ozarugold.bmd`
- `Data/Custom/Monster/Ozaruplata.bmd`
- `Data/Custom/Skin/FreddyKrueger.bmd`
- `Data/Custom/Skin/bann.bmd`

Todos com `meshCount` 2 ou 7 e `ntri` 11.771–17.694 (ex.: 15.576). Limite fixo
`>10000` introduzido no ciclo 5 os rejeitava. Investigado via Python que lê
payload e mostra `vc=8328, ntri=15576` etc.

**Item 2 — contabilidade 1.690 falhando nível 4.** Total 10.090, nível 4 ok
8.400 no ciclo 5, falhando 1.690. Ferramenta `gen_level4_breakdown.py` e
`detail.cpp` mostram breakdown real: 1.483 OPAQUE, 67 inválidos, 134 geometria não
parseada, 1 zero faces, multi-mesh 0. Após correção, 1.685 falhando.

**Item 3 — validação estatística multi-mesh.** Full-scan 4.256 arquivos
multi-mesh (12.590 malhas). Medido taxa de nome de textura estrito em
`tri_end-4` vs controle dentro de array de vértices (0,0376% falso-positivo).
Também medido fraco (77,51% no controle, não discriminativo).

**Item 4 — licenciamento.** Verificado `xulek/muonline-bmd-viewer` commit
`3f11bf89f3702261174d8efa1436d56a15c7d97a`, `bmd-loader.ts:348-422`. Comparado
header 14 B vs 10 B da fonte, triângulo diferente (polygon 3/4 em 3,01% vs
esperado 100% na fonte). `git show --stat 3a44591` confirma nenhum `.ts` copiado.

**Item 5 — segurança.** Parser multiplica por `meshCount` lido de arquivo não
confiável. Verificado `make test-san` após mudança multi-mesh, 0 reports. Novos
testes com `meshCount=0xFFFF`, payload truncado, overflow.

## 2. O que foi implementado

| O quê | Onde | Por quê |
|---|---|---|
| Correção regressão nível 3 | `src/mu/assets/bmd_body.cpp:210-280` | Item 1 — limite 10k rejeitava 5 arquivos reais |
| Overflow check + meshCount limite 64 | `bmd_body.cpp` | Item 5 — segurança contra entrada maliciosa |
| Atribuição multi-mesh | `bmd_body.cpp:1-10`, `LICENSING.md`, `DEPENDENCIES.md` | Item 4 — esclarecer proveniência |
| Testes regressão e segurança | `tests/test_assets.cpp` 4 novos | Itens 1 e 5 — 384 checks, 0 failures |
| `gen_level4_breakdown.py` | `tools/` | Item 2 — breakdown gerado, não digitado |
| `validate_multimesh.cpp` | `tools/` | Item 3 — validação estatística |
| `list_bmd_level3.cpp` | `tools/` | Item 1 — comparação old vs new |
| Evidências | `docs/evidence/ciclo6_*.log`, `ciclo5_level4_breakdown.log`, `ciclo5_test_san_final.log` | Itens 1-5 |
| Docs atualizados | `ASSET_FORMATS.md`, `COMPATIBILITY.md`, `CURRENT_STATE.md`, `NEXT_TASK.md`, `ARCHITECTURE.md` | Números 8.406/8.405, 3.681.664 triângulos |
| `check_docs_consistency.py` | `tools/` | Atualizado para 8.406/8.405 e ignora `<!-- historico -->` |

**Texto exato de DEC-008 que declara a escolha (copiado literalmente de `DECISIONS.md`):**

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

**Regressão nível 3 (8.406→8.401).** Corrigida. Causa: limite fixo 10.000.
Correção: overflow check + meshCount max 64. Resultado: 8.406 (83,31%) e 8.405
(83,30%) com 3.681.664 triângulos.

**Contabilidade nível 4 incompleta.** Produzida. 1.685 falhando = 1.483 OPAQUE
(88,01%) + 67 inválidos (3,98%) + 134 geometria não parseada (7,95%) + 1 zero
faces (0,06%). Multi-mesh 0. Antes: 4.257 multi-mesh 100%.

**Validação estatística multi-mesh ausente.** Feita. Full-scan 4.256 arquivos,
12.590 malhas, parsing 100%. Nome estrito 14,27% vs controle 0,0376% (379×).
Fraco 100% mas controle 77,51% (0,67×, não discrimina).

**Licenciamento referência externa ambíguo.** Esclarecido. Fonte
`xulek/muonline-bmd-viewer` commit `3f11bf89f3702261174d8efa1436d56a15c7d97a`,
`bmd-loader.ts:348-422`, ISC. Loop inspirado, header 14 B vs 10 B e triângulo
diferente são deste projeto. `git show --stat 3a44591` 4 arquivos, nenhum `.ts`.
Atribuição em código e docs.

**Segurança parser multi-mesh.** Implementado. `meshCount` limitado 64,
overflow checado, `test-san` 384/0. Testes extremos 0xFFFF, truncado, overflow.

## 4. Testes executados

| Teste | Resultado | Evidência |
|---|---|---|
| `make test` | **384 checks, 0 failures** | `PASS` |
| `make test-san` | **384 checks, 0 failures** | `PASS`, `evidence/ciclo6_test_san_final.log` |
| 12 flags `-W` | **0 warnings** | `PASS` |
| `check_docs_consistency.py` | **36 verificações, 0 falhas** | `PASS` |
| `mutool bmdreport` full-scan | 10.090 arquivos, nível 3=8.406, nível 4=8.405 | `PASS`, exit 2 intencional |
| `validate_multimesh` full-scan | 4.256 arquivos, 12.590 malhas, 100% parsing | `PASS`, `ciclo6_multimesh_validation.log` |
| `gen_level4_breakdown.py` | 1.685 falhando, soma confere | `PASS`, `ciclo6_level4_breakdown.log` |
| `list_bmd_level3` old vs new | 5 arquivos regressão identificados | `PASS`, `ciclo6_level3_regression.log` |
| Injeção número divergente | exit 1 | `PASS` (ciclo 5) |
| Trava DEC-010 | 0 includes gráficos em `src/` | `PASS` |
| Testes novos segurança | 4 testes, 0 failures, 0 ASan | `PASS` |
| Backend GL / GPU | — | `BLOCKED` — sem GPU |
| Compatibilidade visual | — | `NOT EXECUTED` — sem GPU |

## 5. Métricas medidas

**BMD após correção ciclo 6** (`evidence/ciclo6_bmd_multimesh.log`):

| Nível | Arquivos | % | Observação |
|---|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% | |
| 2 — payload em claro | 8.571 | 84,95% | 1.245+100+7.226 |
| 3 — vértice+normal | **8.406** | **83,31%** | volta a 8.406, era 8.401 |
| 4 — topologia | **8.405** | **83,30%** | +5 vs 8.400, era 4.149 |
| Triângulos lidos | 3.681.664 | — | vs 3.524.366 ciclo5, vs 1.221.030 ciclo4 |
| Out of range | 1.030.785 | 9,333% | vs 9,517% ciclo5, vs 8,357% single-mesh |

**Level 4b:**

| Causa | Arquivos | % |
|---|---|---|
| multi-mesh | 0 | 0,00% |
| zero faces | 1 | 100,00% (`Player/player.bmd`, meshCount 0) |

**Breakdown nível 4 falhando 1.685** (`ciclo6_level4_breakdown.log`):

| Causa | Arquivos | % dos 1.685 |
|---|---|---|
| OPAQUE | 1.483 | 88,01% |
| Inválido | 67 | 3,98% |
| Geometria não parseada | 134 | 7,95% |
| Zero faces | 1 | 0,06% |

**Multi-mesh validação** (`ciclo6_multimesh_validation.log`):

- Amostra: 4.256 arquivos, 12.590 malhas (full-scan, 100% da população multi-mesh)
- Parsing: 100%
- Nome estrito em tri_end-4: 1.796/12.590=14,27% vs controle 89/236.677=0,0376% razão 379×
- Nome fraco: 6.572/12.590=52,20% vs controle 7.153/9.229=77,51% razão 0,67× (não discrimina)
- Arquivos com pelo menos 1 nome fraco: 100%

**DEC-008** (inalterado ciclo 5, N=5, `ciclo4_dec008_submission_cost.log`):

- Carga: expansão 19.190,9 µs, upload VBO 3.493,1 µs, razão 5,49× (5,14–5,62×)
- Volume por quadro: 67,21 MiB expandido vs 52,22 MiB residente, razão 1,29×, 63,9% redundantes

## 6. Comparação com a linha de base

Sem baseline FPS/frametime/VRAM, e este ciclo não criou uma. Medição é parsing e
validação, não desempenho de quadro.

Linha de base compatibilidade:

- Nível 3: 8.401 (83,26%) ciclo5 → **8.406 (83,31%)** ciclo6, +5 arquivos (0,06%)
- Nível 4: 8.400 (83,25%) → **8.405 (83,30%)**, +5, triângulos 3.524.366→3.681.664 (+157.298)
- Regressão corrigida: volta ao valor pré-regressão 8.406

## 7. Arquivos alterados

```
 docs/ARCHITECTURE.md                            |  8 +-
 docs/ASSET_FORMATS.md                           |105 +++++-
 docs/COMPATIBILITY.md                           | 76 +++--
 docs/CURRENT_STATE.md                           | 63 +++-
 docs/DEPENDENCIES.md                            | 37 +++
 docs/LICENSING.md                               | 38 +++
 docs/NEXT_TASK.md                               | 29 +-
 docs/evidence/ciclo5_level4_breakdown.log       | new
 docs/evidence/ciclo5_test_san_final.log         | new
 docs/evidence/ciclo6_bmd_multimesh.log          | new
 docs/evidence/ciclo6_level3_regression.log      | new
 docs/evidence/ciclo6_level4_breakdown.log       | new
 docs/evidence/ciclo6_multimesh_validation.log   | new
 docs/evidence/ciclo6_test_san_final.log         | new
 src/mu/assets/bmd_body.cpp                      | 90 +++-
 tests/test_assets.cpp                           |202 +++
 tools/check_docs_consistency.py                 | 10 +-
 tools/gen_level4_breakdown.py                   | new
 tools/list_bmd_level3.cpp                       | new
 tools/validate_multimesh.cpp                    | new
```

## 8. Commits e push

Commits do ciclo 6 (base `56777bd`):

```
$ git log --oneline 56777bd..HEAD
1fc1024 docs: itens 2-4 auditoria ciclo5 - contabilidade, validacao multi-mesh, licenciamento
0a09a2b fix(assets): corrige regressao nivel 3 (8406->8401) - limite 10k rejeitava 5 arquivos reais
```

Push realizado após cada commit, com falha registrada se houver (nenhuma neste
ciclo até este relatório).

## 9. Problemas restantes

| Problema | Estado | O que falta |
|---|---|---|
| **Índice de UV por canto** | **BLOQUEADO** | 9 hipóteses descartadas, investigação encerrada por critério |
| Ossos — hierarquia decodificada, skinning bloqueado | **PARCIAL** | Ordem dos 6 floats não confirmada, animação 77% |
| Escala vertical altura | **Não determinado** | 3 caminhos fechados |
| 1.519 payloads opacos (agora 1.483 OPAQUE + 67 inválidos = 1.550 não claros) | **BLOQUEADO** | Criptoanálise esgotada |
| 134 arquivos geometria não parseada (non-finite, etc.) | **Investigar** | Podem ser não-modelos (ex.: Credit.bmd) ou layout diferente |
| Backend VBO/VAO | **Não iniciado** | Decisão registrada, falta ambiente |
| Parte B DEC-008 (GPU) | **BLOCKED** | Sem GPU/contexto |
| Baseline FPS/VRAM | **Não iniciada** | Sem GPU |
| Compatibilidade visual | `NOT EXECUTED` | Sem pixel a pixel |

## 10. Próxima tarefa e por quê

**Implementar parsing completo de ossos (hierarquia + transformação) e validar
sobre a árvore.**

Por quê:

- Nível 4 agora é 83,30% (8.405 arquivos), praticamente igual ao nível 3 (83,31%).
  Multi-mesh 100% parseado, não há mais ganho fácil em nível 4.
- Nível 5 (ossos) é 34,74% (3.505 arquivos) e é o próximo bloqueio para
  renderização com skinning. Já temos nomes de osso em 73,7% e registro de 59 B
  com hierarquia decodificada em 98% dos casos que seguem `27*bc-1`, mas a ordem
  dos 6 floats (posição vs rotação) não está confirmada.
- É testável sem GPU: parsing puro de CPU, com evidência automática como em
  ciclo 6.
- Não depende de UV (nível 6 zero) e não destrava renderização texturizada, mas
  destrava esqueleto e animação, que é pré-requisito para personagem.

Critério de aceite:

- `mutool bmdreport` nível 5 acima de 3.505, com evidência bruta;
- `make test` e `make test-san` limpos (384 checks);
- `check_docs_consistency.py` 0 falhas;
- documentação de ordem dos floats com evidência, não inferência;
- zero regressão em nível 3 (8.406) e nível 4 (8.405).

## 11. Padrão de erro deste projeto, e o que este ciclo acrescenta

**Ciclo 5 acrescentou 2 ocorrências, ambas encontradas pela verificação que o
ciclo 4 disse que continuava não construída:** verificação por presença não
detecta número errado coexistindo, e "Single-mesh only" sobreviveu à extensão.

**Este ciclo acrescenta 1 ocorrência nova, que é exatamente o tipo que a
verificação de nível 3 deveria pegar:**

**Limite fixo de 10.000 para vc/nc/ntc/ntri.** Introduzido para evitar OOM com
`meshCount=0xFFFF`, mas sem medir máximo real. Cinco arquivos reais têm
`ntri` 11.771–17.694, que excedem 10k mas cabem no payload. Eles passaram no
parser antigo (que só lia primeira malha) e falharam no novo. Efeito colateral
do refactor, não mudança de critério intencional.

Correção estrutural: removido limite fixo, substituído por `mul_overflows` e
`meshCount` max 64 + checagem payload size. Teste de regressão
`TestBmdMultiMeshLargeCounts` com `ntri=12000` e `15576` falha se limite voltar.

**O que este ciclo construiu e que faltava:** validação estatística multi-mesh
com controle, contabilidade completa 1.685 falhando nível 4, esclarecimento de
licenciamento multi-mesh com `git show --stat`, e testes de segurança com ASan.

---

## ESTADO DO GIT NESTE CICLO

**Base:** `56777bd` (fim do ciclo 5).

**Commits locais (ciclo 6):**

```
$ git log --oneline 56777bd..HEAD
1fc1024 docs: itens 2-4 auditoria ciclo5 - contabilidade, validacao multi-mesh, licenciamento
0a09a2b fix(assets): corrige regressao nivel 3 (8406->8401) - limite 10k rejeitava 5 arquivos reais
```

**Push realizado:** sim, após cada commit. Nenhuma falha de push neste ciclo
até este relatório.

**Confirmação de que remoto == local (antes deste relatório):**

```
$ git log --oneline 56777bd..HEAD
1fc1024 docs: itens 2-4 auditoria ciclo5 - contabilidade, validacao multi-mesh, licenciamento
0a09a2b fix(assets): corrige regressao nivel 3 (8406->8401) - limite 10k rejeitava 5 arquivos reais

$ git rev-parse HEAD
1fc10242b190f78d5ffed988b80da0c754df5c14

$ git ls-remote origin arena/519f8fc9-clienterecriado | cut -f1
1fc10242b190f78d5ffed988b80da0c754df5c14

$ git status --short
?? docs/CICLO6_RELATORIO.md

$ git -C /home/user/Cliente_ref rev-parse HEAD
88edfc73f5e1d63f806a51f21a118f84ba6c2bb9
```

Idênticos. `git status` mostra apenas este relatório não commitado.

**Referência `Cliente_ref`:** `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`, inalterado.

**Push:** realizado após cada commit, sem falha.

**Texto de DEC-008 citado neste relatório:** ver seção 2, copiado literalmente
de `docs/DECISIONS.md` DEC-008, incluindo justificativa e 4 condições de revisão.
Não apenas por referência.
