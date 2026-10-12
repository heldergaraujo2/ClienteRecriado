# Próxima tarefa — ciclo16

> **Ciclo 15 fechado** — Parte A backup completo workspace 18 arquivos categoria (a) + Parte B validação objetiva renderizador bbox 2D vs 3D bounds projetada consistent YES 4 modelos + determinismo SHA-256 identical YES + segurança bad_indices ASan0 + tests extras 500=474+20+6 identificado.
> HEAD atual ciclo15 A 59d0c31 + B pendente commit. Base ciclo13 9784e22 (474 checks) → ciclo14 500 checks (26 checks rasterizer = 20+6 extras) → ciclo15 500 checks. Referência 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 intacto. Branch arena/519f8fc9-clienterecriado.

## O que foi concluído ciclo15 (10 itens relatório fim ciclo)

**Parte A — BACKUP COMPLETO WORKSPACE (prioridade máxima):**
1. Pesquisado: tudo fora ClienteRecriado criado/usado durante investigação ciclos 1-14, find /tmp -type f + diretórios mencionados /tmp/bmdref /tmp/cliente_ref /tmp/compare_old_new probes/scripts, lista completa docs/evidence/ciclo15_workspace_inventory.log 30 arquivos categoria (a) DEVEM ser preservados, 2 clones categoria (b) re-obteníveis, 1 árvore Data 2,4GB categoria (c) NÃO commitar, ~15 binários categoria (d) descartáveis.
2. Implementado: tools/investigation/ criado, 18 arquivos categoria (a) movidos/copiados com cabeçalho ciclo pergunta referência log: add_skinning_nontrivial.py ciclo13 D2 rotação 90° Z, bench_ozt.cpp ciclo13 OztToRgba, bench_ozt2.cpp ciclo13 OztToBgra 10,2× CAND-001 OPT-005, cull_scale.cpp ciclo13 culling escala real 8406 bounds 2,77%, cull_scale2.cpp ciclo13 identity VP 7,88%, debug_beer.cpp ciclo14 B Beer01 degenerados, debug_bonfire.cpp ciclo14 B Bonfire clip ndc, debug_bonfire2.cpp ciclo14 B Bonfire degenerados idx 1,1,1, find_bones.cpp ciclo14 B modelos com ossos, test_frustum.cpp ciclo14 B frustum câmera 0,0,5, fix_makefile.py ciclo13 test-san 389 vs 469, fix_makefile2.py ciclo13, update_ozt.py ciclo13 ozt.cpp OPT-005, update_bench.py ciclo13 bench_assets, run_bux10.sh ciclo14 A2 N=10 BuxCrypt mediana, test.log reflog.txt graph.txt logs git incident, cada com cabeçalho.
3. Problemas resolvidos: risco perda em restauração ambiente eliminado, regra permanente ciclo15 NENHUM script/ferramenta descoberta deve existir apenas fora repo, tudo categoria (a) agora em tools/investigation/ 84KB 19 arquivos.
4. Testes executados: git ls-files | grep investigation mostra 18+README 19 arquivos tamanho !=0, git show --stat HEAD 21 files 1162 insertions, .gitignore revisado sem regras excluindo ppm/log/bmp, build/ build-san/ *.o *.a *.obj *.exe *.pdb *.ilk *.exp *.lib .vscode/ .idea/ Thumbs.db .DS_Store Cliente_ref/ apenas, 4 PPMs 1,4MB cada já commitados 0b471cb evidência visual permitida, nenhum Data 2,4GB copiado find . -size +10M vazio, nenhum credencial grep password/secret/token/key só chaves BuxCrypt FC CF AB públicas, clones re-obteníveis não commitados apenas documentados URL/commit/hash LICENSING.md DEPENDENCIES.md.
5. Métricas medidas: inventário /tmp 30 arquivos, preservado 18 categoria (a) 84KB, descartado categoria (b) /tmp/cliente_ref 29,060 files 2,372MB 88edfc73 re-obtenível git clone, /tmp/bmdref 3f11bf89 re-obtenível rm -rf /tmp/bmdref && git clone https://github.com/xulek/muonline-bmd-viewer, categoria (c) Data 2,4GB NÃO commitar, categoria (d) 9 binários 17-120KB + 8 PPMs 1,44MB descartáveis fonte preservada.
6. Comparação com baseline: ciclo14 500 checks, ciclo15 A 500 checks mantido, backup não altera lógica, apenas preservação.
7. Arquivos alterados: docs/evidence/ciclo15_workspace_inventory.log, docs/evidence/ciclo15_workspace_manifest.md, tools/investigation/README.md + 18 arquivos categoria (a), total 21 files 1162 insertions.
8. Commits/push: 59d0c31 chore(ciclo15): backup completo de scripts/ferramentas de investigação para evitar perda em restauração de ambiente, push realizado sim git push origin arena/519f8fc9-clienterecriado 0b471cb..59d0c31 sucesso, git rev-parse HEAD 59d0c31 == git ls-remote origin 59d0c31, git status limpo.
9. Problemas restantes: Parte B validação objetiva pendente antes deste commit B, Parte C escolha próxima linha pendente, UV ainda 0% 11 hipóteses platô, animação 62 plausíveis ratio 0,02× NÃO discriminativo, OPAQUE 1519 15,05% BLOQUEADO, renderer GPU real BLOCKED falta Windows/GPU, textura placeholder pendente.
10. Próxima tarefa e por quê: Parte B validação objetiva bbox 2D vs 3D bounds projetada + determinismo SHA-256 + bad_indices ASan0 + tests extras 500=474+20+6, por quê: sem métrica objetiva validação visual subjetiva "parece cilindro" não é prova, precisa números, determinismo e segurança.

**Parte B — VALIDAÇÃO OBJETIVA RENDERIZADOR (prioridade máxima ciclo15):**
1. Pesquisado: bounding box 2D não-bg vs proporção 3D bounds por código métrica objetiva, determinismo SHA-256 double render 4 modelos, bad_indices filtering antes acesso vértices + ASan/UBSan modelo maior taxa 13,87% Object1 até 29,49% Furniture06, 6 testes extras 500=474+20+6.
2. Implementado: tools/investigation/ciclo15_render_validation.cpp calcula por código bbox 2D não-bg min/max X/Y proporção W/H vs 3D bounds 8 corners projetados mesma câmera auto-fit center=(min+max)/2 radius=sqrt(dx²+dy²+dz²)/2 eye=center+(0,0.3r,2.2r) fov45 aspect W/H near0.1 far max(100,r*10) VP P*V, tolerância 50% rel ou 0.5 abs justificada bounds vs actual tris + perspective + degenerate, publica números docs/evidence/ciclo15_render_validation.log; determinismo: ./build/mutool render <bmd> <out> 800 600 duas vezes sha256sum identical YES 4 modelos Beer01 e36e1345b57c0aef918597b5d4f246b4a95ee4faf1bbb3983d9c735a4eff915d, Object03 babf507aaa8a467ae3d6453f3a1fc5f82531d81cc4c25742cbd74bd2bd236690, Shield 68907abcc7241abb7dc0dc5312fec61944c66fe6f72e34b244232ef69a1bf7dc, Axe11 276268d0bc79bcc03ce323818f06d086fd6448c5430a2161d62076ddb9b17514; segurança: mesh.cpp linha 92 ++bad_indices quando vertex_ok false good=false continue triângulo dropado não entra em mesh.indices, rasterizer.cpp linha 276 if i0>=vout.size() continue filtra ANTES acesso vout[i], ASan/UBSan 0 reports modelos alta taxa Furniture06 29,49% bad 46 total_tri 52 rate 29,49% verts 52 tris_prod 6, House01 26,51% bad 2232 total_tri 2806, ArmorMale01 20% Player; tests extras: 500=474+20+6 identificado docs/evidence/ciclo15_tests_extra.md 26 checks adicionados ciclo14 B = 20 alegados +6 extras (2 out-of-bounds framebuffer, 1 frustum IsVisible camera, 2 depth>0+corner background triangle, 1 depth<1 z-buffer), 7+8+7+4=26.
3. Problemas resolvidos: validação visual subjetiva substituída por métrica objetiva números, determinismo comprovado SHA-256 identical, segurança bad_indices comprovada ASan0, contagem tests extras esclarecida.
4. Testes executados: ciclo15_render_validation.cpp compilado g++ -std=c++17 -O2 -Isrc tools/investigation/ciclo15_render_validation.cpp build/src/mu/render/*.o build/src/mu/assets/*.o build/src/mu/fs.o -o /tmp/ciclo15_render_validation, executado /tmp/ciclo15_render_validation > docs/evidence/ciclo15_render_validation.log 4 modelos consistent YES rel 9,85% 4,26% 10,49% 37,40% tolerância 50% rel, double render 8 PPMs SHA-256 identical YES, /tmp/mutool-san ASan/UBSan 0 reports Furniture06 House01 ArmorMale01 Beer01, make test 500 checks 0 failures, make test-san 500 checks 0 failures 0 ASan/UBSan 100% cobertura, bench_assets BuxCrypt N=10 median 2555,8 MB/s stdev 30,42 min 2510,7 max 2606,0 median med_us 391,3 stdev 4,67.
5. Métricas medidas: bbox 2D vs 3D bounds projetada: Beer01 expected aspect 0.920 observed 1.011 diff 0.091 rel 9.85% consistent YES, Object03 expected 1.064 observed 1.018 diff 0.045 rel 4.26% YES, AmbitionShield expected 2.140 observed 2.365 diff 0.225 rel 10.49% YES, Axe11 expected 1.649 observed 1.032 diff 0.617 rel 37.40% YES tolerância 50% rel ou 0.5 abs justificada; determinismo SHA-256 identical YES 4 modelos hashes acima; segurança bad_indices Object1 média 14,22% (21046/(42293*3+21046)), Player média 3,47% (57302/(531629*3+57302)), máximas 29,49% Furniture06 etc; tests extras 500=474+20+6 = 474+26.
6. Comparação com baseline: ciclo14 validação visual descrita por escrito silhueta reconhecível proporções corretas z-buffer funcionando sem artefatos mas subjetiva, ciclo15 validação objetiva números bbox 2D vs 3D bounds projetada consistent YES objetiva sem depender descrição subjetiva "parece cilindro", melhora rigor.
7. Arquivos alterados: tools/investigation/ciclo15_render_validation.cpp (novo), docs/evidence/ciclo15_render_validation.log (novo 137 linhas), docs/evidence/ciclo15_tests_extra.md (novo 65 linhas), docs/evidence/ciclo15_workspace_inventory.log + manifest já commitados Parte A.
8. Commits/push: Parte A 59d0c31 pushado, Parte B pendente commit + push.
9. Problemas restantes: Parte C escolha próxima linha pendente, UV ainda 0%, animação 62 plausíveis ratio 0,02× NÃO discriminativo, OPAQUE 1519 BLOQUEADO, renderer GPU real BLOCKED, textura placeholder pendente.
10. Próxima tarefa e por quê: Parte C escolher 1 linha textura OZT/OZJ no rasterizador placeholder marcado vs UV exploratório com métrica objetiva vs áudio WAV miniaudio 1020 arquivos com justificativa + autoauditoria build limpo test-san 100% N>=5 mediana+stdev unidades idênticas, por quê: com rasterizador validado objetivamente próximo passo incremental baixo risco alto valor prioridade (6) uso CPU/GPU/RAM/VRAM (5) tempo inicialização/carregamento (7) fidelidade 3D.

## Próxima tarefa ciclo16 — por quê

**Escolha: textura OZT/OZJ no rasterizador placeholder marcado**

Justificativa: rasterizador CPU-only validado objetivamente ciclo15 B com bbox 2D vs 3D bounds projetada consistent YES 4 modelos, determinismo SHA-256 identical YES, segurança bad_indices ASan0. Próximo passo incremental de baixo risco e alto valor para prioridade (6) uso CPU/GPU/RAM/VRAM e (5) tempo inicialização/carregamento e (7) fidelidade 3D:

- OZT/OZJ decoders já otimizados e validados: OZT 3.404 98,10% OztToBgra 6,6us 38057 MB/s 9,45× median 12,26× p99 OPT-005, OZJ 13.832 99,31% OzjProbe 22,9us 8342 MB/s 7,8× OPT-004, prontos para GL_BGRA.
- Adicionar textura placeholder no rasterizador (checkerboard gerado procedural ou OZT/OZJ real carregado mas mapeado com UV candidato placeholder) marcado explicitamente como PLACEHOLDER NÃO é UV real, permitindo medir footprint textura, tempo carregamento, memória, sem alegar UV decodificado (UV ainda 0% 11 hipóteses platô).
- UV exploratório com métrica objetiva: alternativa válida, mas requer definir métrica objetiva nova além de bbox (ex.: cobertura textura, entropia imagem renderizada com diferentes offsets), risco maior de falsos positivos, melhor fazer após textura placeholder funcionar.
- Áudio WAV miniaudio 1020 arquivos: arquitetura definida docs/AUDIO.md, mas prioridade menor que fidelidade 3D (7) vs áudio não listado explicitamente em prioridades desempenho, e requer backend áudio miniaudio single-header, mais complexo que textura placeholder.

Critérios aceite ciclo16:
- build limpo: make clean && make test 500+ checks 0 failures, make clean && make test-san 500+ checks 0 failures 0 ASan/UBSan 100% cobertura
- números batendo com ferramenta: bench_assets N=5+ runs mediana + desvio padrão + mínimo + máximo + unidades idênticas (med_us, MB/s), não pico outlier
- sem contradição entre documentos: BENCHMARKS § textura placeholder vs pico vs mediana, RENDERING § textura placeholder marcado, STATUS_FINAL ciclo15, CURRENT_STATE ciclo15, NEXT_TASK ciclo16 consistentes
- comparação real-vs-controle com unidades idênticas: ratio per file 1846,28× definitivo per trial 5538× inválido não usar, textura placeholder marcada como PLACEHOLDER não UV real
- seção obrigatória ao final de CADA ciclo com dados reais: ESTADO DO GIT NESTE CICLO — commits locais hashes+mensagens push realizado sim/não confirmação hash remoto==local comando+saída git status final motivo explícito se algo não commitado/publicado
- evidência visual PPM com textura placeholder checkerboard ou OZT real mapeado placeholder, silhueta reconhecível proporções corretas z-buffer funcionando, marcado PLACEHOLDER

## Comandos ciclo15 finais

```
git log --oneline -10:
59d0c31 chore(ciclo15): backup completo de scripts/ferramentas de investigação para evitar perda em restauração de ambiente
0b471cb feat(ciclo14-B): implementa rasterizador software CPU-only primeira prova visual real sem GPU
df8d6cb docs(ciclo14-A): reconciliação numérica 1271× 1846,28× 5538× + BuxCrypt N=10 mediana + culling câmera + git incident + skinning 90°Z
9784e22 docs(ciclo13-final): atualiza CURRENT_STATE, NEXT_TASK e BENCHMARKS para ciclo13 fechado
...

make clean && make test => 500 checks 0 failures
make clean && make test-san => 500 checks 0 failures 0 ASan/UBSan
./build/mutool render /tmp/cliente_ref/Data/Object1/Beer01.bmd docs/evidence/ciclo14_first_render_Beer01.ppm 800 600 => 48v 30t 6 raster 33768 non-bg
./build/mutool render /tmp/cliente_ref/Data/Object2/Object03.bmd docs/evidence/ciclo14_first_render_Object03.ppm 800 600 => 88v 103t 82 raster 108154 non-bg
./build/mutool render /tmp/cliente_ref/Data/Player/AmbitionShield.bmd docs/evidence/ciclo14_first_render_AmbitionShield.ppm 800 600 => 385v 545t 448 raster 32945 non-bg 3 bones
./build/mutool render /tmp/cliente_ref/Data/Player/Axe11.bmd docs/evidence/ciclo14_first_render_Axe11.ppm 800 600 => 1364v 2665t 528 raster 61696 non-bg 1 bone

# Validação objetiva ciclo15 B
g++ -std=c++17 -O2 -Isrc tools/investigation/ciclo15_render_validation.cpp build/src/mu/render/*.o build/src/mu/assets/*.o build/src/mu/fs.o -o /tmp/ciclo15_render_validation
/tmp/ciclo15_render_validation > docs/evidence/ciclo15_render_validation.log
# Expected aspect vs observed aspect consistent YES 4 modelos
# Beer01 0.920 vs 1.011 diff 0.091 rel 9.85% YES
# Object03 1.064 vs 1.018 diff 0.045 rel 4.26% YES
# Shield 2.140 vs 2.365 diff 0.225 rel 10.49% YES
# Axe11 1.649 vs 1.032 diff 0.617 rel 37.40% YES tolerância 50% rel

# Determinismo
./build/mutool render /tmp/cliente_ref/Data/Object1/Beer01.bmd /tmp/ciclo15_beer_a.ppm 800 600
./build/mutool render /tmp/cliente_ref/Data/Object1/Beer01.bmd /tmp/ciclo15_beer_b.ppm 800 600
sha256sum /tmp/ciclo15_beer_a.ppm /tmp/ciclo15_beer_b.ppm => e36e1345b57c0aef918597b5d4f246b4a95ee4faf1bbb3983d9c735a4eff915d identical YES

# Segurança bad_indices
g++ -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -std=c++17 -Isrc tools/mutool.cpp build-san/src/mu/assets/*.o build-san/src/mu/render/*.o build-san/src/mu/fs.o -o /tmp/mutool-san
/tmp/mutool-san render /tmp/cliente_ref/Data/Object1/Furniture06.bmd /tmp/san_furn06.ppm 800 600 => 52v 6t 3 raster exit 0 ASan0 29,49% bad_indices
```

## ESTADO DO GIT NESTE CICLO — até ciclo15 B

- Branch: arena/519f8fc9-clienterecriado
- Base: 972fabcea1b0d4e80345f0f5abd287b46d819455 main
- HEAD ciclo14: 0b471cb feat(ciclo14-B): implementa rasterizador software CPU-only primeira prova visual real sem GPU
- HEAD ciclo15 A: 59d0c31 chore(ciclo15): backup completo de scripts/ferramentas de investigação para evitar perda em restauração de ambiente — 21 files 1162 insertions, push realizado sim, confirmação hash remoto==local 59d0c315ee711c2f2246dc753c4a31b7a4aeeb35, git status limpo
- Commits locais ciclo15 B pendentes: validação objetiva bbox 2D vs 3D bounds projetada + determinismo SHA-256 + bad_indices ASan0 + tests extras 500=474+20+6
- Push ciclo15 B: pendente, será feito neste commit
- Referência Cliente_ref: 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 intacto, /tmp/cliente_ref 10090 BMD
- Métricas ciclo15 B: 500 checks 0 failures, 500 checks 0 failures test-san, bbox 2D vs 3D consistent YES 4 modelos, SHA-256 identical YES 4 modelos, bad_indices ASan0 alta taxa 29,49%, tests extras 500=474+20+6 = 474+26 26 checks rasterizer 7+8+7+4

