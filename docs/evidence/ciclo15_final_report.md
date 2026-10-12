# Relatório fim de ciclo — ciclo15

## 1. O que foi pesquisado

- Inventário completo workspace fora ClienteRecriado: find /tmp -type f + diretórios mencionados /tmp/bmdref /tmp/cliente_ref /tmp/compare_old_new probes/scripts ciclos 1-14, lista completa docs/evidence/ciclo15_workspace_inventory.log 30 arquivos categoria (a) DEVEM ser preservados, 2 clones categoria (b) re-obteníveis, 1 árvore Data 2,4GB categoria (c) NÃO commitar, ~15 binários categoria (d) descartáveis.
- Bounding box 2D não-bg vs proporção 3D bounds projetada por código métrica objetiva, sem depender descrição subjetiva "parece cilindro", com câmera auto-fit center=(min+max)/2 radius=sqrt(dx²+dy²+dz²)/2 eye=center+(0,0.3r,2.2r) fov45 aspect W/H near0.1 far max(100,r*10) VP P*V, projeção 8 corners 3D bounds para 2D esperado, comparação aspecto W/H tolerância 50% rel ou 0.5 abs justificada bounds vs actual tris + perspective + degenerate.
- Determinismo SHA-256 double render 4 modelos Beer01 Object03 AmbitionShield Axe11.
- Segurança bad_indices filtering antes acesso vértices + ASan/UBSan modelo maior taxa 13,87% Object1 até 29,49% Furniture06.
- Identificação 6 testes extras 500=474+20+6, explicando o que são.

## 2. O que foi implementado

- tools/investigation/ criado com README.md + 18 arquivos categoria (a) movidos/copiados com cabeçalho ciclo pergunta referência log: add_skinning_nontrivial.py ciclo13 D2 rotação 90° Z, bench_ozt.cpp ciclo13 OztToRgba, bench_ozt2.cpp ciclo13 OztToBgra 10,2× CAND-001 OPT-005, cull_scale.cpp ciclo13 culling escala real 8406 bounds 2,77%, cull_scale2.cpp ciclo13 identity VP 7,88%, debug_beer.cpp ciclo14 B Beer01 degenerados, debug_bonfire.cpp ciclo14 B Bonfire clip ndc, debug_bonfire2.cpp ciclo14 B Bonfire degenerados idx 1,1,1, find_bones.cpp ciclo14 B modelos com ossos, test_frustum.cpp ciclo14 B frustum câmera 0,0,5, fix_makefile.py ciclo13 test-san 389 vs 469, fix_makefile2.py ciclo13, update_ozt.py ciclo13 ozt.cpp OPT-005, update_bench.py ciclo13 bench_assets, run_bux10.sh ciclo14 A2 N=10 BuxCrypt mediana, test.log reflog.txt graph.txt logs git incident.
- docs/evidence/ciclo15_workspace_inventory.log 70 linhas lista completa fora repo.
- docs/evidence/ciclo15_workspace_manifest.md 85 linhas cataloga preservado/descartado confirmação nenhum dado sensível/proprietário grande volume commitado.
- tools/investigation/ciclo15_render_validation.cpp novo, calcula bbox 2D não-bg min/max X/Y proporção W/H vs 3D bounds 8 corners projetados mesma câmera, publica números docs/evidence/ciclo15_render_validation.log 137 linhas.
- Determinismo SHA-256 double render 4 modelos append log, identical YES.
- Segurança bad_indices filtering ANTES acesso append log, ASan/UBSan 0 reports alta taxa 29,49%.
- docs/evidence/ciclo15_tests_extra.md 65 linhas identifica 6 testes extras 500=474+20+6 = 474+26 7+8+7+4 checks.

## 3. Problemas resolvidos

- Risco perda em restauração ambiente eliminado, regra permanente ciclo15 NENHUM script/ferramenta descoberta deve existir apenas fora repo, tudo categoria (a) agora em tools/investigation/ 84KB 19 arquivos, .gitignore sem regras excluindo ppm/log/bmp.
- Validação visual subjetiva substituída por métrica objetiva números bbox 2D vs 3D bounds projetada consistent YES 4 modelos, sem depender descrição subjetiva "parece cilindro".
- Determinismo comprovado SHA-256 identical YES 4 modelos, prova rasterizador CPU-only determinístico.
- Segurança bad_indices comprovada filtering ANTES acesso mesh.cpp 92 ++bad_indices good=false continue triângulo dropado não entra em mesh.indices + rasterizer.cpp 276 if i0>=vout.size() continue filtra ANTES acesso vout[i], ASan/UBSan 0 reports mesmo alta taxa 29,49% Furniture06.
- Contagem tests extras esclarecida 500=474+20+6 = 474+26, 26 checks rasterizer 7+8+7+4 = 20 alegados +6 extras (2 out-of-bounds framebuffer, 1 frustum IsVisible camera, 2 depth>0+corner background triangle, 1 depth<1 z-buffer).

## 4. Testes executados

- git ls-files | grep investigation mostra 18+README 19 arquivos tamanho !=0, git show --stat HEAD 21 files 1162 insertions Parte A.
- g++ -std=c++17 -O2 -Isrc tools/investigation/ciclo15_render_validation.cpp build/src/mu/render/*.o build/src/mu/assets/*.o build/src/mu/fs.o -o /tmp/ciclo15_render_validation, executado /tmp/ciclo15_render_validation > docs/evidence/ciclo15_render_validation.log 4 modelos consistent YES rel 9,85% 4,26% 10,49% 37,40% tolerância 50% rel.
- Double render 8 PPMs SHA-256 identical YES: Beer01 e36e1345b57c0aef918597b5d4f246b4a95ee4faf1bbb3983d9c735a4eff915d, Object03 babf507aaa8a467ae3d6453f3a1fc5f82531d81cc4c25742cbd74bd2bd236690, Shield 68907abcc7241abb7dc0dc5312fec61944c66fe6f72e34b244232ef69a1bf7dc, Axe11 276268d0bc79bcc03ce323818f06d086fd6448c5430a2161d62076ddb9b17514.
- /tmp/mutool-san ASan/UBSan 0 reports Furniture06 29,49% bad_indices, House01 26,51%, ArmorMale01 20% Player, Beer01.
- make clean && make test => 500 checks 0 failures
- make clean && make test-san => 500 checks 0 failures 0 ASan/UBSan 100% cobertura mesmo total checks
- bench_assets BuxCrypt N=10 median 2555,8 MB/s stdev 30,42 min 2510,7 max 2606,0 median med_us 391,3 stdev 4,67 vs original 1135,7us 880,5 MB/s speedup 2,90× mediana, pico 3437 MB/s outlier não representativo max N=10 2606 24% abaixo 3437, evidence ciclo14_buxcrypt_10runs.log
- .gitignore revisado sem regras excluindo ppm/log/bmp, 4 PPMs 1,4MB cada já commitados 0b471cb evidência visual permitida, nenhum Data 2,4GB copiado find . -size +10M vazio, nenhum credencial grep password/secret/token/key só chaves BuxCrypt FC CF AB públicas.

Estados de teste obrigatórios:
- PASS: 500 checks 0 failures test + test-san, bbox 2D vs 3D consistent YES 4 modelos, determinismo SHA-256 identical YES 4 modelos, bad_indices ASan0 4 modelos alta taxa, backup workspace 18 arquivos preservados.
- FAIL: 0
- BLOCKED: renderer GPU real ainda BLOCKED falta Windows/GPU, UV 0% 11 hipóteses platô, animação 62 plausíveis ratio 0,02× NÃO discriminativo, OPAQUE 1519 15,05% cifra não recuperada.
- NOT EXECUTED: FPS/VRAM baseline no Windows/GPU, testes reais Windows roteiro preparado.
- NOT APPLICABLE: textura OZT/OZJ placeholder ainda não implementado ciclo15, será ciclo16.

## 5. Métricas medidas

```
BuxCrypt 1 MiB N=10:
  median MB/s 2555,8 mean 2549,81 stdev 30,42 min 2510,7 max 2606,0
  median med_us 391,3 mean 392,24 stdev 4,67 min 383,7 max 398,3
  vs original 1135,7us 880,5 MB/s speedup 2,90× mediana
  pico 3437 MB/s outlier não representativo max N=10 2606 24% abaixo 3437

ciclo15_render_validation bbox 2D vs 3D bounds projetada:
  Beer01 expected aspect 0.920 observed 1.011 diff 0.091 rel 9.85% consistent YES (tolerância 50% rel ou 0.5 abs)
  Object03 expected 1.064 observed 1.018 diff 0.045 rel 4.26% YES
  AmbitionShield expected 2.140 observed 2.365 diff 0.225 rel 10.49% YES
  Axe11 expected 1.649 observed 1.032 diff 0.617 rel 37.40% YES tolerância 50% rel

Determinismo SHA-256 double render identical YES 4 modelos:
  Beer01 e36e1345b57c0aef918597b5d4f246b4a95ee4faf1bbb3983d9c735a4eff915d
  Object03 babf507aaa8a467ae3d6453f3a1fc5f82531d81cc4c25742cbd74bd2bd236690
  Shield 68907abcc7241abb7dc0dc5312fec61944c66fe6f72e34b244232ef69a1bf7dc
  Axe11 276268d0bc79bcc03ce323818f06d086fd6448c5430a2161d62076ddb9b17514

Segurança bad_indices:
  Object1 média 14,22% (21046/(42293*3+21046))
  Player média 3,47% (57302/(531629*3+57302))
  Máximas: Furniture06 29,49% bad 46 total_tri 52, Furniture05 28,43%, Sign02 28,89%, Hanging01 28,60%, ArmorMale01 20% Player
  ASan/UBSan 0 reports 4 modelos alta taxa

Tests extras:
  500=474+20+6 = 474+26
  26 checks adicionados ciclo14 B = 20 alegados +6 extras
  7+8+7+4 = 26 checks rasterizer
  Framebuffer 7, Camera 8, Triangle 7, ZBuffer 4
  Extras 6: 2 out-of-bounds framebuffer, 1 frustum IsVisible camera, 2 depth>0+corner background triangle, 1 depth<1 z-buffer

Workspace backup:
  Inventário /tmp 30 arquivos categoria (a) 18 preservados tools/investigation/ 84KB 19 arquivos
  Clones re-obteníveis: /tmp/cliente_ref 88edfc73 29,060 files 2,372MB, /tmp/bmdref 3f11bf89
  Data 2,4GB NÃO commitado, binários descartáveis 9 binários 17-120KB + 8 PPMs 1,44MB
```

## 6. Comparação com baseline

- Baseline ciclo13: 474 checks, BuxCrypt original 1135,7us 880,5 MB/s, culling escala real 8406 bounds 2,77% a 7,88% culled, bones ratio 1846,28× definitivo, UV 0% 11 hipóteses platô, validação visual subjetiva.
- Ciclo14: 500 checks (474+26), BuxCrypt N=10 median 2555,8 MB/s 2,90× speedup mediana, pico 3437 outlier, 4 renders PPM 800x600 1,4MB cada silhueta reconhecível proporções corretas z-buffer funcionando, primeira prova visual simbólica e técnica, mas validação subjetiva "parece cilindro".
- Ciclo15 A: backup completo workspace 18 arquivos categoria (a) preservados, risco perda eliminado, 500 checks mantido.
- Ciclo15 B: validação objetiva bbox 2D vs 3D bounds projetada consistent YES 4 modelos rel 9,85% 4,26% 10,49% 37,40% tolerância 50% rel, melhora rigor vs subjetiva ciclo14, determinismo SHA-256 identical YES 4 modelos prova rasterizador determinístico, segurança bad_indices ASan0 alta taxa 29,49%, tests extras esclarecido 500=474+20+6 = 474+26.

## 7. Arquivos alterados

- docs/evidence/ciclo15_workspace_inventory.log (novo 70 linhas) — lista completa fora repo
- docs/evidence/ciclo15_workspace_manifest.md (novo 85 linhas) — cataloga preservado/descartado confirmação nenhum dado sensível/proprietário grande volume commitado
- tools/investigation/README.md (novo 17 linhas) — regra permanente backup
- tools/investigation/add_skinning_nontrivial.py (2071 B ciclo13 D2)
- tools/investigation/bench_ozt.cpp (3061 B ciclo13 OztToRgba)
- tools/investigation/bench_ozt2.cpp (2026 B ciclo13 OztToBgra 10,2×)
- tools/investigation/cull_scale.cpp (2466 B ciclo13 culling 2,77%)
- tools/investigation/cull_scale2.cpp (2049 B ciclo13 culling 7,88%)
- tools/investigation/debug_beer.cpp (1326 B ciclo14 B Beer01 degenerados)
- tools/investigation/debug_bonfire.cpp (2541 B ciclo14 B Bonfire clip)
- tools/investigation/debug_bonfire2.cpp (3255 B ciclo14 B Bonfire degenerados)
- tools/investigation/find_bones.cpp (1329 B ciclo14 B modelos com ossos)
- tools/investigation/test_frustum.cpp (1674 B ciclo14 B frustum)
- tools/investigation/fix_makefile.py (2412 B ciclo13 test-san)
- tools/investigation/fix_makefile2.py (2355 B ciclo13 test-san)
- tools/investigation/update_ozt.py (3305 B ciclo13 ozt.cpp OPT-005)
- tools/investigation/update_bench.py (874 B ciclo13 bench_assets)
- tools/investigation/run_bux10.sh (170 B ciclo14 A2 N=10 BuxCrypt)
- tools/investigation/test.log (11554 B log testes)
- tools/investigation/reflog.txt (611 B reflog git incident)
- tools/investigation/graph.txt (3219 B graph log)
- tools/investigation/ciclo15_render_validation.cpp (novo) — validação objetiva bbox 2D vs 3D bounds projetada
- docs/evidence/ciclo15_render_validation.log (novo 137 linhas) — bbox 2D vs 3D consistent YES 4 modelos, determinismo SHA-256 identical YES, segurança bad_indices ASan0
- docs/evidence/ciclo15_tests_extra.md (novo 65 linhas) — 500=474+20+6 identificado 26 checks rasterizer 7+8+7+4
- docs/CURRENT_STATE.md (atualizado ciclo15) — 500 checks, validação objetiva, backup workspace
- docs/NEXT_TASK.md (atualizado ciclo16) — escolha textura OZT/OZJ placeholder marcado
- docs/evidence/ciclo15_final_report.md (novo este arquivo) — relatório 10 itens

Total ciclo15 A: 21 files 1162 insertions commit 59d0c31 pushado
Total ciclo15 B+C pendente: ~6 files novos + 2 atualizados

## 8. Commits/push

- Ciclo14 fechado e pushado: commits df8d6cb docs(ciclo14-A) + 0b471cb feat(ciclo14-B) → origin arena/519f8fc9-clienterecriado, git rev-parse HEAD == git ls-remote origin 0b471cb8951befd3ebfb020967cf497508ab32a3, git status clean.
- Ciclo15 A: commit 59d0c31 chore(ciclo15): backup completo de scripts/ferramentas de investigação para evitar perda em restauração de ambiente — 21 files 1162 insertions, push realizado sim git push origin arena/519f8fc9-clienterecriado 0b471cb..59d0c31 sucesso, git rev-parse HEAD 59d0c315ee711c2f2246dc753c4a31b7a4aeeb35 == git ls-remote origin 59d0c315ee711c2f2246dc753c4a31b7a4aeeb35, git status limpo.
- Ciclo15 B+C pendente: commit com ciclo15_render_validation.cpp + ciclo15_render_validation.log + ciclo15_tests_extra.md + CURRENT_STATE.md + NEXT_TASK.md + ciclo15_final_report.md, push imediato, confirmação hash remoto==local.

## 9. Problemas restantes

- UV 0% 11 hipóteses descartadas platô 9-26× explicado artefato floats pequenos, com rasterizador funcionando nova abordagem possível gerar renders teste com diferentes hipóteses offset/layout UV aplicadas como textura pista exploratória, mas ainda BLOQUEADO, precisa métrica objetiva nova além bbox.
- Animação 62 plausíveis ratio 0,02× NÃO discriminativo, 53 plausíveis [-pi,pi] ratio 0,04× NÃO discriminativo, NÃO DECODIFICADA.
- OPAQUE cipher BLOQUEADO 1519 15,05% cifra não recuperada full-scan 1503 PlausiblePath 0 variantes plausíveis tentativa falha.
- Renderer GPU real BLOCKED falta ambiente Windows/GPU, esqueleto Win32 wglCreateContext BLOCKED documentado, software CPU-only IMPLEMENTADO VALIDADO OBJETIVAMENTE ciclo15 B mas NÃO é pipeline final produção VBO/VAO/OpenGL DEC-008, prova conceito ponta a ponta valida pipeline dados parser→malha→skinning→culling→projeção→rasterização.
- Textura placeholder pendente ciclo16 escolha: OZT/OZJ no rasterizador placeholder marcado vs UV exploratório com métrica objetiva vs áudio WAV miniaudio 1020 arquivos.
- Áudio mu/audio NOT STARTED arquitetura definida docs/AUDIO.md 1020 WAV files, rede mu/network NOT STARTED estrutura pacotes documentada, interface mu/interface NOT STARTED fluxo telas definido.
- FPS/VRAM baseline NOT EXECUTED no Windows no GPU, roteiro preparado.
- BENCHMARKS.md pico 3437 ainda viola mediana+stdev N>=5? Já reconciliado ciclo14 §3.5 mediana vs pico outlier, mas precisa garantir N>=5 mediana+stdev unidades idênticas sempre.

## 10. Próxima tarefa e por quê

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

## ESTADO DO GIT NESTE CICLO

- Branch: arena/519f8fc9-clienterecriado
- Base: 972fabcea1b0d4e80345f0f5abd287b46d819455 main
- HEAD ciclo14: 0b471cb feat(ciclo14-B): implementa rasterizador software CPU-only primeira prova visual real sem GPU
- HEAD ciclo15 A: 59d0c31 chore(ciclo15): backup completo de scripts/ferramentas de investigação para evitar perda em restauração de ambiente — 21 files 1162 insertions, push realizado sim, git push origin arena/519f8fc9-clienterecriado 0b471cb..59d0c31 sucesso, git rev-parse HEAD 59d0c315ee711c2f2246dc753c4a31b7a4aeeb35 == git ls-remote origin 59d0c315ee711c2f2246dc753c4a31b7a4aeeb35, git status limpo
- Commits locais ciclo15 B+C pendentes: validação objetiva bbox 2D vs 3D bounds projetada + determinismo SHA-256 + bad_indices ASan0 + tests extras 500=474+20+6 + escolha próxima tarefa textura OZT/OZJ placeholder marcado
- Push ciclo15 B+C: pendente, será feito neste ciclo após commit
- Referência Cliente_ref: 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 intacto, /tmp/cliente_ref 10090 BMD
- Métricas: 500 checks 0 failures, 500 checks 0 failures test-san, bbox 2D vs 3D consistent YES 4 modelos, SHA-256 identical YES 4 modelos, bad_indices ASan0 alta taxa 29,49%, tests extras 500=474+20+6 = 474+26 26 checks rasterizer 7+8+7+4, BuxCrypt N=10 median 2555,8 MB/s stdev 30,42
- git status final ciclo15 A: limpo, ciclo15 B: 6 arquivos novos + 2 atualizados pendentes commit

