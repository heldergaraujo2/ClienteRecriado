# Relatório de fim de ciclo 2

Base do ciclo: `12aa5d1` · Fim: `77662f5` · Branch `arena/519f8fc9-clienterecriado`
Referência `Cliente_ref`: `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` no início e
no fim — **idêntica, não modificada**.

Todos os números abaixo vêm de execução final, do zero, com o código consolidado.
Onde um número de execução intermediária foi supersedido, ele não aparece aqui.

---

## 1. O que foi pesquisado

**Cifra do payload BMD v0x0C.** Cinco abordagens de criptoanálise haviam falhado
no ciclo 1 (entropia, varredura XOR periódica, decifragem por claro conhecido,
modelos de bloco, frequência de palavras). Uma varredura exaustiva de chave XOR
de 4 bytes exigiria ~4,3 bilhões de decifragens — inviável neste ambiente. A
pesquisa então mudou de rota: buscou-se implementação open-source do formato.

**Registro de uma afirmação falsa deste projeto, corrigida neste ciclo.** Este
projeto afirmou repetidamente que essa rota estava bloqueada por falta de acesso
de saída a um host de código. **Era falso**: `github.com`, `codeload.github.com`
e `api.github.com` são acessíveis. A afirmação estava em `NEXT_TASK.md`, em
`docs/evidence/ciclo1_bmd_layout.log` e nas mensagens dos commits `d980216` e
`50a5ceb`. A rota foi então executada e resolveu o bloqueio nº 1.

**Layout do corpo BMD.** Investigado por medição sobre os arquivos reais, com
controle estatístico, e comparado com o projeto de terceiros consultado.

**Topologia (índices de face).** Retomada com método que as tentativas
anteriores não usaram: localização por aritmética de seções confirmada por
controle, em vez de busca ancorada em hipótese não verificada.

**Pipeline de renderização.** Reexaminado o fixed-function do cliente original à
luz do novo requisito de suportar conteúdo pesado.

## 2. O que foi implementado

| Item | Estado |
|---|---|
| `mu::BmdV0CDecrypt` — cifra v0x0C em C++ | **CONCLUÍDO** |
| `PayloadAccess::kV0CDecryptable` + verificação de plausibilidade | **CONCLUÍDO** |
| Ligação da decifragem ao `bmdreport` (níveis 2 e 3) | **CONCLUÍDO** |
| Teste da cifra com bytes calculados à mão | **CONCLUÍDO** |
| `tools/bmd_topology_probe.py` — gerador de evidência versionado | **CONCLUÍDO** |
| Documentos: DEC-007, DEC-008, DEC-009, limites do BMD, importação dual, escalabilidade | **CONCLUÍDO** |
| Leitura de triângulo, UV, ossos | **NÃO IMPLEMENTADO — bloqueado** |
| Pipeline VBO/VAO | **NÃO IMPLEMENTADO** — decisão registrada, código nenhum |

## 3. Problemas resolvidos

**Bloqueio nº 1, parte cifra — RESOLVIDO.** A cifra v0x0C é XOR rolante sobre
tabela de 16 bytes com chave corrente derivada do byte de **texto cifrado**
anterior:

```
plain[i] = (cipher[i] ^ key[i & 15]) - running
running  = cipher[i-1] + 0x3D        running inicial = 0x5E
key = D1 73 52 F6 D2 9A CB 27 3E AF 59 31 37 B3 E7 A2
```

Não é auto-inversa como o XOR-3 do bux. Proveniência: `xulek/muonline-bmd-viewer`
(ISC), reimplementada. Ver item 11 e `docs/LICENSING.md`.

**Três afirmações falsas deste projeto, corrigidas:**

1. A rota open-source "bloqueada por falta de acesso" — acesso existia.
2. A rejeição da hipótese (u,v) — o controle uniforme subestimava a linha de
   base. Registrada como **não confirmada**, não como confirmada: a fonte que a
   sustentava falhou em outro ponto.
3. O "defeito conhecido" de que v0x0C usaria `f16` — **infundado**, descartado
   por medição (400/400 com corpo completo em `f32`).

**Um erro cometido e desfeito neste ciclo.** Uma revisão trocou a ordem dos
campos nos registros de vértice e normal com base numa medição que reportava
"offset 4 vence em 1.050 arquivos, offset 0 em nenhum". A medição lia
deslocamentos de **arquivo** como se fossem de **payload**; os dois diferem pelos
4 bytes do cabeçalho `"BMD"`+versão. Todo número daquela revisão era artefato.
Revertido em `bf1668c`.

**Consequência que ficou registrada:** o layout de registro do projeto ISC **não
corresponde** a estes arquivos. A cifra foi confirmada de forma independente; as
tabelas de layout, não.

## 4. Testes executados

| Teste | Comando | Resultado | Estado |
|---|---|---|---|
| Suíte completa, reconstruída do zero | `make clean && make -j4 test` | **236 checks, 0 failures** | **PASS** |
| Warnings de compilação | 12 flags `-W` | **0 warnings** | **PASS** |
| Sanitizers | `make test-san` (ASan+UBSan) | **236 checks, 0 failures, 0 reports** | **PASS** |
| Base de payload por versão | `TestBmdProbe` (`:348`, `:366`) | 8 para v0x0C, 4 para v0x0A | **PASS** |
| Cifra: ida-e-volta e bytes à mão | `TestBmdV0CCipher` | 5 checks | **PASS** |
| Relatório contra árvore real | `mutool bmdreport` | saída completa anexada | **PASS** |
| Renderização de malha com asset original | — | topologia não decodificada | **BLOCKED** |
| Qualquer teste no Windows | — | sem Windows/GPU no ambiente | **NOT EXECUTED** |
| Skinning / animação | — | ossos não decodificados | **NOT APPLICABLE** |

Evidência bruta, completa e não resumida:

- [`evidence/ciclo2_test_final.log`](evidence/ciclo2_test_final.log) — `make clean && make test` do zero, 58 linhas
- [`evidence/ciclo2_scan_final.log`](evidence/ciclo2_scan_final.log) — relatório completo com o binário recém-compilado
- [`evidence/ciclo2_bmd_topology.log`](evidence/ciclo2_bmd_topology.log) — medição de topologia com controle
- [`evidence/ciclo2_bmdreport_v0c.log`](evidence/ciclo2_bmdreport_v0c.log), [`evidence/ciclo2_test.log`](evidence/ciclo2_test.log)

## 5. Métricas medidas

**Tabela final de três níveis** — execução final, 10.090 arquivos reais:

| Nível | Resultado | Percentual |
|---|---|---|
| 1 — estrutura de contêiner válida | 10.023 | 99,34% |
| 2 — payload obtível em claro | 8.571 | 84,95% |
| 3 — geometria parseável (vértice + normal) | 8.406 | 83,31% |
| 4 — topologia parseável (índices de face) | 4.149 | 41,12% |
| 5 — hierarquia de ossos decodificada | 901 de 919 | 98,0% |

Nível 2 detalhado: 1.245 plaintext + 100 bux + **7.226 v0x0C** = 8.571;
1.519 OPAQUE (15,05%).

**Medições de validação, cada uma com sua linha de base:**

| Medição | Resultado | Linha de base |
|---|---|---|
| Normais v0x0A com norma 1 (deslocamento 0) | 260.030/260.268 = **99,91%** | 32,51% no deslocamento 4 |
| Normais v0x0C com norma 1 (decifradas) | 37.988/37.997 = **99,98%** | 31,46% no deslocamento 4 |
| `u16` no offset 12 do vértice < 200 | **99,94%** | 2,63% no offset 0 |
| Índices de vértice em +2/+6/+10 do triângulo | 134.524/137.205 = **98,05%** | 2,35% controle limpo; 0,71% acaso |
| Nome de textura no ponto previsto | 515/586 = **87,9%** | — (confirmação independente) |
| Byte de polígono = 3 ou 4 | **3,01%** | indistinguível de aleatório → **refutado** |

**Não medido, por falta de condição:** FPS, frametime, VRAM, tempo de
carregamento, latência de entrada. Não há Windows nem GPU. **Não há baseline de
desempenho neste projeto**, portanto nenhuma comparação de desempenho é possível
e nenhuma meta numérica de draw call foi definida.

## 6. Comparação com o baseline

Baseline do ciclo 1, medido nas mesmas condições (mesmo conjunto de 10.090
arquivos, mesma ferramenta):

| Nível | Ciclo 1 | Ciclo 2 | Variação |
|---|---|---|---|
| 1 — estrutura válida | 10.023 (99,34%) | 10.023 (99,34%) | inalterado |
| 2 — payload em claro | 1.496 (14,83%) | **8.571 (84,95%)** | **+7.075 arquivos** |
| 3 — geometria parseável | 1.191 (11,80%) | **8.406 (83,31%)** | **+7.215 arquivos** |
| OPAQUE | 8.594 (85,17%) | **1.519 (15,05%)** | **−7.075 arquivos** |
| Checks de teste | 231 | **236** | +5 |

O teto teórico de 11,83% que constava no relatório do ciclo 1 **deixa de
existir**.

**Comparação de desempenho: NÃO POSSÍVEL.** Sem baseline de FPS/frametime/VRAM
nos dois ciclos, não há o que comparar. Declarar ganho de desempenho aqui seria
inventado.

## 7. Arquivos alterados

Código: `src/mu/assets/bmd.hpp`, `src/mu/assets/bmd.cpp`,
`src/mu/assets/bmd_report.hpp`, `src/mu/assets/bmd_report.cpp`,
`tests/test_assets.cpp`, `tools/mutool.cpp`.

Novos: `tools/bmd_topology_probe.py`.

Documentação: `docs/ASSET_FORMATS.md`, `docs/COMPATIBILITY.md`,
`docs/LICENSING.md`, `docs/DEPENDENCIES.md`, `docs/DECISIONS.md` (DEC-007/008/009),
`docs/ARCHITECTURE.md`, `docs/RENDERING.md`, `docs/CHANGELOG.md`,
`docs/CURRENT_STATE.md`, `docs/NEXT_TASK.md`.

Evidência: `docs/evidence/ciclo2_test_final.log`, `ciclo2_scan_final.log`,
`ciclo2_bmd_topology.log`, `ciclo2_bmdreport_v0c.log`, `ciclo2_test.log`.

## 8. Commits e push

Ver seção **ESTADO DO GIT NESTE CICLO** abaixo, com dados finais.

## 9. Problemas restantes

| Problema | Estado | O que falta |
|---|---|---|
| **Conteúdo do registro de triângulo (64 B)** | **BLOQUEADO** | 58 dos 64 bytes não decodificados. O byte de polígono esperado não está lá (3,01%, aleatório). As tabelas do projeto ISC não se aplicam |
| **Coordenadas UV** | **BLOQUEADO** | Hipótese (u,v) refutada por controle estatístico (razão 1,10×) |
| **Ossos e animação** | **BLOQUEADO** | Nenhum registro localizado |
| 1.519 arquivos v0x0C | **BLOQUEADO** | Verificação de plausibilidade rejeitou a decifragem; causa não determinada |
| Malhas múltiplas | **NÃO MEDIDO** | Toda a aritmética de topologia vale para malha única (586 arquivos) |
| Pipeline VBO/VAO | **NÃO INICIADO** | Decisão registrada, código nenhum |
| Desempenho | **NÃO EXECUTADO** | Sem Windows/GPU |

**Sem índices de face, nenhum modelo BMD pode ser renderizado como malha neste
momento — apenas dados de vértice/normal foram extraídos.**

## 10. Próxima tarefa e por quê

**Decodificar os 58 bytes restantes do registro de triângulo de 64 bytes.**

Por quê esta, e não o pipeline novo:

1. É o que separa 83,31% de "arquivos com vértices lidos" de "modelos
   renderizáveis". O array já está localizado com razão de 138,3× sobre o acaso —
   o problema deixou de ser *onde* e passou a ser *o que há dentro*.
2. É pré-requisito do requisito (a) do novo escopo: reaproveitar 100% dos assets
   originais. Sem topologia, o requisito é inatingível por qualquer quantidade
   de trabalho no pipeline.
3. Também destrava UV e LOD de assets originais, que dependem de topologia.

Teste concreto a executar: varrer os 64 bytes procurando três índices de vértice
adicionais além de +2/+6/+10, e um campo de índice de normal, usando o mesmo
controle limpo (região `f32`) que validou a posição. Rejeitar qualquer campo cuja
razão sobre o acaso fique abaixo de ~10×.

## 11. Licenciamento do código externo usado

| Campo | Valor |
|---|---|
| Repositório | `xulek/muonline-bmd-viewer` |
| URL | https://github.com/xulek/muonline-bmd-viewer |
| Autor | `xulek <xulek@example.com>` |
| Licença declarada | ISC (campo `license` do `package.json`) |
| Commit consultado | `3f11bf89f3702261174d8efa1436d56a15c7d97a` (24 jul 2026) |
| Arquivo | `src/crypto/file-cryptor.ts` |

**Declaração de proveniência.** A cifra v0x0C é **resultado de terceiros validado
por este projeto**, não resultado derivado por este projeto. As chaves derivadas
aqui continuam sendo o XOR-3 do bux e o layout do corpo v0x0A.

**Verificação de que nada foi copiado:** `git ls-files | grep -iE "\.ts$|bmd-viewer|file-cryptor"`
não retorna nada; `git show --stat 6821d94` lista apenas arquivos deste projeto;
o clone de consulta ficou em `/tmp`, fora do repositório, e não foi commitado.

**Ressalva registrada.** O repositório de origem **não tem arquivo `LICENSE`**; a
licença ISC consta apenas do `package.json`, e o e-mail do autor é um domínio de
exemplo. A cadeia de licenciamento não pode ser garantida por este projeto e deve
ser revista antes de qualquer distribuição.

---

## ESTADO DO GIT NESTE CICLO

Dados finais, pós todas as correções.

### Commits locais deste ciclo

**A lista canônica é o próprio Git, não este documento.** Execute:

```
git log --oneline 12aa5d1..HEAD
```

`12aa5d1` é a base do ciclo 2. O intervalo vai até o HEAD atual, então inclui o
commit que contém este relatório — que é exatamente o que uma lista copiada aqui
não consegue fazer.

**Por que este documento não enumera mais os commits.** Quatro versões
anteriores desta seção ficaram defasadas, sempre pela mesma razão: o commit que
atualiza a lista é posterior a ela, então a lista nasce incompleta. Contar
"quinze" ou "dezesseis" aqui é garantir que o número esteja errado no instante
seguinte. A causa não é descuido, é estrutural — e a correção é não duplicar um
dado que o Git já mantém.

O que este documento registra, e que o Git não registra sozinho, é **o que cada
unidade de trabalho fez**. Isso está nas mensagens de commit, que são
descritivas por regra permanente deste projeto.

Um commit não pode conter o próprio hash, então o hash de ponta **não se copia
deste documento**: lê-se com `git rev-parse HEAD` no momento da consulta.

### Push

**Realizado, sim.** `git push origin arena/519f8fc9-clienterecriado` após cada
commit. Nenhuma falha de push ocorreu neste ciclo.

### Confirmação hash remoto == local

```
$ git rev-parse HEAD
1090b6eb3673f7d56b241e249e070d4c3e7b5e36

$ git ls-remote origin arena/519f8fc9-clienterecriado
1090b6eb3673f7d56b241e249e070d4c3e7b5e36	refs/heads/arena/519f8fc9-clienterecriado
```

**Idênticos.** (Este par é um registro da verificação feita, não um valor para
reutilizar: o commit que o gravou é posterior a ele.)

### `git status` final

```
$ git status --short
(sem saída — working tree limpa)
```

### Repositório de referência

```
$ git -C /home/user/Cliente_ref rev-parse HEAD
88edfc73f5e1d63f806a51f21a118f84ba6c2bb9
```

Idêntico ao registrado no início do ciclo. **Nenhuma modificação no repositório
de pesquisa**, conforme a regra de separação inegociável.

### Algo não commitado ou não publicado?

**Não.** Todo o trabalho do ciclo está commitado e publicado. Não há alteração
pendente.
