# Current state — ciclo13

An honest snapshot. What exists, what works, what does not.

## 1. What exists and works — ciclo13

**An asset format library**, C++17, zero dependencies, warning-clean, 474 checks 0 failures, test-san 474 checks 0 ASan/UBSan (fix ciclo13 item1, before 389 vs 469 discrepancy corrected):

| Module | What it does | Verified against | Status ciclo13 |
|---|---|---|---|
| `mu/assets/ozt` | OZT read, write, probe, RGBA conversion, BGRA direct | 3,404 of 3,470 real files 98,10%, OztToRgbaTopDown 63,0us median 89,7us p99 3969 MB/s, OztToBgraTopDown 6,6us median 11,3us p99 38057 MB/s 9,45× median 12,26× p99 vs old 62,4us 138,6us | CONCLUÍDO E VALIDADO + OPT-005 |
| `mu/assets/ozj` | OZJ container + JPEG marker parsing word-at-a-time | 13,832 of 13,928 real files 99,31%, OzjProbe 179,2us→15,2us 11,79× median 8,47× p99 OPT-004 | CONCLUÍDO E VALIDADO + OPT-004 |
| `mu/assets/tga` | TGA probe and uncompressed read | 15 of 15 real files 100% | CONFIRMADO |
| `mu/assets/bmd` | BMD container classification, bux cipher word-at-a-time, entropy, V0C decrypt | 10,023 of 10,090 structurally valid 99,34%, payload in clear 8571 84,95% OPAQUE 1519 15,05% full-scan 1503 PlausiblePath 0 variantes plausíveis, BuxCrypt 1 MiB 382,8us 2598 MB/s 2,96× vs original 1135us 880 MB/s OPT-003 3,9× | CONCLUÍDO E VALIDADO |
| `mu/assets/bmd_report` | Three-level usability reporting (structure / cleartext / geometry) + bones ratio | total 10090 valid 10023 plaintext 8571 OPAQUE 1519 parsed 8406 with_tri 8405 with_bones_loose 3693 36,60% 43,93% geom ratio per file 1846,28× definitivo vs ctrl 0,023795% 2/8405, strict 1186 11,75% 14,11% geom taxa FP 0/25218 limite sup 0,003966% ratio >3557× per trial >1185× per file, strict ⊂ loose provado | CONCLUÍDO E VALIDADO |
| `mu/assets/bmd_body` | v0x0A body parser: vertex positions and normals, multi-mesh 14B header, bones 59B intercalado cycle detection visited[64] self-loop parent==i rejeitado bc>64 | 8406 parsed 83,31%, 8405 with triangles 83,30% 3.681.664 tris, 3693 bones loose 7879 bones, 1186 strict, max_mc 30 Object157/Object06.bmd, max_bc 63 Stick32.bmd limite prático 64 teto aceitação 63 máximo real sem contradição, max_vc 22987 Ozarugold.bmd, max_ntri 39193, 1 textura por mesh sbow06_R.jpg | CONCLUÍDO E VALIDADO |
| `mu/assets/mesh` | Mesh internal representation C1 + Skeleton | positions normals texcoords indices bone_weights vector<vector<BoneWeight>> has_bone_weights has_texcoords bounds skipped_vertices bad_indices, Bone name parent position rotation, Skeleton has_bones, MeshImportResult | CONCLUÍDO E VALIDADO — TestMeshInternalRepresentation 23 checks |
| `mu/render/buffers` | Vertex/index buffer CPU-side VBO/VAO without GL | PackMeshForUpload PackedVertex px,py,pz,nx,ny,nz,u,v,bone_ids[4],bone_weights[4],weight_count PackedBuffers vertices flat indices flat has_texcoords has_bone_weights bounds skipped_vertices bad_indices ok, PackSkeletonForUpload parents names positions rotations has_bones valid parent range self-loop cycle visited[64] | CONCLUÍDO E VALIDADO — TestPackMeshForUpload 21 checks byte a byte |
| `mu/render/skinning` | LBS skinning CPU strict without GL | Mat4 Identity TransformPoint ValidateSkeletonForSkinning has_bones parent range self-loop cycle visited[64], ApplySkinningLBS strict count positions normals mismatch has_bone_weights false retorna original zero movement, true exige bone_weights size == positions, skeleton validado, boneTransforms size == bones size, sum weight_i*(bone*pos) | CONCLUÍDO E VALIDADO — TestSkinningLBS 15 checks (a) identity (b) self-loop fails safe (c) zero movement (d) rotação 90° Z não-trivial (0,1,0) esperado cálculo manual |
| `mu/render/cull` | Frustum culling CPU bounds reais | Plane normal d, Frustum planes[6] left/right/bottom/top/near/far, FrustumFromViewProjection Gribb-Hartmann, CullBox p-vertex conservative safe invalid->kIntersect fail safe, IsVisible true para kInside/kIntersect false só kOutside invalid true, PlaneDistance | CONCLUÍDO E VALIDADO — TestFrustumCull (a) inside 2..8 kInside (b) outside -8..-2 12..18 kOutside (c) invalid never culled, TestFrustumCullRealBounds Mesh sintético 4 vértices (a) inside 0..1 frustum 0..10 kInside (b) outside -20..-10 kOutside (c) invalid true kIntersect, escala real 8406 bounds reais total_bmd 10090 with_bounds 8406 inside 7 outside 233 intersect 8166 culled 2,77% frustum 0..10 X, identity VP with_bounds 8406 inside 2 outside 662 intersect 7742 culled 7,88% |
| `mu/platform/win32_window` | Esqueleto Win32 wglCreateContext BLOCKED | Win32GLConfig width height title fullscreen colorBits depthBits stencilBits, Win32GLWindow handle opaco created flag, CreateGLWindow DestroyGLWindow SwapGLWindow PollEvents GetWin32GLStatus kBlockedNoWindowsSDK kBlockedNoGPU kBlockedNoDisplay kOk só Windows real com GPU, sequência RegisterClassExW CS_OWNDC CreateWindowExW GetDC PIXELFORMATDESCRIPTOR ChoosePixelFormat SetPixelFormat 1× wglCreateContext wglMakeCurrent loop PeekMessage/TranslateMessage/DispatchMessage QPC SwapBuffers cleanup, não inclui <windows.h> nem <GL/gl.h> propositalmente | CONCLUÍDO — esqueleto documentado BLOCKED com aviso literal e lista verificação Windows real, TestWin32GLSkeletonBlocked 4 checks |
| `mu/platform/main_loop` | Esqueleto main loop update/render/present com predição frame budget sem GL | FrameBudget target_fps target_frame_ms update_ms render_ms present_ms total_ms slack_ms over_budget frame_index, InputState quit_requested, UpdateResult visible_objects culled_objects, RenderResult draw_calls_prepared triangles_submitted, PresentResult, MainLoop ctor target_fps Tick retorna FrameBudget ordem update->render->present mede via high_resolution_clock QPC, SetTargetFps PredictOverBudget média móvel 30 frames >90% budget History 120 frames | CONCLUÍDO — TestMainLoopSkeleton 9 checks |
| `mu/assets/terrain` | Terrain classification, bux auto-detect | 265 of 265 real files 100% 97 .att still encrypted | CONFIRMADO |
| `mu/assets/manifest` | Patch manifest parse and CRC verify | 188 entries total 188 match 116 size mismatch 9 crc mismatch 11 missing 52 mutool verify | CONFIRMADO |
| `mu/assets/format` | CRC-32 slice-by-8, status, byte views | zlib-derived known answers, Crc32 1 MiB 507,2us 1971 MB/s before 2553,5us 391 MB/s speedup 5,03× OPT-001 | CONCLUÍDO E VALIDADO |
| `mu/fs` | Recursive walker, POSIX + Win32 | 29,060-file real tree | CONFIRMADO |

**A command line tool**, `build/mutool`, with `scan`, `info`, `verify`, `dedupe`, `bmdreport`. Whole-tree scan of 2,372 MB in 1.67 s. `bmdreport` exits 2, because a report that cannot deliver geometry should not claim success. Now reports 8406 parsed, 8405 with_tri, 3693 bones loose ratio 1846,28×, 1186 strict.

**A test suite**, 474 checks, 0 failures (469 ciclo12 +5 non-trivial skinning), self-generating fixtures, also clean under ASan and UBSan after fix ciclo13 item1 (before 389 vs 469 discrepancy, now 474 both), evidence `ciclo13_test_san_full.log` and `ciclo13_test_san_full_after.log`.

**A micro-benchmark harness** with median, p95, p99, mean, MB, MB/s, reps, stddev, min/max, over three buffer sizes, plus optional per-repetition disk read (MU_BENCH_FILE) so cached and I/O-bound regimes are reported separately. Now includes OztToBgraTopDown 6,6us 38057 MB/s 9,45× vs old 62,4us.

**Documentation**: audit, formats, rendering analysis with §12.6 plano otimização consistente com dados medidos LOD/streaming/draw calls, architecture with C1, decisions with DEC-013 glTF 2.0, dependencies with §7 áudio/rede/interface versão/licença/segurança, audio.md arquitetura definida formatos biblioteca pretendida integração, network.md estrutura pacotes com rótulos CONFIRMADO/PROVÁVEL/DESCONHECIDO/BLOQUEADO, interface.md fluxo telas login→server→char→world como consome assets, baselines, benchmarks, licensing, security, test results, status_final revisado ciclo13.

**Windows validation scripts** in `scripts/`, written but never run — BLOCKED sem Windows/GPU.

## 2. Measured numbers ciclo13

All from real reference tree — 29,060 files, 2,372.04 MB, 10,090 BMD.

```
OZJ   13,928 files -> 13,832 header+geometry parsed  (99.31%)   pixels NOT decoded, OzjProbe 179,2us→15,2us 11,79× median 8,47× p99 OPT-004
OZT    3,470 files ->  3,404 pixels parsed           (98.10%)   OztToRgbaTopDown 62,4us→63,0us median p99 138,6→89,7us 1,54×, OztToBgraTopDown 6,6us 9,45× median 12,26× p99 OPT-005 CAND-001
TGA       15 files ->     15 pixels parsed           (100%)
terrain    265 files ->    265 classified            (100%)   97 .att still encrypted, BuxCrypt 1 MiB 1135,7us 880 MB/s → 382,8us 2598 MB/s 2,96× vs original, 3437 MB/s 3,9× OPT-003
BMD:
  level1 structure valid 10023 / 10090 99,34%
  level2 payload in clear 8571 / 10090 84,95% (1245 plaintext +100 bux +7226 v0x0C) OPAQUE 1519 15,05% full-scan 1503 PlausiblePath 0 variantes plausíveis tentativa falha
  level3 vertex+normal 8406 / 10090 83,31%
  level4 topology 8405 / 10090 83,30% 3.681.664 tris
  level5 bones loose 3693 / 10090 36,60% 43,93% geom 7879 bones ratio per file 1846,28× definitivo vs ctrl 0,023795% 2/8405
  level5b bones strict 1186 / 10090 11,75% 14,11% geom taxa FP 0/25218 limite sup ~0,004% ratio >3557× per trial >1185× per file strict ⊂ loose provado
  UV 0 / 10090 0% 11 hipóteses descartadas platô 9-26× explicado artefato floats pequenos
  Anim extra 764 7,57% divisível bc*12 231 2,29% plausível finite<1e6 62 0,61% ratio 0,02× NÃO discriminativo plausível [-pi,pi] 53 0,53% ratio 0,04× NÃO discriminativo NÃO DECODIFICADA
  Limits: max_mc 30 Object157/Object06.bmd geral 9 com ossos Object74/Object40.bmd, max_bc 63 Stick32.bmd limite prático 64 teto aceitação 63 máximo real sem contradição, max_vc 22987 Ozarugold.bmd, max_ntri 39193, 1 textura por mesh sbow06_R.jpg

Theoretical maximum texture footprint: 2,893.80 MB sum w*h*4 over parseable headers DERIVED NOT MEASURED No process run no GPU memory observed

Integrity vs ListaPatch.txt 188 entries: compared 136 identical 116 different 20 not present 52 all 20 differences also differ file size manifest disagree CRC computation correct

mutool scan 29,060 files: 1,641–1,674 ms
mutool verify 188 entries: 42–49 ms
make test: 474 checks 0 failures
make test-san: 474 checks 0 failures 0 ASan/UBSan (fix ciclo13 item1)
bench_assets:
  OztProbe 256x256 0,0us 8622268 MB/s
  OztRead 256x256 copy 6,0us 41904 MB/s
  OztToRgbaTopDown 256x256 63,0us median p95 77,9us p99 89,7us 3969 MB/s
  OztToBgraTopDown 256x256 (GL_BGRA) 6,6us median p95 7,2us p99 11,3us 38057 MB/s 9,45× median 12,26× p99
  OztRead 512x512 58,0us 17229 MB/s
  OztRead 1024x1024 253,5us 15780 MB/s
  BuxCrypt 1 MiB 384,8us median 2598,9 MB/s vs original 1135,7us 880 MB/s 2,96× (OPT-003 290,9us 3437 MB/s 3,9×)
  ByteEntropy 1 MiB 476,9us 2096 MB/s
  Crc32 64 KiB in cache 30,3us 2060 MB/s
  ManifestParse 5000 entries 954,0us 199,9 MB/s
  OzjProbe 512x512 + 200KB scan 22,9us median 8342 MB/s vs original 179,2us 1064 MB/s 11,79× OPT-004
  LOAD-TIME expand indexed->flat 45174,9us 67,21 MiB ratio expansion/upload 10,98×
  VBO upload unique vertices only 4112,8us 41,02 MiB
  PER-FRAME volume only BLOCKED no GL context: no glDrawElements expanded 67,21 MiB per draw VBO+index buffer resident once 52,22 MiB per-frame volume ratio 1,29× corner sends expanded 2936559 per draw unique vertices indexed 1792108 uploaded once
  Culling escala real: total_bmd 10090 with_bounds 8406 inside 7 outside 233 intersect 8166 culled 2,77% frustum 0..10 X, identity VP with_bounds 8406 inside 2 outside 662 intersect 7742 culled 7,88%
```

## 3. What does not exist or is BLOCKED

| Component | Status | Evidence |
|---|---|---|
| Renderer (any backend) | NOT STARTED, esqueleto Win32 BLOCKED | win32_window.hpp NÃO COMPILADO BLOCKED requer Windows+GPU, sequência wglCreateContext documentada, TestWin32GLSkeletonBlocked BLOCKED |
| Window / input / main loop | PARTIAL — esqueleto MainLoop CONCLUÍDO update/render/present predição budget sem GL, Win32 BLOCKED | main_loop.hpp/.cpp TestMainLoopSkeleton 9 checks, win32_window BLOCKED |
| Resource cache and manager | NOT STARTED | — |
| Texture upload to GPU | NOT STARTED, but OztToBgraTopDown 6,6us ready for GL_BGRA | OPT-005 |
| Model loading | PARTIAL — 8406 vertex+normal, 8405 topology, 3693 bones loose 1186 strict, 7879 bones, UV 0% | bmd_body parser |
| Audio mu/audio | NOT STARTED, arquitetura definida docs/AUDIO.md | AUDIO.md |
| Network mu/network | NOT STARTED, estrutura pacotes documentada com rótulos docs/NETWORK.md | NETWORK.md |
| Interface mu/interface | NOT STARTED, fluxo telas definido docs/INTERFACE.md | INTERFACE.md |
| Animation | NOT DECODED — 62 plausíveis ratio 0,02× NÃO discriminativo, 53 plausíveis [-pi,pi] ratio 0,04× NÃO discriminativo | ciclo11_item5_anim_validation_full.log |
| OPAQUE cipher | BLOQUEADO — 1519 15,05% cifra não recuperada full-scan 1503 PlausiblePath 0 variantes plausíveis tentativa falha | ciclo11_opaque_step_fullscan.log |
| UV | NOT DECODED — 0% 11 hipóteses descartadas platô 9-26× explicado artefato floats pequenos | ciclo10_itemA4_uv_ratio_per_offset.log |
| FPS/VRAM baseline | NOT EXECUTED — no Windows no GPU | PERFORMANCE_BASELINE.md |
| Visual validation Windows | BLOCKED — no renderer | — |

## 4. Security and dependencies

- Parser ossos: detecção ciclo visited[64] self-loop parent==i rejeitado bc>64 limite overflow, reduz FP 8,32%→0,0079% ratio 1846,28×, testes CircularParent ExtremeCount Truncated Overflow 474 checks ASan0
- BuxCrypt word-at-a-time 8B key64[3] phase advances 2 mod 3, memcpy unaligned safe, self-inverse mantido, 880→2598 MB/s 2,96× (3437 MB/s 3,9× OPT-003)
- OzjProbe word-at-a-time 8B chunk has_zero bitmask sem FF skip 8B unaligned safe via memcpy sem strict-aliasing sem leitura além file.size preserva escape FF 00 e FF D0..D7 edge remaining<8 4 casos PASS 0 ASan/UBSan
- OztToRgbaTopDown 2px 8B via uint64_t bit ops (pixel & 0xFF00FF00) | ((pixel & 0xFF)<<16) | ((pixel>>16)&0xFF) memcpy unaligned safe, OztToBgraTopDown BGRA direct via GL_BGRA elimina swap só flip memcpy 10,2× 66,4us→6,5us 9,45× median 12,26× p99
- Nenhum include gráfico em src/ exceto esqueleto BLOCKED win32_window que não inclui <windows.h> nem <GL/gl.h> propositalmente
- Makefile fix ciclo13 item1: test-san agora cobre 100% checks 474, antes 389 vs 469 discrepância corrigida, regra permanente qualquer divergência futura entre test e test-san é bloqueio crítico automático
- Dependências: zero dep build C++17, mas cliente referência tem msvcr100 msvcp100 VS2010, MSVCP140 VCRUNTIME140 VS2015+ Audio.dll, vorbisfile.dll 999KB ogg.dll 53KB Xiph BSD-style, 3DGuard.dll hinetworks.dll License.hinet main.premium etc Comercial não redistribuível, MHPClient.dll MHPVerify.dll, PS.dll Delphi packed, AutoLogin plaintext Settings.ini SECURITY.md, Thumbs.db 12 arquivos 677KB rar/zip 14,3MB ScreenShots 51,7MB não deveriam shippar
- Licença bmdref xulek/muonline-bmd-viewer commit 3f11bf89f3702261174d8efa1436d56a15c7d97a ISC NÃO CONFIRMADA sem LICENSE file e-mail placeholder xulek@example.com, nada copiado em bloco git ls-files | grep -ci bmdref 0, clone /tmp/bmdref e /tmp/cliente_ref fora repo não commitado

## 5. Next task

**Otimizar BuxCrypt e OztToRgbaTopDown já parcialmente feito ciclo13 item10, mas ainda pode melhorar com SIMD SSE2/AVX2 e medir baseline completo**

Após ciclo13 item10, BuxCrypt 382,8us 2598 MB/s (vs original 1135us 880 MB/s 2,96×, vs OPT-003 290,9us 3437 MB/s 3,9×) e OztToBgra 6,6us 9,45× já medidos, mas próxima tarefa pode ser:
- Implementar SIMD SSE2/AVX2 para OztToRgbaTopDown BGRA→RGBA shuffle e BuxCrypt XOR 16B com _mm_xor_si128, medir bench_assets speedup >2× adicional
- Ou avançar para mu/audio decoder_wav via miniaudio, medir bench decode WAV 1.020 arquivos

Critério: build limpo, teste sanitizado cobrindo 100% testes (474 checks), números batendo com ferramenta, sem contradição entre documentos, comparação real-vs-controle com unidades idênticas.

## ESTADO DO GIT NESTE CICLO — até item9

- Branch: arena/519f8fc9-clienterecriado
- Base: 972fabcea1b0d4e80345f0f5abd287b46d819455 main
- HEAD atual: 772d0a4 docs(ciclo13-item9): revisa STATUS_FINAL.md linha a linha corrigindo classificação otimista
- Commits locais ciclo13 até item9: 65bb1c4 fix item1 469 vs 389, bf1b7f0 evidence item2-3 git incident + ratio calc, 2a44d10 evidence item4-8 C1-E3 detalhada, 772d0a4 docs item9 STATUS_FINAL revisado
- Push realizado: pendente, será pushado agora
- Referência Cliente_ref: 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 intacto, mas /tmp/cliente_ref re-clonado ciclo13 item5 para medição escala real culling 8406 bounds reais
- Métricas: 474 checks 0 failures, test-san 474 checks 0 failures 0 ASan/UBSan após fix, culling escala real 8406 bounds 2,77% a 7,88% culled, bones 3693 loose ratio 1846,28× definitivo 1186 strict limite sup 3557× per trial 1185× per file, OPAQUE 1519 15,05%, animação 62 plausíveis não confirmado ratio 0,02×, UV 0% 11 hipóteses platô explicado
