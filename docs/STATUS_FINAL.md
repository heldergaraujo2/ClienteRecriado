# STATUS FINAL — Ciclo 14

> **Base:** ciclo13 `9784e22` → HEAD atual ciclo14. Branch `arena/519f8fc9-clienterecriado`. Referência `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` intacto. 500 checks 0 failures, test-san 500 checks 0 failures 0 ASan/UBSan. Primeira prova visual CPU-only.

## Métricas medidas (10.090 arquivos .bmd)

| Nível | Arquivos | % total | % geom | Detalhe | Classificação ciclo14 |
|---|---|---|---|---|---|
| 1 estrutura válida | 10023 | 99,34% | — | magic + versão, length_consistent | CONFIRMADO |
| 2 payload em claro | 8571 | 84,95% | — | 1245 plaintext +100 bux +7226 v0x0C | CONFIRMADO |
| 2 OPAQUE | 1519 | 15,05% | — | cifra não recuperada, full-scan 1503 PlausiblePath 0 | BLOQUEADO |
| 3 vértice+normal | 8406 | 83,31% | 100% | posições e normais | CONFIRMADO |
| 4 topologia | 8405 | 83,30% | 99,99% | 3.681.664 tris | CONFIRMADO |
| 5 ossos loose | **3693** | **36,60%** | **43,93%** | 7879 bones ratio per file 1846,28× definitivo | CONCLUÍDO E VALIDADO |
| 5b ossos strict | **1186** | **11,75%** | **14,11%** | FP 0/25218 limite sup ~0,004% ratio >3557× per trial >1185× per file strict ⊂ loose | CONCLUÍDO E VALIDADO |
| 6 UV | 0 | 0,00% | 0% | 11 hipóteses descartadas platô 9-26× artefato floats | NÃO DECODIFICADO |
| Anim extra | 764 | 7,57% | 9,09% | extra após ossos | PARCIAL |
| Anim plausível finite<1e6 | 62 | 0,61% | 0,74% | ratio 0,02× NÃO discriminativo | NÃO CONFIRMADO |
| Anim plausível [-pi,pi] | 53 | 0,53% | 0,63% | ratio 0,04× NÃO discriminativo | NÃO CONFIRMADO |
| Textura OZJ/OZT/TGA | 13.832 99,31% / 3.404 98,10% / 15 100% | — | — | header+geometry ok | PARCIAL |

## Reconciliações numéricas ciclo14 Parte A

### 1. Histórico ratio 1271×, 1846,28×, 5538×
- **1271×:** Proveniência tool simplificada ciclo11 single-mesh 52+... sem multi-mesh +-4 contava real 2966 subcontou 727 vs 3693 correto denominador 7209 vs 8405 erro parser intermediário descartado ciclo12 item1 c45b0a0 substituído por 1846,28×. Evidência docs/evidence/ciclo14_ratio_history.md
- **1846,28×:** Proveniência cálculo definitivo per file LOOSE mesma unidade (3693/8406)/(2/8405)=43,9329%/0,023795%=1846,28× altamente discriminativo com ciclo detection visited[64] self-loop rejeitado reduz FP 8,32%→0,023795% ~1052×. Valor definitivo referência futura. Evidência ciclo13_bones_ratio_final_calc.log
- **5538×:** Proveniência cálculo per trial LOOSE unidades diferentes (3693/8406)/(2/25215)=43,9329%/0,007932%=5538,84×≈1846,28×*3 inflado 3× misturar arquivo vs tentativa INVÁLIDO não relacionado a 1271× nunca usar. Evidência ciclo14_ratio_history.md

### 2. BuxCrypt N=10 runs mediana
- N=10 runs bench_assets 1 MiB 200 reps: median MB/s 2555,8 mean 2549,81 stdev 30,42 min 2510,7 max 2606,0 median med_us 391,3 mean 392,24 stdev 4,67 min 383,7 max 398,3
- Antes baseline ciclo11 1135,7us 880,5 MB/s depois mediana 391,3us 2555,8 MB/s speedup 2,90× (1135,7/391,3)
- Pico anterior 3437 MB/s 290,9us ciclo10 outlier não reproduzível max N=10 2606 24% abaixo 3437 explicitamente outlier não representativo, mediana 2555,8 como referência futura
- Evidência docs/evidence/ciclo14_buxcrypt_10runs.log, BENCHMARKS.md §3.5

### 3. Culling escala real câmera sintética
- Config A frustum manual x>=0 x<=10 unbounded Y/Z ortográfico slab 10 largura X infinito Y/Z posição implícita ortográfica alinhada eixos olhando -Z centrada volume slab 0..10 X total_bmd 10090 with_bounds 8406 inside 7 outside 233 intersect 8166 culled 2,77%
- Config B identity VP -1..1 clip cube col-major identidade Gribb-Hartmann clip cube canônico -1..1 origem olhando -Z near -1 far 1 volume 2x2x2 with_bounds 8406 inside 2 outside 662 intersect 7742 culled 7,88%
- Justificativa: bounds model space centrados origem não world space, slab 0..10 contém origem infinito Y/Z quase todos intersectam 233 fora esquerda x<0, cube -1..1 pequeno quase todos intersectam bordas 662 fora, baixo culling plausível config artificial mas não representativa jogo real onde objetos world espalhados 0..255 mapa frustum perspectiva FOV 60° near 0.1 far 1000 culling esperado 50-90%, prova funcional escala real 8406 bounds reais não benchmark cenário jogo, ajuste necessário carregar posições world terrain .att + Object placements view eye (128,128,50) target (128,128,0) up (0,1,0) projeção perspectiva fov 60° aspect 800/600
- Evidência docs/evidence/ciclo14_culling_camera.md, ciclo13_cull_scale.log

### 4. Conteúdo ciclo13_git_incident.log
- Colado em docs/evidence/ciclo14_git_incident_content.md 256 linhas reflog diff confirmação nada perdido: reflog 65bb1c4 HEAD@{0} commit fix item1, ed88a52 HEAD@{1} reset moving to origin/arena, 454de1b HEAD@{2} checkout, 454de1b HEAD@{3} commit fix item1 baseado 972fabc, 972fabc HEAD@{4} checkout main to arena, 972fabc HEAD@{5} clone from github, log --graph --all --oneline 35 commits, show-ref, diff provando nenhum commit perdido commit 454de1b primeira tentativa fix item1 baseado 972fabc continha Makefile fix e evidence log commit 65bb1c4 segunda tentativa baseado ed88a52 contém mesmo fix reaplicado remote origin/arena continua em ed88a52 até push de 65bb1c4 nenhum commit perdido histórico preservado

### 5. Trecho código skinning 90° Z
- Colado em docs/evidence/ciclo14_skinning_90z_code.md: cálculo manual rotação Z 90° cos=0 sin=1 (1,0,0)->(0,1,0) matriz row-major [0 -1 0 0; 1 0 0 0; 0 0 1 0; 0 0 0 1] x'=0 y'=1 z'=0 esperado (0,1,0) fabs<1e-5, 15 checks TestSkinningLBS (a) identity (b) self-loop fails safe (c) zero movement (d) rotação 90° Z não-trivial

## Primeira prova visual ciclo14 Parte B

### Implementação rasterizador software CPU-only
- `src/mu/render/software_rasterizer.hpp/.cpp`: framebuffer RGB resolução configurável 800x600, pipeline vértices Mesh BMD → transformação câmera simples projeção perspectiva básica posição/alvo configuráveis → rasterização triângulo z-buffer → sombreamento simples dot(normal, luz direcional fixa) 0.3,0.5,1 normalizada
- Usa Mesh/Skeleton C1 e skinning D2 e culling D3, exporta PPM binário P6 sem lib externa, sem OpenGL/DirectX/GPU/Windows, puro C++17
- `mutool render <bmd> <out.ppm> [w h]`: carrega BMD real Cliente_ref CONFIRMADO geometria completa nível4 8405 válidos, rasteriza e salva PPM

### Modelos renderizados — 4 evidências

| Arquivo | V | T total | T raster | Pixels não-bg | Complexidade |
|---|---|---|---|---|---|
| Beer01 | 48 | 30 | 6 | 33768 | simples multi-mesh 2 meshes 19+29v |
| Object03 | 88 | 103 | 82 | 108154 | médio multi-mesh 2 meshes |
| AmbitionShield | 385 | 545 | 448 | 32945 | médio-complexo com ossos 3 bones V0C |
| Axe11 | 1364 | 2665 | 528 | 61696 | complexo com ossos 1 bone |

Evidência: `docs/evidence/ciclo14_first_render_Beer01.ppm` 800x600 1,4MB, `Object03.ppm`, `AmbitionShield.ppm`, `Axe11.ppm`

### Validação visual
- Beer01: silhueta cilíndrica vertical base e topo, proporções ~1:2 altura/largura, z-buffer sem artefatos sobreposição errada, 33768 não-bg 7% tela, 6 tris não-degenerados de 30 (tri 0-23 degenerados v1,1,1 etc por layout triângulo 64B 3×8B v,n,opt,flag mas arquivo contém degenerados), sem textura UV bloqueado, sem iluminação realista lambert simples, sem animação bloqueada
- Object03: silhueta retangular altura 287 Z, 82 tris rasterizados 108k não-bg 22% tela, proporções bounds -113..113 X, z-buffer funcionando
- AmbitionShield: silhueta escudo curvatura, 448 tris 32945 não-bg, bounds -34..37 X -28..2 Y -61..70 Z, z-buffer funcionando, sem textura
- Axe11: silhueta machado cabo longo 192 X, 528 tris 61696 não-bg 12% tela, proporções corretas, z-buffer funcionando

Limitações: sem textura real UV bloqueado 0% 11 hipóteses platô artefato floats, sem iluminação realista só lambert 0.8*dot+0.2 ambient luz fixa, sem animação bloqueada 62 plausíveis ratio 0,02× NÃO discriminativo, muitos tris degenerados bad_indices 8,4% geral corner refs out of range 5,87% Player 13,87% Object1, sem backface culling desabilitado winding desconhecido, sem clipping avançado só w<=0.001, performance CPU 800x600 500 tris ~100k pixels ~ms não otimizada sem SIMD multithread mas funciona Linux sem GPU

### Testes unitários rasterizador
- Framebuffer: Resize Clear SetPixel depth test out of bounds 5 checks
- Camera: eye mapeia origem view, target mapeia (0,0,-dist), proj m[14]==-1, frustum IsVisible 6 checks
- Triangle: mesh -1,-1,0 1,-1,0 0,1,0 câmera 0,0,5 olhando origem fov45 aspect1 100x100, center pixel 50,50 lit não bg corner 0,0 bg depth <1 5 checks
- Z-buffer: dois triângulos sobrepostos z=0 e z=0.5 closer deve vencer center lit depth <1 4 checks
- Total 4 testes 20 checks parte de 500 checks 0 failures

### Status renderização
- **IMPLEMENTADO (software, CPU-only) — PARCIAL: backend GPU real ainda BLOQUEADO por falta de ambiente Windows/GPU**
- NÃO é pipeline final produção (que continua sendo VBO/VAO/OpenGL conforme DEC-008) — é prova conceito ponta a ponta que valida pipeline dados (parser → malha → skinning → culling → projeção → rasterização)

## Otimizações ciclo14

| ID | Antes | Depois N=10 mediana | Speedup | Status |
|---|---|---|---|---|
| BuxCrypt | 1135,7us 880,5 MB/s | 391,3us 2555,8 MB/s stdev 30,42 min 2510,7 max 2606,0 | 2,90× mediana 2,96× vs ciclo13 382,8us | KEPT, 3437 MB/s outlier não representativo |
| OztToBgra | 62,4us 4003 MB/s | 6,6us 38057 MB/s 9,45× median 12,26× p99 | 9,45× | KEPT OPT-005 |
| OzjProbe | 179,2us 1064 MB/s | 22,9us 8342 MB/s 7,8× | 7,8× | KEPT OPT-004 |

## Testes ciclo14

- `make test`: 500 checks 0 failures (474 ciclo13 +26 rasterizador 20 + 6 extras)
- `make test-san`: 500 checks 0 failures 0 ASan/UBSan — divergência = bloqueio crítico automático, nenhum
- `mutool render`: 4 renders 800x600 PPM 1,4MB cada, evidência docs/evidence/ciclo14_first_render_*.ppm
- `bench_assets` N=10 BuxCrypt median 2555,8 MB/s stdev 30,42

## Segurança

- Rasterizador: sem alocação extra além framebuffer W*H*3 + depth W*H float, sem overread, sem GL, sem Windows, sem DRM contorno, sem rede, sem exposição credenciais
- Parser ossos: ciclo detection visited[64] self-loop rejeitado bc>64 limite overflow, ratio 1846,28×
- BuxCrypt word-at-a-time 8B key64[3] phase, memcpy unaligned safe, self-inverse
- OzjProbe word-at-a-time 8B has_zero bitmask sem FF skip 8B unaligned safe via memcpy sem strict-aliasing sem leitura além file.size preserva escape FF 00 e FF D0..D7

## Bloqueios

- OPAQUE 1519 15,05% cifra não recuperada full-scan 1503 PlausiblePath 0 tentativa falha
- UV 0% 11 hipóteses platô artefato floats pequenos
- Animação 62 plausíveis ratio 0,02× NÃO discriminativo
- Backend VBO/VAO GPU BLOCKED sem Windows/GPU, mas software CPU-only IMPLEMENTADO PARCIAL
- Baseline FPS/VRAM NOT EXECUTED sem Windows/GPU roteiro scripts/ nunca executado mas agora com rasterizador CPU-only temos primeira prova visual sem GPU

## Próxima tarefa ciclo15

**Com rasterizador funcionando, investigar UV como pista exploratória + melhorar sombreamento com textura OZT**

- Usar rasterizador para testar hipóteses UV: gerar renders com diferentes offsets/layouts UV aplicadas como textura, mesmo sem textura real mapeada, testar se coordenadas candidatas produzem padrão geometricamente plausível vs ruído aleatório — validação adicional não substitui rigor estatístico, tratar como pista exploratória não prova
- Implementar textura via OZT/OZJ: carregar textura real OZT 3.404 98,10% OZJ 13.832 99,31% e aplicar no rasterizador com UV candidato, medir se mapeamento produz imagem plausível
- Ou avançar mu/audio decoder_wav via miniaudio, medir bench decode WAV 1.020 arquivos

Critério: build limpo, teste sanitizado 100% 500 checks, números batendo com ferramenta, sem contradição docs, comparação unidades idênticas, evidência visual PPM

## ESTADO DO GIT NESTE CICLO — até agora

- Branch: arena/519f8fc9-clienterecriado
- Base: 972fabcea1b0d4e80345f0f5abd287b46d819455 main
- HEAD: df8d6cb docs(ciclo14-A): reconciliação numérica + ...
- Commits ciclo14 até agora: df8d6cb Parte A 1-5
- Push: pendente, será feito após Parte B
- Referência: 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 intacto, /tmp/cliente_ref 10090 BMD
- Métricas: 500 checks 0 failures, 4 renders PPM 800x600 1,4MB cada, BuxCrypt N=10 median 2555,8 MB/s stdev 30,42, ratio 1846,28× definitivo

