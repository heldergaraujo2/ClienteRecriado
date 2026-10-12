# Decisions

Each entry records the decision, the alternatives considered, the evidence, and
what would make it worth revisiting.

---

## D-000 — Starting the implementation from scratch was the only option

**Decision.** The reconstruction was built from an empty repository.

**Evidence.** The state of `ClienteRecriado` before this work, verified with
`git show --stat 972fabc` and `git show 972fabc:README.md`:

```
commit 972fabcea1b0d4e80345f0f5abd287b46d819455
    Initial commit

 README.md | 1 +
 1 file changed, 1 insertion(+)
```

The complete literal content of that file was:

```
# ClienteRecriado
```

That is the entire prior state of the repository: one commit, one file, one
line, a heading and nothing else.

**Explicitly recorded, as required:** there was **no** pre-existing
`AGENTS.md`, `CURRENT_STATE.md`, `NEXT_TASK.md`, `DECISIONS.md`, no source code,
no build system, no tests and no documentation of any kind. There was no work by
another agent to preserve and no implementation to continue from. Starting this
front from scratch was therefore not a matter of convenience — there was nothing
to start from.

This is stated because "continue from existing work if there is any" is a
standing instruction, and a reader is entitled to see the check that established
there was none.

**Reference repository hash at the start of cycle 1**, recorded so that any
future claim about modifying the reference client can be checked:

```
$ cd Cliente_ref && git rev-parse HEAD
88edfc73f5e1d63f806a51f21a118f84ba6c2bb9
```

**Revisit when.** Never — this is a record of a past state, not an open choice.

---

## D-001 — Build an independent asset layer rather than patching the client

**Decision.** Read the original assets with new code; do not modify the client.

**Alternatives.**
- Patch or unpack the existing client. Rejected: `Main.exe` is VMProtect-protected
  and the render loop is inside it. Out of scope by policy and by practicality.
- Wrap the existing executable with a launcher. Rejected: it would not improve
  anything that matters, and presenting a wrapper as a reconstruction would be
  dishonest.
- Replace individual assets with optimised versions. Considered and deferred:
  promising, but it requires the formats to be readable first, which is what
  this decision delivers.

**Evidence.** `objdump -h Main.exe` shows `.vmp0` and `.vmp1` sections with
`.text`/`.rdata`/`.data` at zero on-disk size. Meanwhile no asset format in the
tree is protected, and 99.31% of OZJ, 98.10% of OZT, 99.76% of BMD and 100% of
terrain and TGA files now parse.

**Revisit when.** Never, unless the protection situation changes fundamentally.

---

## D-002 — C++17 with zero third-party dependencies

**Decision.** The library builds with a compiler and nothing else.

**Alternatives.**
- C. Rejected: `std::vector`, `std::string` and RAII remove a whole class of
  memory bug in code whose entire job is parsing hostile files.
- Rust. Considered seriously and rejected for this project: the Windows build
  path cannot be tested here, and adding a toolchain that has never been run on
  the target platform is a bigger risk than the language's memory guarantees
  offset.
- C++ with vcpkg/Conan. Rejected: a manifest that has never been executed on
  Windows is a way to ship a broken build.

**Evidence.** The build succeeds in a container with no `GL/gl.h`, no `zlib.h`,
no `libpng`, no `libjpeg`, no SDL and no package registry access.

**Cost.** No JPEG pixel decoder. Recorded as a gap in
[`DEPENDENCIES.md`](DEPENDENCIES.md).

**Revisit when.** A JPEG decoder is needed, at which point `stb_image.h` behind
a pluggable interface is the recommended addition — one header, public domain,
no build integration.

---

## D-003 — Plain `make` instead of CMake

**Decision.** A 60-line Makefile.

**Alternatives.** CMake, Meson, Bazel. All rejected because none is installed in
the development environment, and installing one means the build depends on a
network fetch that cannot be verified on Windows.

**Evidence.** `cmake` and `meson` are both absent from the environment. `make`
4.3 and `g++` 12.2 are present.

**Cost.** Less portable across exotic toolchains. Acceptable: the target is
Windows plus Linux development, both well served.

**Revisit when.** A Visual Studio solution or an IDE integration is needed. A
`CMakeLists.txt` can be added *alongside* the Makefile rather than replacing it.

---

## D-004 — Custom 40-line test harness instead of a framework

**Decision.** No test framework.

**Alternatives.** GoogleTest, Catch2, doctest. All require a download.

**Rationale.** The dependency-free property is worth more than parameterised
tests and matchers. The harness is `Check`, `CHECK`, `CHECK_EQ` and a table of
function pointers — 40 lines that do what is needed.

**Cost.** No test filtering, no XML output, no parameterised cases. Mitigated by
printing `[ RUN ]` / `[ OK ]` / `[ FAIL ]` per test, which is enough to see what
broke.

**Revisit when.** The suite passes a few hundred tests and filtering becomes a
genuine time cost.

---

## D-005 — Accept unusual OZT trailers rather than rejecting them

**Decision.** The OZT loader accepts any trailer up to 4096 bytes and reports
its length, rather than requiring 0 or 26.

**Evidence.** Measured over 400 files: trailer 26 bytes in 362, 0 bytes in 22,
30 bytes in 12, 34 bytes in 1. Strict rejection of anything but 0 or 26 would
fail roughly 3% of the asset base.

**Rationale.** The pixel payload is fully determined by the header, so trailing
metadata cannot corrupt the image. Compatibility with the existing asset base is
a stated project priority, and there was nothing to gain from strictness here.

**Guard.** A trailer above 4096 bytes is still rejected, so a corrupt length
field cannot pass as data.

**Revisit when.** If a trailer turns out to carry meaningful data — a mipmap
chain, say — it should be parsed rather than skipped.

---

## D-006 — Report the OZT `00 4B 6D 4B` variant as unsupported rather than guessing

**Decision.** 43 variant files are recognised and returned as `kUnsupported`
with a message naming the marker.

**Alternatives.** Pick a pixel offset that makes the file length arithmetic
close.

**Why not.** Three measurements on those files cannot all be satisfied by one
layout: image type reads 0 rather than 2, dimensions sit 4 bytes later than the
standard form, and the file is exactly 4 bytes shorter than header-plus-payload.
An offset that closes the arithmetic would overlap the bits-per-pixel field and
produce silently wrong pixels. A texture that loads and displays the wrong image
is worse than one that reports a clear error.

**Evidence.** `Rank01.OZT` is 24,358 bytes; 26 + 78*78*4 = 26,362.

**Revisit when.** The variant's layout is established from a second source —
another file that documents it, or the tool that produced these files.

---

## D-007 — Auto-detect bux encryption by measured gain, not by magic bytes

**Decision.** `BuxDecryptGain` measures how much decrypting raises the zero-byte
fraction; a gain above 0.05 means the file is bux-encrypted.

**Evidence.** `Data/gate.bmd` is bux-encrypted but contains no `BMD` magic once
decrypted — it opens with four repetitions of the key, i.e. twelve plaintext
zero bytes, and decrypts to a data blob. 100 such files exist. Keying detection
on the magic alone would have classified all of them as unknown.

**Guard.** The reverse direction is tested too: high-entropy noise shows no gain
and must not be classified as bux.

**Revisit when.** A bux-encrypted file is found whose plaintext has no zero
padding. None has been seen.

---

## D-008 — Require a decisive gain before applying bux to terrain

**Decision.** `TerrainTryBuxDecrypt` only accepts a decryption that improves
small-byte density by more than 0.10.

**Evidence.** Decrypting 60 `EncTerrain*` files with the bux key improved the
density of exactly 1. The `Enc*` family uses a different cipher, and applying
bux to it would silently corrupt data that a future loader might otherwise
handle correctly.

**Rationale.** A cipher applied to data it does not match still "succeeds" — XOR
always produces output. The only protection against silently corrupting a file
is to check that the result looks more like the expected plaintext.

---

## D-009 — `mu::Status` instead of exceptions or booleans

**Decision.** Every reader returns a status code plus a message.

**Rationale.** A boolean cannot distinguish "not an OZT" from "a truncated
OZT", and that distinction is the difference between a correct inventory and a
wrong one. Exceptions would be reasonable, but they add cost to a hot path that
runs over 29,000 files, and they make the "keep going past one bad file"
behaviour awkward.

**Consequence that matters.** Messages are written to be actionable. `mutool
info` prints all four readers' messages for an unrecognised file, which turns
triage from guesswork into reading.

---

## D-010 — Header-only probing on the inventory path

**Decision.** Separate `Probe` (validate, no copy) from `Read` (copy pixels).
Inventories use `Probe` only.

**Evidence.** `OztProbe` 0.0 µs median against `OztRead` 6.0 µs and
`OztToRgbaTopDown` 62.4 µs. `mutool scan` over 29,060 files and 2,372 MB takes
1.67 s.

**Consequence.** Whole-tree validation is cheap enough to run on every change,
which is what caught the OZJ defect that sampling missed.

---

## D-011 — No original asset bytes in the repository

**Decision.** Fixtures are generated at run time.

**Rationale.** Licensing, first: the assets are third-party intellectual
property. Practicality, second: the suite runs anywhere with no setup and no
large files in Git.

**Cost.** Real-asset validation needs the reference client present. Handled by
making it a separate, documented step (`mutool scan`) rather than part of
`make test`.

---

## D-012 — Record `NOT EXECUTED` explicitly rather than omitting

**Decision.** Every unrun test gets a line saying so, with its blocker.

**Rationale.** An absent line reads as "not relevant". In this project a great
many things are "not yet possible" — no Windows, no GPU, no server — and the
difference between those two states is the difference between an honest status
report and a misleading one.

---

## DEC-007 — Usar a cifra v0x0C de um projeto open-source em vez de continuar a derivá-la

**Data:** ciclo 2 · **Status:** aceita

**Problema.** A cifra do payload BMD v0x0C tornava 8.594 de 10.090 arquivos
opacos e era o bloqueio nº 1 do projeto. Cinco abordagens haviam sido tentadas e
todas falharam: entropia e estatística byte a byte, varredura de chaves XOR
periódicas, decifragem por arquivo claro conhecido, modelos de cifragem em
bloco e varredura por frequência de palavras. Uma varredura exaustiva de chaves
XOR de 4 bytes exigiria ~4,3 bilhões de decifragens — inviável neste ambiente.

**Decisão.** Adotar o algoritmo e a tabela de chave do projeto open-source
`xulek/muonline-bmd-viewer` (ISC), reimplementando-o em C++ em vez de copiar
código.

**Por quê.**

1. A restrição do usuário era **não contornar DRM nem enfraquecer validações de
   servidor**. Usar um formato de arquivo documentado publicamente não viola
   nenhuma das duas: não há verificação de licença envolvida e nenhum servidor é
   afetado.
2. A alternativa era não renderizar 85% dos modelos do cliente. A missão exige
   compatibilidade visual e funcional.
3. A licença ISC é permissiva e compatível com a MIT deste repositório, e exige
   apenas atribuição, que está registrada em três lugares.

**Consequência que precisa ser dita sempre que o resultado for citado.** Este é
**resultado de terceiros validado**, não resultado derivado por este projeto. O
relatório e o `CURRENT_STATE.md` dizem isso explicitamente. As chaves derivadas
aqui continuam sendo o XOR-3 do bux e o layout do corpo v0x0A.

**Alternativas consideradas.** Continuar a criptoanálise (inviável no orçamento
restante), reimplementar sem a tabela (impossível sem a chave) e deixar o formato
sem suporte (incompatível com a missão).

**O que foi validado por este projeto, e não apenas aceito.** A decifragem é
submetida a uma verificação de plausibilidade: o campo de nome de 32 bytes
precisa ser ASCII imprimível, e a decifragem é rejeitada caso contrário. Isso é
o que impede que uma chave errada reclassifique lixo como decifrado, e é o que
produz o número honesto de 7.226 arquivos decifrados em vez de 8.741.

---

## DEC-008 — Substituir o pipeline fixed-function por VBO/VAO como piso mínimo

**Data:** ciclo 2 · **Status:** aceita · **Depende de:** DEC-001 (fidelidade visual)

### Problema

O escopo passou a incluir, além de reaproveitar os assets originais, suportar
malhas e texturas **significativamente mais pesadas** que as originais sem
perder FPS, fluidez ou responsividade. O pipeline do cliente original não
suporta isso, e o motivo é estrutural, não de tuning.

O que foi medido no ciclo 1 sobre o cliente original (evidência em
`docs/RENDERING.md`): OpenGL **1.1 fixed-function**, com `glBegin`/`glEnd`,
display lists e vertex arrays do lado do cliente. Ausentes do binário:
`glGenBuffers`, `glDrawElements`, framebuffer objects, `wglSwapIntervalEXT`.

### Por que fixed-function é incompatível com malha pesada

1. **`glBegin`/`glEnd` envia um vértice por chamada de função.** Cada vértice
   atravessa o driver como uma chamada individual. Não há como o driver
   transferir em lote nem reutilizar entre quadros. O custo escala linearmente
   com a contagem de vértices **por quadro**, mesmo para geometria estática.

2. **Vertex array do lado do cliente reenvia os dados a cada desenho.** Sem
   buffer no lado do servidor, a memória de vértice é copiada a cada
   `glDrawArrays`. Para uma malha pesada desenhada todos os quadros, isso é
   tráfego de barramento contínuo que VBO elimina por construção.

3. **Display lists são estáticas e opacas.** Ajudam para geometria que nunca
   muda, mas não permitem instancing, atualização parcial, nem atributos por
   vértice além do conjunto fixo. Skin moderno e atributos customizados ficam
   fora.

4. **Sem `glDrawElements` no binário, não há índice.** Toda malha é enviada
   expandida. Uma malha com compartilhamento de vértices paga o custo de
   triplicar os dados — exatamente o oposto do que se quer para conteúdo
   pesado.

5. **Fixed-function não tem shader.** LOD por shader, skinning na GPU,
   decalque, PBR — nada disso é expressável. O limite não é de velocidade, é de
   capacidade de expressar o pipeline.

Nenhum desses pontos é contornável com otimização dentro de fixed-function. São
propriedades do modelo de programação.

### Decisão

Adotar **VBO + VAO com draw indexado** como piso mínimo do pipeline de
renderização da reconstrução, dentro da mesma stack OpenGL. Concretamente:

- Geometria residente em buffer do lado do servidor (`GL_ARRAY_BUFFER` /
  `GL_ELEMENT_ARRAY_BUFFER`), enviada uma vez e não a cada quadro.
- Estado de atributos em VAO, para troca de malha custar um bind.
- Draw indexado (`glDrawElements`), para malha com vértice compartilhado não
  pagar expansão.
- Shader mínimo substituindo a função fixa, habilitando skinning e LOD na GPU.

Isso é **mudança de backend dentro da mesma stack**, permitida pelo comando
mestre quando justificada: continua sendo OpenGL, não é troca de API.

### Custo e risco

| Opção | Custo | Risco | Veredicto |
|---|---|---|---|
| Manter fixed-function | zero | não atende ao requisito (b); teto de capacidade intransponível | rejeitada |
| **VBO/VAO em OpenGL (adotada)** | reescrever o caminho de desenho; exige GL 2.0+ no alvo | baixo: é o mesmo conjunto de conceitos, e o hardware que roda o cliente original roda GL 2.0+ | **adotada** |
| Trocar de API (Vulkan/D3D12) | reescrita completa do backend | alto: perde a compatibilidade com hardware antigo que é requisito do projeto | rejeitada |

A opção adotada é a de menor risco que atende ao requisito. Trocar de API daria
mais teto, mas sacrificaria a compatibilidade com o hardware que hoje roda o
cliente — e essa compatibilidade é parte do escopo.

### Compatibilidade com decisões já registradas

- **DEC-001 (fidelidade visual):** preservada. VBO/VAO muda *como* a geometria
  chega à GPU, não *o que* é desenhado. O caminho A (assets originais) produz a
  mesma malha.
- **Zero dependência de build (ver `DEPENDENCIES.md`):** preservada. VBO/VAO é
  OpenGL puro; não entra biblioteca.
- **Prioridades de desempenho do comando mestre:** atende à ordem — primeiro
  eliminar crash/corrupção, depois gargalo de FPS. Este é um gargalo de FPS e de
  tempo de carregamento.

### O que esta decisão NÃO diz

**Nada aqui está implementado ou validado em GPU.** Não há baseline de FPS,
frametime ou VRAM neste projeto, porque não há Windows nem GPU no ambiente.
Qualquer número de ganho só pode ser declarado depois de medido em condições
declaradas, conforme a regra permanente deste projeto.

E **não substitui o bloqueio nº 1.** Ver DEC-009 e o item 10 do comando: teste
de pipeline com geometria sintética é teste de pipeline, não suporte a asset
original.

---

## DEC-008-A — Comparação formal de custo e risco (completando DEC-008)

**Data:** ciclo 4 · **Status:** registrada · **Depende de:** DEC-008 ·
**Exigida por:** DEC-010, que travou todo código de backend até aqui

DEC-008 registrou a intenção. DEC-010 apontou corretamente que intenção não é
decisão, e listou quatro itens que faltavam. Esta seção os responde.

### 1. Comparação explícita: fixed-function/immediate mode contra VBO/VAO

Os quatro eixos que a auditoria pediu, cada um com o que se sabe e de onde vem:

| Eixo | Fixed-function / immediate mode (como o original) | VBO/VAO com draw indexado |
|---|---|---|
| **Capacidade de suportar malha pesada** | Teto estrutural. `glBegin`/`glEnd` envia um vértice por chamada; sem `glDrawElements` no binário, toda malha é submetida expandida. **Medido:** a expansão por quadro custa **20.502,7 µs** de marshalling de CPU para o volume desta árvore — **123,0% do orçamento inteiro de um quadro a 60 FPS**, e isso é só CPU, sem driver nem GPU | O volume único é enviado uma vez e fica residente. **Medido:** submissão indexada por quadro custa **935,7 µs** (**21,91×** menos) e **1.144.451 envios de vértice a menos por quadro (38,97%)** |
| **Número de draw calls** | Uma chamada por primitiva em immediate mode; display list agrupa mas é estática e opaca. Sem índice, não há como reduzir o volume dentro de uma chamada | Uma chamada por malha via `glDrawElements`, volume independente da contagem de vértice. Instancing disponível. **Contagem real de draw call em tempo de execução: BLOQUEADA** — exige contexto |
| **Compatibilidade com hardware e drivers modernos** | OpenGL 1.1 fixed-function está **deprecado** desde GL 3.1 e **removido do perfil core** desde GL 3.2. Drivers modernos mantêm caminho de compatibilidade, mas em camadas de tradução (ex.: D3D12/Vulkan por baixo no Windows), o que tipicamente custa mais, não menos. Hardware antigo que roda o cliente original roda GL 2.0+ | GL 2.0 como piso, disponível em todo hardware que hoje roda o cliente original. Não exige driver recente nem troca de API. **Desempenho em driver específico: BLOQUEADO** |
| **Complexidade de implementação** | Zero — é o que o original faz. Mas não atende ao requisito de conteúdo pesado, então o custo zero compra um teto intransponível | Moderada e conhecida: buffer + VAO + shader mínimo substituindo a função fixa. Mesmo conjunto de conceitos, sem biblioteca nova. Estimativa de esforço: **não medida**, e não será afirmada como prazo |

### 2. Custo e risco de cada alternativa, incluindo o risco de não medir

| Alternativa | Custo de implementação | Risco técnico | Risco de não conseguir medir | Veredicto |
|---|---|---|---|---|
| Manter fixed-function | zero | **alto**: teto de capacidade intransponível; caminho deprecado em drivers modernos | baixo, mas irrelevante — medir confirmaria o teto | **rejeitada** |
| **VBO/VAO em OpenGL (adotada)** | reescrever o caminho de desenho; shader mínimo | **baixo**: mesmos conceitos, mesma stack, sem dependência nova | **alto e declarado**: não há GPU nem contexto aqui, então a validação final não pode ocorrer neste ambiente | **adotada** |
| Trocar de API (Vulkan / D3D12) | reescrita completa do backend | **alto**: perde compatibilidade com hardware antigo, que é requisito do projeto | alto | **rejeitada** |

O risco de não medir é **alto na opção adotada e isso não a desqualifica** —
qualifica a afirmação que se pode fazer sobre ela. A alternativa de menor risco
de medição (manter fixed-function) é justamente a que não atende ao requisito.
Escolher por facilidade de medição seria otimizar a métrica errada.

### 3. O que será medido para decidir, e em que hardware

**Já medido neste ambiente** (evidência:
[`evidence/ciclo4_dec008_submission_cost.log`](evidence/ciclo4_dec008_submission_cost.log)):

| Medida | Valor | Desvio |
|---|---|---|
| Expansão indexado → flat, por quadro | 20.502,7 µs | 6,40% (entre execuções) |
| Submissão indexada, por quadro | 935,7 µs | 10,23% (entre execuções) |
| Upload único do vertex buffer | 3.647,9 µs | 5,59% (entre execuções) |
| Razão expandido/indexado | **21,91×** | — |
| Upload se paga em | **0,19 quadro** a 60 FPS | — |

Contagens reais da árvore (`mutool meshimport`): 1.792.108 vértices, 978.853
triângulos, 4.071 malhas. Razão de compartilhamento medida: 1,64 vértice por
triângulo em vez de 3.

**Critério objetivo de validação — em duas partes, porque uma delas está
bloqueada:**

- **PARTE A — CONCLUÍDA.** Custo de marshalling de CPU, medido sem contexto
  gráfico. Resultado acima. O que sustenta: o modelo expandido custa, em CPU,
  mais que o orçamento inteiro de um quadro a 60 FPS para este volume de
  geometria. Isso independe de driver e de GPU, porque é trabalho que a CPU faz
  antes de qualquer chamada.
- **PARTE B — BLOQUEADA.** Benchmark sintético comparando upload de N vértices
  via immediate mode contra VBO, em contexto real. **Não pode ser executado
  neste ambiente**: não há GPU, não há driver, não há `wglCreateContext`, e o
  ambiente não permite instalá-los (sem root, sem headers de GL). Estado:
  **BLOCKED**, com roteiro reproduzível em
  [`BENCHMARKS.md`](BENCHMARKS.md) e `scripts/benchmark_windows.ps1`.

**O que decidirá entre as opções quando a Parte B puder rodar:** o mesmo
critério da Parte A, medido em GPU — tempo de quadro e contagem de draw call
para volume de geometria declarado, com hardware, driver, resolução e estado de
cache registrados. Se a diferença medida for menor que a diferença de CPU já
estabelecida, a decisão é revisada, não defendida.

### 4. O que acontece com a compatibilidade visual em cada caso

| Alternativa | Efeito sobre DEC-001 (fidelidade visual) |
|---|---|
| Manter fixed-function | Preserva por construção — é o mesmo caminho. Mas o requisito de conteúdo pesado fica não atendido |
| **VBO/VAO (adotada)** | **Preservada em princípio**: muda *como* a geometria chega à GPU, não *o que* é desenhado. O caminho A produz a mesma malha. **Não verificada**: sem GPU não há como comparar pixel a pixel. Estado: `NOT EXECUTED` |
| Trocar de API | Preservada em princípio, com risco maior de diferença de arredondamento e de comportamento de driver |

A compatibilidade visual da opção adotada é **`NOT EXECUTED`, não
"preservada"**. Dizer "preservada" sem comparação medida seria exatamente o
tipo de afirmação que a regra permanente deste projeto proíbe.

### O que destrava agora

Com esta seção registrada, a trava de DEC-010 sobre código de backend deixa de
valer por ausência de decisão. **Permanece valendo por ausência de ambiente:**
escrever código de GL aqui seria não testável, e código não testável de backend
gráfico é o caminho mais curto para afirmar compatibilidade que não foi
verificada. A ordem correta é:

1. Caminho A completo (UV, multi-mesh) — não depende de GPU para ser escrito e testado
2. Backend VBO/VAO — escrito quando houver ambiente que o execute, ou com camada de abstração testável sem GL
3. Validação da Parte B — em Windows com GPU

---

## DEC-009 — Teste de pipeline sintético não é suporte a asset original

**Data:** ciclo 2 · **Status:** aceita · **Obriga:** toda documentação e relatório

### Problema

Adotar VBO/VAO (DEC-008) cria uma tentação de relatório: subir um cubo ou uma
esfera procedural, ver que desenha, e registrar "renderização funcionando" ou
"suporte a modelo pesado". Isso seria falso duas vezes — nem o asset é original,
nem o peso foi validado.

### Decisão

Testes do novo pipeline com geometria sintética ou placeholder são aceitáveis
**apenas como teste de unidade do pipeline**. Exemplo legítimo: verificar que um
VBO sobe, que um VAO binda, que um draw indexado produz os pixels esperados.

Esses testes devem ser rotulados, sempre e sem exceção, como
**"teste sintético de pipeline"**. Nunca como:

- "suporte a modelo pesado validado";
- "asset original funcionando";
- substituto do trabalho pendente em faces/UV/ossos do BMD.

### Por que a regra é permanente

O bloqueio nº 1 continua de pé: **sem índices de face decodificados com
confiança, nenhum modelo BMD pode ser renderizado como malha**. O ciclo 2
localizou o array de triângulos (98,05% de índices plausíveis contra 2,35% no
controle), mas o conteúdo interno do registro de 64 bytes **não** está
determinado — o byte de polígono esperado deu 3,01%, indistinguível de
aleatório.

Um pipeline novo funcionando não muda esse estado. Ele cria a capacidade de
desenhar; não cria o dado a desenhar. Confundir os dois faria o projeto parecer
mais avançado do que está, que é exatamente a falha que a auditoria do ciclo 1
corrigiu.

### Como verificar o cumprimento

Todo relatório que citar renderização deve dizer, na mesma frase, qual é a fonte
da geometria: asset original decodificado, ou sintética. Se não disser, o
relatório está incompleto.

---

## DEC-010 — Frustum culling implementado antes da decisão formal de pipeline

**Data:** ciclo 3 · **Status:** aceita, com registro retroativo · **Depende de:** DEC-008

### O que aconteceu, sem suavizar

`src/mu/render/cull.cpp` foi implementado **antes** de a decisão formal de
pipeline de renderização (DEC-008) ter sido registrada com a comparação de
custo/risco que este projeto exige. A ordem correta seria decidir o pipeline e
depois escrever o código que dele depende. Isso não foi feito.

Esta decisão existe para registrar o fato e estabelecer por que o código pode
permanecer, não para justificá-lo retroativamente como se a ordem estivesse certa.

### Por que o código pode permanecer

Frustum culling é **matemática pura em CPU, sem nenhuma dependência de backend
gráfico**. Verificável no arquivo:

- Não inclui nenhum header de GL, D3D ou Vulkan.
- Não chama nenhuma função de driver.
- Sua entrada é uma matriz 4×4 `float[16]` e uma caixa delimitadora; sua saída é
  um enum de três valores.

Nenhuma dessas três coisas muda conforme o pipeline escolhido seja VBO/VAO,
renderizador por software, ou outro. Um frustum é definido pela matriz
view-projection, e toda API gráfica que existe produz essa matriz. Descartar
geometria fora dele é anterior à pergunta "como desenho o que sobrou".

**Teste de que a afirmação é verdadeira e não conveniente:** se amanhã o projeto
decidir não usar OpenGL, `cull.cpp` compila e passa nos testes sem alteração.
Isso é verificável hoje — o módulo não está em nenhum caminho que toque GL, e
este ambiente não tem headers de GL, então ele já está sendo compilado e testado
sem eles.

### O que isto NÃO autoriza

**Nenhum código que inclua headers de OpenGL/D3D deve ser escrito até DEC-008
estar registrada com a comparação de custo/risco exigida.** Especificamente,
estão bloqueados até lá:

- `wglCreateContext`, `wglMakeCurrent` e qualquer criação de contexto
- `glGenBuffers`, `glBindBuffer`, `glBufferData`, `glGenVertexArrays`
- `glDrawElements`, `glDrawArrays`
- Shaders, FBOs, ou qualquer chamada que presuma uma versão de API

A distinção que torna culling aceitável e estes inaceitáveis: culling tem uma
resposta correta independente da API; uma chamada de GL **é** a escolha da API.

### Decisão formal de pipeline: REGISTRADA no ciclo 4

DEC-008 registrou a **intenção** (VBO/VAO como piso mínimo) sem a comparação de
custo/risco. Esta seção apontou os quatro itens que faltavam, e **DEC-008-A os
respondeu no ciclo 4**:

1. Custo de implementação de cada alternativa considerada — respondido
2. Risco de cada uma, incluindo o risco de não conseguir medir — respondido, e o
   risco de não medir é **alto na opção adotada**, declarado em vez de omitido
3. O que será medido para decidir, e em que hardware — respondido em duas
   partes: Parte A (CPU, **concluída e medida**) e Parte B (GPU, **BLOQUEADA**)
4. O que acontece com a compatibilidade visual — respondido, e o estado é
   **`NOT EXECUTED`**, não "preservada"

**A decisão está registrada.** O que continua impedindo código de backend não é
mais a ausência de decisão, e sim a ausência de ambiente que o execute: sem GPU
nem contexto, backend gráfico escrito aqui seria não testável, e backend não
testável é o caminho mais curto para afirmar compatibilidade não verificada.

### Como verificar o cumprimento

`grep -rn "glGenBuffers\|wglCreateContext\|glDrawElements\|GL/gl.h\|windows.h.*GL" src/`
deve retornar vazio enquanto DEC-008 estiver incompleta. Se retornar algo, o
código foi escrito antes da decisão e deve ser revertido ou a decisão completada.
