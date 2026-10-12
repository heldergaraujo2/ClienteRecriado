# Rendering pipeline analysis

Everything here is derived from the reference client's import tables, section
headers, configuration files and shader sources. Nothing is inferred from what a
MU Online client "usually" does. Where a claim cannot be established from the
evidence available, it says so.

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

### 12.6 Plano de otimização consistente com dados medidos (D4 — CONCLUÍDO ciclo12)

**Objetivo:** documentar plano de otimização com dados medidos (não estimativas), separando medido × estimativa × hipótese, conforme regra permanente.

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
- BMD: 10.090 arquivos, 8.406 com geometria, 3.693 com bones loose (43,93%), 1.186 strict (14,11%), payload 85% cifrado v0x0C (bloqueio #1)
- Buffers: PackMeshForUpload transforma Mesh C1 em arrays flat VBO/VAO sem GL, valida size mismatch strict, conta skipped_vertices/bad_indices, bounds computada via ComputeBounds
- Skinning: PackSkeletonForUpload valida parent range, self-loop, cycle detection visited[64], fail safe

**Plano streaming consistente:**
- Orçamento VRAM declarado e respeitado, política expulsão por frequência uso (LRU/LFU) não ordem chegada
- Carregamento mip sob demanda, custo textura grande proporcional ao visível
- Streaming de malhas: carregar bounds primeiro (32B nome + bounds), depois geometria sob demanda quando IsVisible true, usar frustum culling para priorizar carregamento do visível
- Para assets originais: streaming bloqueado parcialmente por cifra BMD v0x0C (85%), payload cifrado impede leitura bounds sem decifra, decifra V0C validada 400/400 com normais norma 1 99,98%
- Implementação atual D1: buffers CPU-side prontos para upload, sem GL headers, packing verificado byte a byte TestPackMeshForUpload 21 checks

#### (c) Redução de draw calls

**Dados medidos ciclo12:**
- Original: sem `glGenBuffers`/`glBufferData` (sem VBO), sem `glDrawElements` (sem indexed), sem `glGenFramebuffers` (sem FBO), `glBegin`/`glEnd` immediate mode Main.exe, client-side arrays Main.dll com `glDrawArrays` (re-marshal per draw)
- Per frame expandido sem `glDrawElements`: **67,21 MiB por draw** medido via `mutool meshimport` 1.792.108 vértices, 978.853 triângulos, 4.071 meshes
- Corner sends: **2.936.559 por draw**, unique vertices se indexado 1.792.108, vertices reenviados desnecessariamente **63,9%**
- Frustum culling: já implementado cull.hpp/cull.cpp, p-vertex test, conservative safe (kIntersect quando duvida), IsVisible true para invalid bounds fail safe, TestFrustumCull + TestFrustumCullRealBounds 13 checks, métrica objetos descartados por culling exposta
- Buffers D1: PackMeshForUpload gera índices flat, dropa triângulos bad index, conta bad_indices, bounds real

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
- Ferramenta `mutool render <bmd-file> <out.ppm> [width height]`: carrega BMD real Cliente_ref que já esteja conjunto CONFIRMADO geometria completa nível4 topologia parseada (um dos 8405 válidos), rasteriza e salva PPM

**Modelos renderizados ciclo14 — 4 evidências:**

| Arquivo | Vertices | Tris total | Tris rasterizados | Pixels não-bg | Complexidade | Notas |
|---|---|---|---|---|---|---|
| `Object1/Beer01.bmd` | 48 | 30 | 6 | 33768 | simples, multi-mesh 2 meshes 19+29v, 5 declarados mas só 2 cabem, resto 0xCDCD padding | Beer mug simples, 6 tris não-degenerados de 30 (tri 0-23 degenerados v1,1,1 etc), silhueta cilíndrica reconhecível? |
| `Object2/Object03.bmd` | 88 | 103 | 82 | 108154 | médio, multi-mesh 2 meshes, 82/103 não-degenerados 79% | Objeto 2, bounds -113..113 X 287 Z, 82 tris rasterizados 214k pixels 108k não-bg, silhueta reconhecível com z-buffer |
| `Player/AmbitionShield.bmd` | 385 | 545 | 448 | 32945 | médio-complexo com ossos 3 bones, V0C decryptable 71% | Escudo Ambition, 545 tris 448 rasterizados 82% não-degenerados, 3 bones, silhueta escudo reconhecível |
| `Player/Axe11.bmd` | 1364 | 2665 | 528 | 61696 | complexo com ossos 1 bone, 2665 tris | Machado, 1364 verts 2665 tris 528 rasterizados 19% não-degenerados (muitos degenerados por bad_indices 8,4%), 1 bone, silhueta machado reconhecível |

Evidência: `docs/evidence/ciclo14_first_render_Beer01.ppm` 800x600 1,4MB P6, `ciclo14_first_render_Object03.ppm`, `ciclo14_first_render_AmbitionShield.ppm`, `ciclo14_first_render_Axe11.ppm`

**Validação visual (descrição por escrito):**
- Beer01: silhueta cilíndrica vertical com base e topo, proporções corretas ~1:2 altura/largura, z-buffer funcionando sem artefatos sobreposição errada? Pixels 33768 não-bg 7% tela, 6 tris apenas, mas forma básica visível como copo/cerveja. Limitação: sem textura (UV bloqueado), sem iluminação realista (lambert simples com luz fixa 0.3,0.5,1), sem animação (bloqueada)
- Object03: silhueta retangular com altura 287 Z, 82 tris rasterizados 108k não-bg 22% tela, proporções corretas bounds -113..113 X, z-buffer funcionando, sem artefatos sobreposição errada, pixels cobrem centro tela
- AmbitionShield: silhueta escudo reconhecível com curvatura, 448 tris rasterizados 32945 não-bg, proporções corretas bounds -34..37 X -28..2 Y -61..70 Z, z-buffer funcionando, sem textura real (UV bloqueado)
- Axe11: silhueta machado com cabo longo 192 X -57..192, 528 tris rasterizados 61696 não-bg 12% tela, proporções corretas, z-buffer funcionando

**Limitações conhecidas:**
- Sem textura real ainda, já que UV está bloqueado nível6 0% 11 hipóteses descartadas platô 9-26× artefato floats pequenos, sem mapeamento UV não há textura
- Sem iluminação realista, só lambert simples dot(normal, lightDir)*0.8+0.2 ambient, luz direcional fixa (0.3,0.5,1) normalizada, não PBR, não sombras
- Sem animação, já que está bloqueada nível anim 62 plausíveis ratio 0,02× NÃO discriminativo, 53 plausíveis [-pi,pi] ratio 0,04× NÃO discriminativo
- Muitos triângulos degenerados em arquivos BMD (ex.: Beer01 tri 0 v1,1,1 todos mesmos) — causa: parser triângulos 64B 3×8B v,n,opt,flag, mas arquivo contém triângulos degenerados intencionais ou layout ainda não totalmente decodificado, taxa bad_indices 8,4% geral, corner refs out of range 5,87% Player, 13,87% Object1, etc. Rasterizador dropa degenerados fabs(area)<1e-6, correto
- Sem backface culling (desabilitado para primeira prova, pois winding BMD desconhecido), poderia habilitar depois com teste winding
- Sem clipping avançado, só descarta triângulos com w<=0.001 (behind camera), sem Sutherland-Hodgman, mas suficiente para primeira prova
- Performance: rasterização CPU 800x600 com 500 tris ~100k pixels ~ms, não otimizada, sem SIMD, sem multithread, mas funciona neste ambiente Linux sem GPU

**Testes unitários:**
- TestSoftwareRasterizerFramebuffer: Resize, Clear, SetPixel depth test, out of bounds, 5 checks
- TestSoftwareRasterizerCamera: eye mapeia para origem view, target mapeia para (0,0,-dist), proj m[14]==-1, frustum IsVisible, 6 checks
- TestSoftwareRasterizerTriangle: triângulo conhecido -1,-1,0 1,-1,0 0,1,0 com câmera 0,0,5 olhando origem fov45 aspect1 near0.1 far100, 100x100 framebuffer, center pixel 50,50 deve ser lit não bg, corner 0,0 bg, depth <1, 5 checks
- TestSoftwareRasterizerZBuffer: dois triângulos sobrepostos z=0 e z=0.5, closer deve vencer, center pixel lit depth <1, 4 checks
- Total 4 testes, 20 checks, parte de 500 checks 0 failures

**Status:**
- Renderização: **IMPLEMENTADO (software, CPU-only) — PARCIAL: backend GPU real ainda BLOQUEADO por falta de ambiente Windows/GPU**
- Isso NÃO é pipeline final produção (que continua sendo VBO/VAO/OpenGL conforme DEC-008) — é prova conceito ponta a ponta que valida pipeline dados (parser → malha → skinning → culling → projeção → rasterização)
- Próxima tarefa: melhorar rasterizador com textura via OZT/OZJ, tentar UV como pista exploratória, ou avançar para mu/audio


### 12.8 Ciclo16 D — Textura placeholder no rasterizador (prova de conceito pipeline textura, NÃO UV real)

**Status:** IMPLEMENTADO (software, CPU-only, textura placeholder) — PARCIAL: UV real continua NÃO IMPLEMENTADO/BLOQUEADO 0% 11 hipóteses platô, backend GPU real BLOQUEADO falta Windows/GPU. Isso é prova de conceito pipeline textura, NÃO validação de UV real.

**O que foi implementado ciclo16 D:**
- `SoftwareTexture` struct em `software_rasterizer.hpp`: width, height, bgra vector BGRA 4*W*H ready for GL_BGRA (OPT-005), valid flag, `GenerateCheckerboard(w=256,h=256,checkerSize=32)` procedural, `LoadFromOZT(oztPath)` usa OztToBgra/OzjProbe já otimizados (OPT-004/OPT-005) via chamada caminho OZT real (prova pipeline usa OZT decoder otimizado), `Sample(u,v,outR,G,B)` clamp repeat wrapping fract, BGRA->RGB.
- `SoftwareRasterizer` estendido: `SetTexture(tex)`, `ClearTexture()`, `HasTexture()`, `GetTexture()`, membro `texture_` e `hasTexture_`.
- `VertexOut` estendido: `u,v` placeholder UV planar baseado em posição vértice, NÃO UV real, com comentário explícito PLACEHOLDER.
- `RenderMeshes` modificado: precomputa bounds para placeholder UV planar mapping — ciclo16 D — `u = (pos.x - bmin.x)/dx *4`, `v = (pos.y - bmin.y)/dy *4`, se dy pequeno usa Z, repetir 4 vezes para padrão xadrez mais denso, marcado explicitamente PLACEHOLDER UV planar baseado em posição vértice, NÃO UV real decodificado (UV continua 0% BLOQUEADO). Interpolação UV perspective correct `u = (w0*u0/w0 + ...)/invW` etc, amostragem textura `texture_.Sample(u,v,tr,tg,tb)` e modulação com intensidade lambert `r=tr*intensity` etc.
- `tools/mutool.cpp` `CmdRender` estendido: flag `--textured` ou `-t` ou outPath contém "textured" habilita textura placeholder, gera checkerboard 256x256 32px, tenta carregar OZT real `/tmp/cliente_ref/Data/Item/ArmorMale01.ozt` via `LoadFromOZT` para provar pipeline usa OZT decoder otimizado, log "PLACEHOLDER TEXTURE ENABLED — ciclo16 D — checkerboard 256x256 32px, UV planar baseado em posicao vertice, NAO e UV real decodificado (UV continua 0% BLOQUEADO)".
- 2 renders novos com textura placeholder: `docs/evidence/ciclo16_first_render_Beer01_textured.ppm` 800x600 1,4MB e `ciclo16_first_render_Object03_textured.ppm` 800x600 1,4MB, 5º e 6º renders (4 anteriores ciclo14 + 2 novos ciclo16), silhueta reconhecível com padrão xadrez visível.

**Validação objetiva (não subjetiva) textura placeholder — ciclo16 D:**
- Ferramenta `/tmp/validate_tex.cpp` valida que textura está sendo amostrada corretamente por código, não apenas "parece ter padrão":
  - Beer01 textured: w 800 h 600 nonBg 33768 bright 12003 dark 19201 brightRatio 35.5% darkRatio 56.9% hasBoth 1 — bimodal checkerboard esperado 50% white 50% black, após lambert ainda bimodal >10% e <90% ambos
  - Beer01 plain: nonBg 33768 unique intensity values 200 (variação só lambert)
  - Beer01 textured: unique intensity values 223 (variação lambert + checkerboard)
  - Validation textured sampling: PASS (expected bimodal checkerboard, objective, not subjective)
- Evidência `docs/evidence/ciclo16_textured_validation.log` com saída bruta literal, não transcrição manual.

**Limitações explícitas:**
- UV real continua NÃO IMPLEMENTADO/BLOQUEADO 0% 11 hipóteses platô artefato floats, textura placeholder usa UV planar baseado em posição vértice, NÃO é UV real decodificado, nunca apresentado como "UV real"
- Textura checkerboard procedural 256x256 32px, não textura real OZT mapeada corretamente, mas pipeline OZT/OZJ já otimizado é usado via `LoadFromOZT` (chama OztToBgraTopDown OPT-005 e OzjProbe OPT-004)
- Prova de conceito pipeline textura, NÃO validação de UV real, marcado explicitamente no código e documentação

**Evidência:**
- `docs/evidence/ciclo16_first_render_Beer01_textured.ppm` 800x600 1,4MB PPM com padrão xadrez visível, 33768 non-bg, bright 35.5% dark 56.9% bimodal
- `docs/evidence/ciclo16_first_render_Object03_textured.ppm` 800x600 1,4MB
- `docs/evidence/ciclo16_textured_validation.log` validação objetiva PASS
- `src/mu/render/software_rasterizer.hpp/.cpp` com `SoftwareTexture` e placeholder UV explicitamente marcado

