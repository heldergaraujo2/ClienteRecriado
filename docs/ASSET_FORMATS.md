# Asset formats

This document records observed layouts and the limits of each observation. The
historical investigations below contain hypotheses and test history; the current
audit summary here is authoritative where earlier prose differs.

`mutool scan` enumerates the **entire** asset tree, but its per-format counters
are probes/header checks, not proof of pixel decoding, semantic interpretation,
or correct rendering. Reproduce with:

```sh
build/mutool scan /path/to/Cliente_ref/Data
build/mutool bmdreport /path/to/Cliente_ref/Data
build/mutool meshimport /path/to/Cliente_ref/Data
```

The scanner's `MiB` values divide bytes by 1,048,576; older cycle logs may
label these same binary units `MB`. Reference tree measured: 29,060 files,
2,372.04 MiB. Final scan evidence:
[`evidence/ciclo17_scan_final.log`](evidence/ciclo17_scan_final.log).

| Format | Files | Probe accepted | Meaning / limit |
|---|---:|---:|---|
| OZJ | 13,928 | 13,832 (99.31%) | JPEG structure/markers only; image pixels are not decoded |
| OZT | 3,470 | 3,404 (98.10%) | Header/payload probe; standard 24-bit BGR and 32-bit BGRA conversion exists, but scan does not convert every image |
| BMD | 10,090 | 10,066 (99.76%) | Container recognition only; 10,023 are structurally valid in `bmdreport` |
| TGA | 15 | 15 (100%) | Probe acceptance; current pixel reader handles uncompressed type 2, not RLE type 10 |
| `.att` terrain attribute | 105 | 105 (100%) | Probe acceptance, not full terrain rendering |
| `.map` terrain map | 79 | 79 (100%) | Probe acceptance, not full terrain rendering |
| `.obj` terrain object list | 81 | 81 (100%) | Probe acceptance, not semantic/rendering verification |

### Current BMD parser stages

These counts are independent stages measured on the same 10,090-file population;
they are not a fully cumulative hierarchy. Positive topology is a subset of
parsed geometry. The tool's v0x0C-labelled classifier result is experimental;
`AGENTS.md` still marks the v0x0C cipher BLOCKED. The `Enc*` terrain cipher is
also BLOCKED.

| Stage | Files | Rate |
|---|---:|---:|
| 1 — structure | **10.023** | **99.34%** |
| 2 — payload | **8.571** | **84.95%** |
| 3 — vertices | **8.406** | **83.31%** |
| 4 — topology | **8.405** | **83.30%** |
| 5 — loose bones | **3.693** | **36.60%** |
| 6 — UV | **0** | **0.00%** |

There are 1.519 OPAQUE payloads (15.05%). For 3,681,664 triangle records, the
parser counts 523,717 out-of-range vertex references and 507,068 out-of-range
normal references: 1,030,785 / 22,089,984 = 4.666% of reference fields.
`meshimport` drops 513,304 triangles (13.94%) and emits 3,168,360; these are
triangle counts, not reference counts. Details are in
[`COMPATIBILITY.md`](COMPATIBILITY.md) and the cycle-17 evidence logs.

### Image-byte estimates are not runtime memory

The scan's **2,893.80 MiB** is a header/payload-derived image-byte estimate (OZJ
uses an RGBA8-equivalent; OZT/TGA use declared/calculated payload bytes). It is
not decoded OZJ storage, measured RAM, VRAM, or proof that all images are loaded
at once. See [`PERFORMANCE_BASELINE.md`](PERFORMANCE_BASELINE.md).

---

## OZJ — JPEG textures

13,928 files, 971.7 MiB. The largest single format in the client.

### Layout

An OZJ is a **plain JPEG with its own leading 24 bytes duplicated in front of
it**.

```
offset 0    : 24 bytes, a byte-for-byte copy of the JPEG's first 24 bytes
offset 24   : the complete JPEG, starting with its own FF D8 FF
```

### Evidence

Scanned every OZJ in the tree for start-of-image markers in the first 64 bytes:

```
files with >= 2 SOI markers in first 64 bytes: 200/200 sampled
SOI positions: always (0, 24)
prefix[0:24] == bytes[24:48]: True for 200/200
```

The 24 bytes are `FF D8` (SOI) + a 16-byte JFIF APP0 segment + the two-byte
APP1 marker and its two-byte length.

### Why this matters

This was found by a full scan, not by sampling. An earlier inventory sampled 120
OZJ files, saw that 119 began with `FF D8 FF`, and concluded "OZJ is a plain
JPEG". Running the real parser over the whole tree produced:

```
.ozj  13928 files, 1 ok, 13927 fail
      note: expected a marker prefix 0xFF (13750)
```

A parser that walks the marker chain from offset 0 reads the APP1 length out of
the duplicated region, lands mid-segment, and reports a healthy file as corrupt.
**99.99% failure rate from a conclusion that a 120-file sample supported.** After
locating the real image start: 13,832 of 13,928 parse.

The lesson is recorded in `AGENTS.md`: verify format conclusions against the
whole tree.

### Implementation

`mu/assets/ozj.cpp`. `OzjFindImageStart()` walks the markers from offset 0; if
that fails it searches the first 4096 bytes for a second SOI and re-walks from
there. It never assumes the prefix is 24 bytes.

This module does **not** decode JPEG pixels. It extracts geometry, precision,
component count and whether the file terminates correctly — everything the
inventory and VRAM-budget tooling needs. Pixel decoding needs a real JPEG
decoder, which is a dependency decision recorded in
[`DEPENDENCIES.md`](DEPENDENCIES.md).

### JPEG is big-endian

One real bug worth recording. JPEG stores every length field big-endian, unlike
every other format in this client. The first implementation read segment lengths
with the little-endian helper, so a DQT length of `00 43` (67) was read as
17152 and the parser reported "segment runs past the end of the file" on a
perfectly valid file. `mu::ReadU16BE` exists specifically so this cannot recur.

### 96 files that do not parse

These begin `FF 4B 6D` rather than `FF D8 FF`. A separate container variant. Not
resolved; recorded in [`TEST_RESULTS.md`](TEST_RESULTS.md).

---

## OZT — standard TGA-like truecolour textures

3,470 files, 442.0 MiB; 3,404 pass the current header/payload probe. A 400-file
sample measured 32 bits per pixel in all 400. The reader/converter supports the
standard uncompressed 24-bit BGR and 32-bit BGRA layouts; the whole-tree probe
is not a whole-tree pixel-decode test.

### Layout

```
offset 0    : 4-byte signature  00 00 02 00
offset 4    : 18-byte standard TGA image header
                [4]  colour map length
                [5]  colour map type
                [6]  image type            2 = uncompressed truecolour
                [7]  colour map first index (u16)
                [9]  colour map length      (u16)
                [11] colour map entry size
                [12] x origin (u16)
                [14] y origin (u16)
                [16] width  (u16, little endian)
                [18] height (u16, little endian)
                [20] bits per pixel   32 in 400/400 sampled
                [21] image descriptor  8 = 8 alpha bits
offset 22   : width * height * 4 bytes, packed BGRA, bottom row first
trailer     : variable length, see below
```

So a complete file is `22 + w*h*4 + trailer`.

### Evidence

Over 400 sampled files:

```
signature[0:4]: 00000200 in 395, 004b6d4b in 5
image type:     2 in 400/400
bits per pixel: 32 in 400/400
trailer length: 26 bytes in 362 (90.5%)
                 0 bytes in  22 (5.5%)
                30 bytes in  12 (3.0%)
                34 bytes in   1
```

Worked example, `Data/Item/capesnowlucasnuut02.ozt`, 262,192 bytes:
`22 + 256*256*4 + 26 = 22 + 262144 + 26 = 262192`. Exact.

### Trailer handling — a deliberate decision

The first implementation rejected any trailer that was not 0 or 26 bytes. That
fails around 3% of the asset base for no benefit: the pixel payload is fully
determined by the header, so trailing metadata can be ignored safely. The loader
now accepts any trailer up to 4096 bytes, reports its length in
`OztImage::trailer_size`, and rejects only implausibly large ones.

Compatibility with the existing asset base is a stated project priority, and
this is a case where strictness would have cost real files.

### The `00 4B 6D 4B` variant — recognised, not decoded

43 files (for example `Data/Custom/Interface/Rank01.OZT`, `Next.OZT`,
`Main_IE.ozt`) carry an extra 4-byte marker ahead of the signature. Three
measurements on them:

1. Image type at the usual TGA offset reads **0**, not 2.
2. Dimensions sit 4 bytes later than in the standard form: `Rank01.OZT` is
   78x78 with the dimensions at file offset 20 rather than 16.
3. File length is 24,358 bytes. Header-plus-payload is 26 + 78*78*4 = 26,362 —
   **exactly 4 bytes more than the file**.

Those three facts cannot all be satisfied by a single consistent layout. It is
possible to pick a pixel offset that makes the arithmetic close, but it would
overlap the bits-per-pixel field, and the resulting pixels would be silently
wrong. The loader therefore recognises the variant and returns `kUnsupported`
with a message naming the marker, rather than guessing.

`AGENTS.md` records the general rule: a format conclusion that only holds
arithmetically is not a format conclusion.

### Implementation

`mu/assets/ozt.cpp`. `OztProbe` checks the header, dimensions, payload bounds,
and trailer without copying pixel data (22 header bytes inspected). `OztRead`
copies the declared payload. `OztToRgbaTopDown` converts bottom-up BGR/BGRA to
top-down RGBA8; `OztToBgraTopDown` converts to top-down BGRA8. For 24-bit input,
both add opaque alpha. These conversions have synthetic-fixture tests, and one
real OZT passed the read/conversion path in cycle 17. This is not a GPU upload or
visual comparison. `OztWriteRgba` writes valid fixtures for the test suite.

---

## BMD — model container

10,090 files, 585.5 MiB.

### Container layout

```
offset 0 : 'B' 'M' 'D'      ASCII magic
offset 3 : version byte
```

Two layouts, distinguished by the version byte:

**Version 0x0C** — 8,741 files

```
offset 4 : uint32 little endian, payload length
offset 8 : payload
```

File size equals `8 + payload length`. Holds for the great majority; the loader
checks it and reports `length_consistent = false` when it does not, so a
truncated download degrades instead of overflowing a buffer.

**Version 0x0A** — 1,194 files

Legacy layout with **no length field**. Reading offset 4 as a length yields
implausible values (1,635,017,028 on a 21,141-byte file), so the module exposes
everything from offset 4 as payload and does not interpret it.

### Three separate metrics — and why one number was wrong

An earlier report said "BMD: 10,066 of 10,090 read (99.76%)". That figure counted
only whether the **container header** could be classified. It said nothing about
whether a vertex could be extracted, and the difference between those two
answers is 99.76% versus 0.00%. The cycle-1 audit caught this correctly.

`mutool bmdreport <dir>` now reports all three levels, so the number cannot be
misread again. Measured over the whole tree (full output in
[`evidence/ciclo1_bmdreport.log`](evidence/ciclo1_bmdreport.log)):

> **Correção do ciclo 3.** A tabela que estava aqui mostrava "payload em claro
> 1.496 (14,83%)" e "geometria extraída 1.191 (11,80%)". Esses números são do <!-- historico -->
> ciclo 1, **anteriores à recuperação da cifra v0x0C** (DEC-007). Mantê-los ao
> lado dos atuais produziria duas tabelas com o mesmo denominador e valores
> diferentes — exatamente a ambiguidade que a auditoria do ciclo 3 pediu para
> eliminar. Foram substituídos. O salto entre 11,80% e 83,31% é a decifragem. <!-- historico -->

| Nível | O que mede | Arquivos | % de 10.090 |
|---|---|---|---|
| 1 — estrutura de contêiner válida | magic + versão reconhecidos; para v0x0C o tamanho declarado confere | 10.023 | 99,34% |
| 2 — payload obtível em claro | 1.245 plaintext + 100 bux + 7.226 v0x0C | 8.571 | 84,95% |
| 3 — vértice + normal | posições e normais realmente lidas pelo parser | **8.406** | **83,31%** |
| 4 — topologia (índices de face) | triângulos lidos | **8.405** | **83,30%** |
| 5 — ossos parseáveis (loose) | registros de osso localizáveis e legíveis (loose >=3 printable) | **3.693** | **36,60%** |
| 5b — ossos strict (headline conservador) | NUL-padded ASCII >=3 + zeros, taxa falso-positivo 0/25218 limite superior ~0,004% (1/25218) ratio per file >3558× | **1.186** | **11,75%** |
| 6 — UV | índice ligando canto de triângulo a coordenada de textura | **0** | **0,00%** |

1.519 arquivos (15,05%) permanecem OPAQUE. Evidência bruta:
[`evidence/ciclo3_scan_final.log`](evidence/ciclo3_scan_final.log),
[`evidence/ciclo4_topology_subset.log`](evidence/ciclo4_topology_subset.log) e
`evidence/ciclo6_bmd_multimesh.log`.

#### Os níveis NÃO são cumulativos na mesma população

**Os seis níveis são critérios independentes aplicados à mesma população de
10.090 arquivos, não uma hierarquia em que cada linha é subconjunto da
anterior.** Cada linha responde a uma pergunta própria sobre o mesmo arquivo, e
o denominador é sempre 10.090.

Existe **uma** relação de subconjunto entre linhas, verificada em código e por
teste — não inferida:

- **O nível 4 é subconjunto estrito do nível 3.** Em
  `src/mu/assets/bmd_report.cpp`, `triangle_count` só é atribuído dentro do ramo
  que faz `geometry_parsed = true`; o acumulador só incrementa `topology_parsed`
  sob `if (r.geometry_parsed && r.triangle_count > 0)`. Verificado sobre a árvore
  inteira por `TestTopologyIsSubsetOfGeometry`, que também confere que as causas
  de pulo são exaustivas (soma igual a `geometry_parsed - topology_parsed`), de
  modo que uma quinta causa não pode aparecer em silêncio.
- **Os níveis 3, 4 e 5 não formam cadeia.** O nível 4 exige `meshCount == 1`, o
  nível 5 não. Um arquivo pode ter ossos sem topologia e vice-versa.

#### Por que o nível 4 caía 50,6% — e não cai mais (corrigido no ciclo 5)

No ciclo 4, a queda de 8.406 para 4.149 (4.257 arquivos) tinha **causa única e
total, medida no parser**: `meshCount != 1` em 100,00% dos casos (4.256 multi-mesh
verdadeiro `meshCount>1` + 1 zero-faces `meshCount==0` = 4.257). Ferramenta atual
confirma: `mutool bmdreport` → 4.149 meshCount 1, 1 meshCount 0, 4.256 meshCount>1.

| Causa | Arquivos | % dos 4.257 | Nota |
|---|---|---|---|
| multi-mesh (`meshCount>1` verdadeiro) | **4.256** | **99,98%** | população correta multi-mesh, ver reconciliação abaixo | <!-- historico -->
| declara zero faces (`meshCount==0`) | **1** | **0,02%** | `player.bmd` | <!-- historico -->
| total `meshCount !=1` | **4.257** | **100,00%** | 4.256+1 | <!-- historico -->
| array declarado não cabe no payload | 0 | 0,00% | | <!-- historico -->
| payload curto demais para os counts | 0 | 0,00% | | <!-- historico -->

**Ciclo 5 — correção.** O parser foi estendido para multi-mesh. O layout por
malha repete a mesma sequência do caso de malha única (evidência em
`evidence/ciclo5_bmd_multimesh.log`):

```
[14 B header: vc,nc,ntc,ntri,texIdx,u48,u50]
vc*16 vertices, nc*20 normais, ntc*8 texcoords, ntri*64 triângulos,
32 B nome de textura
```

Concatenado `meshCount` vezes. Estrutura de loop inspirada em
`xulek/muonline-bmd-viewer` (`bmd-loader.ts:348-422`, ISC, commit
`3f11bf89f3702261174d8efa1436d56a15c7d97a`), mas header de 14 B vs 10 B da fonte e
layout de triângulo diferente (fonte: polygon em +0, vertex em +2, normal em +10,
texCoord em +18; este projeto: vertex em +0/+8/+16, polygon 3/4 em 3,01% ≈
aleatório, medido). Busca de nome com delta -4 é descoberta deste projeto.

Resultado inicial ciclo 5 (antes da correção da regressão):

| Nível | Arquivos | % | Triângulos |
|---|---|---|---|
| 3 — vértice + normal | 8.401 | 83,26% | — | <!-- historico -->
| 4 — topologia | **8.400** | **83,25%** | **3.524.366** | <!-- historico -->

Aumento de **4.251 arquivos** no nível 4 (de 4.149 para 8.400) no snapshot do ciclo 5,
onde nível 3 estava em 8.401 devido à regressão de limite 10k. No snapshot final do
ciclo 6 (nível 3 8.406), o aumento é **4.256 arquivos** (4.149→8.405). A divergência
4.257/4.251/4.256 vem de três definições diferentes usadas em ciclos distintos:

- **4.257** = arquivos com `meshCount != 1` no snapshot 8.406 (4.256 com `meshCount>1`
  verdadeiro multi-mesh + 1 com `meshCount==0` zero-faces `player.bmd`). Usado no ciclo 4
  para explicar a queda 8.406→4.149 antes do suporte multi-mesh.
- **4.251** = arquivos com `meshCount>1` no snapshot 8.401 do ciclo 5 (regressão de 5 arquivos
  com ntri>10k). 8.401-4.149-1 = 4.251. Valor histórico, não mais atual.
- **4.256** = arquivos com `meshCount>1` verdadeiro multi-mesh no snapshot final 8.406
  (ferramenta atual `mutool bmdreport` confirma: 2.280+1.086+449+190+114+42+38+12+10+6+7+2+13+7
  = 4.256). **Este é o correto para população multi-mesh atual.** Evidência
  `evidence/ciclo6_item1_bmdreport.log` e `ciclo6_level4_breakdown.log`.

O único arquivo de nível 3 sem topologia agora declara zero faces (`player.bmd`,
`meshCount==0`).

**Ciclo 6 — correção da regressão 8.406→8.401 (item 1 da auditoria ciclo 5).**

Causa raiz: limite fixo `vc>10000 || nc>10000 || ntc>10000 || ntri>10000` introduzido
no parser multi-mesh para evitar alocação excessiva. Cinco arquivos reais têm
`ntri` entre 11.771 e 17.694 (ex.: `Monster865.bmd` com `meshCount=2`, `ntri=15576`
na primeira malha; `Ozarugold.bmd` com `meshCount=7`, `ntri=12616`; `bann.bmd`
`ntri=11771`), que excedem 10.000 mas cabem no payload (997 kB para 15.576×64).
Eles passaram no parser antigo (que só lia primeira malha para nível 3) e falharam
no novo (que tenta todas as malhas e rejeitava por limite). Efeito colateral do
refactor.

Correção: removido limite fixo de 10.000; substituído por verificação de overflow
e por limite de segurança em `meshCount` (máx 64, observado máx 18) e por checagem
de que `vc*16`, `nc*20`, `ntc*8`, `ntri*64` não excedem `payload.size`. Assim,
`ntri=15576` é aceito quando cabe, e `meshCount=0xFFFF` é rejeitado graciosamente
sem loop/OOM.

Resultado após correção (evidência em `ciclo6_bmd_multimesh.log` e
`ciclo6_level4_breakdown.log`):

| Nível | Arquivos | % | Triângulos |
|---|---|---|---|
| 3 — vértice + normal | **8.406** | **83,31%** | — |
| 4 — topologia | **8.405** | **83,30%** | **3.681.664** |

Distribuição de `meshCount` sobre os 8.406 arquivos de nível 3 (evidência em
`ciclo6_bmd_multimesh.log`): 0 em 1 (0,01%), 1 em 4.149 (49,36%), 2 em 2.280
(27,12%), 3 em 1.086 (12,92%), 4 em 449 (5,34%), 5 em 190 (2,26%), 6 em 114
(1,36%), 7 em 42 (0,50%), 8 em 38 (0,45%), 9 em 12, 10 em 10, 11 em 6, 12 em 7,
13 em 2, 14 em 13, >15 em 7. Total multi-mesh 4.256 arquivos, todos parseados
(100%).

**Validação estatística do layout multi-mesh (item 3 da auditoria ciclo 5).**

Amostra final: **full-scan dos 4.256 arquivos multi-mesh identificados** (100% da
população multi-mesh, não amostra), total de **12.590 malhas**.

| Medida | Resultado |
|---|---|
| Arquivos multi-mesh com geometria parseada | 4.256/4.256 = **100,00%** |
| Malhas com nome de textura estrito em `tri_end-4` | 1.796/12.590 = **14,27%** |
| Malhas com nome estrito em `tri_end+0` | 1/12.590 = **0,01%** |
| Malhas com nome fraco (>=8 printable) em `tri_end-4` | 6.572/12.590 = **52,20%** |
| Malhas com nome fraco em `tri_end+0` | 6.320/12.590 = **50,20%** |
| Arquivos com pelo menos 1 nome fraco encontrado | 4.256/4.256 = **100,00%** |
| Arquivos com pelo menos 1 nome estrito | 1.651/4.256 = **38,79%** |
| Controle estrito: nome estrito dentro de array de vértices (região `f32`) | 89/236.677 = **0,0376%** |
| Razão hipótese/controle estrito (14,27%/0,0376%) | **379×** |
| Controle fraco: nome fraco dentro de array de vértices | 7.153/9.229 = **77,51%** |
| Razão hipótese/controle fraco (52,20%/77,51%) | **0,67×** — fraco não discrimina, como já medido para UV |

**O que foi buscado, por que mesma classe, risco controle fraco (item 5 auditoria ciclo 6):**

- **O que foi buscado:** nome de textura de 32 B após cada bloco de triângulos. Hipótese:
  `tri_end + delta` onde `delta` ∈ {0,-4,+4,-8,+8} contém string ASCII imprimível terminada em NUL,
  3-31 chars, resto zero (estrito) ou ≥8 printable em 32 B (fraco). Ferramenta
  `tools/validate_multimesh.cpp` varre full-scan 4.256 arquivos (12.590 malhas) e conta
  ocorrências por delta. Evidência `evidence/ciclo6_multimesh_validation.log` e
  `evidence/ciclo6_item4_validate.log`.

- **Por que mesma classe (precisão UV):** na investigação de UV (nona hipótese, ciclo 3),
  procurou-se `f32` em [0,1] dentro do registro de triângulo. Controle inicial com bytes
  aleatórios deu 24,82% vs 79,42% hipótese (razão 3,2×) e pareceria descoberta, mas controle
  correto com **mesma classe** — outro `f32` que não é UV (array de normais) — deu 70,34%
  vs 79,42% (razão 1,13×), rejeitando hipótese. Lição: controle tem que ser da mesma classe
  do que se testa, senão taxa base é subestimada e falso-positivo parece forte.

  Para multi-mesh, mesma exigência: procurar nome dentro de região que **também** contém
  bytes que podem parecer nome por acaso. Controle estrito: buscar nome estrito dentro do
  array de vértices (região `f32` com 4 B floats, não texto). Resultado: 89/236.677 = **0,0376%**
  falso-positivo. Hipótese `tri_end-4` dá 14,27% → razão **379×**, discriminativo.
  Controle fraco: mesmo array de vértices mas critério fraco (≥8 printable). Resultado:
  7.153/9.229 = **77,51%** falso-positivo, hipótese 52,20% → razão **0,67×**, não discrimina.

- **Risco controle fraco:** critério fraco (≥8 printable) parece dar 100% dos arquivos
  (4.256/4.256) e 52,20% das malhas, mas controle fraco dá 77,51%, maior que hipótese.
  Isso significa que bytes de float, quando interpretados como ASCII, frequentemente têm
  ≥8 printable por acaso (ex.: `0x3F800000` = `?` + NULs). Se usássemos só fraco, aceitaríamos
  qualquer offset como válido e leríamos nome no lugar errado, desalinhando parser e
  corrompendo próxima malha em silêncio. Mesmo risco da UV: controle fraco (bytes aleatórios)
  subestima base e faz 79% parecer descoberta. Por isso parser usa **estrito como primário**
  (379× discriminativo) e fraco só como fallback quando estrito falha, mas sempre com
  busca em `tri_end-4` primeiro, que é onde 14,27% dos nomes estritos aparecem vs 0,01% em +0.

Interpretação: nome estrito é discriminativo (379× sobre controle), mas aparece
em só 14,27% das malhas porque muitas texturas têm nomes curtos, com caracteres
não-ASCII, ou estão em subdiretório que não passa no filtro estrito? O fraco dá
100% dos arquivos, mas controle também dá 77,5%, então fraco é contaminado —
mesma armadilha do controle de UV dentro do array. Por isso o parser usa busca
com delta e aceita fraco como fallback, mas não depende só dele.

O layout de 14 B + arrays + 32 B concatenado é o que permite 100% de parsing dos
4.256 arquivos multi-mesh; sem ele, nível 4 ficava em 4.149 (49,36% do nível 3).

**Contabilidade completa dos arquivos falhando no nível 4 (item 2).**

Total .bmd 10.090, nível 4 ok 8.405, falhando **1.685** (evidência gerada por
`tools/gen_level4_breakdown.py`, saída em `evidence/ciclo6_level4_breakdown.log`).

| Causa | Arquivos | % dos 1.685 | Como medido |
|---|---|---|---|
| OPAQUE (cipher não recuperado) | 1.483 | 88,01% | `access==OPAQUE` |
| Container inválido (tamanho declarado não confere) | 67 | 3,98% | `structure_valid==false` |
| Geometria não parseada (counts excedem payload / truncado) | 134 | 7,95% | `geometry_parsed==false` e `structure_valid && !opaque` |
| Nível 3 ok mas declara zero faces | 1 | 0,06% | `skip==ZeroFaces` (`Player/player.bmd`, `meshCount=0`) |
| Multi-mesh ainda não suportado | 0 | 0,00% | `skip==MultiMesh` |
| Array não cabe / payload curto | 0 | 0,00% | `skip==DoesNotFit/NoHeader` |
| **Soma** | **1.685** | **100,00%** | |

Antes da correção multi-mesh (ciclo 4): multi-mesh 4.257 (100% da queda). Após
ciclo 5 inicial: multi-mesh 0, zero faces 1. Após ciclo 6: idem, mas nível 4
subiu de 8.400 para 8.405 (+5 arquivos que eram bloqueados pelo limite de 10k).

Isso não destrava renderização: o nível 6 (UV) continua em zero.

Container kinds, for completeness:

```
BMD_V0C            8,741  (86.63%)
BMD_V0A            1,194  (11.83%)
BMD_OTHER_VERSION     31  ( 0.31%)
BUX_CONTAINER        100  ( 0.99%)
unrecognised          24  ( 0.24%)

payload entropy: max 7.9980 bits/byte, 8,570 above 7.5
```

## Histórico do ciclo 1 — classificação anterior (superada)

> Os percentuais abaixo descrevem o classificador/layout daquele ciclo, não o
> estado atual. Não são evidência de cifra v0x0C validada. A política vigente em
> `AGENTS.md` mantém essa cifra **BLOCKED**; veja o resumo atual no início deste
> documento e `COMPATIBILITY.md`.

**Registro histórico:** naquela revisão, o relatório classificava geometria
BMD como `BLOCKED` para 85,17% dos arquivos e `NOT IMPLEMENTED` para 14,83%. <!-- historico -->
Essa partição foi superada por medições e parser posteriores e não deve ser
usada como status atual.

A observação de então era que 1.396 payloads considerados plaintext seriam
suficientes para desenvolver testes iniciais do parser de corpo v0x0A; isso não
diz nada sobre a cifra v0x0C e não altera o bloqueio atual.

### What has been established about the body, and what has not

Task 1a was started. Full raw output in
[`evidence/ciclo1_bmd_body_analysis.log`](evidence/ciclo1_bmd_body_analysis.log),
over the list of clear files emitted by
`mutool bmdreport <dir> --list-clear` (1,465 entries) so that classification
comes from the tool rather than from a re-derived heuristic.

**Snapshot terminology at cycle 1 (not the current level-2 classification):**

- The then-labelled clear set contained 1,365 `PLAINTEXT` plus 100 `BUX_DECRYPTABLE`. The 100 BUX
  files do **not** begin with `BMD` once decrypted, confirming they are bux
  containers and not models.
- Among that cycle-1 set: 1,194 were v0x0A-labelled and 171 v0x0C-labelled under the then-current classifier.
- **433 of them open with a 32-byte ASCII field holding the source model's
  name**, and all 433 end in `.SMD`. 351 of those contain a path separator, so
  the field holds an authoring path such as `Data2\Item\Bow\...`, not just a
  bare filename. Examples: `Sword40.bmd` opens with `Sword40.SMD`,
  `PrickleGlove_left.bmd` with `PrickleGlove_left.SMD`.

That last point is a real structural finding and it is worth stating precisely:
**433 of 1,465 clear files, not all of them.** The remaining 932 do not begin
with printable ASCII, so the name field is not universal to the format and any
parser must not assume it.

**Not established — layout NOT determined:**

- A search for an (offset, width, record size) triple that divides the payload
  remainder exactly found **no** candidate across all 433 named files.
- A vertex-stride test — longest run of consecutive records whose first three
  floats are plausible coordinates — is **ambiguous**. For `PrickleGlove_left`
  a 12-byte stride wins (1,407 records); for `Sword40` a 48-byte stride wins
  (654). No stride wins on both. Recording either number here would be a guess,
  and a guessed vertex layout produces silently wrong geometry, which is worse
  than no geometry.

**Partially established — a normals array, with evidence, but not a layout:**

Scanning whole files for runs of float32 triples whose Euclidean norm is 1
(tolerance 1e-3) at a 20-byte stride finds, in **16 of 41 named v0x0A files
(39.0%)**, a region where the property holds essentially always — 10 files at
**250/250 = 100.0%**. Mesh normals are unit-length by construction, so this is a
decisive and falsifiable test rather than a plausibility argument.

The test's false-positive rate was measured first, because without it "250 out
of 250" means nothing: over 2,000,000 uniform samples in the accepted range
`[-1.5, 1.5]`, the rate is **4.480e-04**, making 250 consecutive hits a
probability of effectively zero.

What this does **not** establish, and these gaps are the reason no parser
follows:

- The absolute offset ranges from 4 to 358,808 bytes between files and the
  alignment phase varies over 0, 4, 8, 12, 16. That is consistent with a
  variable-length sectioned format but does not say where the normals section
  begins in an arbitrary file. Reaching it requires the header counters, which
  are not decoded.
- **25 of 41 files (61.0%) show no such region above 90%.** Whether that means a
  different layout, quantised normals, or an absent section is unresolved.
- The stride is 20 bytes (5 floats) but only the first three are covered by the
  test. The remaining two floats are **not identified**. Coordinate pair (u, v)
  is the natural hypothesis; it is unverified and must not be assumed.

Classification of that scan alone: **strongly evidenced hypothesis for 39.0% of
named files, not a determined layout.** Raw output in
[`evidence/ciclo1_bmd_normals.log`](evidence/ciclo1_bmd_normals.log).

**Established — a verified header field and record size:**

The scan was then used as an *independent* measurement to test a header field,
which is what turns a hypothesis into a finding. The normals count was **not**
read from the header; it was derived geometrically from the longest run of
unit-norm triples. The two were then compared:

- **The u16 little-endian at body offset 40 is the count of normal records.**
  It equals the geometrically derived count in **145 of 154** testable files
  (**94.2%**).
- **The normals region is exactly `count * 20` bytes** — 145 of 145 of the
  agreeing files, zero exceptions.
- **The record is 20 bytes (5 floats): the first three are the unit normal
  (x, y, z).** The trailing two floats remain **unidentified**.

Because the count comes from a geometric property rather than from the field
being interpreted, the agreement is evidence and not circularity. Raw output in
[`evidence/ciclo1_bmd_normals_field.log`](evidence/ciclo1_bmd_normals_field.log).

So the v0x0A body layout is now known up to this point:

```
offset  size   field
0       32     source model name, ASCII, NUL-padded   (433 of 1,465 files)
32       8     header words, partly undecoded
40       2     u16 normal-record count                (verified, 94.2%)
42      ...    undecoded: spans up to 357,308 bytes before the normals region
?       n*20   normal records, 5 floats each, first 3 unit-length
```

**Still not established, and these are what block a parser:**

- 9 of 154 files disagree (e.g. `Monster290`: derived 599 against u16@40 of
  93). Whether the scan caught a different section or those files use another
  layout is unresolved.
- 46 of 200 files show no detectable run at all; cause not determined.
- The bytes between offset 42 and the start of the normals region are
  undecoded, and that gap ranges from 852 to 357,308 bytes. Reaching the
  normals in an arbitrary file therefore still requires fields this project has
  not read.
- The trailing 2 floats of each record are unidentified. The natural guess is a
  texture coordinate pair, and it was **tested and not supported**: 95.5% of
  those floats fall in `[0, 1]`, but so do **87.2%** of the floats in an
  adjacent equal-sized region where no texture coordinates have any reason to
  be — a ratio of only 1.10x. Arbitrary floats in these files simply land in
  `[0, 1]` often, so the test cannot discriminate. Without that control the
  95.5% would have been reported as confirmation.

### Historical cycle-1 v0x0A body-layout measurements (later extended)

The initial investigation verified two sections with exact arithmetic across
**145 of 145** testable files, zero exceptions. Later cycles added topology and
bone-record parsing; this subsection preserves the original layout evidence, not
the current implementation status. Full initial output in
[`evidence/ciclo1_bmd_layout.log`](evidence/ciclo1_bmd_layout.log).

```
offset    size    field                                status
0         32      source model name, ASCII, NUL-padded verified (433/1465)
32         2      u16, undecoded
34         2      u16, undecoded
36         2      u16, undecoded
38         2      u16 VERTEX COUNT                     VERIFIED
40         2      u16 NORMAL COUNT                     VERIFIED
42        ...     undecoded
52        vc*16   vertex array, 16 B each              VERIFIED
                  [f32 x][f32 y][f32 z][u16][u16]
52+vc*16  nc*20   normal array, 20 B each              VERIFIED
                  [f32 nx][f32 ny][f32 nz][f32][f32]
...               remainder NOT decoded
```

The two predictions that were tested, both falsifiable:

- `52 + u16@38 * 16 == start of the normals region` — **145 of 145 (100.0%)**
- `normals region size == u16@40 * 20` — **145 of 145 (100.0%)**

Both hold because the normals offset and count were derived geometrically first
(unit-norm float triples) and only then compared against the header, so the
agreement is evidence rather than circularity.

Worked example, `Sword40.bmd`: name `Sword40.SMD`, `u16@38 = 88`, `u16@40 = 129`,
vertices at 52..1460 (88*16), normals at 1460..4040 (129*20).

**Still not established:**

- The 2 trailing u16 of each vertex record are unidentified. In `Sword40` the
  first is always 0 and the second varies (670, 681, 0, 512).
- The 2 trailing f32 of each normal record are 0.0000 in `Sword40`. The (u,v)
  reading was tested and **rejected** (see above).
- The section after the normals is **not decoded**. A `nc*8` UV array was
  tested: 92.65% of its floats fall in `[0,1]`, but that is barely above the
  ~87% baseline already measured, and only 28 of 145 files have the whole
  region inside `[0,1]`. Record sizes of 8/12/16/20 bytes were then tested
  against a following face-index array with indices `< max(nc, vc)`: only 12 to
  18 of 99 files passed, so **no size holds**.
- 9 of 154 files disagree on the normal count; 46 of 200 show no detectable
  region.

**Cycle-1 snapshot classification:** vertex positions and normals had exact
verified offsets/counts/record sizes; face indices, texture coordinates and bones
had not yet been decoded at that point. Later work added triangle and partial
bone parsing. Current counts and remaining semantic gaps are in the cycle-17
summary at the top of this document and in `COMPATIBILITY.md`. The layout as a
whole remains **PARTIAL**.

The one paired `.smd`/`.bmd` in the tree, `Object40` (3,847,871 B of plain-text
SMD beside a 1,041,808 B **opaque** BMD), does not currently close the
arithmetic either: 15,390 triangles and 2,234 unique positions leave a remainder
of 58–65 bytes per triangle for every plausible record size. It remains the
best known-plaintext candidate for the cipher itself, and that pairing was
checked explicitly — it is 1 of 10 `.smd` files, not 10 of 10.

### The payload cipher is not recovered — BLOCKED

8,570 of the 10,090 files have a payload with entropy above 7.5 bits/byte.
Sampled in detail at 7.99. What was tested and ruled out:

| Hypothesis | Test | Result |
|---|---|---|
| Repeating XOR, any period 1–32 | Key derived per position class assuming zeros dominate | Best score 0.416 against a random baseline of 0.375 — no signal |
| Repeating XOR, any period up to 600 | Shift-coincidence scan | Peak 0.0056 against a baseline of 0.0039, no periodic spike |
| CFB chaining `p[i] = c[i] ^ c[i-1]` | Decrypt and score | 0.399, entropy 7.83 |
| Rolling subtraction | Decrypt and score | 0.348, entropy 7.81 |
| Index-derived XOR keys (`i`, `7i`, `i^0x5A`, `i%32`) | Decrypt and score | 0.369–0.374, baseline |
| zlib stream | Check for a `78 xx` prefix | No |

The payload is either a stream cipher or a compressed block. Recovering it
requires the loader, which is inside the VMProtect-protected `Main.exe`. Per
`AGENTS.md`, that is out of scope. Status: **BLOCKED**.

What is *not* blocked: the container is parsed, classified, validated and
inventoryed, and the payload is available as opaque bytes.

### Bux-encrypted `.bmd` files that are not models

100 files with a `.bmd` extension are bux-encrypted but contain no `BMD` magic
once decrypted. `Data/gate.bmd` is one: it opens with four repetitions of the
bux key, meaning twelve plaintext zero bytes, and decrypts to a data blob with
no model header anywhere in it.

Detection therefore cannot key on the magic. `mu::BuxDecryptGain()` measures how
much decrypting raises the zero-byte fraction; a real bux file shows a large
positive gain and unrelated data shows none. The threshold is 0.05, and both
directions are tested in `tests/test_assets.cpp`.

---

## The bux cipher — solved

A repeating 3-byte XOR with the key `{ 0xFC, 0xCF, 0xAB }`.

### Evidence

The reference client ships both an encrypted and a plaintext terrain attribute
file, which makes the key directly recoverable.

`Data/World1/Terrain1.att` (encrypted) opens:

```
fc 30 54 f8 cb af f8 cb af f8 cb af f8 cb af f8 cb af f8 cb af f8 cb af f8 cb
```

XOR with `fc cf ab` repeating:

```
00 ff ff 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04
```

`Data/World1/Lorencia.att` (plaintext) opens:

```
00 ff ff 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04
```

Identical. Two independent observations agree: the first three bytes decrypt to
the plaintext file's own header, and the repeating `f8 cb af` decrypts to the
plaintext file's repeating `04`.

A useful property follows: plaintext zero runs encrypt to runs of the key
itself, which is what makes `BuxDecryptGain` work as a detector.

### Scope — an important correction

An early conclusion was that the bux key covered all encrypted terrain files. It
does not. Decrypting 60 `EncTerrain*` files with the bux key and measuring the
density of bytes below `0x40` improved exactly **1 of 60**. The `Enc*` family
uses a different scheme. Measured: `EncTerrain1.att` has entropy 7.9926 and
small-byte density 0.255, which is what uniform random data looks like.

`TerrainTryBuxDecrypt` therefore requires a decisive improvement (gain > 0.10)
before it accepts a decryption, so the `Enc*` family is left untouched rather
than silently corrupted.

Status of `Enc*` terrain: **BLOCKED**, not recovered.

---

## Terrain — `.att`, `.map`, `.obj`

265 files under `Data/World*`. All 265 parse.

### Size families measured

```
.map  196610 bytes   77 files   = 2 + 256*256*3
.att  131076 bytes   68 files   = 4 + 256*256*2
.att   65540 bytes   27 files   = 4 + 256*256
.att   65539 bytes    9 files   = 3 + 256*256
.map  196609 bytes    2 files   = 1 + 256*256*3
```

The constant term is always `256*256`: **every map is a 256x256 tile lattice**,
with 1 or 2 bytes per tile for attributes and 3 bytes per tile for maps. The
small prefix (1–4 bytes) differs per family and is treated as an opaque header
rather than guessed at.

Encryption split over the whole tree:

```
.att  97 encrypted,   8 plaintext
.map  77 encrypted,   2 plaintext
.obj  79 encrypted,   2 plaintext
```

Plaintext detection uses the density of bytes below `0x40`, which measures
0.70–1.00 on real plaintext terrain and about 0.25 on ciphertext. The threshold
is 0.55, with margin on both sides.

`Data/World1/Lorencia.att` measures density 1.000 (plaintext);
`Data/World1/Terrain1.att` measures 0.000 (fully bux-encrypted);
`Data/World1/EncTerrain1.att` measures 0.255 (unknown cipher).

### `.obj` is not a Wavefront object file

Despite the extension, these are the terrain object lists — variable-length
records, no fixed grid. They are classified separately and never forced into the
256x256 model.

---

## TGA

15 files, 7.85 MiB. All 15 parse.

Standard 18-byte header. `mu/assets/tga.cpp` reads uncompressed 24/32-bit
images and **detects but does not decode** RLE (image type 10). None of the 15
files in the reference client use RLE, and shipping an untested RLE decoder
would be worse than reporting the gap. The status code makes the gap explicit.

---

## Formats present but not yet implemented

| Format | Files | Status |
|---|---|---|
| `.ozb` | 100 | Binary height/light maps. Header not resolved — sampled files open with long zero runs, so the structure needs more work. NOT STARTED |
| `.smd` | 10 | Valve StudioModelData, **plain text** (`version 1`, `nodes`, `skeleton`, `triangles`). Trivial to parse; not needed yet |
| `.cg` | 6 | Cg shaders, plain text. Read and analysed in [`RENDERING.md`](RENDERING.md) |
| `.lua` | 22 | Plain text Lua |
| `.ozp` | 1 | Unknown |
| `.cws` | 1 | 92 bytes, unknown |
| `.bmp` | 2 | Standard bitmap |
| `.dat` | 2 | 108 bytes total. `Dec2.dat` is 54 bytes of what appear to be encrypted little-endian floats |

---

## Junk in the asset tree

Not a format, but found by the same scan and worth recording:

```
Thumbs.db        12 files    677,376 bytes
Data/**/*.rar     1 file     1.2 MiB
Data/**/*.zip     1 file    13.1 MiB
Data/**/*.att1    1 file   131,076 bytes   (typo for .att)
```

`Thumbs.db` is a Windows Explorer thumbnail cache. It leaks the directory
listing of whoever packaged the client and serves no purpose in a distribution.
`mutool scan` counts these separately so they can be excluded from a build.

---

# Anexo histórico do ciclo 2 — afirmações sobre v0x0C e layout (não autoritativo)

Este anexo preserva as medições e decisões registradas naquela fase; **não**
substitui o resumo atual no início deste documento. A política vigente em
`AGENTS.md` mantém a cifra BMD v0x0C **BLOCKED**. A função e os rótulos existentes
são observações da implementação/classificador, não prova de cifra validada,
autorização para continuar a engenharia reversa ou compatibilidade semântica.
Use as tabelas iniciais e `COMPATIBILITY.md` para o estado atual.

## Registro histórico da transformação v0x0C — status atual: BLOCKED

```
plain[i] = (cipher[i] ^ key[i & 15]) - running
running  = cipher[i-1] + 0x3D        running inicial = 0x5E

key = D1 73 52 F6 D2 9A CB 27 3E AF 59 31 37 B3 E7 A2
```

XOR rolante sobre uma tabela de 16 bytes combinado com uma chave corrente
derivada do byte de **texto cifrado** anterior. Por isso **não é auto-inversa**
como o XOR-3 do bux. Determinística, passada única, O(n).

**Proveniência: projeto de terceiros.** Algoritmo e tabela vêm de
`xulek/muonline-bmd-viewer` (licença ISC), reimplementados aqui. Ver
`docs/LICENSING.md` e `docs/DECISIONS.md` (DEC-007).

**Registro histórico do classificador:** a implementação existente aplica um
filtro de plausibilidade (nome de 32 bytes ASCII imprimível) e classificou 7.226
de 8.741 arquivos v0x0C com esse critério; 1.519 ficaram OPAQUE. Isso não
valida a cifra ou o filtro, não demonstra semântica, e não autoriza ampliar esse
trabalho. `AGENTS.md` mantém a cifra BLOCKED.

## Layout do corpo — PARCIAL, com uma fonte externa que NÃO se confirmou

### Cabeçalho do corpo (deslocamentos de *payload*, não de arquivo)

O payload começa depois do cabeçalho de 4 bytes `"BMD"` + versão. **Todo
deslocamento abaixo é relativo ao início do payload.** Confundir as duas bases
desloca cada leitura em 4 bytes — foi exatamente esse erro que produziu uma
revisão inteira de resultados falsos neste ciclo, registrada mais adiante.

```
32  u16 meshCount           mediana 1, max 18, zero em 3 arquivos
34  u16 boneCount
36  u16 actionCount
38  u16 numVertices         mediana 68,  max 5.082
40  u16 numNormals          mediana 69,  max 5.376
42  u16 numTexCoords
44  u16 numTriangles
46  u16 textureIndex
48   2  u16, provável contagem — 0 em 57,4%, depois 1, 2, 4, 3 (2.500 arquivos)
50   2  u16, parcialmente padding — 0 em 60,0%; 0xCDCD em 62 arquivos
52  vc*16  array de vértices
```

As contagens em 38 e 40 foram verificadas por este projeto em 145/145 arquivos
testáveis. Os nomes dos campos em 32..46 vêm do projeto ISC e são **coerentes**
com as distribuições medidas, mas não foram verificados um a um.

### Registros — medidos, e DIFERENTES da fonte externa

| Registro | Tamanho | Conteúdo |
|---|---|---|
| Vértice | 16 B | `f32 x` + `f32 y` + `f32 z` + `u16 aux0` + `u16 aux1` |
| Normal | 20 B | `f32 nx` + `f32 ny` + `f32 nz` + `f32 aux0` + `f32 aux1` |
| Triângulo | 64 B | 3 referências de 8 B em +0/+8/+16: `u16 vértice` + `u16 normal` + `u16 opcional` + `u8 flag` + `u8 pad` |
| Osso | não verificado neste projeto | — |

**As coordenadas vêm primeiro nos dois registros.** Evidência, medida sobre os
1.194 arquivos v0x0A:

- Normais lidas no deslocamento interno **0**: 260.030 de 260.268 com norma 1 =
  **99,91%**. No deslocamento 4: 84.611 = **32,51%**.
- No registro de vértice, o `u16` no offset **12** é menor que 200 em **99,94%**
  dos registros e o do offset **14** em **95,53%**. Nos offsets 0 e 2, as cifras
  são 2,63% e 0,82%. Os inteiros pequenos ficam no fim.

**Correção de uma identificação que este projeto fez sem base.** Esses dois
campos foram nomeados `node` e `bindVertex` — índice de osso e de vértice — por
serem inteiros pequenos. Medido em 358 arquivos, o `u16@12` tem mediana **1**,
p99 **68** e **máximo 88**. `boneCount` nesses arquivos é tipicamente 1 a 8.
**Um índice de osso não chegaria a 88**, então `node` está errado.

Também não é índice de UV: testado contra `numTexCoords`, dá 99,89% de validade,
mas o teste não discrimina — `numTexCoords` tem mediana 59 e o campo é quase
sempre menor que 200, então a validade vem da aritmética, não de ser índice.

**Estado dos dois campos: inteiros pequenos, sem identificação.** Voltam a
`aux0`/`aux1` no código, que é o que a evidência sustenta. Nomear um campo é uma
afirmação; sem evidência, o nome vira fato falso que outros passos passam a
assumir.

O projeto `xulek/muonline-bmd-viewer` inicia cada registro com índice de osso e
padding, colocando as coordenadas no deslocamento 4. **Isso não se aplica a
estes arquivos.** O código-fonte de terceiros descreve sua própria transformação;
este projeto não a valida como prova sobre os arquivos v0x0C. As tabelas de
layout citadas também não foram confirmadas aqui.

### Duas correções de afirmações deste projeto

1. **Os dois `f32` finais da normal NÃO estão confirmados como (u,v).** Uma
   versão anterior deste documento afirmou que eram, citando a fonte externa.
   Como o layout de registro daquela fonte não corresponde aos dados, essa
   citação perdeu a base. O que se sabe: 95,5% dos valores estão em [0,1] contra
   87,2% num controle uniforme — razão 1,10×, insuficiente para concluir.
   Seguem como `aux0`/`aux1` sem nome, deliberadamente.

2. **A varredura de UV que deu razão 0,146 a 0,824 continua inconclusiva.** A
   hipótese de corpo organizado por malha permanece plausível, mas não foi
   demonstrada; foi inferida da mesma fonte cujo layout de registro falhou.

### Nova evidência de alinhamento (2026-10-13)

A inspeção executada pelo usuário no arquivo local `agnecklace.bmd` revelou que
o parser anterior começava o registro de triângulo quatro bytes tarde. O registro
real começa no offset **1044 do arquivo** (offset **1040 do payload**) e seus
campos decodificados são: tamanho de polígono 3; índices de vértices 0, 1, 2;
índices de normais 0, 1, 2; índices candidatos de UV 0, 1, 2. Os registros têm
64 bytes. No registro, o tamanho fica em +0, os índices de vértices em +2/+4/+6
(+8 para o quarto canto), os índices de normais em +10/+12/+14 (+16), e os
índices candidatos de UV em +18/+20/+22 (+24).

A causa do deslocamento é que o parser usa cursores que começam nos valores
float dos registros de vértice/normal, quatro bytes depois do início do registro.
Ao calcular o array seguinte, o cursor deve ser corrigido por -4. O parser agora
faz esse ajuste, lê os índices paralelos e divide quadriláteros em dois
triângulos. A estrutura UV candidata ainda **não** está semanticamente validada.

**Escopo da evidência:** um arquivo real inspecionado e fixtures sintéticas
adicionadas ao teste; não é uma varredura completa da árvore. As contagens e
percentuais do ciclo 17 abaixo são uma linha de base do parser anterior e precisam
ser recalculados depois que `make`, `make test`, `make test-san` e
`python3 tools/check_docs_consistency.py` forem executados na cópia atualizada.
Até lá, os resultados novos de compilação/testes estão **NOT EXECUTED**.

### Interpretação anterior do registro (histórica; supersedida pela evidência acima)

> **Correção do ciclo 3.** Este título dizia "conteúdo interno NÃO DETERMINADO" e
> o corpo dizia que a leitura era prova de conceito em Python. **Ambos estavam
> defasados.** O registro de triângulo foi decodificado (commit `03052f0`) e o
> parser C++ lê o array desde o commit `bee2a5e`. Há testes (`TestBmdTriangles`,
> `TestMeshFromBmd`) e o nível 4 do `mutool bmdreport` sai do parser, não de
> script. A evidência completa está no item 1 do relatório de ciclo 3.
>
> O que **não** foi determinado é o **índice de UV por canto** — outra coisa, e
> o único bloqueio restante para texturizar.

O array de triângulos foi localizado com controle estatístico, o que este projeto
não havia conseguido nas tentativas anteriores.

**Localização, para arquivos de malha única:**

```
tstart = 52 + numVertices*16 + numNormals*20 + numTexCoords*8
stride = 64 bytes por triângulo
índices de vértice em +2, +6 e +10 dentro de cada registro
```

**Evidência** (reproduzível por `python3 tools/bmd_topology_probe.py <Data>`,
saída em [`evidence/ciclo2_bmd_topology.log`](evidence/ciclo2_bmd_topology.log)):

| Medida | Resultado |
|---|---|
| Hipótese — índices em +2/+6/+10 | 134.524/137.205 = **98,05%** |
| Controle limpo — região `f32` de vértices | 5.961/254.025 = **2,35%** |
| Acaso — `vc/65536` médio | **0,71%** |
| Razão hipótese/acaso | **138,3×** |
| Razão hipótese/controle | **41,78×** |
| Nome de textura no ponto previsto | 515/586 = **87,9%** |

A última linha é uma confirmação **independente**: se o stride de 64 estiver
errado, o nome da textura não cai logo depois do último triângulo. Ela não
depende de interpretar índice nenhum.

**Por que as tentativas anteriores falharam.** Elas buscavam o array de faces
ancorando em `uv_end` derivado da hipótese `nc*8` sem confirmá-la, e testavam
stride 8/10/12/14/16 bytes. O stride é 64, muito maior que qualquer um testado.

### Registro de triângulo — DECODIFICADO em sua parte essencial

Avanço posterior à localização. A varredura dos 64 bytes por deslocamento, com o
mesmo controle limpo, mostrou que os campos `u16` reais estão em deslocamentos
**pares** — os ímpares (~67%) são leituras sobrepostas dos mesmos valores. Isso
revelou a estrutura: **três referências de 8 bytes** no início do registro.

```
+0   u16 índice de vértice      < numVertices  em 97,66% dos casos
+2   u16 índice de normal       < numNormals   em 97,20%
+4   u16 campo opcional         mediana 0, máximo 65535 — NÃO é índice de UV
+6   u8  flag
+7   u8  padding
     (repete em +8 e +16: são os três vértices do triângulo)
+60  u16 não decodificado       valor 3 em 53,3% dos registros
+62  u16 não decodificado       mediana 24
```

**Isto é topologia utilizável.** Um triângulo é lido como três pares
(vértice, normal), que é o suficiente para montar a malha.

**O que continua não decodificado, sem eufemismo:**

- O campo em +4 — **resolvido como padding, não como índice**. A distribuição
  medida em 40.902 amostras é 64,95% zero e **18,18% o valor 0xCDCD**, que é o
  padrão de preenchimento de memória não inicializada do MSVC. Excluindo essas
  sentinelas, o campo é menor que `numTexCoords` em apenas **5,33%** dos casos.
  Não é índice de coisa nenhuma: é padding que o exportador não zerou.

  A leitura anterior ("campo opcional com sentinela −1, mediana 0, máximo
  65535") era uma inferência que os dados não sustentam. O máximo 65535 vinha de
  leituras desalinhadas, não de uma sentinela real.
- **The UV-per-corner index remains unlocated.** The +4 field was a natural
  candidate and was rejected. The byte region calculated as a coordinate-array
  candidate is readable, but its semantic role has not been verified; see below.
- Os bytes 24..59. Todos zero em apenas 40,9% dos registros, então há conteúdo
  ali que não foi identificado.
- Os `u16` em +60 e +62.
- O primeiro byte **não** é tamanho do polígono: valores 3 e 4 somam 3,01%,
  indistinguível de aleatório. As tabelas do projeto ISC não se aplicam.

**Historical measurement limitation.** The 97.66%/97.20% rates came from
58,836 corner references in 319 single-mesh files and were limited to the first
80 triangles per file. Older full-scan notes quoted 8.357% and 9.517% using a
different denominator/measurement version; those figures are not comparable to
the current six-field accounting and are not current rates. The old statement
that roughly one in twelve references would be read out of bounds is withdrawn.

**Current cycle-17 accounting** covers 3,681,664 triangle records and six
vertex/normal reference fields per triangle: 1,030,785/22,089,984 = 4.666% of
fields are out of range. Mesh import separately drops 513,304/3,681,664 triangles
(13.94%). The importer validates references and drops each affected triangle
once. Current full-tree evidence is linked near the top of this document and in
`COMPATIBILITY.md`.

### Candidate coordinate array — semantic role UNVERIFIED

```
calculated_start = 52 + numVertices*16 + numNormals*20
ntc candidate entries of 8 bytes each, interpreted in the investigation as [f32 u][f32 v]
```

The body parser uses header counts to locate later regions, but it does not
expose or semantically validate these candidate values as texture coordinates.
The investigation below measured finite-float/range distributions in selected
files. Those statistics can support a hypothesis that the bytes contain floats;
they cannot establish that the values are UVs, that the calculated layout is
universally correct, or which coordinate belongs to a triangle corner.

**Evidência** (250.450 leituras em 319 arquivos de malha única):

| Medida | Resultado |
|---|---|
| Leituras `f32` não-finitas | **0** de 250.450 |
| Esperado por acaso (expoente todo-1 = NaN/Inf, ~0,39%) | **~976** |
| Componentes em [0,1] | 245.773/250.450 = **98,13%** |
| Distribuição | mín −0,4266 · p1 0,0083 · mediana 0,5058 · p99 1,5518 · máx 1,9980 |

**Zero non-finite values in 250,450 reads is evidence about representation, not
meaning.** Under the stated random-byte model, arbitrary bytes interpreted as
`f32` would produce NaN/Inf in roughly 0.39% of cases (about 976 here). This
supports a float-like interpretation for the candidate region, but it does not
identify the semantic attribute.

The measured range is concentrated in [0,1] with a tail to about 2.0. That is
compatible with texture tiling, but also does not distinguish UVs from another
normalized float attribute. No material/UV association or rendered comparison
was made; classify this as a candidate only.

**Um controle que falhou e precisa constar.** Deslocar a leitura 4 bytes deu
97,91% em [0,1] contra 98,13% da hipótese — razão 1,00×, sem poder
discriminatório. O controle estava contaminado: deslocar dentro de um array de
pares `f32` ainda lê floats válidos do par vizinho. É a mesma armadilha do
controle dentro do array de triângulos. **Controle válido precisa sair da região,
não se deslocar dentro dela.**

**What remains blocked.** The candidate byte region is readable, but the parser
does not validate it as a texture-coordinate array and **the index that would
map each triangle corner to an entry has not been located**. The current importer
therefore has no verified BMD UVs and cannot produce a verified textured model.

Três hipóteses para essa ligação foram testadas e **todas descartadas**:

1. **Campo em +4 do registro de triângulo.** É padding: 64,95% zero e 18,18%
   0xCDCD (preenchimento de memória não inicializada do MSVC). Excluindo
   sentinelas, menor que `numTexCoords` em só 5,33%.
2. **Um quarto canto em +24.** Não existe: offsets +24 a +56 dão 50–81% de
   validade de índice, o nível de leitura contaminada, contra 94–100% em
   +0/+8/+16.
3. **Relação implícita por posição.** Descartada: `numTexCoords` é igual a
   `numVertices` em apenas 5,74% dos arquivos de malha única e a `numNormals` em
   3,87% — medido em 802 arquivos. Em 93% dos casos difere de ambos, então o
   índice de UV não pode ser o de vértice nem o de normal.

**Quarta hipótese testada e descartada: os bytes 24..59.** Varridos os 39
deslocamentos de 24 a 62 medindo a fração de `u16` abaixo de `numTexCoords`, com
controle limpo (mesma leitura sobre o array `f32` de UV, 5,50%; acaso 0,15%).

O resultado **não é um pico, é um platô**: todos os deslocamentos de 24 a 62
ficam entre 40% e 94%, com razão de 7,3× a 17,1× sobre o controle, variando de
forma gradual. Não há nenhum deslocamento que se destaque.

Isso é assinatura de **teste sem poder discriminativo**, não de um campo real. A
comparação é direta: quando o índice de vértice foi procurado no mesmo registro,
os deslocamentos +0/+8/+16 deram 94–100% e os vizinhos caíram — um pico nítido.
Aqui não há pico, então não há campo.

A explicação mais provável para o platô é que esses bytes contenham **dados em
ponto flutuante**, não índices: interpretar `f32` como `u16` produz valores
pequenos com frequência, porque o expoente ocupa os bits altos. Isso é coerente
com o platô gradual e incompatível com um array de índices.

**Estado do índice de UV: NÃO RESOLVIDO.** Quatro caminhos fechados:

| Hipótese | Resultado |
|---|---|
| Campo em +4 do registro | padding (64,95% zero, 18,18% 0xCDCD) |
| Quarto canto em +24 | não existe (nível de ruído) |
| Relação implícita por posição | refutada (93% dos arquivos diferem) |
| Bytes 24..59 | platô sem pico; provavelmente `f32`, não índices |

**A explicação foi confirmada: os bytes 24..59 são ponto flutuante.** Lidos como
`f32`, dão **0,055%** de não-finitos (68 em 122.706 leituras) contra **0,39%**
esperado por acaso — seriam ~478. Bytes arbitrários não produzem esse resultado.
O controle (bytes 0..23, sabidamente índices inteiros) dá 0,027%, também abaixo
do acaso, porque inteiro pequeno interpretado como `f32` é finito.

Isso fecha a questão: **não há índice de UV nos bytes 24..59 porque ali não há
índices.** O que são esses floats — vetor de luz, coordenada de lightmap,
atributo por face — não foi determinado.

**O segundo caminho também foi examinado.** Os 4 bytes do cabeçalho em payload
48..51, medidos em 2.500 arquivos:

| Campo | Distribuição |
|---|---|
| `u16@48` | zero em 57,4%; depois 1 (177×), 2 (96×), 4 (82×), 3 (63×); máximo 57.856 |
| `u16@50` | zero em 60,0%; 0xCDCD em 62 arquivos; máximo 52.685 |

`u16@48` é dominado por inteiros pequenos — assinatura de **contagem**, não de
índice. `u16@50` contém 0xCDCD, o preenchimento de memória não inicializada do
MSVC, então é ao menos parcialmente padding. **Nenhum dos dois é índice de UV.**

### Índice de UV por canto — NÃO RESOLVIDO, investigação encerrada

**Sétima e última hipótese testada: array paralelo de `ntri` entradas fora do
registro.** Varrido o corpo a partir do fim do array de triângulos, em passos de
2 bytes, procurando um bloco de `ntri` valores `u16` todos menores que
`numTexCoords`. Resultado em 161 arquivos: encontrado em **19 = 11,8%**, com
distâncias dispersas (84, 66, 40, 636, 3252, 2650, 42, 10) e **nenhum offset
consistente**.

Um array paralelo real apareceria na mesma distância em quase todos os arquivos.
A dispersão mostra que os 19 acertos são coincidência — blocos de valores
pequenos que existem por acaso, não uma estrutura.

**Investigação encerrada por critério, não por falta de tempo.** Onze hipóteses
descartadas em sequência (9 iniciais ciclo 3 + 2 novas ciclo 9 Fase A):

| # | Hipótese | Resultado |
|---|---|---|
| 1 | Campo em +4 do registro | padding — 64,95% zero, 18,18% 0xCDCD |
| 2 | Quarto canto em +24 | não existe — nível de ruído |
| 3 | Relação implícita por posição | refutada — 93% dos arquivos diferem |
| 4 | Bytes 24..59 do registro | platô sem pico; confirmado serem `f32` |
| 5 | `u16@48` do cabeçalho | contagem, não índice |
| 6 | `u16@50` do cabeçalho | parcialmente padding (0xCDCD) |
| 7 | Array paralelo no corpo | 11,8% com distâncias dispersas — coincidência |
| 8 | `u16@12`/`@14` do vértice | teste não discrimina — ambos preveem inteiros pequenos |
| 9 | Bytes 24..59 como UV por canto | 79,42% contra controle de 70,34% — razão 1,13× |
| 10 | Espaço entre fim triângulos e início ossos | 0 bytes — textura 32B em tri_end, ossos em tri_end+32 (single) ou tri_end-4+32 (multi) +4 extra, sem sobra |
| 11 | UV derivado aritmeticamente de vertex/normal | não igual, ntc mediana 59 ≠ vc, 60% < ntc mas não 100% |

**Ciclo 10 Fase A — tabela ratio-por-offset UV esclarecendo platô 9-26× (evidência `ciclo10_itemA4_uv_ratio_per_offset.log`):**

Full-scan 4149 single-mesh com ntri>0 ntc>0, cada offset 0..62 como u16 < ntc, controle = mesmo offset %20 no array de normais como u16 (mesma classe).

| Offset | Rate real | Rate controle | Ratio | Interpretação |
|---|---|---|---|---|
| 0 | 61,80% | 4,80% | 12,89× | vertex index (< vc, mas 61% < ntc porque ntc mediana 59 similar vc) |
| 2 | 61,87% | 3,48% | 17,78× | normal index |
| 4 | 84,02% | 4,46% | 18,82× | optional field mediana 0, 84% < ntc |
| 24 | 84,23% | 4,46% | 18,87× | float bytes como u16 — platô |
| 26 | 84,45% | 3,20% | 26,43× | float bytes como u16 — platô |
| 60 | 83,26% | 4,80% | 17,36× | tri_end-4 awareness, mesmo platô |

Platô explicado: bytes 24..59 são 9 floats, quando lidos como u16 dão valores pequenos (< ntc) com alta frequência porque floats pequenos têm bytes baixos (0.0 = 0x00000000 → u16 0 < ntc). Controle normais também são floats, mas distribuição diferente (normais norma 1, componentes 1.0 = 0x3F800000 → u16 0x8000=32768 > ntc, taxa menor). Por isso ratio 9-26× aparece em muitos offsets, não indica posição específica UV, e sim artefato de floats pequenos. Offsets 12,14,16,18 têm rate 100% real e 63-99% controle ratio ~1×, porque são bytes de float 1.0 que como u16 dão 0 ou 0x8000 etc. Nenhum offset tem pico único discriminativo; UV permanece NÃO LOCALIZADO.

### Nona hipótese descartada — bytes 24..59 como coordenadas UV por canto

Testada no ciclo 3, e é genuinamente diferente das oito anteriores: não procura um
**índice** para o array de UV, procura as **próprias coordenadas** dentro do
registro de triângulo. Os bytes 24..59 são 36 bytes = 9 floats, e nunca haviam sido
testados para isso.

**The comparison uses the same numeric range statistic:** fraction of values in [0,1]. The candidate array is not semantically validated as UV.

| Medição | % em [0,1] |
|---|---|
| Bytes 24..59 do registro de triângulo | **79,42%** (217.160 de 273.429) |
| **Controle: array de normais** (f32 reais, sabidamente não UV) | **70,34%** |
| Controle: bytes aleatórios como `f32` | 24,82% |
| **Candidate float region (UV role unverified)** | **98.13%** |

**A razão sobre o controle válido é 1,13×.** Dados `f32` ordinários já caem em
[0,1] em 70% dos casos, porque valores normalizados são comuns em geometria. Os
79,42% estão pouco acima do controle da mesma classe e abaixo dos 98,13% observados na região candidata. Como essa região não foi verificada semanticamente como UV, a comparação não identifica coordenadas de textura.

A distribuição por posição também não ajuda: os nove floats vão de 72,07% a
85,78%, sem o pareamento `(u, v)` que coordenadas de textura produziriam.

**Veredicto: rejeitada.** É a nona hipótese. O que estes floats são continua não
determinado — a única coisa estabelecida é que são `f32` (0,046% de não-finitos
contra 0,39% do acaso) e que não são UV.

**Nota sobre o controle.** O primeiro controle tentado foi bytes aleatórios, que
deu 24,82% e faria os 79,42% parecerem fortes (razão 3,2×). **Esse controle é
inválido aqui**, porque a pergunta não é "estes bytes são dados ou ruído" — já se
sabia que são `f32`. A pergunta é "estes `f32` são UV", e o controle correto é
outro `f32` que não seja UV. Usar o controle errado teria produzido um
falso-positivo, que é exatamente o erro que este projeto cometeu com os 95,5% em
[0,1] no ciclo 2.

Sete falhas seguidas são evidência de que **a pergunta pode estar mal formulada**,
não de que falta tentar mais uma vez. As possibilidades que este projeto não
conseguiu formular incluem: o índice de UV ser derivado aritmeticamente dos
outros índices; não existir UV por canto neste formato, com texturização feita
por outro mecanismo; ou `numTexCoords` não significar o que se assumiu.

**Consequência prática, declarada:** os 8.401 arquivos com vértice e normal e os
8.400 com topologia **não podem ser texturizados**. Malha sem textura pode servir
para colisão, caixa delimitadora e análise, não para renderização fiel.

### Ossos — localização PARCIAL, 73,7% → layout final 37,66% (ciclo 8)

**Histórico.** Primeiro avanço: após o array de triângulos vem o nome da textura
(32 B) e depois os nomes de osso, também em blocos de 32 B. `boneCount` é o `u16`
no offset de payload **34**. Em 468 arquivos de malha única, 345 = 73,7% tinham
`boneCount` nomes ASCII válidos a partir de +32 (`Box01`, `Bip01`, etc.). Em 26,3%
a leitura não batia.

**Correção intercalado.** A leitura inicial assumiu que todos os nomes vinham
primeiro e as transformações depois. Dump mostrou o contrário: logo após o
primeiro nome aparece outro nome (`t04`), não dados numéricos. **Os registros são
intercalados.**

### Registro de osso — 59 B intercalado, confirmado ciclo 8

Layout final medido e implementado em `src/mu/assets/bmd_body.cpp`:

```
single-mesh:  bone_base = triangles_end + 32
              cada osso exceto último: 32 B nome NUL-padded + u16 parent (0xFFFF=root) + 6×f32 + 1 B pad 0 = 59 B
              último osso: 32 + 2 + 24 = 58 B (sem pad)
              total: boneCount × 59 − 1

multi-mesh:   bone_base = offset após último nome de textura (busca delta -4/0/4)
              extra 4 B = u32 actionCount quando remain == 4+bc*59-1 e extra == ac ou <256
              total: 4 + bc×59 − 1
```

**Evidência ciclo 8 (hex dump bruto):**

- `Box01.bmd` bc=1 remain=58 `Box01\0... FFFF 00000000 BD378635...` parent 65535, floats `0.0, 1e-06, 0.0, 1.57, 0,0` last byte 0 → 58 =32+2+24 sem pad.
- `Sapiatttres2.bmd` bc=2 remain=117 bone0 len59 `zx01` parent FFFF pad 00, bone1 len58 `Object04` parent0 → prova último sem pad, total bc*59-1 =117 =59+58.
- `PrickleGlove.bmd` (multi, mc=4, bc=1, ac=1) len 78732 base 78670 remain 62 extra 1 bone `PrickleGlo` parent FFFF floats `0,-5e-06,2e-06,-1.57,1.57,0` → valida multi com extra 4B ac, remain 62 =4+58.

**Transformação decodificada em 174 ossos (ciclo 7) e confirmada em 8.001 ossos (ciclo 8):**

```
+0   2   u16  índice do osso pai      65535 = sem pai (raiz)
+2  24   6× f32                       100% finitos (ciclo 8: 100% dos 3.800 arquivos)
+26  1   u8   sempre 0 (174 de 174 ciclo7, 100% ciclo8 para rec_size 59)
```

Distribuição parent em 174 ossos ciclo7: 65535 132 (75,9%) raiz, 0 27 (15,5%), etc.
Em 8.001 ossos ciclo8: root 5.018, parent válido 8.001 (100%).

**Critério de parsing ciclo 8:** `boneCount` 1..64, `base+bc*59-1 <= size` ou `base+bc*59 <= size`,
cada osso com pelo menos 3 chars printable em 32 B (loose) e todos os 6 f32 finitos e parent
em [0,bc) ou 0xFFFF. Requer que todos os ossos tenham nome loose (>=3 printable) para evitar
falso-positivo em payload aleatório (controle: random daria ~0% nomes).

**Medição ciclo 8 (full-scan, `mutool bmdreport` + `bone_measure2`):**

- total .bmd 10.090, geom parsed 8.406 (83,31%)
- with bones parsed **3.800 (45,21% de geom, 37,66% de total)** — era 3.505 (34,74%) no ciclo 7
- total bones 8.001, nomes válidos 8.001 (100% loose), parent root 5.018, parent válido 8.001
- multi-mesh contribui: `Sword37` bc2 remain121 extra1, `Sword42` bc6 remain357 extra1, etc.
- Custom bc51 remain 2698 vs expected 3008 diff -310 → filtrado (não segue 59B), permanece fora do nível 5.

**Medição ciclo 10 Fase A (após detecção de ciclo, full-scan `mutool bmdreport` + `/tmp/validate_A1_fixed`):**

- total .bmd 10.090, geom parsed 8.406 (83,31%)
- with bones loose **3.693 (43,93% de geom, 36,60% de total)** — era 3.800 ciclo 8, -107 filtrados por detecção de ciclo A->B->A e self-loop parent==0
- total bones loose 7.879, parent válido 100%, ciclo detection rejeita A->A e A->B->A
- with bones strict **1.186 (14,11% de geom, 11,75% de total)** NUL-padded ASCII >=3 + zeros após NUL, headline conservador, taxa falso-positivo 0/25218, limite superior ~0,004% (1/25218), ratio >3558× per file (controle 0/25218), strict ⊂ loose provado 0 falhas (evidência `ciclo10_itemA3_strict_subset.log`)
- ratio per file corrigido mesma unidade: loose 43,93% / 0,0238% = **1846,5× altamente discriminativo** (antes 5,43× marginal com unidades diferentes e sem ciclo detection que dava 8,32% falso-positivo), strict taxa 0/25218 limite superior ~0,004% ratio >3558×
- multi-mesh e bc>8 mantidos: PrickleGlove mc4 bc1 strict, Sword48 bc10, Statue_Blue bc15, etc, todos com ciclo detection válido
- Custom bc51 permanece filtrado

**O que permanece não determinado:**

- Ordem e sistema de coordenadas dos 6 floats (inferido xyz pos + xyz rot, 1.57 rad ≈ 90° observado).
- Se rotação é graus/radianos e convenção Euler/quaternion.
- Seção de animação após ossos: `boneCount × keyframes × 12` bytes (3×f32), 500 arquivos, 385 divisível 77% — **não decodificada**.
- Por que último sem pad (59→58) e por que extra 4B ac em multi.

**Estado ciclo 8: hierarquia de ossos decodificada em 37,66% da árvore (3.800 arquivos), 45,21% dos
arquivos com geometria. Skinning ainda BLOQUEADO — falta animação e confirmação de ordem dos floats,
mas parser já expõe `BmdBone {name, parent, raw[6], has_name, floats_finite}` em `bmd_body.hpp`.

**Estado ciclo 10 Fase A (após detecção de ciclo e correção ratio per file):** hierarquia decodificada em 36,60% da árvore (3693 loose, 7879 bones) com ratio per file 1846× altamente discriminativo (antes 5,43× marginal), strict 11,75% (1186) headline conservador taxa falso-positivo 0/25218, limite superior ~0,004% (1/25218), ratio >3558×, strict ⊂ loose provado, parser com detecção de ciclo A->B->A e self-loop, 389 checks 0 falhas, ASan 0. Skinning ainda BLOQUEADO — falta animação e confirmação ordem floats, mas base para Fase B OPAQUE/animação.

  | Sobra | Arquivos |
  |---|---|
  | 26 bytes | 627 |
  | 53 bytes | 60 |
  | 368 bytes | 23 |
  | 80 bytes | 9 |

  **A conta fecha com o modelo `27 × boneCount − 1`**, equivalente a 26 bytes
  para o primeiro osso e 27 para cada osso seguinte. Verificado agrupando a sobra
  por `boneCount` em 1.525 arquivos:

  | `boneCount` | Sobra modal | Arquivos | Previsto `27·bc−1` | Bate? |
  |---|---|---|---|---|
  | 1 | 26 | 1.222 de 1.328 | 26 | **sim** |
  | 2 | 53 | 82 de 93 | 53 | **sim** |
  | 3 | 368 | 23 de 50 | 80 | não |
  | 4 | 107 | 3 de 13 | 107 | **sim** |
  | 5 | 134 | 5 de 7 | 134 | **sim** |
  | 6 | 1.745 | 8 de 13 | 161 | não |
  | 8 | 215 | 3 de 7 | 215 | **sim** |
  | 9 | 242 | 2 de 2 | 242 | **sim** |
  | 11 | 296 | 2 de 4 | 296 | **sim** |

  **1.319 de 1.525 = 86,5%** dos arquivos seguem o modelo na moda de cada
  contagem.

  Os que divergem têm sobra muito maior que o previsto (bc=3 → 368 em vez de 80;
  bc=6 → 1.745 em vez de 161). O padrão é coerente com **presença de dados de
  animação** nesses arquivos, que vêm depois das transformações. Isso é hipótese,
  não resultado.

  **27 bytes por osso não se decompõe em floats de forma inteira**: 6×4 = 24,
  sobram 3. A decomposição mais plausível é `u16` índice do osso pai + 6 floats
  (24 B) + 1 byte não identificado. **Hipótese, não resultado** — o byte extra e
  o `−1` do modelo não foram explicados.
- A hierarquia pai/filho.
- A seção de animação, que vem depois dos ossos.
- Por que 26,3% falham.

**Estado: PARCIAL.** Nomes de osso localizados na maioria dos arquivos;
transformações, hierarquia e animação não determinados. Skinning continua
**BLOQUEADO** — exige transformações, não nomes.

### Seção de animação — localizada, NÃO decodificada

Existe conteúdo após o último osso nos arquivos cuja sobra excede
`boneCount × 59 − 1`. Exemplo medido: `Rank01.bmd` tem `boneCount=3`,
`actionCount=1` (o `u16` em payload 36), sobra de 464 bytes contra 176 previstos
— **288 bytes de animação**.

Os primeiros bytes após os ossos:

```
00 00 00 00 00 00 00 00 00 00 00 ff ff
26 1e d0 bc  bd 37 86 35  df 45 82 c1
26 1e d0 bc  bd 37 86 35  8e d1 4e c1
26 1e d0 bc  bd 37 86 35  d4 e8 3c ..
```

Três observações, todas registradas como observação e não como resultado:

1. Há um `ff ff` (65535) após 13 bytes zero — o mesmo valor que marca "sem pai"
   na transformação de osso, o que sugere outra sentinela de fim de lista.
2. Depois dele, grupos de **três `f32`** em que **os dois primeiros se repetem
   idênticos** (`26 1e d0 bc bd 37 86 35`) e **o terceiro varia** a cada grupo.
   Esse padrão é coerente com keyframes de animação, onde dois componentes
   permanecem e um avança.
3. O mesmo arquivo da série `Rank0N` produz bytes idênticos nesta região, o que é
   esperado se compartilham o esqueleto.

**O tamanho do registro foi medido sobre a árvore completa** (`mutool bmdanim`,
evidência em `docs/evidence/ciclo2_bmdanim.log`). Em **500 arquivos** com conteúdo
após os ossos, os bytes extras são divisiveis por `12 × boneCount` em
**385 = 77,00%**:

**Correção de uma generalização.** A primeira medição, feita por script com teto de
3.000 arquivos varridos, encontrou 88 arquivos com animação e deu **86,4%**. Sobre
a árvore completa são **500 arquivos e 77,00%**. O modelo se sustenta, mas o número
publicado era de um subconjunto. É o quinto caso deste ciclo em que medir em
subconjunto produziu número errado — os anteriores foram o cabeçalho BMD, o array
de UV, o registro de triângulo e a suavidade de altura. Por isso a medição virou
comando: um script com teto não declara o teto na saída, e o número parece geral.

```
animação = boneCount × keyframes × 12 bytes
12 bytes = 3 × f32
```

Keyframes por osso, nos 385 que fecham (trecho; a lista completa está na
evidência):

| Keyframes | Arquivos |
|---|---|
| 8 | 28 |
| 14 | 28 |
| 12 | 19 |
| 10 | 11 |
| 16 | 8 |
| 4, 6, 15 | 1–2 cada |

A concentração em valores pequenos e pares é coerente com comprimentos de
animação; valores isolados altos provavelmente são arquivos com mais de uma ação,
onde o modelo de uma ação só não se aplica.

**Os 115 arquivos de 500 cujo extra não é divisível** não foram investigados.
Podem ter mais de uma ação, ou o modelo pode não se aplicar a eles.

**O que continua não determinado:** o que os três floats de cada keyframe
representam. O dump mostra dois constantes e um que varia, o que sugere posição ou
rotação num eixo com os outros fixos — mas isso é leitura de um exemplo, não
resultado. `actionCount` em payload 36 vale 1 na maioria destes arquivos, e seu
significado não foi verificado.

**Estado: animação BLOQUEADA.** É o último bloco do formato BMD não decodificado.

### Altura de terreno — decodificada

Os dados de altura **não estão** nos arquivos `.att`, que este projeto vinha
tratando como terreno. Estão em `TerrainHeight.OZB`, um por mundo:

```
Data/World<N>/TerrainHeight.OZB    66.618 bytes = 1.082 de prefixo + 65.536 de corpo
corpo = 256 × 256 bytes, um u8 de altura por célula da grade
```

**Evidência de que o corpo é altura e não bytes arbitrários.** A diferença média
entre células vizinhas horizontais, medida sobre **todos os 76 arquivos**
(`mutool terrain`, evidência em `docs/evidence/ciclo2_terrain.log`):

| Métrica | Valor |
|---|---|
| Média entre os 76 arquivos | **8,038** |
| Arquivo mais suave | 0,000 |
| Arquivo mais rugoso | 138,237 |
| Abaixo de 30,0 (campo suave) | 72 de 76 = **94,74%** |
| Esperado para `u8` aleatório | ~85,3 |

Uma razão de cerca de 10× abaixo do aleatório na média, com 94,74% dos arquivos
claramente suaves. Superfície de terreno é um campo suave; bytes arbitrários não
são.

**Correção de uma generalização que este projeto fez.** A primeira medição, sobre
um único arquivo (`World1/TerrainHeight.OZB`), deu 4,400 e foi registrada como se
descrevesse o formato. Sobre os 76 arquivos a média é **8,038**, quase o dobro, e
**4 arquivos não são suaves** — o mais rugoso chega a 138,237, acima do
aleatório. Uma amostra não é o formato.

**Os 4 arquivos fora do padrão foram investigados, e dois deles não são grades de
altura.**

| Arquivo | Tamanho | Prefixo | Suavidade | O que é |
|---|---|---|---|---|
| `World43/TerrainHeight.OZB` | 196.666 | 131.130 | 138,237 | **imagem BMP** — abre com `42 4d 36 00` = `"BM6\0"` |
| `World64/TerrainHeight.OZB` | 196.666 | 131.130 | 68,577 | **imagem BMP** — mesma assinatura |
| `World108/TerrainHeight.OZB` | 66.620 | 1.084 | 71,509 | prefixo 2 bytes maior; conteúdo não determinado |
| `World88/TerrainHeight.OZB` | 66.620 | 1.084 | 71,509 | idem |

**Dois desses arquivos são imagens BMP com nome de `TerrainHeight.OZB`.** O leitor
deste projeto tomava os últimos 65.536 bytes de qualquer arquivo com esse nome e
os tratava como altura — o que produziria um campo de altura feito de dados de
pixel, e **pareceria plausível, porque pixels também são bytes 0..255**. A
suavidade alta (138,2 e 68,6, contra média de 8,0) era o único sinal, e ele só
apareceu porque a medição foi feita sobre todos os arquivos e não sobre um.

**Correção aplicada:** `TerrainParseHeight` rejeita arquivos que abrem com `"BM"`
retornando `kUnsupported`. Verificar a assinatura custa nada e impede um campo de
altura silenciosamente errado.

Os outros dois (`World108`, `World88`) têm prefixo de 1.084 bytes em vez de 1.082
e suavidade 71,5. **Não foram determinados** — podem ser mapas abruptos de
verdade ou podem ter layout diferente. Permanecem como limite conhecido.

**Também medido: o arquivo NÃO é cifrado bux.** Começa com zeros, e aplicar a
chave XOR-3 transforma esses zeros em `FC CF AB` — o que seria altura espúria.
A extensão `.OZB` induz ao erro: nem todo `.OZB` deste cliente usa bux.

**O prefixo não contém fator de escala — ele é inteiramente zero.** Medido em
`World1/TerrainHeight.OZB`: 1.082 bytes, **nenhum diferente de zero**. Procurar
escala vertical aí é caminho fechado.

**Isso revela que provavelmente existe UM formato, não dois.** A aritmética fecha:

```
1.078 = 14 (cabeçalho de arquivo BMP) + 40 (cabeçalho DIB) + 1.024 (paleta 256×4)
1.082 = 1.078 + 4   ← os 4 bytes do prefixo duplicado
```

Os 29 arquivos que este projeto vinha tratando como "grade crua com prefixo" têm
exatamente o tamanho de um BMP de 8 bits com prefixo duplicado, **com o cabeçalho
zerado**. Ou seja: são o mesmo formato dos 47 BMP, apenas sem cabeçalho legível.

**A hipótese foi testada e se sustenta.** Medido nos 29 arquivos — que são 100%
da população, não amostra: a árvore tem 76 arquivos `TerrainHeight`, 47 com `"BM"`
e 29 sem, e 47 + 29 = 76:

| Tamanho | Arquivos | Leitura |
|---|---|---|
| 66.618 | 15 | exatamente `4 + 1.078 + 65.536` |
| 66.620 | 14 | 2 bytes acima — variação não explicada |

E o prefixo é **inteiramente zero nos 29 de 29**, não apenas no arquivo examinado
primeiro.

Nenhum dos 29 tem 65.536 bytes, que é o que uma grade crua pura teria. Todos têm
o tamanho de um BMP de 8 bits com prefixo duplicado.

**O que continua sem explicação:** os 14 arquivos com 2 bytes a mais. Podem ter
paleta ligeiramente maior ou um campo extra. Não foi determinado.

**Correção de número (item 5 da auditoria do ciclo 3).** Este documento dizia
"cabeçalho válido no offset 4 em 44 dos 47". Medido de novo com o critério que o
parser de fato aplica (tamanho declarado confere **e** `bpp == 8` **e** `pixoff`
cabe no arquivo), são **41 no offset 4 + 1 no offset 0 = 42**, e 5 sem cabeçalho
legível. O "44" vinha de um teste mais frouxo, que conferia só o tamanho
declarado. O número que corresponde ao comportamento do código é 42.

**Consequência prática, ainda não aplicada ao código:** o parser trata esses 29
como "grade crua" e os 47 como BMP, quando provavelmente são o mesmo formato.
Unificar os dois caminhos é uma simplificação pendiente, não uma correção de
bug — a saída atual já é a mesma, porque em ambos os casos o corpo fica nos
últimos 65.536 bytes.

### Escala vertical da altura — três caminhos fechados

A escala vertical não foi determinada, e três lugares plausíveis foram examinados
e fechados:

| Caminho | Resultado |
|---|---|
| Prefixo dos 29 arquivos de grade crua | **inteiramente zero** em 29 de 29 |
| `xPixelsPerMeter` / `yPixelsPerMeter` do cabeçalho BMP | **2.834 em 42 de 45** — é o padrão BMP para 72 DPI (2.834 ÷ 25,4 ≈ 111,6 px/cm), valor default e não medição |
| Paleta BMP de 256 entradas | **identidade**: `palette[i] = (i, i, i)` em tons de cinza |

A paleta identidade é informativa por si: ela diz que o valor do byte **é** a
altura, sem remapeamento. Não há tabela de conversão escondida na paleta.

**Conclusão:** a escala não está no arquivo de altura. Ou está em outro arquivo do
mundo, ou é uma constante do motor. **Este projeto não a determinou, e sem ela a
altura é relativa, não métrica.**

Para renderizar terreno isso pode não ser bloqueante: a escala pode ser um
parâmetro de renderização ajustado visualmente. Mas afirmar que o terreno está
"pronto para renderizar" sem essa escala seria errado — as proporções verticais
ficariam arbitrárias.
- A escala vertical: se cada unidade vale 1, 0,5 ou outro fator. Sem isso a
  altura é relativa, não métrica.
- Se os outros mundos seguem o mesmo prefixo.

**Consequência:** terreno renderizável passa a depender de textura, não de
geometria — a grade 256×256 com altura por célula já está legível. Isso é
independente do bloqueio de UV dos modelos `.bmd`.

### Medição histórica do classificador v0x0C (não é validação independente)

Uma amostra histórica de 400 arquivos que a implementação existente classificou
como v0x0C-label produziu corpos que o parser aceitou sob o layout v0x0A; o
parser observou normais unitárias em **99,98%** (37.988/37.997), contra 31,46%
no deslocamento 4. Isso é um resultado da mesma cadeia de código/classificador,
não uma validação independente da cifra, do filtro ou da compatibilidade v0x0C.
Não comprova autorização e não altera o bloqueio em `AGENTS.md`; não use estes
números para ampliar o trabalho de cifra.

**Atenção à base do cabeçalho, que difere por versão:**

| Versão | Cabeçalho | Início do payload |
|---|---|---|
| v0x0A | `"BMD"` + versão = 4 B | offset 4 |
| v0x0C | `"BMD"` + versão + `u32 payloadSize` = 8 B | offset 8 |

O registro de ciclo 2 atribuiu a diferença observada ao deslocamento de base:
usar 4 em vez de 8 bytes produziu "54 de 8.741 com corpo completo" e "normais
unitárias 0,02%". Isso documenta uma falha daquela medição de layout; não é
validação da cifra e não modifica o status BLOCKED.

### Registro de um erro de método deste ciclo

Uma revisão deste ciclo trocou a ordem dos campos nos registros com base numa
medição que reportava "offset 4 vence em 1.050 arquivos, offset 0 em nenhum". A
medição estava errada: os scripts liam deslocamentos de **arquivo** tratando-os
como de **payload**, defasando tudo em 4 bytes. A reversão está no commit
`bf1668c`.

Regra que fica: **toda medição sobre o payload precisa declarar a base.** Um
resultado "decisivo" obtido com a base errada é mais perigoso que um resultado
inconclusivo, porque encerra a investigação.

## Estado de implementação

| Etapa | Status |
|---|---|
| Implementação histórica da transformação v0x0C (`mu::BmdV0CDecrypt`) | **Existe no código; validação/autorização BLOCKED** por `AGENTS.md` |
| Vértices + normais (v0x0A) | **PARCIAL** — layout aceito nos dados e testes registrados |
| Vértices + normais em arquivos v0x0C-labelled | **Observação do classificador** — 400/400 na amostra histórica; não é validação do cipher nem compatibilidade geral |
| Triângulos, UV, nomes de textura | **PARCIAL / não semântico** — alguns registros são lidos; UV/material não estão verificados |
| Ossos | **PARCIAL** — alguns registros são localizados, sem semântica de rig/weights validada |
| Animações | **NÃO VALIDADO** — frames/chaves e ordem de transformação não identificados |

---

# Limites reais do formato BMD — conhecidos vs desconhecidos

Levantamento do ciclo 2. A regra aqui é separar o que foi **medido nos arquivos**
do que só poderia ser conhecido decodificando seções ainda bloqueadas. Omitir a
segunda coluna seria esconder o tamanho do que falta.

## Valores observados nos layouts aceitos pelo parser (inventário histórico ciclo 12; sem prova semântica)

| Limite | Valor | Origem da medição ciclo12 |
|---|---|---|
| Precisão de coordenadas | `f32` observados nos layouts aceitos pelo parser | A observação v0x0C vem da amostra do classificador legado; a cifra permanece **BLOCKED** e isso não é validação v0x0C |
| Contagem de vértices por malha | campo `u16` → **teto teórico 65.535**, **máximo observado 22.987** (Custom/Monster/Ozarugold.bmd) — sem limite superior conhecido documentado além do campo u16, não há evidência de limite menor | `u16` offset 38, tool /tmp/find_max_vert, 8406 arquivos com geometria parseada |
| Contagem de normais por malha | campo `u16` → teto codificável 65.535; máximo observado no inventário citado: 5.376 | payload offset 40 nos layouts aceitos; a contagem de vértices de outro arquivo não é usada como proxy |
| Contagem de UV por malha | campo `u16` → teto 65.535, máximo observado não determinado (UV não localizado) | payload offset 42 |
| Contagem de triângulos por malha | campo `u16` → teto 65.535, máximo observado 39.193 (geral) / 15.390 (com ossos) | payload offset 44, tool /tmp/find_max_vert |
| Registro de vértice | **16 bytes** (3×f32 posição + 1×u16? actually 16B) | Verificado 260 mil+ registros |
| Registro de normal | **20 bytes** | Verificado 260.268 registros 99,91% norma 1 |
| Registro de triângulo | **64 bytes**; os primeiros 24 B são três registros de canto de 8 B (`u16` vertex, `u16` normal, `u16` raw/optional, `u8` flag, `u8` pad); os 40 B restantes não têm semântica confirmada | Parser lê os campos conhecidos em layouts aceitos; associação de material/UV continua não verificada |
| Registro de osso | **59 B** nos layouts aceitos (32 B name + u16 parent + 6×f32 + pad exceto o último; total `bc*59-1`) | Parser localiza alguns registros; nomes, pesos, transforms e animação não foram validados semanticamente |
| Contagem de malhas (meshCount) | campo `u16`, **máximo observado 30** (Object157/Object06.bmd) entre geometria parseada, **9** entre com ossos (Object74/Object40.bmd), mediana 1 | payload offset 32, tool /tmp/find_max_vert e /tmp/find_max_bones3 |
| Contagem de ossos (boneCount) | campo `u16`, **máximo observado 63** (Item/Stick32.bmd) entre arquivos com has_bones true, **limite prático 64** (parser rejeita >64 por segurança, mas observado 63), total 7879 bones em 3693 arquivos | payload offset 34, tool /tmp/find_max_bones3, mutool bmdreport 3693 loose 1186 strict |
| Nome do modelo de origem | **32 bytes**, ASCII NUL-pad, >=3 printable loose / NUL-padded strict | Campo inicial corpo |
| Nome de textura / material | Uma região de 32 B após cada array de triângulos é observada em layouts aceitos; a associação e o papel material não foram confirmados | Busca de nome em posições candidatas no parser; estatística do ciclo 6 é evidência de localização de texto, não semântica |

**Consequência prática do teto `u16`.** Uma malha não pode exceder 65.535
vértices nem 65.535 triângulos. Para conteúdo pesado isso é restritivo e é um
argumento concreto a favor do caminho B (assets customizados em formato moderno)
para modelos novos — ver `docs/ARCHITECTURE.md`.

## Semânticas e limites ainda não estabelecidos (inventário ciclo12; cifra v0x0C/Enc* BLOCKED)

| Limite | Por que não se sabe / Estado ciclo12 |
|---|---|
| **Número de materiais/texturas por modelo** | **CONHECIDO parcial ciclo12 C3: 1 por mesh observado** — cada mesh tem 1 nome textura 32B após triângulos, não observado >1 por mesh, evidência ciclo6_bmd_multimesh.log, mas não validado para todos multi-mesh |
| **Máximo de ossos por modelo** | **CONHECIDO ciclo12 C3: até 64 observados, máximo 63 (Stick32.bmd), limite prático 64** — campo u16 offset 34, parser rejeita >64 por segurança, total 7879 bones em 3693 arquivos loose ratio 1846,28× definitivo strict 1186 headline conservador |
| **Máximo de vértices por modelo** | **CONHECIDO parcial: campo u16 teto 65.535, máximo observado 22.987 (Ozarugold.bmd), sem limite superior conhecido documentado além do campo** — não há evidência de limite menor, tool /tmp/find_max_vert |
| **Máximo de frames de animação** | Depende de ossos e seção ações, ambas parcialmente decodificadas: 764 com extra após ossos, 231 divisível bc*12, 62 plausíveis finite<1e6 mas ratio 0,02× NÃO discriminativo, 53 plausíveis [-pi,pi] ratio 0,04× NÃO discriminativo, animação permanece NÃO DECODIFICADA |
| **Bytes 24..59 do registro de triângulo** | Não zero em 59,1% registros, sem quarto canto offsets +24..+56 dão 50–81% nível ruído, conteúdo não identificado |
| **UV por vértice do triângulo** | O candidato em +4 descartado, UV segue sem localização 0% 11 hipóteses descartadas platô 9-26× explicado como artefato floats pequenos lidos como u16 |
| **Limite de malhas por modelo** | **CONHECIDO ciclo12 C3: máximo observado 30 (Object157/Object06.bmd) geral, 9 com ossos (Object74/Object40.bmd), mediana 1** — campo u16 offset 32, tool /tmp/find_max_vert |
| **Semântica dos u16 em 32/34/36** | **CONHECIDO parcial: 32=meshCount, 34=boneCount, 36=actionCount? coerentes mas não verificados um a um, exceto meshCount e boneCount validados via parsing** |

**Registro explícito, como exigido:** a ausência de índices de face decodificados
impede levantar limites que dependem de topologia. Isso é limitação do
levantamento, não propriedade conhecida do formato.

## Restrição que se aplica a tudo acima

As medições de estrutura interna valem para **arquivos de malha única** (586 dos
1.500 examinados no levantamento de topologia). Para malhas múltiplas o corpo se
repete por malha e a aritmética de bloco único não se aplica — não foi medido.
