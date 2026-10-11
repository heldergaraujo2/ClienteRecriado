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

> ### **BLOQUEADO — geometria BMD utilizável: 1.191 de 10.090 arquivos (11,80%)**
>
> **8.594 arquivos (85,17%) têm payload cifrado cuja cifra não foi recuperada.**
> Um parser do corpo v0x0A foi implementado no ciclo 1 e extrai **posições de
> vértice e normais** de 1.191 dos 1.194 arquivos v0x0A. Isso **não** é um modelo
> renderizável: **índices de face, coordenadas de textura e ossos continuam não
> decodificados**. Os 8.594 arquivos v0x0C (85,17%) seguem inteiramente opacos.
>
> Este é o bloqueio mais importante do projeto. Ele impede: renderização de
> personagens, monstros, itens, asas, objetos de mundo, terreno decorado e toda
> animação esquelética. **Nenhum trabalho de renderização de mundo ou entidades
> pode avançar antes dele.**
>
> Medido por `mutool bmdreport`; saída completa em
> [`evidence/ciclo1_bmdreport.log`](evidence/ciclo1_bmdreport.log).

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
| BMD modelo | 10.090 | 10.023 estrutura (99,34%) | **BLOQUEADO** — 0% de geometria; ver acima |
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
| Renderização de modelos | **BLOQUEADO** (cifra BMD) |
| Animação esquelética | **BLOQUEADO** (cifra BMD) |
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

