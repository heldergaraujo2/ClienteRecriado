# Compatibility

Visual and functional compatibility status, classified on the project's
official scale:

| Classification | Meaning |
|---|---|
| **CONCLUÍDO E VALIDADO** | Implemented and verified against real data |
| **IMPLEMENTADO MAS NÃO VALIDADO** | Implemented, not yet verified against real data |
| **PARCIAL** | Works for a subset; the gap is measured and stated |
| **BLOQUEADO** | Cannot progress without something this project cannot produce |
| **NÃO IMPLEMENTADO** | Not started |

---

## BLOQUEIO Nº 1 DO PROJETO — geometria BMD

> ## **A topologia (índices de face) ESTÁ resolvida e implementada em C++.
> ## O que impede renderizar é UV, não topologia.**
>
> Este documento afirmava que a leitura de topologia "não está implementada em
> C++ — foi medida em Python como prova de conceito". **Essa afirmação estava
> errada e foi corrigida no ciclo 3.** O parser C++ lê triângulos desde o commit
> `bee2a5e`; há testes que passam; e `mutool bmdreport` reporta o nível 4 a
> partir do parser, não de script. A evidência está no item 1 do relatório de
> ciclo 3.
>
> O bloqueio real e único para renderizar um modelo completo é **UV**: o índice
> que liga cada canto do triângulo a uma entrada do array de coordenadas. Oito
> hipóteses foram testadas e descartadas.

### Estado por capacidade — execução final do ciclo 2

Medido por `mutool bmdreport` sobre os 10.090 arquivos reais, reconstruído do
zero (`make clean && make test`). Saída completa em
[`evidence/ciclo2_scan_final.log`](evidence/ciclo2_scan_final.log).

| Capacidade | Status | Evidência |
|---|---|---|
| Decifragem v0x0C | **CONCLUÍDO E VALIDADO** | 7.226 de 8.741 decifrados; validação independente: 400/400 amostrados com corpo completo e normais de norma 1 em 99,98% |
| Posições de vértice | **CONCLUÍDO E VALIDADO** | 8.401 arquivos (83,26%); `f32 x,y,z` no deslocamento 0 do registro, medido em 740.514 componentes |
| Normais | **CONCLUÍDO E VALIDADO** | 260.030 de 260.268 com norma 1 (99,91%) em v0x0A; 37.988 de 37.997 (99,98%) em v0x0C |
| Índices de face (topologia) | **CONCLUÍDO — implementado em C++** | 3 referências de 8 B em +0/+8/+16. `bmd_body.cpp` lê o array; `TestBmdTriangles` passa; `mutool bmdreport` nível 4 = 8.400 arquivos, 3.524.366 triângulos (ciclo 5) |
| Array de coordenadas UV | **CONCLUÍDO E VALIDADO** | `52+vc*16+nc*20`, `ntc` entradas de 8 B `[f32 u][f32 v]`. Zero não-finitos em 250.450 leituras (acaso daria ~976); 98,13% em [0,1] |
| **Índice de UV por canto do triângulo** | **BLOQUEADO — sem localização** | O candidato em +4 foi descartado (68,35–68,56% contra os três limites, indistinguível). Sem ele não há como ligar cada canto a uma coordenada |
| **Contagem de malhas** | **BLOQUEADO** | `u16@32` tem mediana 1 e máximo 18, coerente com contagem de malhas, mas não foi verificado um a um |
| Nomes de osso e tamanho do registro | **CONCLUÍDO — 98,0%** | Registro intercalado de 59 B (32 B nome + 27 B transformação); `boneCount` em payload 34. 901 de 919 arquivos cujo tamanho bate com `bc×59−1` |
| Hierarquia de ossos | **CONCLUÍDO** | `u16` pai em +0 do bloco de 27 B; 65535 = raiz (75,9%), resto em índices pequenos. 6 floats 100% finitos |
| **Ordem dos 6 floats de transformação** | **NÃO CONFIRMADO** | Padrão de dois grupos de três sugere posição + rotação, mas a ordem não foi verificada. Aplicar trocado daria esqueleto silenciosamente errado |
| Tamanho do registro de animação | **PARCIAL — 77,00%** | `boneCount × keyframes × 12 B`, com 12 B = 3 `f32`. 385 de 500 arquivos na árvore completa; keyframes por osso concentrados em 8, 12, 14 |
| **Conteúdo dos 3 floats de keyframe** | **BLOQUEADO** | Dois constantes e um variável num dump; leitura de exemplo, não resultado |
| **Animação** | **BLOQUEADO** | Depende de ossos, que estão bloqueados |
| 1.519 arquivos v0x0C | **BLOQUEADO** | A verificação de plausibilidade rejeitou a decifragem; causa não determinada |

### O que isso impede, concretamente

Com vértices, normais e índices de face, a malha **é** montada pelo parser C++ —
`MeshFromBmd` produz posições, normais, índices e caixa delimitadora, com testes.

**O que falta para renderizar um modelo completo e texturizado, em ordem:**

1. **Índice de UV por canto — BLOQUEADO.** É o único bloqueio entre a malha
   montada e a malha texturizada. **Nove** hipóteses descartadas e documentadas.
   Sem isso o modelo desenha **sem textura** (cor sólida), não desenha errado —
   `has_texcoords` fica `false` de propósito para impedir texturização com
   coordenadas de outro canto.
2. **Ordem dos 6 floats de transformação de osso — não confirmada.** Posição
   antes de rotação é inferência pelo padrão de frequência. Aplicar trocado
   daria esqueleto silenciosamente errado. Bloqueia skinning, não renderização
   estática.
3. **Conteúdo dos 3 floats de keyframe — não determinado.** O tamanho do
   registro é conhecido, o significado não. Bloqueia animação.
4. **Escala vertical da altura de terreno — não determinada.** Três caminhos
   fechados dentro do arquivo. Sem ela as proporções verticais são arbitrárias.

**Referências fora de faixa (~8,4%) já são tratadas**, não pendentes: o
importador descarta o triângulo e conta em `bad_indices`. Clamp moveria vértice
e deformaria a malha em silêncio.

Portanto:

- Renderização de modelo **sem textura**: **possível em código**, não executada
  — não há pipeline OpenGL (DEC-008 pendente) nem GPU neste ambiente.
- Renderização **texturizada**: **BLOQUEADA** por UV.
- Animação esquelética: **BLOQUEADA** por ordem dos floats e conteúdo de keyframe.
- Terreno: geometria legível, **BLOQUEADO** por escala e textura.
- Cálculo de caixa delimitadora, nuvem de pontos e análise de footprint:
  **POSSÍVEL** — usa apenas posições.

Um trabalho de renderização que precise de malha **não pode avançar** sobre os
assets originais. O que pode avançar é o pipeline de renderização em si, testado
com geometria sintética — desde que rotulado como teste de pipeline e nunca como
suporte a asset original.

### Tabela final de compatibilidade — seis níveis

Árvore completa, 10.090 arquivos `.bmd`, `mutool bmdreport`. Evidência bruta em
`docs/evidence/ciclo3_scan_final.log` e `docs/evidence/ciclo4_topology_subset.log`.

| Nível | O que mede | Arquivos | % de 10.090 |
|---|---|---|---|
| 1 — estrutura de contêiner válida | magic + versão reconhecidos; para v0x0C o tamanho declarado confere | 10.023 | 99,34% |
| 2 — payload obtível em claro | 1.245 plaintext + 100 bux + 7.226 v0x0C | 8.571 | 84,95% |
| 3 — vértice + normal | posições e normais realmente lidas pelo parser | **8.406** | **83,31%** |
| 4 — topologia (índices de face) | triângulos lidos | **8.405** | **83,30%** |
| 5 — ossos parseáveis | registros de osso localizáveis e legíveis | 3.505 | 34,74% |
| 6 — UV | índice ligando canto de triângulo a coordenada de textura | **0** | **0,00%** |

1.519 arquivos (15,05%) permanecem OPAQUE no nível 2.

#### Os níveis NÃO são cumulativos na mesma população

Esta é a ambiguidade que a auditoria do ciclo 3 pediu para eliminar, e a
resposta é: **os seis níveis são critérios independentes aplicados à mesma
população de 10.090 arquivos, não uma hierarquia em que cada linha é
subconjunto da anterior.** Cada linha responde a uma pergunta própria sobre o
mesmo arquivo, e o denominador é sempre 10.090.

Há **uma** relação de subconjunto entre linhas, e ela foi verificada em código e
por teste, não inferida:

- **O nível 4 é subconjunto estrito do nível 3.** Todo arquivo contado no nível
  4 também está no nível 3. Prova: em `src/mu/assets/bmd_report.cpp`,
  `triangle_count` só é atribuído dentro do ramo que faz `geometry_parsed = true`
  e vale 0 caso contrário; o acumulador só incrementa `topology_parsed` sob
  `if (r.geometry_parsed && r.triangle_count > 0)`. Verificado sobre a árvore
  inteira por `TestTopologyIsSubsetOfGeometry`.
- **Os níveis 3, 4 e 5 não formam cadeia.** O nível 5 (ossos) não exige
  `meshCount == 1`; um arquivo pode ter ossos sem topologia e vice-versa.
  Comparar 8.400 com 3.505 como se o segundo fosse subconjunto do primeiro é
  errado.

#### Por que o nível 4 caía 50,6% — e não cai mais (corrigido no ciclo 5)

No ciclo 4, a queda de 8.406 para 4.149 (4.257 arquivos) tinha **causa única e
total, medida no parser**: `meshCount != 1` em 100,00% dos casos (4.256 verdadeiro
multi-mesh `meshCount>1` + 1 zero-faces `meshCount==0` = 4.257). Ferramenta atual
`mutool bmdreport` confirma 4.149 meshCount 1, 1 meshCount 0, 4.256 meshCount>1.
Evidência `evidence/ciclo6_item1_bmdreport.log`.

| Causa | Arquivos | % dos 4.257 | Nota |
|---|---|---|---|
| multi-mesh (`meshCount>1` verdadeiro) | **4.256** | **99,98%** | população correta multi-mesh | <!-- historico -->
| declara zero faces (`meshCount==0`) | **1** | **0,02%** | `player.bmd` | <!-- historico -->
| total `meshCount !=1` | **4.257** | **100,00%** | 4.256+1 | <!-- historico -->
| array declarado não cabe no payload | 0 | 0,00% | | <!-- historico -->
| payload curto demais para os counts | 0 | 0,00% | | <!-- historico -->

**Reconciliação da divergência 4.257/4.251/4.256 (auditoria ciclo 6 item 1):**
- 4.257 = `meshCount!=1` no snapshot 8.406 (4.256 multi-mesh +1 zero), usado ciclo 4.
- 4.251 = `meshCount>1` no snapshot 8.401 ciclo 5 (regressão 5 arquivos ntri>10k), histórico.
- 4.256 = `meshCount>1` correto no snapshot final 8.406 (2.280+1.086+449+190+114+42+38+12+10+6+7+2+13+7=4.256).
  Ferramenta: `./build/mutool bmdreport /home/user/Cliente_ref/Data` → evidência
  `evidence/ciclo6_item1_bmdreport.log`. Erro anterior: contar 4.257 como multi-mesh puro
  sem separar zero-faces, e usar 4.251 de snapshot com regressão como se fosse atual.

**Ciclo 5 — correção.** O parser foi estendido para multi-mesh. O layout por
malha repete a mesma sequência do caso de malha única (evidência em
`evidence/ciclo5_bmd_multimesh.log`):

```
[14 B header: vc,nc,ntc,ntri,texIdx,u48,u50]
vc*16 vertices, nc*20 normais, ntc*8 texcoords, ntri*64 triângulos,
32 B nome de textura
```

Concatenado `meshCount` vezes. Estrutura de loop inspirada em
`xulek/muonline-bmd-viewer` (`bmd-loader.ts:348-422`, ISC), mas header 14 B vs
10 B e triângulo diferente. Evidência bruta em `evidence/ciclo5_bmd_multimesh.log`,
gerada por `mutool bmdreport` sem números digitados.

Resultado inicial ciclo 5 (antes da correção da regressão):

| Nível | Arquivos | % | Triângulos |
|---|---|---|---|
| 3 — vértice + normal | 8.401 | 83,26% | — | <!-- historico -->
| 4 — topologia | **8.400** | **83,25%** | **3.524.366** | <!-- historico -->

**Ciclo 6 — correção da regressão 8.406→8.401 (item 1 auditoria ciclo 5).**

Causa: limite fixo `>10000` rejeitava 5 arquivos reais com `ntri` 11.771–17.694
(`Monster865.bmd` `ntri=15576`, `Ozarugold.bmd` `12616`, `bann.bmd` `11771`).
Corrigido para verificação de overflow + limite `meshCount` 64 (observado máx 18).

Resultado após correção (evidência `ciclo6_bmd_multimesh.log` e
`ciclo6_level4_breakdown.log`):

| Nível | Arquivos | % | Triângulos |
|---|---|---|---|
| 3 — vértice + normal | **8.406** | **83,31%** | — |
| 4 — topologia | **8.405** | **83,30%** | **3.681.664** |

Aumento de **4.256 arquivos** no nível 4 (de 4.149 para 8.405). O único arquivo
de nível 3 sem topologia declara zero faces (`Player/player.bmd`, `meshCount=0`):

| Causa | Arquivos | % do 1 |
|---|---|---|
| multi-mesh (`meshCount != 1`) | 0 | 0,00% |
| declara zero faces | **1** | **100,00%** |
| array declarado não cabe no payload | 0 | 0,00% |
| payload curto demais para os counts | 0 | 0,00% |

Distribuição de `meshCount` sobre os 8.406 arquivos de nível 3 (evidência em
`ciclo6_bmd_multimesh.log`): 0 em 1 (0,01%), 1 em 4.149 (49,36%), 2 em 2.280
(27,12%), 3 em 1.086 (12,92%), 4 em 449 (5,34%), 5 em 190 (2,26%), 6 em 114
(1,36%), 7 em 42 (0,50%), 8 em 38 (0,45%), 9 em 12, 10 em 10, 11 em 6, 12 em 7,
13 em 2, 14 em 13, >15 em 7. Total multi-mesh 4.256 arquivos, todos parseados
100%.

**Validação estatística multi-mesh (item 3):** full-scan 4.256 arquivos, 12.590
malhas, parsing 100%. Nome estrito em `tri_end-4` 1.796/12.590=14,27% vs controle
0,0376% (razão 379×). Nome fraco 100% dos arquivos mas controle fraco 77,51%
(0,67×, não discrimina). Evidência em `tools/validate_multimesh.cpp` e
`evidence/ciclo6_multimesh_validation.log`.

**Contabilidade nível 4 falhando (item 2):** total 10.090, nível 4 ok 8.405,
falhando 1.685 = 1.483 OPAQUE (88,01%) + 67 inválidos (3,98%) + 134 geometria não
parseada (7,95%) + 1 zero faces (0,06%). Multi-mesh 0. Evidência gerada por
`tools/gen_level4_breakdown.py` em `evidence/ciclo6_level4_breakdown.log`.

Isso não destrava renderização: o nível 6 (UV) continua em zero.

#### Nenhum nível é modelo renderizável

O nível 5 conta arquivos com ossos localizáveis; o nível 4, com índices de face.
Nenhum dos dois implica o outro, e nenhum dos seis produz modelo renderizável,
porque **UV é zero**. A distância entre 99,34% de estrutura válida e um modelo
texturizado na tela é exatamente o bloqueio descrito no topo deste documento.

### Por que um relatório anterior estava errado

Um relatório anterior afirmou "BMD: 10.066/10.090 lidos (99,76%)". Aquela
métrica contava apenas se o **cabeçalho do contêiner** podia ser classificado.
Ela não dizia nada sobre conteúdo utilizável, e a diferença entre as duas
respostas é de 99,76% contra 0,00%. A auditoria do ciclo 1 apontou isso
corretamente. As três métricas estão agora separadas de forma permanente, e
produzidas por ferramenta em vez de afirmadas em prosa.

### Uma tabela antiga deste documento foi removida

Uma versão anterior trazia "payload em claro 1.496 (14,83%)" e "geometria <!-- historico -->
extraída 1.191 (11,80%)". Esses números são do ciclo 1, **anteriores à <!-- historico -->
recuperação da cifra v0x0C** (DEC-007), e contradiziam a tabela de seis níveis
acima. Foram removidos em vez de mantidos lado a lado: duas tabelas com
denominadores iguais e valores diferentes é exatamente a ambiguidade que a
auditoria do ciclo 3 pediu para eliminar. Os valores atuais são 8.571 (84,95%) e
8.401 (83,26%), e o salto entre eles é a decifragem v0x0C.

Entropia máxima de payload observada: **7,9980 bits/byte**.

### O que ainda destravaria os 1.519 opacos

O algoritmo da cifra restante, obtido legitimamente — por exemplo de uma
implementação open-source de cliente MU sob licença compatível, ou do autor do
formato. **Não** por desempacotamento do módulo protegido. Ver `AGENTS.md`.

### O que já foi possível

Os arquivos com payload em claro permitiram implementar e verificar as seções de
vértice, normal e topologia do corpo **sem esperar pela cifra restante**. O
layout de vértice e normal foi confirmado em 145 de 145 arquivos por aritmética
exata, e a topologia em 8.400 arquivos com 3.524.366 triângulos (após extensão
multi-mesh no ciclo 5). É resultado real, mas não habilita renderização: UV é
zero.

---

## 1. Compatibilidade de formatos de asset

Verificada contra a árvore completa — 29.060 arquivos, 2.372,04 MB — não contra
amostra.

| Formato | Arquivos | Lidos | Classificação |
|---|---|---|---|
| OZT textura | 3.470 | 3.404 (98,10%) | **CONCLUÍDO E VALIDADO** — pixels carregados e round-trip testado |
| TGA | 15 | 15 (100%) | **CONCLUÍDO E VALIDADO** — não comprimido 24/32 bits |
| `.att` terreno | 105 | 105 (100%) | **PARCIAL** — contêiner + grade; 97 cifrados |
| `.map` terreno | 79 | 79 (100%) | **PARCIAL** — contêiner + grade; 77 cifrados |
| `.obj` terreno | 81 | 81 (100%) | **PARCIAL** — contêiner; layout de registro não resolvido |
| OZJ textura | 13.928 | 13.832 (99,31%) | **PARCIAL** — cabeçalho e geometria validados; **pixels não decodificados**, não há decodificador JPEG |
| BMD modelo | 10.090 | 8.401 com vértice/normal (83,26%); 8.400 com topologia (83,25%) | **Malha montável; BLOQUEADO para renderização texturizada por UV** |
| `.ozb` | 100 | 0 | **NÃO IMPLEMENTADO** |
| `.smd` | 10 | 0 | **NÃO IMPLEMENTADO** (texto simples) |
| `.lua` | 22 | 0 | **NÃO IMPLEMENTADO** (texto simples) |
| `.cg` | 6 | 0 | **NÃO IMPLEMENTADO** (analisados, não compilados) |

### O que "lido" significa em cada caso

Para OZT e TGA, "pixels carregados" significa que os bytes são lidos, as
dimensões validadas e o payload round-tripa por um writer e de volta. **Não**
significa que a imagem foi exibida e comparada com a original — não há
renderer. Comparação visual: `NÃO EXECUTADO`.

Para OZJ, apenas o cabeçalho é validado. O maior formato do cliente (971,7 MB)
não tem pixels decodificáveis neste projeto.

## 2. Compatibilidade de renderização

O renderer original está totalmente caracterizado a partir das tabelas de
import ([`RENDERING.md`](RENDERING.md)): OpenGL 1.1 fixed-function, immediate
mode mais client-side vertex arrays mais display lists, fontes bitmap via GDI,
`glReadPixels` no caminho de apresentação, pós-processamento de glow em
Cg/`NV_texture_rectangle`.

**Nenhum renderer substituto existe. Todos os itens abaixo são
`NÃO IMPLEMENTADO`, exceto onde indicado:**

| Capacidade | Classificação |
|---|---|
| Janela e contexto GL | NÃO IMPLEMENTADO |
| Renderização de modelos | **BLOQUEADO** (topologia resolvida; falta UV, e não há pipeline OpenGL) |
| Animação esquelética | **BLOQUEADO** (ossos BMD não determinados) |
| Renderização de terreno | **BLOQUEADO** (cifra `Enc*`) |
| Upload de textura para GPU | NÃO IMPLEMENTADO |
| Partículas e efeitos de skill | NÃO IMPLEMENTADO |
| Interface | NÃO IMPLEMENTADO |
| Renderização de texto | NÃO IMPLEMENTADO |
| Pós-processamento de glow | NÃO IMPLEMENTADO (analisado) |
| Névoa, skybox, dia/noite | NÃO IMPLEMENTADO |
| Câmera | NÃO IMPLEMENTADO |

## 3. Compatibilidade de configuração

A superfície de configuração está documentada em
[`REFERENCE_AUDIT.md`](REFERENCE_AUDIT.md) seção 5. **NÃO IMPLEMENTADO** — não
existe leitor para `Settings.ini` nem `CESettings.ini`.

Resolução e modo de janela **não** estão nesses arquivos: ficam no registro,
gravados pelos 16 `.reg` em `Scripts/`.

## 4. Compatibilidade de rede e protocolo

`NÃO EXECUTADO`. Não há servidor de teste autorizado.

Conhecido da referência: 32 funções `WS2_32` incluindo `WSASend`; camada Lua em
`Data/Custom/Script/` com `System/ProtocolCore.lua` e pasta `TesteProtocol`,
indicando handlers de protocolo customizados em runtime. Versões em
`ListaPatch.txt`: `512.2011.006` e `512.2025.005` (linha Season 6).

## 5. Compatibilidade de áudio

**NÃO IMPLEMENTADO.** A referência toca 1.020 WAV (211,9 MB) e 61 MP3 + 3 OGG
(198 MB) via `wzAudio.dll` com `vorbisfile` e DirectSound. MP3 é o formato
dominante de música e exigiria decodificador próprio.

## 6. Compatibilidade de plataforma

| Alvo | Classificação |
|---|---|
| Windows x86 (32 bits) | **NÃO EXECUTADO** — nunca compilado. Este é o alvo real. |
| Windows x64 | NÃO APLICÁVEL — a referência é inteiramente 32 bits |
| Linux | CONCLUÍDO E VALIDADO para biblioteca e ferramentas. Não é alvo de distribuição. |

A lacuna de build Windows é significativa: **a plataforma-alvo nunca foi
compilada**. Ver [`BUILD.md`](BUILD.md).

## 7. Metodologia de comparação visual

Ainda não possível, pois não há renderer. Quando for: cena, câmera, resolução e
configurações idênticas nos dois clientes; comparação pixel a pixel com métrica
reportada; diferenças esperadas listadas **antes**; e **nunca modificar um asset
para esconder um bug do renderer**.

