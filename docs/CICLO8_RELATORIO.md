# Ciclo 8 — relatório final

> **Missão `/goal`:** reconstrução progressiva de cliente MU Online com foco em FPS, fluidez, latência, carregamento, estabilidade e compatibilidade visual/funcional. Ordem exigida: auditoria → diagnóstico → motor gráfico → plano otimização → reconstrução incremental → compatibilidade → testes automatizados → segurança → roteiro Windows → build/distribuição.

## 1. O que foi pesquisado

- **Layout final de osso BMD:** confirmado `bc*59-1` single e `4+bc*59-1` multi com `u32 actionCount`. Cada osso 32B nome NUL-padded + u16 parent 0xFFFF=root + 6×f32 + 1B pad exceto último 58B. Hex prova `PrickleGlove.bmd` remain 62 = `01 00 00 00` + `PrickleGlo\0...` + `FFFF` + 6 floats. Evidência `ciclo8_bones_multi.log`, `ciclo8_bones_layout.log`.
- **Estatística multi-mesh corrigida:** com extra 4B, `Sword37` bc2 remain121 extra1, `Sword42` bc6 remain357 extra1, `Sword43` bc5 remain298, `umbrella_eyes_object` bc2 remain121. Custom bc51 remain 2698 vs expected 3008 diff -310 → filtrado, não segue 59B.
- **Critério parsing:** bc 1..64, base+bc*59-1 <= size ou base+bc*59 <= size, todos os ossos com >=3 printable em 32B e floats finitos e parent válido. Evita falso-positivo (random ~0% nomes).
- **Referência externa:** `xulek/muonline-bmd-viewer` commit `3f11bf89f3702261174d8efa1436d56a15c7d97a` re-clone `/tmp/bmdref` confirma Author `xulek <xulek121@gmail.com>` 2026-07-24, `package.json` author `xulek@example.com` placeholder, ISC sem LICENSE file. Evidência `ciclo6_bmdref_commit_date.log`.

## 2. O que foi implementado

- **Parser de ossos:** `src/mu/assets/bmd_body.hpp` com `struct BmdBone {string name; int parent; float raw[6]; bool has_name; bool floats_finite;}` + `declared_bone_count/action_count`, `vector<BmdBone> bones`, `has_bones`. Doc `bc*59-1` e `4+bc*59-1`.
- **Lógica parsing:** `src/mu/assets/bmd_body.cpp` reescrito com `TryParseBonesAt()` que valida nome ASCII NUL-padded/loose, parent, 6 floats finitos, pad. Single base `triangles_end+32`, multi base offset após último nome textura (busca delta -4/0/4/-8/8) +4 se `extra==ac`.
- **Relatório:** `src/mu/assets/bmd_report.hpp/cpp` estendidos com `has_bones`, `bone_count`, `bones_parsed`, `total_bones`. `tools/mutool.cpp` imprime LEVEL 5: `with bones 3800 (37.66%)`, `total bones 8001`, layout doc.
- **Checker:** `tools/check_docs_consistency.py` 53→56 verificações, novo check nível 5 `with bones 3800 (37.66%)`, 0 falhas.
- **Evidência:** `docs/evidence/ciclo8_bones_bmdreport.log`, `ciclo8_bones_measure.log`, `ciclo8_bones_layout.log`, `ciclo8_bones_multi.log`, `ciclo8_bones_single.log`, `ciclo8_bones_double.log`, `ciclo8_test.log`, `ciclo8_test_san.log`.

## 3. Problemas resolvidos

| Problema | Antes | Depois | Como |
|---|---|---|---|
| Nível 5 ossos 34,74% (3.505) estagnado desde ciclo 3 | 3.505 | **3.800 (37,66%) +295** | Parser multi-mesh com extra ac + critério loose >=3 printable |
| `bmd_report` não contava ossos | Level 4 83,30% último | Level 5 37,66% impresso | Extensão `BmdSummary` com `bones_parsed` |
| `bmd_body.cpp` não parseava multi-mesh bones | 0 multi | 3800 inclui multi | `TryParseBonesAt` com base offset+4 |
| `check_docs_consistency` sem nível 5 | 53 checks | 56 checks | Novo CHECK `with bones` |
| Documentação desatualizada ASSET_FORMATS | 73,7% parcial, 98% intercalado | Layout final 59B confirmado | Seção reescrita ciclo 8 com hex dumps |

## 4. Testes executados

| Comando | Resultado | Evidência |
|---|---|---|
| `make clean && make test` | **384 checks, 0 failures** — PASS | `ciclo8_test.log` |
| `make clean && make test-san` | **384 checks, 0 failures, ASan 0** — PASS | `ciclo8_test_san.log` |
| `mutool bmdreport` full-scan | exit 0, 10.090 arquivos — PASS | `ciclo8_bones_bmdreport.log` |
| `bone_measure2` full-scan | 8406 geom, 3800 bones — PASS | `ciclo8_bones_measure.log` |
| `check_docs_consistency.py` | **56 verificações, 0 falhas** — PASS | saída log |
| `grep glGenBuffers\|wglCreateContext\|GL/gl.h src/` | 1 comentário, 0 código — PASS | DEC-010 |
| Linkar OpenGL | BLOCKED — sem headers | — |
| Executar no Windows | NOT EXECUTED — sem acesso | roteiro em `TEST_STRATEGY.md` |
| Medir FPS/frametime/VRAM | NOT EXECUTED — sem GPU | — |

Estados de teste obrigatórios: PASS/FAIL/BLOCKED/NOT EXECUTED/NOT APPLICABLE sempre com evidência ou justificativa — atendido.

## 5. Métricas medidas

**Tabela final níveis BMD** (full-scan, `ciclo8_bones_bmdreport.log`):

| Nível | Arquivos | % de 10.090 |
|---|---|---|
| 1 — estrutura válida | 10.023 | 99,34% |
| 2 — decifrado (1.245+100+7.226) | 8.571 | 84,95% |
| 3 — vértice+normal | 8.406 | 83,31% |
| 4 — topologia | 8.405 | 83,30% |
| 5 — ossos | **3.800** | **37,66%** |
| 6 — UV | 0 | 0,00% |

- OPAQUE 1.519 (15,05%), total bones 8.001, parent root 5.018, parent válido 8.001 (100%)
- 45,21% dos arquivos com geometria têm ossos (3800/8406)
- Level 5 é subconjunto estrito de level 3, verificado
- Triângulos 3.681.664, bad corner refs 1.030.785 (9,333%)

**Ossos** (`bone_measure2`): total bmd 10.090, geom parsed 8.406, with bones 3.800 (45,21% de geom, 37,66% total), total bones 8.001, bone names valid 8.001 (100% loose), parent root 5.018, parent valid 8.001.

**Animação** (`bmdanim` histórico): 500 arquivos com bloco após ossos, 385 divisível por `12*boneCount` (77,00%) — não decodificada, BLOQUEADA.

**Terreno:** 76 arquivos, suavidade média 8,038 (94,74% suaves), 2 BMP, 2 prefixo maior — já documentado.

## 6. Comparação com baseline

**Baseline desempenho:** não há FPS/frametime/VRAM/latência — sem GPU/Windows, NOT EXECUTED. Única otimização com antes/depois medido continua OPT-001 CRC32 391,6→2.553,5 MB/s 5,03× ciclo 1.

**Comparação parsing:**

| Métrica | Ciclo 7 | Ciclo 8 | Delta |
|---|---|---|---|
| Nível 3 geom | 8.406 (83,31%) | 8.406 (83,31%) | 0 |
| Nível 4 topo | 8.405 (83,30%) | 8.405 (83,30%) | 0 |
| Nível 5 ossos | 3.505 (34,74%) | **3.800 (37,66%)** | **+295 (+2,92pp)** |
| Total bones | — | 8.001 | novo |
| Checks | 384 | 384 | 0 |
| Docs checks | 53 | 56 | +3 |

**Efeitos colaterais:** zero regressão nível 3/4, 0 falhas teste, 0 includes gráficos, ASan 0.

## 7. Arquivos alterados

- `src/mu/assets/bmd_body.hpp` — adiciona `BmdBone`, `declared_bone_count/action_count`, `bones/has_bones`, doc `bc*59-1` e `4+bc*59-1`
- `src/mu/assets/bmd_body.cpp` — reescrito com `TryParseBonesAt()`, single `triangles_end+32`, multi offset+4 extra ac
- `src/mu/assets/bmd_report.hpp` — adiciona `has_bones`, `bone_count`, `declared_bone_count/action_count`, `bones_parsed`, `total_bones`
- `src/mu/assets/bmd_report.cpp` — popula bone fields e acumula `bones_parsed`
- `tools/mutool.cpp` — imprime LEVEL 5, atualiza comentário multi-mesh+bones ciclo 8
- `tools/check_docs_consistency.py` — adiciona check nível 5 `with bones 3800 (37,66%)`, 53→56 verificações
- `docs/ASSET_FORMATS.md` — tabela nível 5 3.800/37,66%, seção ossos reescrita com layout final 59B e evidência ciclo 8
- `docs/COMPATIBILITY.md` — tabela nível 5 3.800/37,66%
- `docs/CURRENT_STATE.md` — insere seção ciclo 8 com métricas 3.800/37,66%, 8.001 bones
- `docs/NEXT_TASK.md` — ciclo 8 fechado, próxima tarefa UV/animação
- `docs/evidence/ciclo8_bones_bmdreport.log` — full-scan bmdreport
- `docs/evidence/ciclo8_bones_measure.log` — bone_measure2
- `docs/evidence/ciclo8_bones_layout.log` — dumps Rank02 bc1 remain58 Box01, Sapiatttres2 bc2 remain117, etc.
- `docs/evidence/ciclo8_bones_multi.log` — PrickleGlove multi mc4 bc1 ac1 remain62 extra1
- `docs/evidence/ciclo8_test.log` — 384 checks
- `docs/evidence/ciclo8_test_san.log` — 384 checks ASan

## 8. Commits/push

Commits deste ciclo (a preencher no final com `git log`):

- `bmd_body.hpp/cpp` layout final 59B + multi 4+bc*59-1 + TryParseBonesAt
- `bmd_report.hpp/cpp` + `mutool.cpp` nível 5
- `check_docs_consistency.py` + `ASSET_FORMATS.md` + `COMPATIBILITY.md` + `CURRENT_STATE.md` + `NEXT_TASK.md` + evidências ciclo 8
- `CICLO8_RELATORIO.md` final

Push: a confirmar com `git push origin arena/519f8fc9-clienterecriado`

## 9. Problemas restantes

- **Nível 6 UV = 0%** — 9 hipóteses descartadas ciclo 3, índice UV por canto não localizado, bloqueia textura. Investigação encerrada com critério parada, mas pode estar em outro arquivo (.o3d etc) — evidência negativa pendente.
- **Animação BLOQUEADA** — 500 arquivos com bloco extra, 385 divisível 77% por `12*boneCount`, 3 f32 por keyframe não decodificados, actionCount não verificado.
- **Ossos 62,34% sem parsing** — 8.406 geom - 3.800 bones = 4.606 arquivos com geometria sem ossos parseados (custom bc51 etc). Podem ser arquivos sem esqueleto ou com layout diferente.
- **Ordem dos 6 floats** — inferido xyz pos + xyz rot (1.57 rad ≈90°), não confirmado com modelo origem.
- **Backend gráfico BLOCKED** — sem GPU/contexto, DEC-010 trava `glGenBuffers` etc. Zero includes gráficos verificado.
- **OPAQUE 1.519 (15,05%)** — cifra não recuperada, bloqueia 15% da árvore.
- **Windows/GPU/FPS** — NOT EXECUTED, sem acesso, roteiro em `TEST_STRATEGY.md`.

## 10. Próxima tarefa e por quê

**Próxima tarefa: UV por canto e animação (nível 6 + skinning)**

Por que esta, e não outra:

- Nível 4 83,30% (8.405) praticamente igual nível 3 83,31% (8.406) — não há ganho fácil em topologia, multi-mesh 100% parseado.
- Nível 5 37,66% (3.800) com layout final confirmado ciclo 8 — próximo bloqueio para modelo renderizável texturizado e animado é nível 6 (0%) e animação (77% divisível mas não decodificada).
- Já temos vértices, normais, faces, ossos, texturas OZT/OZJ/TGA, altura terreno. Falta UV para textura e animação para skinning.
- É testável sem GPU: parsing puro CPU, evidência automática como ciclo 6-8.

Critério aceite:

1. `mutool bmdreport` nível 6 >0 ou animação decodificada com evidência bruta versionada, OU relatório conclusivo de que UV está em outro arquivo com evidência negativa
2. `make test` e `make test-san` limpos (384 checks)
3. `check_docs_consistency.py` 0 falhas (56 verificações)
4. Documentação UV/animação com evidência em `ASSET_FORMATS.md`
5. Zero regressão nível 3/4/5

Depois: backend VBO/VAO com skinning (BLOCKED GPU), escala vertical altura, Parte B DEC-008 benchmark real, baseline FPS/VRAM (BLOCKED GPU), validação visual Windows.

---

## ESTADO DO GIT NESTE CICLO

> Seção obrigatória ao final de CADA ciclo, com dados reais — regra permanente item 8.

- `git rev-parse HEAD` no início do ciclo: `1e9f74dbf4d87939bdac00c8331f5c05333bc4df` (fim ciclo 7)
- `git rev-parse HEAD` no fim do ciclo: `16f0e8332b7e598311cb4e83c355022fc8032e4b`
- Commits locais (hashes + mensagens):
  - `a5c3b9e` feat(assets): ossos layout final 59B bc*59-1 + multi 4+bc*59-1 com TryParseBonesAt
  - `6804040` feat(report): nivel 5 ossos no bmdreport, 3800 arquivos 37,66%
  - `1b7f6aa` docs: atualiza formatos e estado ciclo 8, nivel 5 3800 37,66% + checker 56
  - `d4a1261` evidence: ciclo 8 ossos nivel 5, 3800 arquivos, 8001 bones, 384 checks
  - `16f0e83` docs: ciclo 8 fechado, proxima tarefa UV/animacao nivel 6 + relatorio final 10 itens
- Push realizado (sim/não): **sim**
  - Comando: `git push origin arena/519f8fc9-clienterecriado`
  - Saída: `To https://github.com/heldergaraujo2/ClienteRecriado.git
   1e9f74d..16f0e83  arena/519f8fc9-clienterecriado -> arena/519f8fc9-clienterecriado`
  - Exit code 0
- Confirmação hash remoto == local (comando + saída):
  - `git rev-parse HEAD` → `16f0e8332b7e598311cb4e83c355022fc8032e4b`
  - `git ls-remote origin arena/519f8fc9-clienterecriado` → `16f0e8332b7e598311cb4e83c355022fc8032e4b	refs/heads/arena/519f8fc9-clienterecriado`
  - **Hash remoto == local: confirmado**
- `git status` final:
  - `git status --short` → vazio, working tree limpa (após commits e push)
- Motivo explícito se algo não foi commitado/publicado: **nenhum**, tudo commitado e publicado em 5 commits atômicos

**Referência `Cliente_ref` intacta:** `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`, idêntica ao início do ciclo, não modificada — verificado com `git -C /home/user/Cliente_ref rev-parse HEAD` no início e fim.
- Início ciclo 8: `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`
- Fim ciclo 8: `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9`

**Evidência bruta ciclo 8:**

- `docs/evidence/ciclo8_bones_bmdreport.log` — `mutool bmdreport` full-scan 10.090 arquivos, nível 5 3800 (37,66%)
- `docs/evidence/ciclo8_bones_measure.log` — `bone_measure2` 8406 geom, 3800 bones, 8001 total bones
- `docs/evidence/ciclo8_bones_layout.log` — dumps Rank02 bc1 remain58 Box01 parent FFFF floats 0,1e-06,0,1.57,0,0
- `docs/evidence/ciclo8_bones_multi.log` — PrickleGlove mc4 bc1 ac1 base 78670 remain62 extra1 bone PrickleGlo parent FFFF
- `docs/evidence/ciclo8_test.log` — 384 checks, 0 failures
- `docs/evidence/ciclo8_test_san.log` — 384 checks, 0 failures ASan 0
- `docs/evidence/ciclo6_bmdref_commit_date.log` — Author `xulek121@gmail.com` 2026-07-24, placeholder `example.com` em package.json
