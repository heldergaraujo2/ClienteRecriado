# Rendering pipeline analysis

Everything here is derived from the reference client's import tables, section
headers, configuration files and shader sources. Nothing is inferred from what a
MU Online client "usually" does. Where a claim cannot be established from the
evidence available, it says so.

## Cycle-17 audit correction — status and evidence boundary

The reference import-table analysis below is static evidence about the original
modules, not a runtime comparison. The implementation described in sections
12.7–12.8 is a CPU-only diagnostic rasterizer and does **not** satisfy the
product's first graphics milestone (Windows executable + real GPU device +
correctly interpreted original model).

Earlier cycle-14/15 descriptions such as “recognizable silhouette”, “correct
proportions”, or “no visual artifacts” were subjective/self-consistency claims
without an equivalent original-client render. They are withdrawn as claims of
visual compatibility. The saved PPMs and mathematical checks remain evidence
that the CPU code emits deterministic frames and handles its test geometry; they
do not prove semantic geometry, texture association, UVs, or fidelity.

Cycle 17 also corrected BMD index accounting: 1,030,785 out-of-range reference
fields (4.666% of six fields per triangle) are distinct from 513,304 triangles
dropped by mesh import (13.94%). See [`COMPATIBILITY.md`](COMPATIBILITY.md).
A real OZT was loaded explicitly alongside a different BMD in a diagnostic run;
no asset association or visual correctness was established. BMD UV/material
semantics remain unresolved.

## 1. The API is fixed-function OpenGL 1.1

`Main.exe` imports 51 functions from `OPENGL32.dll` and 2 from `GLU32.dll`.
`Main.dll` imports 37 more. `3DPlugin.dll`, `hinetworks.dll`, `CEffect.dll` and
`arcadia.dll` each import a small set. **No module imports `d3d9`, `d3d11`,
`d3d12` or `dxgi`.** The renderer is OpenGL, and only OpenGL.

Context creation is `wglCreateContext` / `wglMakeCurrent` on a GDI window whose
pixel format comes from `ChoosePixelFormat` / `SetPixelFormat`. Presentation is
GDI `SwapBuffers`.

## 2. Three submission paths, all pre-1999 vintage

The function sets divide cleanly, and the division is the central technical
finding of this document.

**`Main.exe` — immediate mode on the presentation path**

```
glBegin glEnd glVertex2f glVertex3f glVertex3fv glTexCoord2f
glColor3f glColor4f glColor3ub glColor4ub glNormal3f
glRotatef glTranslatef glScalef glLoadIdentity glPushMatrix glPopMatrix
```

Every vertex here crosses the API boundary as an individual call. `glColor3ub`
and `glColor4ub` in particular indicate per-vertex colour changes interleaved
with geometry.

**`Main.dll` — client-side vertex arrays and display lists**

```
glVertexPointer glColorPointer glTexCoordPointer glEnableClientState
glDisableClientState glDrawArrays
glGenLists glCallLists glListBase
```

Client-side arrays mean the driver must marshal vertex data out of application
memory on every `glDrawArrays`. Display lists are a GL 1.0 feature that modern
drivers frequently emulate rather than accelerate.

**`arcadia.dll` — direct texture upload**

```
glGenTextures glBindTexture glTexImage2D glTexSubImage2D glDeleteTextures
```

`glTexSubImage2D` here means partial texture updates, which is the right call
for animated textures and is worth preserving in any reconstruction.

## 3. What is absent, and why it matters

Absent from every import table in the client:

| Missing | Consequence |
|---|---|
| `glGenBuffers`, `glBufferData` | No vertex or index buffer objects. Vertex data lives in client memory and is re-marshalled per draw. |
| `glDrawElements` | No indexed drawing, so shared vertices are resubmitted rather than referenced. |
| `glGenFramebuffers`, `glFramebufferTexture2D` | No render targets. Any full-screen effect must read back through `glReadPixels`. |
| `wglSwapIntervalEXT`, `wglSwapInterval` | No explicit vsync control. Interval is whatever the driver defaults to. |
| `glGetStringi` | No extension enumeration through the modern entry point. |

There is no path from this renderer to instancing, batching by material, or
GPU-driven culling. That is not a criticism of code written for 2003 hardware;
it is the reason a performance gain requires new code rather than a patch.

### 3.1 Load-time cost and per-frame cost are different things

This distinction had to be made explicit, because an earlier revision of
`DEC-008` conflated them and the cycle-4 audit caught it.

**"Re-marshalled per draw" is the driver re-reading the array, not the
application rebuilding it.** In the `Main.dll` path a flat client-side vertex
array exists in application memory. The natural implementation builds it **once**,
when the model is loaded, and reuses it. Nothing in the import table says the
array is reconstructed each frame. So the index-to-flat expansion is a
**load-time** cost, and comparing it against the budget of a single frame was
wrong. The correct comparison is against the loading budget. See
`DECISIONS.md`, DEC-008-A §3.1.

**What does recur every frame is the submission volume.** With no
`glGenBuffers`, the vertex data lives in client memory and the driver marshals
it out on every draw. With no `glDrawElements`, shared vertices are resubmitted
rather than referenced, so the volume per frame is the **expanded** size. That
is an exact quantity, derived from these absent imports and independent of any
GPU:

| Per frame | Value |
|---|---|
| Expanded submission, no `glDrawElements` | **67.21 MiB per draw** |
| Corner sends | **2,936,559 per draw** |
| Unique vertices, if indexed | 1,792,108, uploaded once |
| Vertices resent unnecessarily | **63.9% of sends** |

Counts measured on the real tree by `mutool meshimport`: 1,792,108 vertices,
978,853 triangles across 4,071 meshes; sharing ratio 1.64 vertices per triangle
rather than 3.

**The cost of that volume cannot be measured in this environment.** It is driver
marshalling, bus transfer, and per-vertex call overhead — all of which need a GL
context. Recorded as `BLOCKED` in DEC-008-A §3.3 (Part B).

In the `Main.exe` immediate-mode path (`glBegin`/`glEnd`/`glVertex3f`) there is
no persisted array at all: each vertex crosses the API boundary as an individual
call, every frame. The per-frame cost there is structural, but it is call
overhead, and it is equally unmeasurable here.

## 4. `glReadPixels` on the presentation path

`Main.exe` imports `glReadPixels`. A readback forces the driver to finish all
queued work and flush the pipeline before the CPU can proceed, which is a
classic full stall.

**What this is used for is not established.** Plausible uses given the rest of
the import set are screenshot capture (`ScreenShots/` exists with 47 files),
minimap generation, or feeding the glow post-process. Determining which would
require reading the protected module, which is out of scope. Recorded as a
hypothesis, not a finding: **UNVERIFIED**.

## 5. Text rendering goes through GDI

`Main.dll` imports `wglUseFontBitmapsA`, and `Main.exe` imports `CreateFontA`,
`CreateDIBSection`, `CreateCompatibleDC`, `GetTextExtentPoint32W`, `TextOutW`.
`Settings.ini` has a matching `[Font]` section:

```ini
FontEnable = 1
FontName1..5 = "Tahoma"      FontSize1..5 = 13
FontNameMini = "Arial"       FontSizeMini = 11
```

So text is rasterised by GDI into bitmap glyphs and drawn as GL bitmaps. This
means glyph generation depends on the installed Windows font set, and text
rendering quality differs between machines. A reconstruction can do better with
a cached glyph atlas, which also removes a per-frame GDI dependency.

## 6. The glow post-process

Six Cg shaders under `Data/Effect/`:

```
glow_blur_fp.cg    glow_blur_vp.cg
glow_combine_fp.cg glow_combine_vp.cg
glow_darken_fp.cg  glow_darken_vp.cg
```

`glow_blur_fp.cg` in full:

```c
FragOut main(VertOut vin,
    uniform samplerRECT tex0 : TEXUNIT0,
    uniform samplerRECT tex1 : TEXUNIT1,
    uniform samplerRECT tex2 : TEXUNIT2,
    uniform samplerRECT tex3 : TEXUNIT3)
{
    float4 col1 = texRECT(tex0, vin.tex0) * 0.105f;
    float4 col2 = texRECT(tex1, vin.tex1) * 0.86f;
    float4 col3 = texRECT(tex2, vin.tex2) * 0.085f;
    float4 col4 = texRECT(tex3, vin.tex3) * 0.86f;
    float4 avg1 = (col1 + col2) / 2.0f;
    float4 avg2 = (col3 + col4) / 2.0f;
    fout.col = avg1 + avg2;
    return fout;
}
```

Observations:

- `samplerRECT` / `texRECT` are the `NV_texture_rectangle` extension. Rectangle
  textures use unnormalised coordinates and are removed from every modern API.
  A reconstruction must use `sampler2D` with normalised coordinates.
- Four taps with weights 0.105 / 0.86 / 0.085 / 0.86, then two pairwise averages.
  This is a cheap separable-ish blur, not a proper Gaussian. The weights are
  asymmetric, which will read as a slight directional bias.
- Three passes (blur, combine, darken) with vertex and fragment programs each.
  This is the `DisableGlowEffect` and `TronEffects` path.

Reproducing this exactly in GLSL is straightforward, and the weight constants are
known, so visual parity is achievable rather than approximate.

## 7. Timer-driven main loop

`Main.exe` imports `timeGetTime`, `timeBeginPeriod`, `timeEndPeriod`,
`timeGetDevCaps`, `timeSetEvent` and `timeKillEvent` from `WINMM`.

`timeBeginPeriod` raises the system timer resolution, which is the standard
precondition for a millisecond-granularity frame loop. `timeSetEvent` /
`timeKillEvent` indicate a multimedia timer callback.

This is a 1 ms-resolution timer loop. On a 60 Hz display a frame budget is
16.7 ms, so 1 ms quantisation costs up to 6% of a frame in timing error, and on
a 144 Hz display (6.9 ms budget) up to 14%. A reconstruction should use a
high-resolution monotonic clock — `QueryPerformanceCounter` on Windows, which
this client does not import.

## 8. An FPS control exists and is configurable

`strings -n 6 Main.dll | grep -i fps` returns exactly three matches:

```
DisableNewFPS
SwapBuffers
timeBeginPeriod
```

`DisableNewFPS` is a key in `Settings.ini`:

```ini
[Graphics]
DisableNewFPS = 0
```

So there are two distinct FPS-handling code paths and the client ships with the
"new" one enabled. **What either path does is not established**, because both
live in protected or non-instrumented code. Any claim about an artificial FPS
cap in this client would be a guess. Recorded as **NOT ESTABLISHED**.

## 9. Existing quality and performance switches

The client already exposes a substantial tuning surface. These are legitimate,
supported optimisation levers that require no modification of protected code.

`Settings.ini`:

```ini
[AntiLag]
DisableStaticEffect  = 0     ; static world effects
DisableDynamicEffect = 0     ; dynamic effects
DisableSkillEffect   = 0     ; skill effects
DisableGlowEffect    = 0     ; the glow post-process above
DisableWings         = 0     ; wing rendering
ReduceMemory         = 0     ; memory reduction mode

[Graphics]
Anisotropy    = 1            ; anisotropic filtering enabled
MaxAnisotropy = 32           ; requests up to 16x
Linear        = 1
Fog           = 1
TronEffects   = 1
MiniMap       = 1
DisableNewFPS = 0

[Other]
WideScreenMode = 1
Debug          = 0

[Configuracion]
SkyBox   = 1
DayNight = 1
```

`CESettings.ini` adds a second, independent set:

```ini
[RenderMesh]            CEffect, CMapRenderMesh, CEffectRenderMesh
[EffectStaticDynamic]   CMapEffect, CSetEffect
[Animation]             CAnimationItem, CAnimationWings
[Interface]             CEffectRenderMeshIngame, FontRenderBitmap
[MapEx]                 CMapEX
[Extra]                 CWingBright
[HotKeys]               IndexKeyx10, IndexKeyx100
```

`MaxAnisotropy = 32` is worth flagging: 16x anisotropic filtering is expensive
on the hardware generation this client targets, and it is the single
configuration value here most likely to trade measurable frame time for visual
detail that is invisible at gameplay camera distance. Testing that trade-off is
a concrete, low-risk task — see [`NEXT_TASK.md`](NEXT_TASK.md).

## 10. Resolution and window mode are registry-driven

`Scripts/` contains 16 `.reg` files that write these settings to the registry
rather than to a config file:

```
640 x 480    800 x 600    1024 x 768   1280 x 1024
1366 x 768   1440 x 900   1600x900     1680 x 1050   1920 x 1080
LANGUAGE ENGLISH / PORTUGUESE / SPANISH
SOUND ON / SOUND OFF
_ FULL SCREEN MODE / _ WINDOW MODE
```

Two consequences for compatibility work. First, changing resolution means
writing to the registry, which needs elevation on some Windows configurations
and is a poor user experience. Second, the enumerated list stops at 1920x1080 —
there is no preset for 2560x1440, 3440x1440 or any refresh rate above what the
display reports.

## 11. Conclusions for the reconstruction

Ordered by expected impact, all evidence-backed:

1. **Replace immediate mode and client-side arrays with VBO/IBO batching.** The
   import tables prove neither exists. This is the single largest structural
   difference available between the original renderer and a new one.
2. **Eliminate the `glReadPixels` dependency** by rendering full-screen effects
   to framebuffer objects. Requires knowing what the readback is for, which is
   currently unverified.
3. **Reimplement the glow pass in GLSL with `sampler2D`.** The weight constants
   are known, so parity is achievable.
4. **Use a high-resolution clock.** `QueryPerformanceCounter` instead of
   `timeGetTime`.
5. **Cache glyphs in an atlas** instead of GDI bitmap fonts.
6. **Keep `glTexSubImage2D` for animated textures.** The original does this
   correctly and a naive reimplementation that re-uploads whole textures would
   be a regression.

What cannot be done: modify the existing renderer. It is inside a
VMProtect-protected module.

---

## 12. Requisitos de escalabilidade (ciclo 2) — estratégia, ainda sem implementação

Adotado com o novo escopo de suportar conteúdo significativamente mais pesado
que o original. Ver `DECISIONS.md` (DEC-008).

**Nada desta seção está implementado ou medido.** Não há baseline de FPS,
frametime ou VRAM neste projeto: não há Windows nem GPU no ambiente. São
requisitos de arquitetura a validar por benchmark depois, não afirmações de
desempenho.

### 12.1 Frustum culling — obrigatório desde o início

Não é otimização tardia. É requisito de projeto, porque sem ele o custo de
desenho escala com o tamanho do mundo e não com o que é visível, e nenhum outro
item desta seção compensa isso.

- Caixa delimitadora calculada **na importação**, não no desenho.
- Cull por nó de cena antes de qualquer bind de estado, para que o custo de um
  objeto invisível seja uma comparação, não um bind descartado.
- Contagem de objetos descartados por culling exposta como métrica, para que a
  eficácia seja medida e não presumida.

### 12.2 LOD para malhas pesadas

- Cadeia de LOD por malha, com seleção por cobertura de tela, não por distância
  absoluta — distância absoluta se comporta mal com campo de visão variável.
- Transição com *dither* ou *morph* para evitar *popping*, que é o defeito
  visual que mais chama atenção em LOD mal feito.
- Para o caminho A: o BMD original **não** traz cadeia de LOD. Gerar LOD exige
  redução de malha, que por sua vez exige topologia — **portanto LOD para assets
  originais está bloqueado pelo mesmo bloqueio nº 1**. Registrado para não dar a
  entender que é só implementar.
- Para o caminho B: `MSFT_lod` em glTF, com geração na ferramenta de autoria.

### 12.3 Streaming e cache de textura

- Orçamento de VRAM declarado e respeitado, com política de expulsão por
  frequência de uso, não por ordem de chegada.
- Carregamento de mip sob demanda, para que o custo de uma textura grande seja
  proporcional ao que está visível.
- Para os assets originais: o inventário do ciclo 1 estima **2.893,80 MB** de
  footprint máximo teórico de textura (`w*h*4` somado). **Isso é estimativa
  teórica, não medição** — não havia GPU para medir VRAM real. Serve como
  argumento de ordem de grandeza para exigir orçamento explícito, não como
  número de consumo.

### 12.4 Orçamento de draw calls

**Meta numérica: não definida, e deliberadamente.** Este projeto não tem baseline
de FPS nem frametime em nenhuma configuração, portanto qualquer número escrito
agora seria inventado — e a regra permanente proíbe meta arbitrária antes de
linha de base.

O que fica definido é o **método**, para que a meta possa ser derivada depois:

1. Medir frametime com geometria sintética de complexidade crescente, no hardware
   de destino, com condições declaradas (resolução, driver, cena, número de
   iterações, desvio padrão, cache quente ou frio).
2. Identificar o ponto em que o frametime deixa de ser dominado por CPU e passa
   a ser dominado por GPU, ou vice-versa.
3. Definir o orçamento logo abaixo desse ponto, com folga para o pior caso
   observado, não para a mediana.

Até lá, o que se pode afirmar é qualitativo: draw call é caro por chamada, não
por triângulo, então o objetivo é **minimizar chamadas** com agrupamento por
material e por estado — não minimizar triângulos.

### 12.5 Rotulagem obrigatória

Todo teste desta seção feito com geometria sintética deve ser rotulado
**"teste sintético de pipeline"**, conforme `DECISIONS.md` (DEC-009). Nenhum
deles conta como validação de asset pesado nem como progresso no bloqueio nº 1.

### 12.6 Historical optimization plan (cycle 12; not a current runtime baseline)

**Cycle-17 qualification:** numeric summaries in this section are historical asset/import experiments. In particular, the 4,071-mesh / 67.21 MiB figures are from the earlier snapshot and must not be read as the current whole-tree import totals or as measured per-frame cost. The current reference counters are in `COMPATIBILITY.md`. The existing tool labels 7,226 payloads as v0x0C-decryptable, but `AGENTS.md` still marks that cipher BLOCKED; this is not authorization or an official format-compatibility claim.

**Original objective:** record measured inputs separately from estimates and hypotheses, not to claim runtime performance.

#### (a) LOD — Level of Detail

**Dados medidos ciclo12:**
- `Data/` 10.090 arquivos BMD, 8.406 com geometria parseada (tool /tmp/find_max_vert)
- Vértices por mesh: campo u16 teto 65.535, máximo observado 22.987 (Custom/Monster/Ozarugold.bmd), mediana não medida mas meshCount mediana 1
- Triângulos por mesh: máximo observado 39.193 geral, 15.390 com ossos (Staff36.bmd, Object40 etc)
- Malhas por modelo: máximo observado 30 geral (Object157/Object06.bmd), 9 com ossos (Object74/Object40.bmd)
- Bones: máximo 63 observado (Item/Stick32.bmd), limite prático 64, total 7.879 bones em 3.693 arquivos loose (ratio per file 1.846,28× definitivo vs ctrl 0,023795%)
- Importação original: 1.792.108 vértices únicos, 978.853 triângulos em 4.071 meshes, sharing ratio 1,64 vértices por triângulo vs 3 sem índice, 63,9% de reenvios desnecessários sem `glDrawElements`

**Plano LOD consistente:**
- Caminho A (BMD original): LOD bloqueado pelo bloqueio nº 1 (geometria cifrada 85%), gerar LOD exige topologia decodificada, não é só implementar — registrar bloqueio explícito
- Caminho B (glTF 2.0): `MSFT_lod` com geração na ferramenta de autoria, cadeia de LOD por mesh, seleção por cobertura de tela (screen coverage) não distância absoluta, transição dither/morph para evitar popping
- Métrica de validação: frametime vs cobertura de tela, contar triângulos antes/depois LOD, medir redução draw calls por LOD, condições hardware declaradas (resolução, driver, cena, iterações, desvio padrão, cache quente/frio)
- Implementação atual: frustum culling já reduz custo de desenho para o que é visível, LOD complementa reduzindo custo do que permanece visível

#### (b) Streaming de assets

**Dados medidos ciclo12:**
- Texturas: inventário ciclo1 soma teórica `w*h*4` = 2.893,80 MB footprint máximo teórico — **estimativa teórica, não medição** (não havia GPU para medir VRAM real), serve como argumento ordem de grandeza para exigir orçamento explícito, não como número consumo
- Historical BMD counts: 10,090 files, 8,406 geometry accepted by the parser, 3,693 loose-bone records, 1,186 strict. The tool labels 7,226 payloads v0x0C-decryptable, but `AGENTS.md` keeps the cipher **BLOCKED**; do not treat this as authorized/official compatibility.
- Buffers: `PackMeshForUpload` transforms the internal mesh to CPU-side arrays without GL, validates size mismatches, and keeps skipped vertices, dropped triangles, and invalid index references as separate metrics; bounds are computed by `ComputeBounds`.
- Skinning: PackSkeletonForUpload valida parent range, self-loop, cycle detection visited[64], fail safe

**Plano streaming consistente:**
- Orçamento VRAM declarado e respeitado, política expulsão por frequência uso (LRU/LFU) não ordem chegada
- Carregamento mip sob demanda, custo textura grande proporcional ao visível
- Streaming de malhas: carregar bounds primeiro (32B nome + bounds), depois geometria sob demanda quando IsVisible true, usar frustum culling para priorizar carregamento do visível
- For original assets, streaming depends on parsing supported payloads. The current tool's v0x0C label remains subject to the BLOCKED policy in `AGENTS.md`; do not describe that cipher as officially validated. Bounds are available only for models accepted by the current parser.
- Implementação atual D1: buffers CPU-side prontos para upload, sem GL headers, packing verificado byte a byte TestPackMeshForUpload 21 checks

#### (c) Redução de draw calls

**Dados medidos ciclo12:**
- Original: sem `glGenBuffers`/`glBufferData` (sem VBO), sem `glDrawElements` (sem indexed), sem `glGenFramebuffers` (sem FBO), `glBegin`/`glEnd` immediate mode Main.exe, client-side arrays Main.dll com `glDrawArrays` (re-marshal per draw)
- Per frame expandido sem `glDrawElements`: **67,21 MiB por draw** medido via `mutool meshimport` 1.792.108 vértices, 978.853 triângulos, 4.071 meshes
- Corner sends: **2.936.559 por draw**, unique vertices se indexado 1.792.108, vertices reenviados desnecessariamente **63,9%**
- Frustum culling: já implementado cull.hpp/cull.cpp, p-vertex test, conservative safe (kIntersect quando duvida), IsVisible true para invalid bounds fail safe, TestFrustumCull + TestFrustumCullRealBounds 13 checks, métrica objetos descartados por culling exposta
- Buffer-preparation code generates flat index arrays and separately counts dropped triangles and invalid references; this is CPU-side preparation, not a GPU upload.

**Plano redução draw calls consistente:**
- Substituir immediate mode e client-side arrays por VBO/IBO batching (DEC-008): PackMeshForUpload já prepara vertices flat e indices flat, pronto para `glBufferData` + `glDrawElements`
- Agrupar por material e estado: 1 textura por mesh observado (sbow06_R.jpg etc), cada mesh tem 1 nome textura 32B após triângulos, agrupar meshes por textura reduz bind calls
- Frustum culling antes de qualquer bind: custo objeto invisível = 1 comparação Bounds vs 6 planos, não bind descartado, métrica eficácia medida por contagem descartados
- Instancing para objetos repetidos (árvores, itens): não medido ainda, requer análise instâncias no mapa, registrar como hipótese até medição
- Eliminar `glReadPixels`: render full-screen effects para FBO, não readback, requer saber uso readback (screenshot vs minimap vs glow) — atualmente UNVERIFIED
- Método para definir orçamento: medir frametime com geometria sintética complexidade crescente no hardware destino, condições declaradas (resolução, driver, cena, iterações, desvio padrão, cache quente/frio), identificar ponto CPU vs GPU dominado, definir orçamento logo abaixo pior caso observado não mediana, sem meta arbitrária antes baseline (regra permanente)

**Rotulagem:** todo teste sintético de pipeline rotulado "teste sintético de pipeline" conforme DEC-009, não conta como validação asset pesado nem progresso bloqueio #1.


Todo teste desta seção feito com geometria sintética deve ser rotulado
**"teste sintético de pipeline"**, conforme `DECISIONS.md` (DEC-009). Nenhum
deles conta como validação de asset pesado nem como progresso no bloqueio nº 1.

### 12.7 Primeira prova visual — rasterizador software CPU-only (B — IMPLEMENTADO ciclo14)

**Objetivo ciclo14:** produzir PRIMEIRA imagem real de asset do jogo original sendo desenhado, usando SOMENTE CPU, sem qualquer dependência OpenGL/DirectX/GPU/Windows. Roda neste ambiente Linux atual.

**Implementação:**
- `src/mu/render/software_rasterizer.hpp/.cpp`: framebuffer em memória RGB resolução configurável (800x600), pipeline mínimo vértices (Mesh já carregado via parser BMD existente) → transformação por câmera simples (projeção perspectiva básica, posição/alvo configuráveis) → rasterização triângulo com z-buffer → sombreamento simples intensidade baseada dot(normal, luz direcional fixa) — não fisicamente correto, só visível
- Usa representação interna Mesh/Skeleton C1 ciclo12 como entrada, incluindo skinning D2 quando modelo tiver ossos (ApplySkinningLBS)
- Aplica culling D3 antes rasterizar: FrustumFromViewProjection + CullBox p-vertex conservative safe invalid->kIntersect fail safe
- Exporta framebuffer final como PPM binário P6 simples sem biblioteca externa
- CLI atual: `mutool render <bmd-file> <out.ppm> [width height] [--texture <file.ozt> | --checkerboard]`. It parses a supplied BMD and saves a CPU PPM; successful parsing/rasterization is not visual-compatibility validation.

**Modelos renderizados ciclo14 — 4 evidências:**

| File | Parsed vertices | Body triangles | Rasterized triangles in the CPU run | Scope |
|---|---:|---:|---:|---|
| `Object1/Beer01.bmd` | 48 | 30 | 6 | Cycle-14 diagnostic output; no visual-compatibility claim |
| `Object2/Object03.bmd` | 88 | 103 | 82 | Cycle-14 diagnostic output; no visual-compatibility claim |
| `Player/AmbitionShield.bmd` | 385 | 545 | 448 | Cycle-14 diagnostic output; no visual-compatibility claim |
| `Player/Axe11.bmd` | 1,364 | 2,665 | 528 | Cycle-14 diagnostic output; no visual-compatibility claim |

These are parser/rasterizer counters only. In particular, a triangle not
rasterized is not necessarily a format-level degenerate triangle; clipping,
invalid references, or rasterizer conditions may contribute. The counts do not
establish that the BMD body layout or apparent object identity is fully correct.

Evidência: `docs/evidence/ciclo14_first_render_Beer01.ppm` 800x600 1,4MB P6, `ciclo14_first_render_Object03.ppm`, `ciclo14_first_render_AmbitionShield.ppm`, `ciclo14_first_render_Axe11.ppm`

**Visual equivalence: NOT EXECUTED.** No equivalent reference-client render or
independent image comparison was performed. Earlier written descriptions such as
“recognizable”, “correct proportions”, and “no artifacts” are withdrawn as
compatibility claims. The PPMs and bounding-box checks test the CPU pipeline's
own output, not whether the parsed model matches the game.

**Known implementation limits:**
- No semantically verified BMD UVs or material/texture association.
- Lighting is a simple Lambert-like diagnostic, not the original lighting.
- Animation/skinning semantics are not established by the parsed bone records.
- Out-of-range BMD reference fields and triangles dropped by import are separate
  counters; current whole-tree values are in `COMPATIBILITY.md`.
- Back-face culling is disabled because winding has not been validated.
- Clipping is limited; triangles crossing the near plane are not fully clipped.
- No runtime performance measurement was made; this section reports no FPS or
  frame-time result.

**Unit tests:** The current suite covers framebuffer bounds/depth, camera
transforms, triangle rasterization, z-buffering, OZT pixel loading, and use of
mesh UVs when explicitly populated. The complete cycle-17 suite is
**539 checks, 0 failures**; see [`TEST_RESULTS.md`](TEST_RESULTS.md) for the
reproducible commands and current result.

**Status:**
- CPU rendering is **IMPLEMENTED as a diagnostic tool**; production GPU backend
  is **NOT IMPLEMENTED / NOT EXECUTED**.
- The CPU path exercises parsing → mesh conversion → optional skinning/culling →
  projection/rasterization, but does not validate original-client appearance.
- The GPU API remains to be selected by an on-target Windows prototype; see
  [`NEXT_TASK.md`](NEXT_TASK.md).


### 12.8 Cycles 16–17 — texture diagnostics, not validated material rendering

**Status:** CPU-only texture sampling is implemented for diagnostics. BMD UVs and
material association remain **unverified**, and a production GPU backend is
**NOT IMPLEMENTED / NOT EXECUTED**. This does not validate original-texture
rendering.

**Implementation:**
- `SoftwareTexture` stores BGRA8 pixels, supports a procedural checkerboard and
  an explicit OZT file load via `OztRead` and OZT-to-BGRA conversion. It does not
  use the OZJ decoder; OZJ pixels are not decoded by this path.
- `SoftwareRasterizer` can receive/clear a texture and sample it. Mesh UVs are
  used only when the mesh reports a verified mapping. Otherwise the renderer
  emits a diagnostic status and uses planar position-based coordinates; those
  coordinates are placeholders, not decoded BMD UVs.
- The current CLI is
  `mutool render <bmd-file> <out.ppm> [width height] [--texture <file.ozt> | --checkerboard]` (`--textured`/`-t` are legacy checkerboard aliases).
  Texture selection is explicit; the output filename does not enable texturing.
- A cycle-16 checkerboard test measured bright/dark pixel classes in a synthetic
  diagnostic render. That demonstrates that the software sampler changes the
  output as expected; it does not validate UVs, original texture association,
  or appearance.

**Cycle-17 real-file exercise:** `Axes06.ozt` (64×64, 32 bpp, 16,384-byte
payload, 26-byte trailer) was explicitly loaded with `Player/Axe11.bmd` (1,364
vertices, 2,665 triangles). The CPU render produced an 800×600 PPM with exit 0.
The OZT and BMD are separate assets and were not shown to be a matching pair;
the log reports planar diagnostic mapping because no BMD UVs were decoded.
Evidence: `evidence/ciclo17_ozt_real_asset_info.log`,
`evidence/ciclo17_ozt_real_asset_load.log`, and
`evidence/ciclo17_ozt_real_asset_status.log`. The generated PPM was temporary
and is not retained as an original-asset deliverable.

**Limits:**
- No semantic UV-per-corner mapping, material selection, or BMD-to-texture link
  is verified.
- No comparison against equivalent output from the original client was run.
- The CPU rasterizer is not the product GPU renderer and produces no GPU
  performance measurement. See `NEXT_TASK.md` for the required Windows/GPU
  milestone.

#### Historical cycle-16 checkerboard test (synthetic only)

The cycle-16 `validate_tex` run counted bright/dark classes in a software render
using a generated 256×256 checkerboard and planar position-based coordinates.
It found both expected intensity classes (Beer01: bright 35.5%, dark 56.9%;
plain render: 200 intensity values, textured checkerboard: 223). This confirms
only that the CPU sampler affected that synthetic render. It does **not** test
an original texture, BMD UV semantics, asset association, or visual fidelity.
Raw evidence remains in `evidence/ciclo16_textured_validation.log`; historical
PPM files are described there and are not a current product compatibility claim.
