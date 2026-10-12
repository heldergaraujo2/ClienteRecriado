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
| Posições de vértice | **CONCLUÍDO E VALIDADO** | 8.406 arquivos (83,31%); `f32 x,y,z` no deslocamento 0 do registro, medido em 740.514 componentes |
| Normais | **CONCLUÍDO E VALIDADO** | 260.030 de 260.268 com norma 1 (99,91%) em v0x0A; 37.988 de 37.997 (99,98%) em v0x0C |
| Índices de face (topologia) | **CONCLUÍDO — implementado em C++** | 3 referências de 8 B em +0/+8/+16. `bmd_body.cpp` lê o array; `TestBmdTriangles` passa; `mutool bmdreport` nível 4 = 4.149 arquivos, 1.221.030 triângulos |
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
   montada e a malha texturizada. Oito hipóteses descartadas e documentadas.
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

### Tabela final de três níveis — execução final do ciclo 2

| Nível | Resultado | Percentual |
|---|---|---|
| 1 — estrutura de contêiner válida | 10.023 de 10.090 | 99,34% |
| 2 — payload obtível em claro | 8.571 de 10.090 | 84,95% |
| 3 — geometria parseável (vértices e normais) | 8.406 de 10.090 | 83,31% |

Detalhamento do nível 2: 1.245 plaintext + 100 bux + 7.226 v0x0C = 8.571;
1.519 OPAQUE.

O nível 3 **não** significa modelo renderizável. Ele conta arquivos dos quais
posições e normais foram extraídas. A distância entre 83,31% e um modelo
renderizável é exatamente o que está bloqueado na tabela acima.

### Por que um relatório anterior estava errado

Um relatório anterior afirmou "BMD: 10.066/10.090 lidos (99,76%)". Aquela
métrica contava apenas se o **cabeçalho do contêiner** podia ser classificado.
Ela não dizia nada sobre conteúdo utilizável, e a diferença entre as duas
respostas é de 99,76% contra 0,00%. A auditoria do ciclo 1 apontou isso
corretamente. As três métricas estão agora separadas de forma permanente, e
produzidas por ferramenta em vez de afirmadas em prosa.

### As três métricas, medidas na árvore completa

`mutool bmdreport /home/user/Cliente_ref/Data` — 10.090 arquivos `.bmd`:

| Nível | O que mede | Arquivos | % |
|---|---|---|---|
| **1. Estrutura válida** | magic + versão reconhecidos e, para v0x0C, o tamanho declarado confere com o tamanho real | **10.023** | **99,34%** |
| **2. Payload em claro** | bytes obtíveis sem cifra desconhecida | **1.496** | **14,83%** |
| ↳ plaintext | já está em claro | 1.396 | 13,84% |
| ↳ decifrável por bux | XOR-3 `{0xFC,0xCF,0xAB}` | 100 | 0,99% |
| ↳ **OPACO** | **cifra não recuperada** | **8.594** | **85,17%** |
| **3. Geometria extraída** | posições de vértice e normais realmente lidas | **1.191** | **11,80%** |

Entropia máxima de payload observada: **7,9980 bits/byte**; 8.570 arquivos acima
de 7,5.

### O que destravaria

O algoritmo da cifra, obtido legitimamente — por exemplo de uma implementação
open-source de cliente MU sob licença compatível, ou do autor do formato.
**Não** por desempacotamento do módulo protegido. Ver `AGENTS.md`.

### O que já foi possível sem destravar

Os arquivos com payload em claro permitiram implementar e verificar duas seções
do corpo v0x0A **sem esperar pela cifra**. Isso foi feito no ciclo 1: 1.191
arquivos agora têm posições de vértice e normais extraídas, com o layout
confirmado em 145 de 145 arquivos por aritmética exata. É um resultado real, mas
parcial — não habilita renderização.

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
| BMD modelo | 10.090 | 8.406 com vértice/normal (83,31%); 4.149 com topologia (41,12%) | **Malha montável; BLOQUEADO para renderização texturizada por UV** |
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

