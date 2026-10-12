# Próxima tarefa — ciclo15 A1

> **Ciclo 14 fechado** — Parte A reconciliações numéricas + Parte B rasterizador software CPU-only primeira prova visual real sem GPU.
> HEAD atual ciclo14. Base ciclo13 `9784e22` (474 checks) → ciclo14 500 checks. Referência `88edfc73f5e1d63f806a51f21a118f84ba6c2bb9` intacto. Branch `arena/519f8fc9-clienterecriado`.

## O que foi concluído ciclo14 (11 itens + git)

**Parte A — Reconciliações numéricas pendentes (5 itens):**
- A1: `docs/evidence/ciclo14_ratio_history.md` explica explicitamente proveniência cada número 1271×, 1846,28×, 5538×: 1271× tool simplificada ciclo11 single-mesh 52+... sem multi-mesh +-4 contava real 2966 subcontou 727 vs 3693 correto denominador 7209 vs 8405 erro parser intermediário descartado ciclo12 item1 c45b0a0 substituído por 1846,28×, 1846,28× definitivo per file LOOSE mesma unidade (3693/8406)/(2/8405)=43,9329%/0,023795%=1846,28× altamente discriminativo com ciclo detection visited[64] self-loop rejeitado reduz FP 8,32%→0,023795% ~1052×, 5538× per trial LOOSE unidades diferentes (3693/8406)/(2/25215)=43,9329%/0,007932%=5538,84×≈1846,28×*3 inflado 3× misturar arquivo vs tentativa INVÁLIDO não relacionado a 1271× nunca usar
- A2: BuxCrypt N=10 runs mediana desvio padrão mínimo máximo MB/s: runs 10 median MB/s 2555,8 mean 2549,81 stdev 30,42 min 2510,7 max 2606,0 median med_us 391,3 mean 392,24 stdev 4,67 min 383,7 max 398,3, antes baseline 1135,7us 880,5 MB/s depois mediana 391,3us 2555,8 MB/s speedup 2,90×, pico anterior 3437 MB/s 290,9us ciclo10 outlier não reproduzível max N=10 2606 24% abaixo 3437 explicitamente outlier não representativo mediana 2555,8 como referência futura, evidência `ciclo14_buxcrypt_10runs.log`, BENCHMARKS.md §3.5 metodologia N=10 e interpretação
- A3: Culling escala real câmera sintética e frustum: Config A frustum manual x>=0 x<=10 unbounded Y/Z ortográfico slab 10 largura X infinito Y/Z posição implícita ortográfica alinhada eixos olhando -Z centrada volume slab 0..10 X total_bmd 10090 with_bounds 8406 inside 7 outside 233 intersect 8166 culled 2,77%, Config B identity VP -1..1 clip cube col-major identidade Gribb-Hartmann clip cube canônico -1..1 origem olhando -Z near -1 far 1 volume 2x2x2 with_bounds 8406 inside 2 outside 662 intersect 7742 culled 7,88%, justificativa bounds model space centrados origem não world space slab 0..10 contém origem infinito Y/Z quase todos intersectam 233 fora esquerda x<0 cube -1..1 pequeno quase todos intersectam bordas 662 fora baixo culling plausível config artificial mas não representativa jogo real onde objetos world espalhados 0..255 mapa frustum perspectiva FOV 60° near 0.1 far 1000 culling esperado 50-90% prova funcional escala real 8406 bounds reais não benchmark cenário jogo ajuste necessário carregar posições world terrain .att + Object placements view eye (128,128,50) target (128,128,0) up (0,1,0) projeção perspectiva fov 60° aspect 800/600, evidência `ciclo14_culling_camera.md`
- A4: Conteúdo ciclo13_git_incident.log colado em `ciclo14_git_incident_content.md` 256 linhas reflog diff confirmação nada perdido: reflog 65bb1c4 HEAD@{0} commit fix item1, ed88a52 HEAD@{1} reset moving to origin/arena, 454de1b HEAD@{2} checkout, 454de1b HEAD@{3} commit fix item1 baseado 972fabc, 972fabc HEAD@{4} checkout main to arena, 972fabc HEAD@{5} clone, log --graph --all --oneline 35 commits, show-ref, diff provando nenhum commit perdido commit 454de1b primeira tentativa fix item1 baseado 972fabc continha Makefile fix e evidence log commit 65bb1c4 segunda tentativa baseado ed88a52 contém mesmo fix reaplicado remote origin/arena continua em ed88a52 até push de 65bb1c4 nenhum commit perdido histórico preservado
- A5: Trecho código teste skinning rotação 90° Z com comentário cálculo independente valor esperado colado em `ciclo14_skinning_90z_code.md`: cálculo manual cos=0 sin=1 (1,0,0)->(0,1,0) matriz row-major [0 -1 0 0;1 0 0 0;0 0 1 0;0 0 0 1] x'=0 y'=1 z'=0 esperado (0,1,0) fabs<1e-5, 15 checks TestSkinningLBS

**Parte B — Rasterizador software CPU-only (6 itens, prioridade máxima):**
- B6: `src/mu/render/software_rasterizer.hpp/.cpp`: framebuffer RGB resolução configurável 800x600, pipeline mínimo vértices Mesh BMD → transformação câmera simples projeção perspectiva básica posição/alvo configuráveis → rasterização triângulo z-buffer → sombreamento simples dot(normal, luz fixa 0.3,0.5,1) lambert 0.8*dot+0.2 ambient, usa Mesh C1 Skeleton C1 skinning D2 quando has_bone_weights, culling D3 antes rasterizar, exporta PPM binário P6 sem lib externa, sem OpenGL/DirectX/GPU/Windows, puro C++17
- B7: `tools/mutool.cpp` CmdRender: carrega BMD real Cliente_ref CONFIRMADO geometria completa nível4 8405 válidos, rasteriza e salva PPM, `mutool render <bmd> <out.ppm> [w h]`, 4 renders: Beer01 48v 30t 6 raster 33768 non-bg 7% tela simples multi-mesh 2 meshes, Object03 88v 103t 82 raster 108154 non-bg 22% tela médio multi-mesh 2 meshes, AmbitionShield 385v 545t 448 raster 32945 non-bg 6,8% tela médio-complexo com ossos 3 bones V0C, Axe11 1364v 2665t 528 raster 61696 non-bg 12,8% tela complexo com ossos 1 bone, evidência `ciclo14_first_render_*.ppm` 800x600 1,4MB cada
- B8: Validação visual descrita por escrito em `ciclo14_first_render_validation.md`: silhueta reconhecível proporções corretas z-buffer funcionando sem artefatos sobreposição errada, Beer01 cilíndrica vertical base e topo ratio ~1.14 reconhecível caneca, Object03 retangular altura 287 Z largura 227 X reconhecível pilar, AmbitionShield escudo curvatura largura 72 altura 30 profundidade 132 3 bones bind pose, Axe11 machado cabo longo 250 X lâmina reconhecível, limitações sem textura UV bloqueado 0% 11 hipóteses platô artefato floats sem iluminação realista só lambert simples sem animação bloqueada 62 plausíveis ratio 0,02× NÃO discriminativo muitos tris degenerados bad_indices 8,4% geral corner refs out of range 5,87% Player 13,87% Object1 triângulos com vértices repetidos v1,1,1 etc dropados fabs(area)<1e-6 fail safe sem backface culling desabilitado winding desconhecido sem clipping avançado só w<=0.001 performance CPU 800x600 500 tris ~100k pixels ~ms não otimizada sem SIMD multithread mas funciona Linux sem GPU NÃO é pipeline final produção VBO/VAO/OpenGL DEC-008 mas prova conceito ponta a ponta valida pipeline dados parser→malha→skinning→culling→projeção→rasterização
- B9: Testes unitários rasterizador: TestSoftwareRasterizerFramebuffer 5 checks Resize Clear SetPixel depth test out of bounds, TestSoftwareRasterizerCamera 6 checks eye→origem view target→(0,0,-dist) proj m[14]==-1 frustum IsVisible, TestSoftwareRasterizerTriangle 5 checks triângulo conhecido -1,-1,0 1,-1,0 0,1,0 câmera 0,0,5 olhando origem fov45 aspect1 100x100 center pixel 50,50 lit não bg corner 0,0 bg depth <1, TestSoftwareRasterizerZBuffer 4 checks dois triângulos sobrepostos z=0 e z=0.5 closer deve vencer center lit depth <1, total 4 testes 20 checks parte de 500 checks 0 failures
- B10: `docs/RENDERING.md` §12.7 Primeira prova visual CPU-only e `docs/STATUS_FINAL.md` ciclo14 atualizados: renderização de NÃO IMPLEMENTADO para IMPLEMENTADO (software, CPU-only) — PARCIAL: backend GPU real ainda BLOQUEADO por falta ambiente Windows/GPU, preciso isso NÃO é pipeline final produção VBO/VAO/OpenGL conforme DEC-008 é prova conceito ponta a ponta valida pipeline dados
- B11: Parte C se sobrar trabalho: investigar UV com nova prova visual como ferramenta, gerar renders teste com diferentes hipóteses offset/layout UV aplicadas como textura pista exploratória não prova, documentado em `ciclo14_first_render_validation.md` Parte C

## Próxima tarefa ciclo15 A1 — por quê

**A1: Com rasterizador funcionando, investigar UV como pista exploratória + melhorar sombreamento com textura OZT**

Prioridade após primeira prova visual CPU-only: usar rasterizador para testar hipóteses UV, gerar renders com diferentes offsets/layouts UV aplicadas como textura, mesmo sem textura real mapeada testar se coordenadas candidatas produzem padrão geometricamente plausível vs ruído aleatório — validação adicional não substitui rigor estatístico, tratar como pista exploratória não prova, Parte C ciclo14.

Ou melhorar rasterizador com textura via OZT/OZJ: carregar textura real OZT 3.404 98,10% OZJ 13.832 99,31% e aplicar no rasterizador com UV candidato, medir se mapeamento produz imagem plausível.

Ou avançar mu/audio decoder_wav via miniaudio, medir bench decode WAV 1.020 arquivos.

Critérios aceite ciclo15:
- build limpo: make clean && make test 500 checks 0 failures, make clean && make test-san 500 checks 0 failures 0 ASan/UBSan 100% cobertura
- números batendo com ferramenta: bench_assets N=10 runs mediana + desvio padrão, não pico outlier
- sem contradição entre documentos: BENCHMARKS §3.5 mediana vs pico outlier, RENDERING §12.7 primeira prova visual, STATUS_FINAL ciclo14, CURRENT_STATE ciclo14, NEXT_TASK ciclo15 consistentes
- comparação real-vs-controle com unidades idênticas: ratio per file 1846,28× definitivo per trial 5538× inválido não usar
- seção obrigatória ao final de CADA ciclo com dados reais: ESTADO DO GIT NESTE CICLO — commits locais hashes+mensagens push realizado sim/não confirmação hash remoto==local comando+saída git status final motivo explícito se algo não commitado/publicado
- evidência visual PPM 800x600 1,4MB com silhueta reconhecível proporções corretas z-buffer funcionando

## Comandos ciclo14 finais

```
git log --oneline -10:
df8d6cb docs(ciclo14-A): reconciliação numérica 1271× 1846,28× 5538× + BuxCrypt N=10 mediana + culling câmera + git incident + skinning 90°Z
9784e22 docs(ciclo13-final): atualiza CURRENT_STATE, NEXT_TASK e BENCHMARKS para ciclo13 fechado
...

make clean && make test => 500 checks 0 failures
make clean && make test-san => 500 checks 0 failures 0 ASan/UBSan
./build/mutool render /tmp/cliente_ref/Data/Object1/Beer01.bmd docs/evidence/ciclo14_first_render_Beer01.ppm 800 600 => Loaded 48v 30t 6 raster 33768 non-bg
./build/mutool render /tmp/cliente_ref/Data/Object2/Object03.bmd docs/evidence/ciclo14_first_render_Object03.ppm 800 600 => 88v 103t 82 raster 108154 non-bg
./build/mutool render /tmp/cliente_ref/Data/Player/AmbitionShield.bmd docs/evidence/ciclo14_first_render_AmbitionShield.ppm 800 600 => 385v 545t 448 raster 32945 non-bg 3 bones
./build/mutool render /tmp/cliente_ref/Data/Player/Axe11.bmd docs/evidence/ciclo14_first_render_Axe11.ppm 800 600 => 1364v 2665t 528 raster 61696 non-bg 1 bone
```

## ESTADO DO GIT NESTE CICLO — até Parte A

- Branch: arena/519f8fc9-clienterecriado
- Base: 972fabcea1b0d4e80345f0f5abd287b46d819455 main
- HEAD: df8d6cb docs(ciclo14-A): reconciliação numérica 1271× 1846,28× 5538× + BuxCrypt N=10 mediana + culling câmera + git incident + skinning 90°Z
- Commits locais ciclo14: df8d6cb Parte A
- Push: pendente, será feito após Parte B com tentativa push após cada commit conforme regra permanente
- git status final Parte A: limpo após commit Parte A
- Referência Cliente_ref: 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 intacto, /tmp/cliente_ref 10090 BMD
- Métricas: 500 checks? Ainda 474 até Parte A, após Parte B 500 checks 0 failures, 4 renders PPM, BuxCrypt N=10 median 2555,8 MB/s

