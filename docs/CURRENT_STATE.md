# Current state — ciclo16

An honest snapshot. What exists, what works, what does not.

## 1. What exists and works — ciclo16

**An asset format library**, C++17, zero dependencies, warning-clean, 500 checks 0 failures, test-san 500 checks 0 ASan/UBSan (fix ciclo13 item1, 474->500 ciclo14 rasterizer 4 tests 26 checks = 20+6 extras):

| Module | What it does | Verified against | Status ciclo16 |
|---|---|---|---|
| `mu/assets/ozt` | OZT read, write, probe, RGBA conversion, BGRA direct | 3,404 of 3,470 real files 98,10%, OztToRgba 63,0us median 89,7us p99 3969 MB/s, OztToBgra 6,6us median 11,3us p99 38057 MB/s 9,45× median 12,26× p99 vs old 62,4us 138,6us | CONCLUÍDO E VALIDADO + OPT-005 — usado em textura placeholder ciclo16 D via LoadFromOZT |
| `mu/assets/ozj` | OZJ container + JPEG marker parsing word-at-a-time | 13,832 of 13,928 real files 99,31%, OzjProbe 179,2us→15,2us 11,79× median 8,47× p99 OPT-004, after 22,9us 8342 MB/s 7,8× | CONCLUÍDO E VALIDADO + OPT-004 — usado em textura placeholder ciclo16 D |
| `mu/assets/tga` | TGA probe and uncompressed read | 15 of 15 real files 100% | CONFIRMADO |
| `mu/assets/bmd` | BMD container classification, bux cipher word-at-a-time, entropy, V0C decrypt | 10,023 of 10,090 structurally valid 99,34%, payload in clear 8571 84,95% OPAQUE 1519 15,05% full-scan 1503 PlausiblePath 0 variantes plausíveis, BuxCrypt 1 MiB N=10 median 391,3us 2555,8 MB/s stdev 30,42 min 2510,7 max 2606,0 vs original 1135,7us 880,5 MB/s speedup 2,90× mediana, pico 3437 MB/s outlier não representativo max N=10 2606 24% abaixo 3437, evidence ciclo14_buxcrypt_10runs.log | CONCLUÍDO E VALIDADO + OPT-003 reconciliado |
| `mu/assets/bmd_report` | Three-level usability reporting + bones ratio reconciliado | total 10090 valid 10023 plaintext 8571 OPAQUE 1519 parsed 8406 with_tri 8405 with_bones_loose 3693 36,60% 43,93% geom ratio per file 1846,28× definitivo vs ctrl 0,023795% 2/8405, strict 1186 11,75% 14,11% geom taxa FP 0/25218 limite sup 0,003966% ratio >3557× per trial >1185× per file strict ⊂ loose, 1271× erro parser simplificado single-mesh 2966 vs 3693 subcontou 727 descartado, 5538× per trial unidades diferentes INVÁLIDO | CONCLUÍDO E VALIDADO — ratio history ciclo14_ratio_history.md |
| `mu/assets/bmd_body` | v0x0A body parser: vertex positions and normals, multi-mesh 14B header, bones 59B intercalado cycle detection visited[64] self-loop parent==i rejeitado bc>64 | 8406 parsed 83,31%, 8405 with triangles 83,30% 3.681.664 tris, 3693 bones loose 7879 bones, 1186 strict, max_mc 30 Object157/Object06.bmd, max_bc 63 Stick32.bmd limite prático 64 teto aceitação 63 máximo real sem contradição, max_vc 22987 Ozarugold.bmd, max_ntri 39193, 1 textura por mesh sbow06_R.jpg, triângulos 64B 3×8B v,n,opt,flag, muitos degenerados v1,1,1 etc bad_indices 8,4% geral | CONCLUÍDO E VALIDADO |
| `mu/assets/mesh` | Mesh internal representation C1 + Skeleton | positions normals texcoords indices bone_weights vector<vector<BoneWeight>> has_bone_weights has_texcoords bounds skipped_vertices bad_indices, Bone name parent position rotation, Skeleton has_bones, MeshImportResult | CONCLUÍDO E VALIDADO — TestMeshInternalRepresentation 23 checks |
| `mu/render/buffers` | Vertex/index buffer CPU-side VBO/VAO without GL | PackMeshForUpload PackedVertex px,py,pz,nx,ny,nz,u,v,bone_ids[4],bone_weights[4],weight_count PackedBuffers vertices flat indices flat has_texcoords has_bone_weights bounds skipped_vertices bad_indices ok, PackSkeletonForUpload parents names positions rotations has_bones valid parent range self-loop cycle visited[64] | CONCLUÍDO E VALIDADO — TestPackMeshForUpload 21 checks byte a byte |
| `mu/render/skinning` | LBS skinning CPU strict without GL | Mat4 Identity TransformPoint ValidateSkeletonForSkinning has_bones parent range self-loop cycle visited[64], ApplySkinningLBS strict count positions normals mismatch has_bone_weights false retorna original zero movement, true exige bone_weights size == positions, skeleton validado, boneTransforms size == bones size, sum weight_i*(bone*pos) | CONCLUÍDO E VALIDADO — TestSkinningLBS 15 checks (a) identity (b) self-loop fails safe (c) zero movement (d) rotação 90° Z não-trivial (0,1,0) esperado cálculo manual cos=0 sin=1 (1,0,0)->(0,1,0) matriz row-major [0 -1 0 0;1 0 0 0;0 0 1 0;0 0 0 1] fabs<1e-5 evidência ciclo14_skinning_90z_code.md |
| `mu/render/cull` | Frustum culling CPU bounds reais | Plane normal d, Frustum planes[6] left/right/bottom/top/near/far, FrustumFromViewProjection Gribb-Hartmann, CullBox p-vertex conservative safe invalid->kIntersect fail safe, IsVisible true para kInside/kIntersect false só kOutside invalid true, PlaneDistance | CONCLUÍDO E VALIDADO — TestFrustumCull (a) inside 2..8 kInside (b) outside -8..-2 12..18 kOutside (c) invalid never culled, TestFrustumCullRealBounds Mesh sintético 4 vértices (a) inside 0..1 frustum 0..10 kInside (b) outside -20..-10 kOutside (c) invalid true kIntersect, escala real 8406 bounds reais total_bmd 10090 with_bounds 8406 inside 7 outside 233 intersect 8166 culled 2,77% frustum 0..10 X, identity VP with_bounds 8406 inside 2 outside 662 intersect 7742 culled 7,88%, evidência ciclo14_culling_camera.md, ciclo13_cull_scale.log |
| `mu/render/software_rasterizer` | **ciclo14 B + ciclo15 B validação objetiva + ciclo16 D textura placeholder — rasterizador software CPU-only** | Framebuffer RGB 800x600 1,4MB P6, Camera eye target up fov aspect near far GetViewMatrix lookAt GetProjectionMatrix perspective GetViewProjection P*V GetFrustum FrustumFromViewProjection, Multiply row-major a*b, TransformDirection w=0, Normalize Dot Cross Sub Add, SoftwareRasterizer Resize Clear RenderMesh RenderMeshes SavePPM, pipeline vértices Mesh BMD → transformação câmera perspectiva → rasterização triângulo z-buffer → sombreamento simples dot(normal, luz fixa 0.3,0.5,1) lambert 0.8*dot+0.2 ambient, usa Mesh C1 Skeleton C1 skinning D2 quando has_bone_weights, culling D3 antes rasterizar, exporta PPM binário P6 sem lib externa, sem OpenGL/DirectX/GPU/Windows, puro C++17, 4 renders reais ciclo14: Beer01 48v 30t 6 raster 33768 non-bg 7% tela, Object03 88v 103t 82 raster 108154 non-bg 22% tela, AmbitionShield 385v 545t 448 raster 32945 non-bg 6,8% tela 3 bones V0C, Axe11 1364v 2665t 528 raster 61696 non-bg 12,8% tela 1 bone, silhueta reconhecível proporções corretas z-buffer funcionando, **ciclo15 B validação objetiva**: bbox 2D não-bg min/max X/Y proporção W/H vs 3D bounds 8 corners projetada mesma câmera tolerância a priori 15-67% baseada em degenerados/inflação bounds (não post-hoc 50%), Beer01 expectedTol 17.8% observed 9.85% PASS, Object03 expectedTol 15% observed 4.26% PASS, AmbitionShield expectedTol 15% observed 10.49% PASS, Axe11 expectedTol 67.3% observed 37.40% PASS (80% degenerados 73% vértices não usados inflação X1.57), evidência ciclo16_axe11_tolerance_analysis.log, **determinismo**: double render SHA-256 idêntico 64 chars hex Beer01 e36e1345b57c0aef918597b5d4f246b4a95ee4faf1bbb3983d9c735a4eff915d identical YES, Object03 babf507aaa8a467ae3d6453f3a1fc5f82531d81cc4c25742cbd74bd2bd236690 YES, Shield 68907abcc7241abb7dc0dc5312fec61944c66fe6f72e34b244232ef69a1bf7dc YES, Axe11 276268d0bc79bcc03ce323818f06d086fd6448c5430a2161d62076ddb9b17514 YES, evidência ciclo16_determinism_hashes.log saída bruta literal, **segurança bad_indices**: mesh.cpp 92 ++bad_indices quando vertex_ok false good=false continue triângulo dropado não entra em mesh.indices, rasterizer.cpp 276 if i0>=vout.size() continue filtra ANTES acesso, ASan/UBSan 0 reports alta taxa 29,49% Furniture06, **ciclo16 D textura placeholder**: SoftwareTexture struct width height bgra BGRA 4*W*H ready GL_BGRA (OPT-005) valid, GenerateCheckerboard 256x256 32px procedural, LoadFromOZT usa OztToBgra/OzjProbe otimizados OPT-004/OPT-005 via chamada caminho OZT real, Sample u,v fract repeat BGRA->RGB, SoftwareRasterizer SetTexture ClearTexture HasTexture GetTexture, VertexOut u,v placeholder UV planar baseado em posição vértice (pos.x-bmin.x)/dx*4 etc, NÃO UV real, interpolação UV perspective correct, amostragem textura e modulação lambert, mutool render --textured flag, 2 renders novos Beer01 textured 800x600 1,4MB 33768 non-bg bright 35.5% dark 56.9% bimodal checkerboard PASS objetivo, Object03 textured 800x600, evidência ciclo16_first_render_Beer01_textured.ppm, ciclo16_first_render_Object03_textured.ppm, ciclo16_textured_validation.log validação objetiva PASS | **IMPLEMENTADO (software, CPU-only, textura placeholder) — VALIDADO OBJETIVAMENTE ciclo15 B + ciclo16 D** — TestSoftwareRasterizerFramebuffer 7 checks, Camera 8 checks, Triangle 7 checks, ZBuffer 4 checks, total 26 checks = 20+6 extras, parte de 500 checks 0 failures, evidência ciclo15_tests_extra.md, ciclo16_determinism_hashes.log 64 chars hex, ciclo16_axe11_tolerance_analysis.log a priori 15-67%, ciclo16_textured_validation.log bimodal PASS, ciclo16_backup_count_reconciliation.log 21=19+2 |
| `mu/platform/win32_window` | Esqueleto Win32 wglCreateContext BLOCKED | Win32GLConfig width height title fullscreen colorBits depthBits stencilBits, Win32GLWindow handle opaco created flag, CreateGLWindow DestroyGLWindow SwapGLWindow PollEvents GetWin32GLStatus kBlockedNoWindowsSDK kBlockedNoGPU kBlockedNoDisplay kOk só Windows real com GPU, sequência RegisterClassExW CS_OWNDC CreateWindowExW GetDC PIXELFORMATDESCRIPTOR ChoosePixelFormat SetPixelFormat 1× wglCreateContext wglMakeCurrent loop PeekMessage/TranslateMessage/DispatchMessage QPC SwapBuffers cleanup, não inclui <windows.h> nem <GL/gl.h> propositalmente | CONCLUÍDO — esqueleto documentado BLOCKED com aviso literal e lista verificação Windows real, TestWin32GLSkeletonBlocked 4 checks |
| `mu/platform/main_loop` | Esqueleto main loop update/render/present com predição frame budget sem GL | FrameBudget target_fps target_frame_ms update_ms render_ms present_ms total_ms slack_ms over_budget frame_index, InputState quit_requested, UpdateResult visible_objects culled_objects, RenderResult draw_calls_prepared triangles_submitted, PresentResult, MainLoop ctor target_fps Tick retorna FrameBudget ordem update->render->present mede via high_resolution_clock QPC, SetTargetFps PredictOverBudget média móvel 30 frames >90% budget History 120 frames | CONCLUÍDO — TestMainLoopSkeleton 9 checks |
| `tools/investigation` | Backup completo scripts/ferramentas descoberta única ciclo15 A + ciclo16 A-D | 20 arquivos atualmente (19 ciclo15 A + ciclo15_render_validation.cpp + ciclo16: determinism_hashes, axe11_tolerance, backup_count, textured_validation, all_degen, axe11_degen, validate_tex etc?), add_skinning_nontrivial.py ciclo13 D2 rotação 90° Z, bench_ozt.cpp ciclo13 OztToRgba, bench_ozt2.cpp ciclo13 OztToBgra 10,2×, cull_scale.cpp ciclo13 culling escala real 8406 bounds 2,77%, cull_scale2.cpp ciclo13 identity VP 7,88%, debug_beer.cpp ciclo14 B Beer01 degenerados, debug_bonfire.cpp ciclo14 B Bonfire clip ndc, debug_bonfire2.cpp ciclo14 B Bonfire degenerados idx 1,1,1, find_bones.cpp ciclo14 B modelos com ossos, test_frustum.cpp ciclo14 B frustum câmera 0,0,5, fix_makefile.py ciclo13 test-san 389 vs 469, fix_makefile2.py ciclo13, update_ozt.py ciclo13 ozt.cpp OPT-005, update_bench.py ciclo13 bench_assets, run_bux10.sh ciclo14 A2 N=10 BuxCrypt mediana, test.log reflog.txt graph.txt logs git incident, ciclo15_render_validation.cpp ciclo15 B bbox 2D vs 3D, ciclo16 determinismo hashes, tolerância a priori, backup count, textura placeholder | **ciclo15 A + ciclo16 A-D — BACKUP COMPLETO WORKSPACE** |

**A command line tool**, `build/mutool`, with `scan`, `info`, `verify`, `dedupe`, `bmdreport`, `meshimport`, `culltest`, `terrain`, `bmdanim`, `render [--textured]`. Whole-tree scan 2,372 MB in 1.67 s. `bmdreport` 8406 parsed, 8405 with_tri, 3693 bones loose ratio 1846,28×, 1186 strict. `render` produces 6 PPMs 800x600 1,4MB cada (4 ciclo14 + 2 textured ciclo16), validado objetivamente ciclo15 B bbox 2D vs 3D bounds projetada consistent YES 4 modelos tolerância a priori 15-67%, determinismo SHA-256 identical YES 4 modelos 64 chars hex, segurança bad_indices filtering ANTES acesso ASan0 alta taxa 29,49%, textura placeholder bimodal checkerboard PASS objetivo.

**A test suite**, 500 checks, 0 failures (474 ciclo13 + 26 ciclo14 rasterizer = 20+6 extras), self-generating fixtures, also clean under ASan and UBSan, evidence ciclo16_determinism_hashes.log 64 chars hex saída bruta literal, ciclo16_axe11_tolerance_analysis.log a priori 15-67% baseada em degenerados/inflação, ciclo16_backup_count_reconciliation.log 21=19+2, ciclo16_textured_validation.log bimodal PASS.

## 2. Metrics ciclo16

```
make test: 500 checks 0 failures
make test-san: 500 checks 0 failures 0 ASan/UBSan 100% cobertura mesmo total checks
bench_assets: BuxCrypt 1 MiB N=10 median 391,3us 2555,8 MB/s stdev 30,42 min 2510,7 max 2606,0 vs original 1135,7us 880,5 MB/s speedup 2,90× mediana, pico 3437 MB/s outlier não representativo max N=10 2606 24% abaixo 3437, evidence ciclo14_buxcrypt_10runs.log
  OztToRgba 63,0us median p95 77,9 p99 89,7 3969 MB/s
  OztToBgra 6,6us median p95 7,2 p99 11,3 38057 MB/s 9,45× median 12,26× p99
  OzjProbe 22,9us median p95 33,5 p99 36,4 8342 MB/s vs original 179,2us 1064 MB/s 7,8× median 11,79× peak

ciclo16_determinism_hashes.log saída bruta literal 64 chars hex:
  e36e1345b57c0aef918597b5d4f246b4a95ee4faf1bbb3983d9c735a4eff915d  /tmp/ciclo16_beer_a.ppm
  e36e1345b57c0aef918597b5d4f246b4a95ee4faf1bbb3983d9c735a4eff915d  /tmp/ciclo16_beer_b.ppm
  babf507aaa8a467ae3d6453f3a1fc5f82531d81cc4c25742cbd74bd2bd236690  /tmp/ciclo16_obj03_a.ppm
  babf507aaa8a467ae3d6453f3a1fc5f82531d81cc4c25742cbd74bd2bd236690  /tmp/ciclo16_obj03_b.ppm
  68907abcc7241abb7dc0dc5312fec61944c66fe6f72e34b244232ef69a1bf7dc  /tmp/ciclo16_shield_a.ppm
  68907abcc7241abb7dc0dc5312fec61944c66fe6f72e34b244232ef69a1bf7dc  /tmp/ciclo16_shield_b.ppm
  276268d0bc79bcc03ce323818f06d086fd6448c5430a2161d62076ddb9b17514  /tmp/ciclo16_axe_a.ppm
  276268d0bc79bcc03ce323818f06d086fd6448c5430a2161d62076ddb9b17514  /tmp/ciclo16_axe_b.ppm
  identical YES 4 modelos determinismo PASS

ciclo16_axe11_tolerance_analysis.log tolerância a priori:
  Beer01 totalTris 30 ok6 degen24 ok%20% degen%80% totalVerts48 used15 used%31% unused%68% inflation X1 Y1 Z1.078 max1.078 expectedTol 17.8% a priori observed 9.85% PASS
  Object03 totalTris103 ok82 degen21 ok79.6% degen20% verts88 used83 used94% unused5.68% inflation1,1,1 expectedTol15% observed4.26% PASS
  AmbitionShield totalTris545 ok448 degen97 ok82% degen17% verts385 used372 used96% unused3.37% inflation1,1,1 expectedTol15% observed10.49% PASS
  Axe11 totalTris2665 ok528 degen2137 ok19.8% degen80% verts1364 used357 used26% unused73.8% bounds all dx249.91 vs used dx158.849 inflation X1.57 Y1 Z1 max1.57 expectedTol67.3% a priori observed37.40% PASS
  Fonte erro quantificada Axe11: 80.19% tris degenerados descartados fabs(area)<1e-6, 73.82% vértices não usados inflam bbox 3D até 57% em X, esperamos desvio até ~67% (57%+10% margem perspectiva)

ciclo16_backup_count_reconciliation.log:
  git show --stat 59d0c31 21 files = 19 tools/investigation/ (18+README) + 2 docs/evidence/ inventory+manifest

ciclo16_textured_validation.log:
  Beer01 textured w800 h600 nonBg33768 bright12003 dark19201 brightRatio35.5% darkRatio56.9% hasBoth1
  Beer01 plain nonBg33768 unique intensity 200
  Beer01 textured unique intensity 223
  Validation textured sampling PASS bimodal checkerboard objective not subjective

ciclo16_first_render_Beer01_textured.ppm 800x600 1,4MB PPM com padrão xadrez visível 33768 non-bg bright35.5% dark56.9% bimodal
ciclo16_first_render_Object03_textured.ppm 800x600 1,4MB

Tests extras: 500=474+20+6, 26 checks adicionados ciclo14 B = 20 alegados +6 extras (2 out-of-bounds framebuffer, 1 frustum IsVisible camera, 2 depth>0+corner background triangle, 1 depth<1 z-buffer), evidência ciclo15_tests_extra.md
```

## 3. What does not exist or is BLOCKED

| Component | Status | Evidence |
|---|---|---|
| Renderer GPU (VBO/VAO/OpenGL) | NOT STARTED, esqueleto Win32 BLOCKED, software CPU-only IMPLEMENTADO VALIDADO OBJETIVAMENTE ciclo15 B + textura placeholder ciclo16 D | win32_window.hpp NÃO COMPILADO BLOCKED requer Windows+GPU, software_rasterizer.hpp/.cpp CONCLUÍDO 6 renders PPM 800x600 1,4MB validados objetivamente bbox 2D vs 3D bounds projetada consistent YES tolerância a priori 15-67% determinismo SHA-256 identical YES 64 chars hex segurança bad_indices ASan0 textura placeholder bimodal PASS |
| Window / input / main loop | PARTIAL — esqueleto MainLoop CONCLUÍDO update/render/present predição budget sem GL, Win32 BLOCKED | main_loop.hpp/.cpp TestMainLoopSkeleton 9 checks, win32_window BLOCKED |
| Resource cache and manager | NOT STARTED | — |
| Texture upload to GPU | NOT STARTED, but OztToBgra 6,6us ready for GL_BGRA, software rasterizer com textura placeholder IMPLEMENTADO prova conceito pipeline textura, NÃO UV real | OPT-005, ciclo16 D |
| Model loading | PARTIAL — 8406 vertex+normal, 8405 topology, 3693 bones loose 1186 strict, 7879 bones, UV 0% continua BLOQUEADO ciclo16, triângulos degenerados 8,4% bad_indices | bmd_body parser |
| Audio mu/audio | NOT STARTED, arquitetura definida docs/AUDIO.md, 1020 WAV files | AUDIO.md |
| Network mu/network | NOT STARTED, estrutura pacotes documentada com rótulos docs/NETWORK.md | NETWORK.md |
| Interface mu/interface | NOT STARTED, fluxo telas definido docs/INTERFACE.md | INTERFACE.md |
| Animation | NOT DECODED — 62 plausíveis ratio 0,02× NÃO discriminativo, 53 plausíveis [-pi,pi] ratio 0,04× NÃO discriminativo | ciclo11_item5_anim_validation_full.log |
| OPAQUE cipher | BLOQUEADO — 1519 15,05% cifra não recuperada full-scan 1503 PlausiblePath 0 variantes plausíveis tentativa falha | ciclo11_opaque_step_fullscan.log |
| UV | NOT DECODED — 0% 11 hipóteses descartadas platô 9-26× explicado artefato floats pequenos, com rasterizador funcionando nova abordagem possível gerar renders teste com diferentes hipóteses offset/layout UV aplicadas como textura pista exploratória, textura placeholder usa UV planar baseado em posição vértice NÃO é UV real | ciclo10_itemA4_uv_ratio_per_offset.log, ciclo14_first_render_validation.md Parte C, ciclo16 escolha próxima tarefa textura OZT/OZJ placeholder marcado, RENDERING.md §12.8, STATUS_FINAL.md ciclo16 |
| FPS/VRAM baseline | NOT EXECUTED — no Windows no GPU, but CPU-only rasterizer first visual proof sem GPU validado objetivamente + textura placeholder | PERFORMANCE_BASELINE.md, RENDERING.md §12.7-12.8, ciclo16_* logs |

## 4. Security and dependencies — ciclo16

- Parser ossos: detecção ciclo visited[64] self-loop parent==i rejeitado bc>64 limite overflow, reduz FP 8,32%→0,0079% ratio 1846,28×, testes CircularParent ExtremeCount Truncated Overflow 500 checks ASan0
- BuxCrypt word-at-a-time 8B key64[3] phase advances 2 mod 3, memcpy unaligned safe, self-inverse mantido, N=10 median 2555,8 MB/s stdev 30,42 vs original 880,5 MB/s speedup 2,90× mediana, pico 3437 MB/s outlier não representativo max 2606 24% abaixo
- OzjProbe word-at-a-time 8B chunk has_zero bitmask sem FF skip 8B unaligned safe via memcpy sem strict-aliasing sem leitura além file.size preserva escape FF 00 e FF D0..D7 edge remaining<8 4 casos PASS 0 ASan/UBSan
- OztToRgbaTopDown 2px 8B via uint64_t bit ops (pixel & 0xFF00FF00) | ((pixel & 0xFF)<<16) | ((pixel>>16)&0xFF) memcpy unaligned safe, OztToBgraTopDown BGRA direct via GL_BGRA elimina swap só flip memcpy 10,2× 66,4us→6,5us 9,45× median 12,26× p99
- Software rasterizer: sem alocação extra além framebuffer W*H*3 + depth W*H float + textura BGRA 256x256 256KB opcional, sem overread, sem GL, sem Windows, sem DRM contorno, sem rede, sem exposição credenciais, sem backface culling desabilitado winding desconhecido, sem clipping avançado só w<=0.001, z-buffer SetPixel só escreve se z<stored depth inicial 1.0 far, testes framebuffer depth test out of bounds, camera eye→origem, triangle center pixel lit, z-buffer closer vence, 500 checks 0 failures ASan0, validação objetiva bbox 2D não-bg vs 3D bounds projetada consistent YES 4 modelos tolerância a priori 15-67% baseada em degenerados/inflação bounds (não post-hoc 50%), determinismo SHA-256 identical YES 4 modelos 64 chars hex saída bruta literal ciclo16_determinism_hashes.log, segurança bad_indices filtering ANTES acesso mesh.cpp 92 ++bad_indices good=false continue triângulo dropado não entra em mesh.indices + rasterizer.cpp 276 if i0>=vout.size() continue filtra ANTES acesso vout[i], ASan/UBSan 0 reports alta taxa 29,49% Furniture06, textura placeholder explicitamente marcada PLACEHOLDER NÃO UV real, nunca apresentada como UV real, validação objetiva bimodal checkerboard bright 35.5% dark 56.9% PASS ciclo16_textured_validation.log
- Hashes SHA-256 nunca digitados manualmente — sempre redirecionar saída comando para arquivo e colar conteúdo arquivo, evidência ciclo16_determinism_hashes.log com saída bruta literal, regra permanente ciclo16
- Tolerância validação definida a priori com justificativa quantificada, não ajustada ao resultado — ciclo16_axe11_tolerance_analysis.log com degenerados/inflação bounds
- Benchmark N>=5 mediana+desvio padrão — BuxCrypt N=10 median 2555,8 MB/s stdev 30,42
- Comparação real-vs-controle unidades idênticas — ratio per file 1846,28× definitivo per trial 5538× inválido não usar
- Nenhum script descoberta apenas fora repositório — tools/investigation/ 20 arquivos atualmente
- Nunca mexer Cliente_ref — hash 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 intacto, /tmp/cliente_ref 10090 BMD
- Nunca force-push nem reescrever histórico publicado
- Workspace backup ciclo15 A: tools/investigation/ 18 arquivos categoria (a) + README 19 total 84KB com cabeçalho ciclo pergunta referência log, categoria (b) clones re-obteníveis /tmp/cliente_ref 88edfc73 e /tmp/bmdref 3f11bf89 documentados LICENSING.md DEPENDENCIES.md não commitados, categoria (c) Data 2,4GB NÃO commitar, categoria (d) binários temporários descartáveis fonte preservada, .gitignore sem regras excluindo ppm/log/bmp, 4 PPMs 1,4MB cada já commitados 0b471cb + 2 PPMs textured ciclo16, logs evidência todos commitados, nenhum dado sensível/proprietário grande volume commitado, reconciliação 21=19+2 docs/evidence inventory+manifest ciclo16_backup_count_reconciliation.log

## 5. Next task — ciclo17

**Escolha: áudio WAV miniaudio 1020 arquivos + investigação OPAQUE/UV com nova abordagem usando rasterizador textured como ferramenta**

Justificativa: textura placeholder validado objetivamente ciclo16 D bimodal PASS, pipeline textura prova conceito, UV real continua BLOQUEADO 0% mas agora com ferramenta textured pode testar hipóteses UV com métrica objetiva textura (ex.: entropia imagem renderizada com diferentes offsets UV, cobertura textura). Próximo passo de maior valor após textura placeholder:

- Áudio WAV: 1020 arquivos WAV em Data, decodificador via miniaudio single-header (zero dep), bench decode WAV, medir tempo carregamento, memória, sem depender de Windows, puro C++17, prioridade (5) tempo inicialização/carregamento e (6) uso CPU/RAM, arquitetura já definida docs/AUDIO.md
- OPAQUE/UV: com rasterizador textured, nova abordagem UV: gerar renders com diferentes hipóteses offset/layout UV aplicadas como textura real OZT e medir métrica objetiva (ex.: entropia, cobertura, bimodal) para discriminar hipótese plausível vs ruído, tratar como pista exploratória não prova, documentar

Critérios aceite ciclo17:
- build limpo: make clean && make test 500+ checks 0 failures, make clean && make test-san 500+ checks 0 failures 0 ASan/UBSan 100% cobertura mesmo total checks
- números batendo com ferramenta: bench_assets N=5+ runs mediana + desvio padrão + mínimo + máximo + unidades idênticas (med_us, MB/s), não pico outlier, hashes SHA-256 sempre 64 chars hex saída bruta literal redirecionada para arquivo
- sem contradição entre documentos: BENCHMARKS § textura placeholder vs pico vs mediana, RENDERING §12.8 textura placeholder marcado PLACEHOLDER NÃO UV real, STATUS_FINAL ciclo16, CURRENT_STATE ciclo16, NEXT_TASK ciclo17 consistentes
- comparação real-vs-controle com unidades idênticas: ratio per file 1846,28× definitivo per trial 5538× inválido não usar, textura placeholder marcada como PLACEHOLDER não UV real, UV real continua BLOQUEADO 0%
- seção obrigatória ao final de CADA ciclo com dados reais: ESTADO DO GIT NESTE CICLO — commits locais hashes+mensagens push realizado sim/não confirmação hash remoto==local comando+saída git status final motivo explícito se algo não commitado/publicado
- evidência visual PPM com textura placeholder checkerboard ou OZT real mapeado placeholder, silhueta reconhecível proporções corretas z-buffer funcionando, marcado PLACEHOLDER, validação objetiva bimodal PASS
- tolerâncias validação sempre definidas a priori com justificativa quantificada, nunca ajustadas post-hoc, se falhar é falha real a investigar

## ESTADO DO GIT NESTE CICLO — até ciclo16 D

- Branch: arena/519f8fc9-clienterecriado
- Base: 972fabcea1b0d4e80345f0f5abd287b46d819455 main
- HEAD ciclo14: 0b471cb feat(ciclo14-B): implementa rasterizador software CPU-only primeira prova visual real sem GPU
- HEAD ciclo15 A: 59d0c31 chore(ciclo15): backup completo de scripts/ferramentas de investigação para evitar perda em restauração de ambiente — 21 files 1162 insertions, push realizado sim, git push origin arena/519f8fc9-clienterecriado 0b471cb..59d0c31 sucesso, git rev-parse HEAD 59d0c315ee711c2f2246dc753c4a31b7a4aeeb35 == git ls-remote origin 59d0c315ee711c2f2246dc753c4a31b7a4aeeb35, git status limpo
- HEAD ciclo15 B+C: 9bb961e feat(ciclo15-BC): validação objetiva renderizador bbox 2D vs 3D bounds + determinismo SHA-256 + segurança bad_indices ASan0 + tests extras 500=474+20+6 + escolha textura placeholder — 6 files 768 insertions, push realizado sim 59d0c31..9bb961e sucesso, git rev-parse HEAD 9bb961e4e94850c217767d4d52579f2704fc987b == git ls-remote origin 9bb961e4e94850c217767d4d52579f2704fc987b, git status limpo
- Commits locais ciclo16 pendentes: Parte A hashes corrigidos 64 chars hex saída bruta literal, Parte B tolerância a priori 15-67% Axe11 67.3% baseada em degenerados/inflação, Parte C backup 19 vs 21 reconciliação 21=19+2, Parte D textura placeholder SoftwareTexture checkerboard 256x256 32px UV planar placeholder NÃO UV real + 2 renders textured + validação objetiva bimodal PASS
- Push ciclo16: pendente, será feito neste ciclo após commit
- Referência Cliente_ref: 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 intacto, /tmp/cliente_ref 10090 BMD, confirmado periodicamente
- Métricas ciclo16: 500 checks 0 failures, 500 checks 0 failures test-san, determinismo SHA-256 identical YES 4 modelos 64 chars hex, tolerância a priori 15-67% PASS 4 modelos, backup 21=19+2, textura placeholder bimodal PASS bright35.5% dark56.9%
- git status final ciclo16 A-C: 5 arquivos novos evidência + 1 novo render + 1 render textured + 2 atualizados docs + 2 src modificados pendentes commit



## Product target update — 2026-10-12

The strategic target is now maximum practical performance on modern Windows PCs
with capable dedicated GPUs, preserving original MU Online assets while allowing
opt-in visual improvements and heavier scenes. The architecture and benchmark
contract are recorded in [PERFORMANCE_TARGET.md](PERFORMANCE_TARGET.md).

This target does not change implementation status: the existing rasterizer is
CPU-only and its texture coordinates are explicitly placeholders. A production
Windows/GPU renderer, real-asset GPU rendering, FPS/frame-time/VRAM/input-latency
measurements, long-session stability and authorised server compatibility remain
unverified or not executed until actual evidence is recorded. The next task is
the Windows GPU renderer vertical slice, as defined in [NEXT_TASK.md](NEXT_TASK.md).
