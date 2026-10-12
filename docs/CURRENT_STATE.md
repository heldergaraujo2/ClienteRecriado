# Current state — ciclo14

An honest snapshot. What exists, what works, what does not.

## 1. What exists and works — ciclo14

**An asset format library**, C++17, zero dependencies, warning-clean, 500 checks 0 failures, test-san 500 checks 0 ASan/UBSan (fix ciclo13 item1, 474->500 ciclo14 rasterizer 4 tests 26 checks):

| Module | What it does | Verified against | Status ciclo14 |
|---|---|---|---|
| `mu/assets/ozt` | OZT read, write, probe, RGBA conversion, BGRA direct | 3,404 of 3,470 real files 98,10%, OztToRgba 63,0us median 89,7us p99 3969 MB/s, OztToBgra 6,6us median 11,3us p99 38057 MB/s 9,45× median 12,26× p99 vs old 62,4us 138,6us | CONCLUÍDO E VALIDADO + OPT-005 |
| `mu/assets/ozj` | OZJ container + JPEG marker parsing word-at-a-time | 13,832 of 13,928 real files 99,31%, OzjProbe 179,2us→15,2us 11,79× median 8,47× p99 OPT-004, after 22,9us 8342 MB/s 7,8× | CONCLUÍDO E VALIDADO + OPT-004 |
| `mu/assets/tga` | TGA probe and uncompressed read | 15 of 15 real files 100% | CONFIRMADO |
| `mu/assets/bmd` | BMD container classification, bux cipher word-at-a-time, entropy, V0C decrypt | 10,023 of 10,090 structurally valid 99,34%, payload in clear 8571 84,95% OPAQUE 1519 15,05% full-scan 1503 PlausiblePath 0 variantes plausíveis, BuxCrypt 1 MiB N=10 median 391,3us 2555,8 MB/s stdev 30,42 min 2510,7 max 2606,0 vs original 1135,7us 880,5 MB/s speedup 2,90× mediana, pico 3437 MB/s outlier não representativo max N=10 2606 24% abaixo 3437, evidence ciclo14_buxcrypt_10runs.log | CONCLUÍDO E VALIDADO + OPT-003 reconciliado |
| `mu/assets/bmd_report` | Three-level usability reporting + bones ratio reconciliado | total 10090 valid 10023 plaintext 8571 OPAQUE 1519 parsed 8406 with_tri 8405 with_bones_loose 3693 36,60% 43,93% geom ratio per file 1846,28× definitivo vs ctrl 0,023795% 2/8405, strict 1186 11,75% 14,11% geom taxa FP 0/25218 limite sup 0,003966% ratio >3557× per trial >1185× per file strict ⊂ loose, 1271× erro parser simplificado single-mesh 2966 vs 3693 subcontou 727 descartado, 5538× per trial unidades diferentes INVÁLIDO | CONCLUÍDO E VALIDADO — ratio history ciclo14_ratio_history.md |
| `mu/assets/bmd_body` | v0x0A body parser: vertex positions and normals, multi-mesh 14B header, bones 59B intercalado cycle detection visited[64] self-loop parent==i rejeitado bc>64 | 8406 parsed 83,31%, 8405 with triangles 83,30% 3.681.664 tris, 3693 bones loose 7879 bones, 1186 strict, max_mc 30 Object157/Object06.bmd, max_bc 63 Stick32.bmd limite prático 64 teto aceitação 63 máximo real sem contradição, max_vc 22987 Ozarugold.bmd, max_ntri 39193, 1 textura por mesh sbow06_R.jpg, triângulos 64B 3×8B v,n,opt,flag, muitos degenerados v1,1,1 etc bad_indices 8,4% geral | CONCLUÍDO E VALIDADO |
| `mu/assets/mesh` | Mesh internal representation C1 + Skeleton | positions normals texcoords indices bone_weights vector<vector<BoneWeight>> has_bone_weights has_texcoords bounds skipped_vertices bad_indices, Bone name parent position rotation, Skeleton has_bones, MeshImportResult | CONCLUÍDO E VALIDADO — TestMeshInternalRepresentation 23 checks |
| `mu/render/buffers` | Vertex/index buffer CPU-side VBO/VAO without GL | PackMeshForUpload PackedVertex px,py,pz,nx,ny,nz,u,v,bone_ids[4],bone_weights[4],weight_count PackedBuffers vertices flat indices flat has_texcoords has_bone_weights bounds skipped_vertices bad_indices ok, PackSkeletonForUpload parents names positions rotations has_bones valid parent range self-loop cycle visited[64] | CONCLUÍDO E VALIDADO — TestPackMeshForUpload 21 checks byte a byte |
| `mu/render/skinning` | LBS skinning CPU strict without GL | Mat4 Identity TransformPoint ValidateSkeletonForSkinning has_bones parent range self-loop cycle visited[64], ApplySkinningLBS strict count positions normals mismatch has_bone_weights false retorna original zero movement, true exige bone_weights size == positions, skeleton validado, boneTransforms size == bones size, sum weight_i*(bone*pos) | CONCLUÍDO E VALIDADO — TestSkinningLBS 15 checks (a) identity (b) self-loop fails safe (c) zero movement (d) rotação 90° Z não-trivial (0,1,0) esperado cálculo manual cos=0 sin=1 (1,0,0)->(0,1,0) matriz row-major [0 -1 0 0;1 0 0 0;0 0 1 0;0 0 0 1] fabs<1e-5 evidência ciclo14_skinning_90z_code.md |
| `mu/render/cull` | Frustum culling CPU bounds reais | Plane normal d, Frustum planes[6] left/right/bottom/top/near/far, FrustumFromViewProjection Gribb-Hartmann, CullBox p-vertex conservative safe invalid->kIntersect fail safe, IsVisible true para kInside/kIntersect false só kOutside invalid true, PlaneDistance | CONCLUÍDO E VALIDADO — TestFrustumCull (a) inside 2..8 kInside (b) outside -8..-2 12..18 kOutside (c) invalid never culled, TestFrustumCullRealBounds Mesh sintético 4 vértices (a) inside 0..1 frustum 0..10 kInside (b) outside -20..-10 kOutside (c) invalid true kIntersect, escala real 8406 bounds reais total_bmd 10090 with_bounds 8406 inside 7 outside 233 intersect 8166 culled 2,77% frustum 0..10 X, identity VP with_bounds 8406 inside 2 outside 662 intersect 7742 culled 7,88%, câmera sintética posição/orientação: Config A slab 0..10 X ortográfico unbounded Y/Z posição implícita ortográfica alinhada eixos olhando -Z centrada volume slab 0..10 X, Config B identity VP -1..1 clip cube col-major identidade, bounds model space centrados origem não world space baixo culling plausível config artificial mas não representativa jogo real onde objetos world espalhados 0..255 mapa frustum perspectiva FOV 60° near 0.1 far 1000 culling esperado 50-90%, prova funcional escala real 8406 bounds reais não benchmark cenário jogo, evidência ciclo14_culling_camera.md, ciclo13_cull_scale.log |
| `mu/render/software_rasterizer` | **NOVO ciclo14 B — rasterizador software CPU-only primeira prova visual real sem GPU** | Framebuffer RGB 800x600 1,4MB P6, Camera eye target up fov aspect near far GetViewMatrix lookAt GetProjectionMatrix perspective GetViewProjection P*V GetFrustum FrustumFromViewProjection, Multiply row-major a*b, TransformDirection w=0, Normalize Dot Cross Sub Add, SoftwareRasterizer Resize Clear RenderMesh RenderMeshes SavePPM, pipeline vértices Mesh BMD → transformação câmera perspectiva → rasterização triângulo z-buffer → sombreamento simples dot(normal, luz fixa 0.3,0.5,1) lambert 0.8*dot+0.2 ambient, usa Mesh C1 Skeleton C1 skinning D2 quando has_bone_weights, culling D3 antes rasterizar, exporta PPM binário P6 sem lib externa, sem OpenGL/DirectX/GPU/Windows, puro C++17, 4 renders reais: Beer01 48v 30t 6 raster 33768 non-bg 7% tela simples multi-mesh 2 meshes, Object03 88v 103t 82 raster 108154 non-bg 22% tela médio multi-mesh 2 meshes, AmbitionShield 385v 545t 448 raster 32945 non-bg 6,8% tela médio-complexo com ossos 3 bones V0C, Axe11 1364v 2665t 528 raster 61696 non-bg 12,8% tela complexo com ossos 1 bone, silhueta reconhecível proporções corretas z-buffer funcionando sem artefatos sobreposição errada, limitações sem textura UV bloqueado 0% sem iluminação realista só lambert simples sem animação bloqueada muitos tris degenerados bad_indices 8,4% geral sem backface culling desabilitado winding desconhecido sem clipping avançado só w<=0.001 performance CPU 800x600 500 tris ~100k pixels ~ms não otimizada sem SIMD multithread mas funciona Linux sem GPU, NÃO é pipeline final produção VBO/VAO/OpenGL DEC-008 mas prova conceito ponta a ponta valida pipeline dados parser→malha→skinning→culling→projeção→rasterização | **IMPLEMENTADO (software, CPU-only) — PARCIAL: backend GPU real ainda BLOQUEADO por falta ambiente Windows/GPU** — TestSoftwareRasterizerFramebuffer 5 checks, TestSoftwareRasterizerCamera 6 checks, TestSoftwareRasterizerTriangle 5 checks, TestSoftwareRasterizerZBuffer 4 checks, total 4 testes 20 checks parte de 500 checks 0 failures |
| `mu/platform/win32_window` | Esqueleto Win32 wglCreateContext BLOCKED | Win32GLConfig width height title fullscreen colorBits depthBits stencilBits, Win32GLWindow handle opaco created flag, CreateGLWindow DestroyGLWindow SwapGLWindow PollEvents GetWin32GLStatus kBlockedNoWindowsSDK kBlockedNoGPU kBlockedNoDisplay kOk só Windows real com GPU, sequência RegisterClassExW CS_OWNDC CreateWindowExW GetDC PIXELFORMATDESCRIPTOR ChoosePixelFormat SetPixelFormat 1× wglCreateContext wglMakeCurrent loop PeekMessage/TranslateMessage/DispatchMessage QPC SwapBuffers cleanup, não inclui <windows.h> nem <GL/gl.h> propositalmente | CONCLUÍDO — esqueleto documentado BLOCKED com aviso literal e lista verificação Windows real, TestWin32GLSkeletonBlocked 4 checks |
| `mu/platform/main_loop` | Esqueleto main loop update/render/present com predição frame budget sem GL | FrameBudget target_fps target_frame_ms update_ms render_ms present_ms total_ms slack_ms over_budget frame_index, InputState quit_requested, UpdateResult visible_objects culled_objects, RenderResult draw_calls_prepared triangles_submitted, PresentResult, MainLoop ctor target_fps Tick retorna FrameBudget ordem update->render->present mede via high_resolution_clock QPC, SetTargetFps PredictOverBudget média móvel 30 frames >90% budget History 120 frames | CONCLUÍDO — TestMainLoopSkeleton 9 checks |
| `mu/assets/terrain` | Terrain classification, bux auto-detect | 265 of 265 real files 100% 97 .att still encrypted | CONFIRMADO |
| `mu/assets/manifest` | Patch manifest parse and CRC verify | 188 entries total 188 match 116 size mismatch 9 crc mismatch 11 missing 52 mutool verify | CONFIRMADO |
| `mu/assets/format` | CRC-32 slice-by-8, status, byte views | zlib-derived known answers, Crc32 1 MiB 507,2us 1971 MB/s before 2553,5us 391 MB/s speedup 5,03× OPT-001 | CONCLUÍDO E VALIDADO |
| `mu/fs` | Recursive walker, POSIX + Win32 | 29,060-file real tree | CONFIRMADO |

**A command line tool**, `build/mutool`, with `scan`, `info`, `verify`, `dedupe`, `bmdreport`, `meshimport`, `culltest`, `terrain`, `bmdanim`, `render`. Whole-tree scan 2,372 MB in 1.67 s. `bmdreport` 8406 parsed, 8405 with_tri, 3693 bones loose ratio 1846,28×, 1186 strict. `render` produces first visual proof CPU-only PPM 800x600 1,4MB.

**A test suite**, 500 checks, 0 failures (474 ciclo13 +26 rasterizer 20 + 6 extras), self-generating fixtures, also clean under ASan and UBSan, evidence `ciclo14_buxcrypt_10runs.log` etc.

**A micro-benchmark harness** with median p95 p99 mean MB MB/s reps stddev min/max, N=10 runs BuxCrypt median 2555,8 MB/s stdev 30,42 min 2510,7 max 2606,0 median med_us 391,3 stdev 4,67, OztToBgra 6,6us 38057 MB/s 9,45×, OzjProbe 22,9us 8342 MB/s 7,8×.

**Documentation**: audit, formats, rendering analysis with §12.6 plano otimização e §12.7 primeira prova visual CPU-only 4 renders, architecture with C1, decisions with DEC-013 glTF 2.0, dependencies with §7 áudio/rede/interface, audio.md, network.md, interface.md, baselines, benchmarks §3.5 BuxCrypt N=10 mediana vs pico outlier, licensing, security, test results, status_final ciclo14, ratio history, culling camera, git incident content, skinning 90Z code, first render validation.

**Windows validation scripts** in `scripts/`, written but never run — BLOCKED sem Windows/GPU, but software rasterizer CPU-only IMPLEMENTADO PARCIAL.

## 2. Measured numbers ciclo14

```
OZJ   13,928 files -> 13,832 header+geometry parsed  (99.31%)   OzjProbe 179,2us→22,9us 7,8× median 11,79× peak OPT-004
OZT    3,470 files ->  3,404 pixels parsed           (98.10%)   OztToRgba 63,0us 3969 MB/s, OztToBgra 6,6us 38057 MB/s 9,45× median 12,26× p99 OPT-005
TGA       15 files ->     15 pixels parsed           (100%)
terrain    265 files ->    265 classified            (100%)   BuxCrypt N=10 median 391,3us 2555,8 MB/s stdev 30,42 min 2510,7 max 2606,0 vs original 1135,7us 880,5 MB/s speedup 2,90× mediana, pico 3437 MB/s outlier não representativo max N=10 2606 24% abaixo 3437
BMD:
  level1 structure valid 10023 / 10090 99,34%
  level2 payload in clear 8571 / 10090 84,95% OPAQUE 1519 15,05% full-scan 1503 PlausiblePath 0
  level3 vertex+normal 8406 / 10090 83,31%
  level4 topology 8405 / 10090 83,30% 3.681.664 tris
  level5 bones loose 3693 / 10090 36,60% 43,93% geom 7879 bones ratio per file 1846,28× definitivo vs ctrl 0,023795% 2/8405, 1271× erro parser simplificado single-mesh 2966 vs 3693 subcontou 727 descartado, 5538× per trial unidades diferentes INVÁLIDO
  level5b bones strict 1186 / 10090 11,75% 14,11% geom taxa FP 0/25218 limite sup 0,003966% ratio >3557× per trial >1185× per file strict ⊂ loose
  UV 0 / 10090 0% 11 hipóteses descartadas platô 9-26× artefato floats pequenos
  Anim extra 764 7,57% divisível bc*12 231 2,29% plausível finite<1e6 62 0,61% ratio 0,02× NÃO discriminativo plausível [-pi,pi] 53 0,53% ratio 0,04× NÃO discriminativo NÃO DECODIFICADA
  Limits: max_mc 30 Object157/Object06.bmd geral 9 com ossos Object74/Object40.bmd, max_bc 63 Stick32.bmd limite prático 64 teto aceitação 63 máximo real sem contradição, max_vc 22987 Ozarugold.bmd, max_ntri 39193, 1 textura por mesh sbow06_R.jpg
  Triângulos degenerados: taxa bad_indices 8,4% geral, corner refs out of range 5,87% Player 13,87% Object1, triângulos com vértices repetidos v1,1,1 etc, rasterizador dropa fabs(area)<1e-6 fail safe

Rasterizer software CPU-only:
  Beer01 48v 30t total 6 raster 33768 non-bg 7% tela 800x600 1,4MB PPM
  Object03 88v 103t total 82 raster 108154 non-bg 22% tela
  AmbitionShield 385v 545t total 448 raster 32945 non-bg 6,8% tela 3 bones V0C
  Axe11 1364v 2665t total 528 raster 61696 non-bg 12,8% tela 1 bone
  Testes: framebuffer 5 checks, camera 6 checks, triangle 5 checks, z-buffer 4 checks, total 20 checks parte de 500 checks 0 failures
  Status: IMPLEMENTADO (software, CPU-only) — PARCIAL: backend GPU real ainda BLOQUEADO por falta ambiente Windows/GPU, NÃO é pipeline final produção VBO/VAO/OpenGL DEC-008 mas prova conceito ponta a ponta valida pipeline dados parser→malha→skinning→culling→projeção→rasterização

Theoretical maximum texture footprint: 2,893.80 MB sum w*h*4 over parseable headers DERIVED NOT MEASURED No process run no GPU memory observed

Integrity vs ListaPatch.txt 188 entries: compared 136 identical 116 different 20 not present 52 all 20 differences also differ file size manifest disagree CRC computation correct

mutool scan 29,060 files: 1,641–1,674 ms
mutool verify 188 entries: 42–49 ms
make test: 500 checks 0 failures
make test-san: 500 checks 0 failures 0 ASan/UBSan
bench_assets N=10 BuxCrypt median 391,3us 2555,8 MB/s stdev 30,42 min 2510,7 max 2606,0 vs original 1135,7us 880,5 MB/s speedup 2,90× mediana, pico 3437 MB/s outlier 24% abaixo max
  OztToRgba 63,0us median p95 77,9 p99 89,7 3969 MB/s
  OztToBgra 6,6us median p95 7,2 p99 11,3 38057 MB/s 9,45× median 12,26× p99
  OzjProbe 22,9us median p95 33,5 p99 36,4 8342 MB/s vs original 179,2us 1064 MB/s 7,8× median 11,79× peak
```

## 3. What does not exist or is BLOCKED

| Component | Status | Evidence |
|---|---|---|
| Renderer GPU (VBO/VAO/OpenGL) | NOT STARTED, esqueleto Win32 BLOCKED, software CPU-only IMPLEMENTADO PARCIAL | win32_window.hpp NÃO COMPILADO BLOCKED requer Windows+GPU, software_rasterizer.hpp/.cpp CONCLUÍDO 4 renders PPM 800x600 1,4MB |
| Window / input / main loop | PARTIAL — esqueleto MainLoop CONCLUÍDO update/render/present predição budget sem GL, Win32 BLOCKED | main_loop.hpp/.cpp TestMainLoopSkeleton 9 checks, win32_window BLOCKED |
| Resource cache and manager | NOT STARTED | — |
| Texture upload to GPU | NOT STARTED, but OztToBgra 6,6us ready for GL_BGRA, software rasterizer sem textura UV bloqueado | OPT-005 |
| Model loading | PARTIAL — 8406 vertex+normal, 8405 topology, 3693 bones loose 1186 strict, 7879 bones, UV 0%, triângulos degenerados 8,4% bad_indices | bmd_body parser |
| Audio mu/audio | NOT STARTED, arquitetura definida docs/AUDIO.md | AUDIO.md |
| Network mu/network | NOT STARTED, estrutura pacotes documentada com rótulos docs/NETWORK.md | NETWORK.md |
| Interface mu/interface | NOT STARTED, fluxo telas definido docs/INTERFACE.md | INTERFACE.md |
| Animation | NOT DECODED — 62 plausíveis ratio 0,02× NÃO discriminativo, 53 plausíveis [-pi,pi] ratio 0,04× NÃO discriminativo | ciclo11_item5_anim_validation_full.log |
| OPAQUE cipher | BLOQUEADO — 1519 15,05% cifra não recuperada full-scan 1503 PlausiblePath 0 variantes plausíveis tentativa falha | ciclo11_opaque_step_fullscan.log |
| UV | NOT DECODED — 0% 11 hipóteses descartadas platô 9-26× explicado artefato floats pequenos, agora com rasterizador funcionando nova abordagem possível gerar renders teste com diferentes hipóteses offset/layout UV aplicadas como textura pista exploratória | ciclo10_itemA4_uv_ratio_per_offset.log, ciclo14_first_render_validation.md Parte C |
| FPS/VRAM baseline | NOT EXECUTED — no Windows no GPU, but CPU-only rasterizer first visual proof sem GPU | PERFORMANCE_BASELINE.md, RENDERING.md §12.7 |
| Visual validation Windows | BLOCKED — no renderer GPU, but CPU-only 4 renders PPM 800x600 1,4MB com silhueta reconhecível proporções corretas z-buffer funcionando | ciclo14_first_render_*.ppm, ciclo14_first_render_validation.md |

## 4. Security and dependencies

- Parser ossos: detecção ciclo visited[64] self-loop parent==i rejeitado bc>64 limite overflow, reduz FP 8,32%→0,0079% ratio 1846,28×, testes CircularParent ExtremeCount Truncated Overflow 500 checks ASan0
- BuxCrypt word-at-a-time 8B key64[3] phase advances 2 mod 3, memcpy unaligned safe, self-inverse mantido, N=10 median 2555,8 MB/s stdev 30,42 vs original 880,5 MB/s speedup 2,90× mediana, pico 3437 MB/s outlier não representativo max 2606 24% abaixo
- OzjProbe word-at-a-time 8B chunk has_zero bitmask sem FF skip 8B unaligned safe via memcpy sem strict-aliasing sem leitura além file.size preserva escape FF 00 e FF D0..D7 edge remaining<8 4 casos PASS 0 ASan/UBSan
- OztToRgbaTopDown 2px 8B via uint64_t bit ops (pixel & 0xFF00FF00) | ((pixel & 0xFF)<<16) | ((pixel>>16)&0xFF) memcpy unaligned safe, OztToBgraTopDown BGRA direct via GL_BGRA elimina swap só flip memcpy 10,2× 66,4us→6,5us 9,45× median 12,26× p99
- Software rasterizer: sem alocação extra além framebuffer W*H*3 + depth W*H float, sem overread, sem GL, sem Windows, sem DRM contorno, sem rede, sem exposição credenciais, sem backface culling desabilitado winding desconhecido, sem clipping avançado só w<=0.001, z-buffer SetPixel só escreve se z<stored depth inicial 1.0 far, testes framebuffer depth test out of bounds, camera eye→origem, triangle center pixel lit, z-buffer closer vence, 500 checks 0 failures ASan0
- Nenhum include gráfico em src/ exceto esqueleto BLOCKED win32_window que não inclui <windows.h> nem <GL/gl.h> propositalmente, e software_rasterizer que não inclui GL, puro C++17
- Makefile fix ciclo13 item1: test-san agora cobre 100% checks 500, antes 389 vs 469 discrepância corrigida, regra permanente qualquer divergência futura entre test e test-san é bloqueio crítico automático
- Dependências: zero dep build C++17, mas cliente referência tem msvcr100 msvcp100 VS2010, MSVCP140 VCRUNTIME140 VS2015+ Audio.dll, vorbisfile.dll 999KB ogg.dll 53KB Xiph BSD-style, 3DGuard.dll hinetworks.dll License.hinet main.premium etc Comercial não redistribuível, MHPClient.dll MHPVerify.dll, PS.dll Delphi packed, AutoLogin plaintext Settings.ini SECURITY.md, Thumbs.db 12 arquivos 677KB rar/zip 14,3MB ScreenShots 51,7MB não deveriam shippar
- Licença bmdref xulek/muonline-bmd-viewer commit 3f11bf89f3702261174d8efa1436d56a15c7d97a ISC NÃO CONFIRMADA sem LICENSE file e-mail placeholder xulek@example.com, nada copiado em bloco git ls-files | grep -ci bmdref 0, clone /tmp/bmdref e /tmp/cliente_ref fora repo não commitado

## 5. Next task

**Com rasterizador funcionando, investigar UV como pista exploratória + melhorar sombreamento com textura OZT**

Após ciclo14, rasterizador software CPU-only IMPLEMENTADO PARCIAL com 4 renders PPM 800x600 1,4MB cada, silhueta reconhecível proporções corretas z-buffer funcionando, primeira prova visual simbólica e técnica. Próxima tarefa pode ser:
- Implementar textura via OZT/OZJ no rasterizador: carregar textura real OZT 3.404 98,10% OZJ 13.832 99,31% e aplicar no rasterizador com UV candidato, medir se mapeamento produz imagem plausível vs ruído aleatório — validação adicional não substitui rigor estatístico, tratar como pista exploratória não prova, Parte C ciclo14
- Gerar renders teste com diferentes hipóteses offset/layout UV aplicadas como textura, testar se coordenadas candidatas produzem padrão geometricamente plausível
- Ou avançar mu/audio decoder_wav via miniaudio, medir bench decode WAV 1.020 arquivos

Critério: build limpo, teste sanitizado cobrindo 100% testes (500 checks), números batendo com ferramenta, sem contradição entre documentos, comparação real-vs-controle com unidades idênticas, evidência visual PPM.

## ESTADO DO GIT NESTE CICLO — até Parte A

- Branch: arena/519f8fc9-clienterecriado
- Base: 972fabcea1b0d4e80345f0f5abd287b46d819455 main
- HEAD: df8d6cb docs(ciclo14-A): reconciliação numérica 1271× 1846,28× 5538× + BuxCrypt N=10 mediana + culling câmera + git incident + skinning 90°Z
- Commits locais ciclo14: df8d6cb Parte A
- Push: realizado? Pendente, será feito após Parte B
- Referência: 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 intacto, /tmp/cliente_ref 10090 BMD
- Métricas: 500 checks? Ainda 474 até Parte A, após Parte B 500 checks 0 failures, 4 renders PPM, BuxCrypt N=10 median 2555,8 MB/s

