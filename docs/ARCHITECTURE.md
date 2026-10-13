# Architecture

## Cycle-17 status correction

This document contains historical design decisions and snapshots. The
cycle-17/current-state documents are authoritative for implementation status.
Current architecture is an asset library/tools, partial BMD importer, CPU-side
buffer preparation, and a CPU diagnostic rasterizer; there is no playable client
or production GPU backend. The BMD v0x0C and `Enc*` ciphers remain BLOCKED under
`AGENTS.md`. Candidate BMD coordinate floats are not verified UVs, and a PPM is
not a visual-compatibility result. See [`CURRENT_STATE.md`](CURRENT_STATE.md),
[`COMPATIBILITY.md`](COMPATIBILITY.md), and [`NEXT_TASK.md`](NEXT_TASK.md).

## 1. Design constraints

Four constraints determined this architecture. They come from the audit, not
from preference.

1. **The engine cannot be modified.** `Main.exe` is VMProtect-protected and the
   render loop is inside it. A patch-based optimisation strategy is impossible.
2. **Only subsets of the asset tree are currently parsed.** Probes cover formats
   more broadly than pixel/body decoders; some BMD payloads remain OPAQUE, the
   v0x0C cipher is blocked by policy, and `Enc*` terrain is blocked. No probe is
   evidence of visual parity.
3. **Windows is the target, but development happens on Linux** with no GPU and
   no Windows machine. Everything must be testable headlessly, and the Windows
   build path must be verified by someone who has the machine.
4. **No original asset bytes may be committed.** Test fixtures must be generated
   by the code itself.

## 2. Layering

```
+--------------------------------------------------------------+
| tools/mutool         inspection, reports, CPU PPM diagnostic  |
+--------------------------------------------------------------+
| bench/bench_assets   CPU microbenchmarks                      |
| tests/test_assets    unit tests, generated fixtures           |
+--------------------------------------------------------------+
| src/mu/assets        partial readers (OZT/OZJ/TGA/BMD/terrain) |
| src/mu/render        CPU rasterizer, culling, buffer packing   |
| src/mu/platform      main-loop/window skeletons               |
| src/mu/fs            portable directory walking               |
+--------------------------------------------------------------+
          (future) GPU backend, resource manager, playable client
+--------------------------------------------------------------+
```

The dependency direction is strictly downward. `src/mu/assets` knows nothing
about `tools/`. Shared code has no graphics API dependency; the CPU rasterizer
is a diagnostic, and the platform files are not a functional Windows client or
GPU backend.

## 3. Core conventions

**`mu::Status` instead of exceptions or booleans.**

```cpp
struct Status {
  enum Code { kOk, kNotThisFormat, kTruncated, kCorrupt, kUnsupported, kIoError };
  Code code;
  std::string message;
};
```

A boolean cannot distinguish "this is not an OZT" from "this is a truncated
OZT", and that distinction is the whole difference between a correct inventory
and a wrong one. Every message is specific enough to diagnose a file on its
own — `"OZT variant with a 00 4B 6D 4B marker: layout not resolved"` rather
than `"parse failed"`. `mutool info` prints all four readers' messages for an
unrecognised file, which is what makes triage fast.

**`mu::ByteView` instead of owned buffers.**

```cpp
struct ByteView { const uint8_t* data; std::size_t size; };
```

Non-owning, so a caller can memory-map a file instead of copying it. On a
2,372 MiB asset tree the difference between copying and mapping is the
difference between a usable tool and an unusable one. `sub()` clamps instead of
trusting its arguments.

**Explicit endianness accessors.** `ReadU16LE`, `ReadU32LE`, `ReadU16BE`,
`ReadU32BE`. Every format in this client is little-endian except JPEG, which is
big-endian throughout. Mixing them is quiet and catastrophic: reading a JPEG
segment length as little-endian turns `00 43` into 17152 and reports a valid
file as truncated. That bug existed here and was caught only by a test.

**Readers never throw and never abort.** Every input is an untrusted file that
may be truncated, corrupt, or hostile. No reader indexes a buffer before
checking its length, and no reader trusts a size field read from the file
without comparing it against the bytes actually available.

## 4. Why no third-party dependencies

The library builds with a C++17 compiler and nothing else. That decision costs
a JPEG pixel decoder, which is the main thing a dependency would have provided.
It buys:

- A Windows build that needs no package manager, no vcpkg, no vendored tarball
  and no network access. This matters because the Windows path is the one that
  cannot be tested here, so it must be as simple as possible.
- No licence obligations to track for the code itself. See
  [`LICENSING.md`](LICENSING.md).
- Reproducible builds. `make` on any machine with a C++17 compiler produces the
  same result.

The JPEG decoder is a deliberate gap rather than an oversight. When it is added
it should be a pluggable backend so the choice of decoder stays reversible — see
[`DEPENDENCIES.md`](DEPENDENCIES.md).

## 5. Build system

Plain `make`, 60 lines, with generated header dependencies:

```make
DEPFLAGS := -MMD -MP
-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)
```

The header dependency tracking is not optional decoration. Without it, editing
`ozt.hpp` leaves `test_assets.o` compiled against the old `OztImage` layout
linked against a `libmuassets.a` built with the new one. That is an ODR
violation, and it presents as a segfault with no connection to the edit that
caused it. **This has already happened once in this repository.** The `-include`
is prefixed with `-` so a fresh checkout with no `.d` files still builds.

Warnings are errors in practice: `-Wall -Wextra -Wpedantic -Wshadow
-Wconversion -Wsign-conversion -Wold-style-cast -Wuseless-cast
-Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wdouble-promotion
-Wformat=2`, and the build is kept clean. Do not narrow the flags to silence a
warning.

## 6. Test architecture

A ~40-line harness inside `tests/test_assets.cpp`. No framework is downloaded,
which keeps the "compiler and nothing else" property intact.

The important property is **self-generating fixtures**. `OztWriteRgba` writes
valid OZT files, so the OZT reader is tested by round-tripping through the
writer. `MakeFakeJpeg` builds a structurally valid JPEG marker chain. Terrain
and bux fixtures are synthesised from the known key.

This means the test suite is complete without a single original asset byte in
the repository, and it stays complete when the reference client is not
available.

Tests are written against **behaviour that real data exposed**, not against
idealised inputs. Three examples:

- `TestOzjDuplicatedHeaderPrefix` exists because 13,927 real files failed.
- `TestOztVariantWithExtraPrefix` exists because 43 real files carry a marker
  the standard layout does not describe.
- `TestBmdProbe` covers a bux-encrypted non-model because `Data/gate.bmd` is
  one.

## 7. Product modules and current implementation

The names below are architectural targets, not a claim that the client is
playable. Some partial foundations exist; their acceptance limits are in
`CURRENT_STATE.md` and `COMPATIBILITY.md`.

| Module / area | Purpose | Current status |
|---|---|---|
| `mu/assets` | Format readers and asset reports | Partial; probes exceed decoders; see compatibility matrix |
| `mu/render` CPU | Diagnostic rasterizer, culling, CPU buffer packing/skinning helpers | Implemented for tests/PPM only; not a product renderer |
| `mu/gfx` | GPU backend abstraction | Not implemented; API-policy conflict remains open under DEC-014 |
| `mu/gfx/<backend>` | OpenGL VBO/VAO (formal DEC-008); D3D12/Vulkan/D3D11 (target-plan candidates) | No backend implemented or prototyped on Windows/GPU; see DEC-014 |
| `mu/res` | Resource cache/lifetime/budgeting | Not implemented |
| `mu/world` | Terrain/maps/objects | Partial parsers; no complete world renderer |
| `mu/anim` | Skeletal animation | Not implemented as an animation system; bone records are partial and semantics remain unverified; cipher restrictions remain in force |
| `mu/net` | Authorized game protocol/session | Not implemented; no authorized server test |
| `mu/audio` | Audio playback | Not implemented |
| `mu/app` | Window/input/main application | Only skeletons exist; no runnable Windows game client |

Animation is not blocked simply because no geometry exists: some bodies produce
vertices/topology, but BMD weights, transform meanings, and keyframe values are
not verified, and the v0x0C cipher remains BLOCKED under `AGENTS.md`.

## 8. Decisions that shaped this

Recorded in full, with alternatives considered, in
[`DECISIONS.md`](DECISIONS.md).

---

## 9. Shared mesh representation and importer status

The architecture defines one internal mesh/skeleton representation so future
asset importers and render backends can share data. The C++ structures exist;
there is no implemented glTF importer, and the BMD import path is partial.
Historical design decisions are recorded in `DECISIONS.md` (DEC-008/009).

### 9.1 Internal representation (C1 — data structures implemented)

The `Mesh`, `Skeleton`, and packing types live in `src/mu/assets/mesh.hpp` and
`src/mu/render/buffers.hpp`. They are data structures and CPU helpers, not a
complete asset/rendering system.

```cpp
struct Vec3 { float x, y, z; };
struct Vec2 { float u, v; };
struct BoneWeight { uint32_t bone; float weight; };
constexpr int32_t kNoBoneParent = -1;
struct Bone { std::string name; int32_t parent = kNoBoneParent; Vec3 position; Vec3 rotation; };
struct Bounds { Vec3 min, max; bool valid = false; };
struct Mesh {
  std::string name;
  std::vector<Vec3> positions, normals;
  std::vector<Vec2> texcoords;          // empty for current BMD import
  std::vector<uint32_t> indices;        // triangle indices, count multiple of 3
  std::vector<std::vector<BoneWeight>> bone_weights;
  bool has_texcoords = false;           // requires verified corner-to-UV mapping
  bool has_bone_weights = false;        // requires validated weights/bone references
  Bounds bounds;
  std::size_t skipped_vertices = 0;
  std::size_t dropped_triangles = 0;    // once per rejected triangle
};
struct Skeleton { std::vector<Bone> bones; bool has_bones = false; };
struct MeshImportResult { Mesh mesh; Skeleton skeleton; bool ok; std::string problem; };
```

| Capability | Current status |
|---|---|
| Position/normal arrays and bounds | Implemented in the BMD path for accepted layouts; unit tests and full-tree parser counts exist; not visual-equivalence validation |
| Separate triangle indices | Implemented; 3,681,664 body triangles read in 8,405 files, with invalid refs handled separately |
| UV storage field | **Data structure only**; BMD UV semantics and mapping are unverified, so BMD importer leaves `has_texcoords` false |
| Bone/skeleton structures | **Data structure only** for import/packing; BMD bone records are partially parsed, but transform semantics and usable skinning are not verified |
| Bone weights | Field exists; no verified BMD weight import or glTF importer |
| Bounds | `ComputeBounds` is implemented and unit-tested |
| Counts wider than source `u16` | Internal containers use `std::size_t` / `uint32_t`; this does not change BMD's on-disk limits |

### 9.2 Original asset path (BMD / OZT / OZJ / terrain)

| Area | Current status |
|---|---|
| BMD structural probe | 10,066 probe-recognized; 10,023 structurally valid in the current report |
| BMD payload access | 8,571 classified clear by the current tool (1,245 plaintext, 100 BUX-labelled, 7,226 v0x0C-labelled); `AGENTS.md` still marks v0x0C cipher **BLOCKED** |
| BMD geometry | 8,406 accepted by the current body parser (83.31%); see `COMPATIBILITY.md` for scope and index counters |
| BMD topology | 8,405 files, 3,681,664 triangle records; 513,304 triangles are dropped by mesh import because of invalid vertex/normal refs |
| Candidate coordinate bytes | Float-like statistical observations exist, but the region is not semantically validated as UV and is not exposed as BMD UV data |
| BMD per-corner UV/material mapping | **BLOCKED / not located**; no correctly associated textured-model render is verified |
| BMD bone records | 3,693 files under the loose criterion; names/parent/layout observations do not prove transform order or animation semantics |
| BMD animation | **NOT IMPLEMENTED as a semantic animation decoder**; trailing-size arithmetic is size-only |
| OZT | Probe/read and 24-bit BGR / 32-bit BGRA-to-BGRA8 conversion; one real file loaded; no original-client visual comparison |
| OZJ | Container/JPEG-marker probing only; pixel decoder not implemented |
| Terrain | Some grid parsing exists; `Enc*` cipher remains BLOCKED and vertical scale/rendering are unresolved |

The reported BMD counters are parser/classifier outcomes, not a claim that
cipher work is authorized or every accepted geometry is visually correct. On
the current reference snapshot, geometry is extracted in **8.406** of **10.090**
BMD files (**83,31%**) and triangle records are read in **8.405** (**83,30%**).
These counts do not imply verified UV semantics or rendering. Current
invalid-index accounting is 1,030,785/22,089,984 reference fields (4.666%),
versus 513,304/3,681,664 triangles dropped (13.94%). These quantities have
different units.

The multi-mesh parser work recovered the earlier implementation limit; it does
not solve UV/material semantics. Older cycle-4/5 tables that describe
`meshCount != 1` as an unhandled cause are historical snapshots and should not be
read as current behavior. See the current six-stage table in `COMPATIBILITY.md`.

### 9.3 Future custom-asset path — glTF decision, importer not implemented

The earlier architecture selected glTF 2.0 (`.gltf`/`.glb`) as a possible
interchange format for new custom assets. The comparison below records that
design decision only: this repository does **not** currently contain a glTF
importer, and this path is not part of the current original-asset compatibility
claim.

| Critério | **glTF 2.0** | FBX | OBJ + MTL |
|---|---|---|---|
| Skinning | Nativo, com pesos e hierarquia | Nativo | **Não tem** |
| PBR | Nativo (metálico-rugosidade) | Parcial, depende de exportador | **Não tem** |
| Maturidade de bibliotecas | Alta; é o formato de intercâmbio da Khronos | Alta mas proprietária | Alta |
| Licença | Especificação aberta, sem royalties | **SDK Autodesk, licença restritiva** | Aberta |
| Parsing sem dependência problemática | `.glb` é binário simples: header + chunks JSON + binário. Um parser mínimo é viável sem biblioteca | Exige o SDK | Trivial, mas incapaz |
| LOD | Extensão `MSFT_lod` | Manual | Manual |
| Compressão de geometria | `EXT_meshopt_compression`, KTX2 para textura | Parcial | Nenhuma |
| Teto de contagem | 32 bits | 32 bits | sem índice |

**Por que não FBX:** licença do SDK é restritiva e incompatível com a postura de
zero dependência e distribuição livre deste projeto.

**Por que não OBJ:** não tem skinning nem PBR. Seria incapaz para personagens
animados, que é exatamente o conteúdo pesado que se quer suportar.

`glTF` vence em capacidade e é o único dos três cuja especificação é aberta e
cuja variante binária pode ser lida por um parser próprio, preservando a
propriedade de zero dependência do build.

### 9.4 A custom-asset path does not replace original assets

- The proposed glTF path is for new/custom content only and currently has no
  importer implementation.
- It does not resolve, bypass, or make optional the unfinished BMD topology,
  UV/material, and bone-semantic work.
- Parsing a subset of the 10,090 BMD files is not full original-asset
  compatibility.
- A synthetic or future custom-format test is not evidence that an original BMD
  asset is interpreted correctly (DEC-009).

---

## 10. Limites reais do formato BMD, medidos

The following observations come from real assets. Distinguish on-disk limits
from the subset currently accepted by the parser.

| Field / region | Observation | Current parser/semantic limit |
|---|---|---|
| Vertex/normal/coordinate/triangle counts | `u16` fields, maximum representable value 65,535 per declared field | A representable count is not proof the corresponding array is semantically valid |
| Mesh count | Maximum observed: 30 in the current report history; multi-mesh parsing is implemented for accepted layouts | 4,256 of 8,406 geometry-parsed files have `meshCount > 1`; this is no longer an unsupported-by-design category |
| Bone count | Maximum observed: 63; parser's defensive bound is 64 | Record-name/layout acceptance does not confirm transform meaning or weights |
| Vertex record | 16 bytes: three `f32` values plus two `u16` auxiliary fields | Auxiliary semantics are not established |
| Normal record | 20 bytes: five `f32` values | Trailing pair is not confirmed as texture coordinates |
| Triangle record | 64 bytes: three 8-byte corner records plus other bytes | Only vertex/normal fields are used; other fields are not decoded as UV |
| Bone record | 59-byte interleaved layout in accepted bodies | Transform order and keyframe semantics remain unverified |
| Texture-name region | 32-byte name observed after mesh triangle arrays | Name positioning does not establish complete material/texture association |
| Terrain grid | 256×256 byte-grid observation in supported inputs | Vertical scale and `Enc*` decryption/rendering remain unresolved |

These observations inform the internal count widths but do not establish complete
format compatibility. Internal `std::size_t`/`uint32_t` storage avoids an
unnecessary implementation limit; it does not change on-disk constraints.

---

## 11. Requisitos de escalabilidade de renderização

O que o renderer precisa fazer para que malha pesada não custe FPS. Cada item
diz o que é, por que importa, e **se está medido ou não** — este ambiente não tem
GPU, então quase nada aqui está medido.

### 11.1 Redução de draw calls

**Por que:** `glBegin`/`glEnd` envia um vértice por chamada. Com malha pesada, o
custo de driver domina antes do custo de rasterização.

**Status:** CPU-side buffer preparation and a CPU diagnostic rasterizer exist, but **no production GPU backend/device path exists**. The graphics API must be selected by an on-target prototype; indexed GPU buffers remain a design requirement, not a measured performance result.

**Medido:** não. Sem GPU.

### 11.2 Frustum culling

**Por que:** descartar objetos inteiros antes do draw é a redução mais barata que
existe, porque custa matemática em CPU e economiza uma chamada inteira.

**Estado:** **implementado e testado** (`src/mu/render/cull.cpp`). Conservador na
direção segura: pode custar uma draw call extra, nunca descartar geometria visível.

**Measured:** the current synthetic `mutool culltest` classifies 8,086/8,406
bounds (96.19%) as culled. This is **not a game metric**: objects use an
artificial grid because BMD does not provide world transforms. **FPS impact is
not measured.**

### 11.3 Níveis de detalhe

**Por que:** malha pesada longe da câmera desperdiça preenchimento e largura de
banda de vértice.

**Estado:** **não implementado.** O BMD não carrega LOD; o caminho B (glTF) tem a
extensão `MSFT_lod` disponível.

**Medido:** não.

### 11.4 Streaming e orçamento de memória

**Por que:** a varredura do ciclo 17 somou **2.893,80 MiB** em estimativas de bytes de imagem derivadas de cabeçalhos/payloads (OZJ como equivalente RGBA8; OZT/TGA pelos payloads declarados/calculados). Isso **não** mede alocação decodificada, RAM/VRAM, residência simultânea, nem demonstra que texturas sejam o gargalo dominante ou um limite superior de memória. Consulte [`PERFORMANCE_BASELINE.md`](PERFORMANCE_BASELINE.md).

**Estado:** **não implementado.** Não há gerenciador de residência de textura.

**Medido:** não. Orçamento e streaming dependem de uma medição real de residência em Windows/GPU.

### 11.5 O que este ambiente pode e não pode estabelecer

| Requisito | Pode ser estabelecido aqui? |
|---|---|
| Culling descarta o que deve descartar | **SIM** — testes com respostas conhecidas por construção |
| Contagens de 32 bits não estouram | **SIM** — testes de limite |
| Caixa delimitadora correta | **SIM** — testes |
| Draw calls em cena real | **NÃO** — exige posições reais e pipeline |
| FPS, frametime, stuttering | **NÃO** — exige GPU |
| Latência de entrada | **NÃO** — exige GPU e dispositivo de entrada |
| VRAM real | **NÃO** — exige GPU |

**Regra que decorre disso:** nenhum relatório deste projeto pode citar melhoria de
desempenho sem dizer onde foi medida. `PERFORMANCE_BASELINE.md` continua sem
baseline de FPS, e isso é uma limitação declarada, não uma omissão.
