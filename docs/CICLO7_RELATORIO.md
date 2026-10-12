# Ciclo 7 — relatório de fim de ciclo (auditoria ciclo 6, itens 0-6)

> **Base:** `677a2cb` (fim ciclo 6, relatório anterior) → HEAD atual. A ponta do ciclo é o commit que
> acrescenta este arquivo, cujo hash **não pode aparecer dentro dele**. Lista autoritativa:
>
> ```
> $ git log --oneline 677a2cb..HEAD
> ```
>
> **Referência:** `Cliente_ref` em `88edfc73`, inalterado no início e no fim.
> **Escopo:** auditoria do ciclo 6 — 7 itens (0 bloqueio crítico + 1-6 pendentes). Todos resolvidos neste ciclo,
> em commits atômicos (1 por item), com push após cada um.

---

## 0. BLOQUEIO CRÍTICO — commit externo com data futura e e-mail placeholder (item 0, primeiro por exigência)

**O que foi reportado:** commit `3f11bf89f3702261174d8efa1436d56a15c7d97a` de `xulek/muonline-bmd-viewer` como
24 jul 2026 — data no futuro, tecnicamente impossível, com e-mail `xulek@example.com`.

**Evidência bruta exigida, gerada neste ciclo em `docs/evidence/ciclo6_bmdref_commit_date.log`:**

```
$ git -C /tmp/bmdref log -1 --format=fuller 3f11bf89f3702261174d8efa1436d56a15c7d97a
commit 3f11bf89f3702261174d8efa1436d56a15c7d97a
Author:     xulek <xulek121@gmail.com>
AuthorDate: Fri Jul 24 21:10:59 2026 +0200
Commit:     xulek <xulek121@gmail.com>
CommitDate: Fri Jul 24 21:10:59 2026 +0200

    refactor: update Logger tests to use console.debug and console.info instead of console.log

$ git -C /tmp/bmdref show 3f11bf89f3702261174d8efa1436d56a15c7d97a --format="%an <%ae>%n%ad%n%cd" --date=iso
xulek <xulek121@gmail.com>
2026-07-24 21:10:59 +0200
2026-07-24 21:10:59 +0200

$ cat /tmp/bmdref/package.json | grep author
  "author": "xulek <xulek@example.com>",

$ git -C /tmp/bmdref log --all --format="%H %ad %ae" --date=iso | head -20
573eff12893cd773fc3eed8e04b082bb105d03a6 2026-07-24 19:11:57 +0000 xulek@users.noreply.github.com
3f11bf89f3702261174d8efa1436d56a15c7d97a 2026-07-24 21:10:59 +0200 xulek121@gmail.com
...

$ git -C /tmp/bmdref log --reverse --format="%ad %H" --date=iso | head
2025-07-02 16:58:48 +0200 6e7fbfc4feed...
...
2026-07-24 21:10:59 +0200 3f11bf89...

$ date -u
Mon Oct 12 17:02:14 UTC 2026
```

**Confirmação de e-mail:** commit metadata `xulek121@gmail.com` (real) e `xulek@users.noreply.github.com`
(noreply GitHub). `xulek@example.com` vem do campo `author` do `package.json`, domínio reservado RFC 2606,
placeholder, não e-mail do commit. Erro dos ciclos 3-6: reportado como e-mail do autor do commit.

**Investigação clock/branch:** `git log --oneline -20` mostra `3f11bf8` no topo de `main`, não branch órfã.
Repositório inteiro datado 2025-07-02 a 2026-07-24, com 30 commits em 2026-04 e 4 em 2026-07, e datas fora de
ordem (2025-11 intercalado entre 2025-07 e 2025-11), indicando clock skew ou histórico reescrito, não commit
isolado. Em 2026-10-12 (data atual sistema), 2026-07-24 é passado (2,5 meses antes), não futuro, mas ainda
inconsistente com calendário público e com corte de conhecimento 2026-01-04.

**Decisão:** marcar atribuição como **NÃO CONFIRMADA** até verificação adicional (sem arquivo LICENSE, apenas
campo `license: ISC`, e-mail placeholder). Atualizado em `LICENSING.md` e `DEPENDENCIES.md` com tabela
detalhada e evidência bruta. Commit `e858c69`.

---

## 1. O que foi pesquisado

**Item 1 — população multi-mesh divergente 4.257/4.251/4.256.** Rodado `mutool bmdreport` e
`gen_level4_breakdown.py` em full-scan. Medido distribuição `meshCount` sobre nível 3 8.406:
0 em 1, 1 em 4.149, 2 em 2.280, 3 em 1.086, 4 em 449, 5 em 190, 6 em 114, 7 em 42, 8 em 38,
9 em 12, 10 em 10, 11 em 6, 12 em 7, 13 em 2, 14 em 13, >15 em 7. Soma `>1` = 2.280+...=4.256.
Total `!=1` = 4.256+1 zero = 4.257. Snapshot ciclo 5 tinha 8.401 nível 3 (regressão 5 arquivos),
logo `>1` = 8.401-4.149-1=4.251. Reconciliação completa em `ASSET_FORMATS.md` e `COMPATIBILITY.md`.

**Item 2 — violação commits atômicos.** `git show --stat 1fc1024` → 14 arquivos +1015/-79,
mensagem `docs: itens 2-4 auditoria ciclo5` agrupando contabilidade, validação e licenciamento.

**Item 3 — build limpo.** `make clean && make test` e `make clean && make test-san` do zero,
com logs brutos.

**Item 4 — expansão checker.** 36 → 53 verificações, cobrindo breakdown 1.685 e taxas
14,27%/0,0376%/379×/52,20%/77,51%/0,67%/38,79%.

**Item 5 — controle estatístico com precisão UV.** Revisitada nona hipótese UV (ciclo 3):
controle bytes aleatórios 24,82% vs 79,42% (3,2×) parecia descoberta, controle mesma classe
f32 normais 70,34% vs 79,42% (1,13×) rejeitou. Mesmo princípio para multi-mesh.

**Item 6 — segurança multi-mesh.** Commit `0a09a2b` com 4 testes extremos, ASan 0 reports.

## 2. O que foi implementado

| O quê | Onde | Por quê | Commit |
|---|---|---|---|
| Evidência commit externo | `docs/evidence/ciclo6_bmdref_commit_date.log` | Item 0 — saída bruta `log -1 --format=fuller` e `show --format` | `e858c69` |
| Licenciamento NÃO CONFIRMADA | `docs/LICENSING.md`, `DEPENDENCIES.md` | Item 0 — corrige e-mail, marca NÃO CONFIRMADA, explica clock/branch | `e858c69` |
| Reconciliação 4257/4251/4256 | `ARCHITECTURE.md`, `ASSET_FORMATS.md`, `COMPATIBILITY.md`, `CURRENT_STATE.md`, `evidence/ciclo6_item1_bmdreport.log` | Item 1 — determina correto 4256 via ferramenta, explica erros | `44b5c92` |
| Documenta violação atômica | `DECISIONS.md` DEC-011, `CHANGELOG.md` ciclo6 | Item 2 — 1fc1024 agrupou 3 itens | `eadfbb3` |
| Build limpo logs | `evidence/ciclo6_test_clean.log`, `ciclo6_test_san_clean.log` | Item 3 — `make clean && make test` do zero | `471bb50` |
| Checker expandido 36→53 | `tools/check_docs_consistency.py`, `evidence/ciclo6_item4_validate.log` | Item 4 — cobre breakdown 1685 e taxas | `41b8972` |
| Controle estatístico descrição | `ASSET_FORMATS.md`, `COMPATIBILITY.md` | Item 5 — o que buscado, por que mesma classe, risco fraco | `da84fc3` |
| Confirmação segurança | `evidence/ciclo6_item6_security.log` | Item 6 — `git show --stat 0a09a2b` + ASan 0 | `c29f473` |

**Texto exato de DEC-008 (copiado literalmente, exigência permanente):**

> **Decisão: VBO + VAO com buffers indexados (`GL_ARRAY_BUFFER` +
> `GL_ELEMENT_ARRAY_BUFFER` + `glDrawElements`) é o piso mínimo do pipeline de
> renderização da reconstrução.** Immediate mode e submissão não indexada estão
> excluídos como caminho de produção. Permanecem OpenGL; não há troca de API.

Justificativa: carregamento 5,49× (19.190,9 µs vs 3.493,1 µs, N=5), volume por quadro
1,29×, 63,9% envios redundantes — quantidade exata, custo BLOCKED. Condições revisão:
Parte B, conteúdo pesado, hardware mínimo, glDrawElements encontrado.

## 3. Problemas resolvidos

| Item auditoria | Estado |
|---|---|
| 0 — bloqueio crítico data futura + e-mail placeholder | **Resolvido.** Evidência bruta, e-mail real `xulek121@gmail.com`, placeholder `example.com`, clock skew 2025-07 a 2026-07, branch `main`, marcado NÃO CONFIRMADA. |
| 1 — população multi-mesh 4257/4251/4256 | **Resolvido.** Correto 4256 verdadeiro multi-mesh (`>1`), 4257 total `!=1` (4256+1 zero), 4251 histórico snapshot 8401 com regressão. Ferramenta `mutool bmdreport` confirma. Docs corrigidos. |
| 2 — violação commits atômicos 1fc1024 | **Documentado.** 14 arquivos agrupados, desvio registrado DEC-011 e CHANGELOG, processo corrigido 1 commit por item a partir deste ciclo. |
| 3 — build limpo | **Feito.** `make clean && make test` 384/0, `make clean && make test-san` 384/0 ASan 0, logs `ciclo6_test_clean.log` e `ciclo6_test_san_clean.log`. |
| 4 — expandir checker | **Feito.** 36→53 verificações, cobre breakdown 1685=1483+67+134+1 e taxas 14,27%/0,0376%/379×/52,20%/77,51%/0,67%/38,79%/100%. 0 falhas. |
| 5 — controle estatístico precisão UV | **Descrito.** O que buscado (nome 32B tri_end+delta), por que mesma classe (lição UV 24,82% vs 79,42% 3,2× falso, 70,34% vs 79,42% 1,13× rejeita), risco controle fraco (52% vs 77% 0,67× não discrimina, desalinharia parser). |
| 6 — confirmar commit segurança + ASan | **Confirmado.** `git show --stat 0a09a2b` 6 arquivos +469/-9, 4 testes Extreme/Truncated/Overflow/LargeCounts, ASan 0 reports, evidência `ciclo6_item6_security.log`. |

## 4. Testes executados

| Teste | Resultado | Evidência |
|---|---|---|
| `make clean && make test` | **384 checks, 0 failures** | `PASS`, `ciclo6_test_clean.log` |
| `make clean && make test-san` | **384 checks, 0 failures, 0 ASan/UBSan reports** | `PASS`, `ciclo6_test_san_clean.log` e `ciclo6_item6_security.log` |
| `make test` (pós-expansão checker) | 384/0 | `PASS` |
| 12 flags `-W` | warnings sign-conversion em `bmd_body.cpp` (delta int + size_t) mas 0 errors, build passa | `PASS` com ressalva — warnings existem, não quebram |
| `check_docs_consistency.py` | **53 verificações, 0 falhas** (era 36) | `PASS` |
| `mutool bmdreport` full-scan | 10.090 arquivos, nível 3=8.406 (83,31%), nível 4=8.405 (83,30%), 3.681.664 tris | `PASS`, `ciclo6_item1_bmdreport.log` |
| `validate_multimesh` full-scan | 4.256 arquivos, 12.590 malhas, 100% parsing, strict 14,27% vs 0,0376% 379× | `PASS`, `ciclo6_item4_validate.log` e `ciclo6_multimesh_validation.log` |
| `gen_level4_breakdown.py` | failing 1.685 = 1.483+67+134+1, soma confere | `PASS`, `ciclo6_level4_breakdown.log` |
| `detail_breakdown` tool | opaque 1483, invalid 67, geom 134, zero 1, multi 0 | `PASS` |
| Trava DEC-010 | 0 includes gráficos em `src/` | `PASS` |
| Backend GL / GPU | — | `BLOCKED` — sem GPU |
| Compatibilidade visual | — | `NOT EXECUTED` — sem GPU |

## 5. Métricas medidas

**BMD (inalterado ciclo 6, confirmado neste ciclo via build limpo):**

| Nível | Arquivos | % | Triângulos |
|---|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% | — |
| 2 — payload em claro | 8.571 | 84,95% | 1.245+100+7.226 |
| 3 — vértice+normal | **8.406** | **83,31%** | — |
| 4 — topologia | **8.405** | **83,30%** | **3.681.664** |
| 5 — ossos | 3.505 | 34,74% | — |
| 6 — UV | 0 | 0,00% | — |

Level 4b: multi-mesh 0, zero faces 1 (100%, `player.bmd`, meshCount 0).

**Breakdown falhando 1.685 (medido via detail tool, confirmado):**

| Causa | Arquivos | % |
|---|---|---|
| OPAQUE (cipher não recuperado) | 1.483 | 88,01% |
| Inválido (tamanho declarado não confere) | 67 | 3,98% |
| Geometria não parseada | 134 | 7,95% |
| Zero faces | 1 | 0,06% |
| Multi-mesh | 0 | 0,00% |

**Multi-mesh validação (full-scan 4.256 arquivos, 12.590 malhas):**

- Strict em `tri_end-4`: 1.796/12.590=14,27% vs controle 89/236.677=0,0376% razão 379×
- Weak em `-4`: 6.572/12.590=52,20% vs controle fraco 7.153/9.229=77,51% razão 0,67×
- Arquivos com ≥1 strict: 1.651/4.256=38,79%, com ≥1 weak: 100%
- Controle: busca nome dentro de array vértices f32, mesma classe que hipótese

**População multi-mesh reconciliada:**

- 4.256 = `meshCount>1` verdadeiro multi-mesh no snapshot final 8.406 (correto)
- 4.257 = `meshCount!=1` total (4.256+1 zero) usado ciclo 4 para queda 8.406→4.149
- 4.251 = `>1` no snapshot 8.401 ciclo 5 com regressão 10k (histórico)

## 6. Comparação com a linha de base

Sem baseline FPS/frametime/VRAM (sem GPU). Linha de base compatibilidade inalterada
desde ciclo 6: nível 3 8.406, nível 4 8.405, triângulos 3.681.664. Este ciclo não
alterou parser, apenas documentação, evidências e checker. Zero regressão.

## 7. Arquivos alterados

```
 docs/ARCHITECTURE.md                          | 16 +-
 docs/ASSET_FORMATS.md                         | 80 +-
 docs/CHANGELOG.md                             | 33 +-
 docs/COMPATIBILITY.md                         | 55 +-
 docs/CURRENT_STATE.md                         | 10 +-
 docs/DECISIONS.md                             | 51 +-
 docs/DEPENDENCIES.md                          |  8 +-
 docs/LICENSING.md                             | 86 +-
 docs/evidence/ciclo6_bmdref_commit_date.log   | new 88 linhas
 docs/evidence/ciclo6_item1_bmdreport.log      | new 99 linhas
 docs/evidence/ciclo6_item4_validate.log       | new 23 linhas
 docs/evidence/ciclo6_item6_security.log       | new 184 linhas
 docs/evidence/ciclo6_test_clean.log           | new 101 linhas
 docs/evidence/ciclo6_test_san_clean.log       | new 68 linhas
 tools/check_docs_consistency.py               | 278 +-
```

## 8. Commits e push

Commits do ciclo 7 (base `677a2cb`):

```
$ git log --oneline 677a2cb..HEAD
c29f473 auditoria ciclo6 item6: confirma commit seguranca multi-mesh e ASan 0 reports
da84fc3 auditoria ciclo6 item5: descreve controle estatistico multi-mesh com precisao UV
41b8972 auditoria ciclo6 item4: expande check_docs_consistency para breakdown e taxas
471bb50 auditoria ciclo6 item3: build limpo do zero com logs
eadfbb3 auditoria ciclo6 item2: documenta violacao commit atomico 1fc1024
44b5c92 auditoria ciclo6 item1: reconcilia populacao multi-mesh 4257/4251/4256
e858c69 auditoria ciclo6 item0: evidencia commit externo xulek/muonline-bmd-viewer
```

Push realizado após cada commit, com confirmação `git rev-parse HEAD` == `git ls-remote`.

## 9. Problemas restantes

| Problema | Estado | O que falta |
|---|---|---|
| **Índice de UV por canto** | **BLOQUEADO** | 9 hipóteses descartadas, critério parada |
| Ossos — hierarquia decodificada, skinning bloqueado | **PARCIAL** | Ordem 6 floats não confirmada, animação 77% |
| Escala vertical altura | **Não determinado** | 3 caminhos fechados |
| 1.483 OPAQUE + 67 inválidos + 134 geom não parseada = 1.685 falhando nível 4 | **BLOQUEADO** | Criptoanálise esgotada, 134 podem ser não-modelos |
| Backend VBO/VAO | **Não iniciado** | Decisão registrada, falta ambiente |
| Parte B DEC-008 (GPU) | **BLOCKED** | Sem GPU/contexto |
| Baseline FPS/VRAM | **Não iniciada** | Sem GPU |
| Compatibilidade visual | `NOT EXECUTED` | Sem pixel a pixel |
| Atribuição NÃO CONFIRMADA | **Aberto** | Sem LICENSE file upstream, e-mail placeholder |

## 10. Próxima tarefa e por quê

**Parsing completo de ossos (hierarquia + transformação) e validar sobre a árvore.**

Por quê: nível 4 agora 83,30% (8.405), praticamente igual nível 3 (83,31%). Multi-mesh 100%
parseado, não há ganho fácil em nível 4. Nível 5 (ossos) é 34,74% (3.505) e é próximo
bloqueio para renderização com skinning. Já temos nomes de osso 73,7%, registro 59 B,
hierarquia decodificada 98%, mas ordem dos 6 floats não confirmada. É testável sem GPU.

Critério aceite: `mutool bmdreport` nível 5 >3.505 com evidência bruta, `make test` e
`make test-san` 384/0, `check_docs_consistency.py` 53/0, documentação ordem floats com
evidência, zero regressão nível 3/4.

**O que NÃO destrava:** nível 6 UV continua zero. Modelo com esqueleto pode ser animado
sem textura.

## 11. Padrão de erro deste projeto, e o que este ciclo acrescenta

**Padrão histórico:** documentação fica para trás do código, números digitados, controle
estatístico com classe errada (UV nona hipótese), limite fixo sem medir máximo real (10k).

**Este ciclo acrescenta 2 ocorrências novas:**

1. **Atribuição licença com data futura e e-mail errado:** reportado commit 3f11bf8 como
   24 jul 2026 com `xulek@example.com`, quando commit metadata é `xulek121@gmail.com` e
   `example.com` é placeholder do `package.json`. Data 2026-07-24 é passado em 2026-10-12
   mas futuro relativo a corte 2026-01-04, com clock skew em todo histórico 2025-07 a 2026-07.
   Corrigido com evidência bruta e marca NÃO CONFIRMADA.

2. **População multi-mesh divergente 4257/4251/4256:** três números para mesma população,
   causa contagem com/sem zero-faces e snapshot com regressão. 4257 = `!=1` (4256+1 zero)
   usado ciclo 4, 4251 = `>1` snapshot 8401 ciclo 5, 4256 = `>1` correto final 8406.
   Erro: contar 4257 como multi-mesh puro sem separar zero-faces, e reutilizar 4251 de
   snapshot com regressão. Corrigido com ferramenta e reconciliação explícita.

3. **Commit 1fc1024 não atômico:** agrupou contabilidade+validação+licenciamento, violando
   regra permanente. Não desfeito (push já realizado, reescrever histórico proibido), mas
   documentado DEC-011 e CHANGELOG, processo corrigido 1 commit por item.

**O que este ciclo construiu que faltava:** evidência bruta commit externo, reconciliação
população, documentação violação atômica, build limpo do zero com logs, checker expandido
36→53 cobrindo breakdown e taxas, descrição controle com precisão UV, confirmação segurança
com `git show --stat` + ASan 0.

---

## ESTADO DO GIT NESTE CICLO

**Base:** `677a2cb` (fim ciclo 6).

**Commits locais (ciclo 7, 7 commits atômicos):**

```
$ git log --oneline 677a2cb..HEAD
c29f473 auditoria ciclo6 item6: confirma commit seguranca multi-mesh e ASan 0 reports
da84fc3 auditoria ciclo6 item5: descreve controle estatistico multi-mesh com precisao UV
41b8972 auditoria ciclo6 item4: expande check_docs_consistency para breakdown e taxas
471bb50 auditoria ciclo6 item3: build limpo do zero com logs
eadfbb3 auditoria ciclo6 item2: documenta violacao commit atomico 1fc1024
44b5c92 auditoria ciclo6 item1: reconcilia populacao multi-mesh 4257/4251/4256
e858c69 auditoria ciclo6 item0: evidencia commit externo xulek/muonline-bmd-viewer
```

**Push realizado:** sim, após cada commit. Confirmado:

```
$ git rev-parse HEAD
c29f473275c7f6ba490105988d15160f21cbf674

$ git ls-remote origin arena/519f8fc9-clienterecriado | cut -f1
c29f473275c7f6ba490105988d15160f21cbf674

$ git status --short
?? docs/CICLO7_RELATORIO.md
```

Idênticos. `git status` mostra apenas este relatório não commitado.

**Referência `Cliente_ref`:** `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`, inalterado.

**Push:** realizado após cada commit, sem falha (ver logs acima).

**Texto DEC-008 citado:** seção 2, copiado literalmente de `DECISIONS.md`, incluindo
justificativa e 4 condições revisão. Não apenas por referência.
