# Manifesto workspace — ciclo15 Parte A item4

Cataloga o que foi preservado, o que foi intencionalmente descartado e por quê, e confirmação de que nenhum dado sensível/proprietário de grande volume foi commitado.

## O que foi preservado — lista de arquivos + para onde foram movidos

Todos os itens categoria (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO foram movidos/copiados de /tmp para tools/investigation/ com cabeçalho ciclo pergunta referência log.

| Origem fora repo | Destino no repo | Ciclo | Pergunta respondida | Evidência | Tamanho |
|---|---|---|---|---|---|
| /tmp/add_skinning_nontrivial.py | tools/investigation/add_skinning_nontrivial.py | ciclo13 item4 | Adiciona teste rotação 90° Z não-trivial para D2, corrige classificação otimista só identity/self-loop/zero | docs/evidence/ciclo13_c1_e3_detailed.log | 2071 B |
| /tmp/bench_ozt.cpp | tools/investigation/bench_ozt.cpp | ciclo13 item10 | Benchmark OztToRgbaTopDown old vs new 2-pixel swap bit ops, mede ganho otimização OZT | docs/evidence/ciclo13_bench_after.log | 3061 B |
| /tmp/bench_ozt2.cpp | tools/investigation/bench_ozt2.cpp | ciclo13 item10 | Benchmark OztToBgraTopDown GL_BGRA direct 10,2× speedup CAND-001→OPT-005 | docs/evidence/ciclo13_bench_after.log | 2026 B |
| /tmp/cull_scale.cpp | tools/investigation/cull_scale.cpp | ciclo13 item5 | Medição culling escala real 8406 bounds reais total_bmd 10090 with_bounds 8406 inside 7 outside 233 intersect 8166 culled 2,77% frustum 0..10 X | docs/evidence/ciclo13_cull_scale.log | 2466 B |
| /tmp/cull_scale2.cpp | tools/investigation/cull_scale2.cpp | ciclo13 item5 | Medição culling escala real identity VP -1..1 clip cube with_bounds 8406 inside 2 outside 662 intersect 7742 culled 7,88% | docs/evidence/ciclo13_cull_scale.log | 2049 B |
| /tmp/debug_beer.cpp | tools/investigation/debug_beer.cpp | ciclo14 B | Debug Beer01.bmd triângulos degenerados v1,1,1 area 0, por que rasterizador só 6/30 tris | docs/evidence/ciclo14_first_render_validation.md | 1326 B |
| /tmp/debug_bonfire.cpp | tools/investigation/debug_bonfire.cpp | ciclo14 B | Debug Bonfire01.bmd clip ndc x/w y/w z/w, por que culled_frustum 72 | docs/evidence/ciclo14_first_render_validation.md | 2541 B |
| /tmp/debug_bonfire2.cpp | tools/investigation/debug_bonfire2.cpp | ciclo14 B | Debug Bonfire áreas degeneradas idx 1,1,1 area 0 w, por que rasterizado 0 | docs/evidence/ciclo14_first_render_validation.md | 3255 B |
| /tmp/find_bones.cpp | tools/investigation/find_bones.cpp | ciclo14 B | Encontra arquivos com bones has_bones true, identifica modelos com ossos para render | docs/evidence/ciclo14_first_render_*.ppm | 1329 B |
| /tmp/test_frustum.cpp | tools/investigation/test_frustum.cpp | ciclo14 B | Teste frustum culling com câmera 0,0,5 olhando origem, por que Bonfire culled far plane 100 vs radius*10 | docs/evidence/ciclo14_culling_camera.md | 1674 B |
| /tmp/fix_makefile.py | tools/investigation/fix_makefile.py | ciclo13 item1 | Fix Makefile test-san 389 vs 469 discrepância, sanitização cobrir 100% checks | docs/evidence/ciclo13_test_san_full.log | 2412 B |
| /tmp/fix_makefile2.py | tools/investigation/fix_makefile2.py | ciclo13 item1 | Segunda tentativa fix Makefile test-san 389 vs 469 | docs/evidence/ciclo13_test_san_full.log | 2355 B |
| /tmp/update_ozt.py | tools/investigation/update_ozt.py | ciclo13 item10 | Atualiza ozt.cpp com otimização 2px + BGRA direct OztToBgraTopDown, implementar OPT-005 | docs/evidence/ciclo13_bench_after.log | 3305 B |
| /tmp/update_bench.py | tools/investigation/update_bench.py | ciclo13 item10 | Atualiza bench_assets.cpp com bench OztToBgraTopDown 256x256 GL_BGRA | docs/evidence/ciclo13_bench_after.log | 874 B |
| /tmp/run_bux10.sh | tools/investigation/run_bux10.sh | ciclo14 A2 | Roda N=10 benchmark BuxCrypt mediana desvio padrão, reconciliação pico 3437 vs mediana | docs/evidence/ciclo14_buxcrypt_10runs.log | 170 B |
| /tmp/test.log | tools/investigation/test.log | ciclo13 | Log testes intermediário | docs/evidence/ciclo13_test_san_full.log | 11554 B |
| /tmp/reflog.txt | tools/investigation/reflog.txt | ciclo13 item2 | Reflog git incident | docs/evidence/ciclo13_git_incident.log | 611 B |
| /tmp/graph.txt | tools/investigation/graph.txt | ciclo13 item2 | Graph log git incident | docs/evidence/ciclo13_git_incident.log | 3219 B |

Total preservado: 18 arquivos categoria (a), todos com cabeçalho ciclo pergunta referência log, tamanho total ~50KB, em tools/investigation/.

## O que foi intencionalmente descartado e por quê

### Categoria (b) CLONE DE REPOSITÓRIO EXTERNO RE-OBTENÍVEL — NÃO commitado

- /tmp/cliente_ref — clone de https://github.com/heldergaraujo2/Cliente.git — 29,060 files 2,372.04 MB 10,090 BMD — re-obtenível via `git clone https://github.com/heldergaraujo2/Cliente.git /tmp/cliente_ref`, hash 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9 documentado em REFERENCE_AUDIT.md §4.2/4.3, LICENSING.md §2, DEPENDENCIES.md §1, CURRENT_STATE.md, STATUS_FINAL.md, NEXT_TASK.md, ciclo15_workspace_inventory.log. Não precisa ser commitado, pode ser re-clonado, URL/commit/hash já documentado para permitir re-obtenção exata.
- /tmp/bmdref — clone de https://github.com/xulek/muonline-bmd-viewer commit 3f11bf89f3702261174d8efa1436d56a15c7d97a — re-obtenível via `rm -rf /tmp/bmdref && git clone https://github.com/xulek/muonline-bmd-viewer /tmp/bmdref && git checkout 3f11bf89f3702261174d8efa1436d56a15c7d97a`, evidência docs/evidence/ciclo6_bmdref_commit_date.log com Author xulek <xulek121@gmail.com> AuthorDate 2026-07-24 21:10:59 +0200 CommitDate idem, package.json author xulek <xulek@example.com> placeholder license ISC sem LICENSE file, histórico 2025-07-02 a 2026-07-24 com desordem 2025-11-09 clock skew, nada copiado em bloco git ls-files | grep -ci bmdref 0. Não precisa ser commitado.
- /tmp/compare_old_new — mencionado em relatórios anteriores como diretório temporário para comparar versões, não existe atualmente em /tmp, se existiu era volátil e re-criável, não precisa ser commitado.

Justificativa descarte categoria (b): clones são re-obteníveis via URL pública, commit hash documentado, não são descoberta única, não precisam ocupar espaço no repo.

### Categoria (c) DADO BRUTO VOLÁTIL GRANDE — NÃO commitar

- /tmp/cliente_ref/Data/ — cópia completa árvore Data 2,4 GB 29,060 files — NÃO commitar viola regra permanente "Não converter, comprimir, redimensionar ou modificar em lote os assets originais sem verificar compatibilidade e manter cópia de referência. Preferir carregadores compatíveis com os formatos existentes." e regra "Não publicar assets proprietários", documentar caminho /tmp/cliente_ref/Data e hash origem https://github.com/heldergaraujo2/Cliente.git commit 88edfc73f5e1d63f806a51f21a118f84ba6c2bb9, inventário em REFERENCE_AUDIT.md §4.2/4.3, bmdreport 10090 files, mutool scan 29,060 files 2,372.04 MB.
- /tmp/cliente_ref/ScreenShots/ — 51,7 MB screenshots — NÃO commitar, não deveriam shippar conforme SECURITY.md, DEPENDENCIES.md.
- /tmp/cliente_ref/Data/Custom/ etc — assets proprietários — NÃO commitar, mesma justificativa.

Justificativa descarte categoria (c): dados brutos grandes proprietários não podem ser publicados, violam licença, tamanho excede cap 128MB, já documentado hash e caminho para re-obtenção via clone.

### Categoria (d) ARTEFATO DE BUILD/BINÁRIO TEMPORÁRIO — descartável

- /tmp/bench_ozt — binário compilado de bench_ozt.cpp 17240 bytes — descartável, fonte preservada em tools/investigation/bench_ozt.cpp
- /tmp/bench_ozt2 — binário de bench_ozt2.cpp 17240 bytes — descartável, fonte preservada
- /tmp/cull_scale — binário de cull_scale.cpp 56608 bytes — descartável, fonte preservada
- /tmp/cull_scale2 — binário de cull_scale2.cpp 56536 bytes — descartável, fonte preservada
- /tmp/debug_beer — binário de debug_beer.cpp 85712 bytes — descartável, fonte preservada
- /tmp/debug_bonfire — binário de debug_bonfire.cpp 116040 bytes — descartável, fonte preservada
- /tmp/debug_bonfire2 — binário de debug_bonfire2.cpp 120040 bytes — descartável, fonte preservada
- /tmp/find_bones — binário de find_bones.cpp 85976 bytes — descartável, fonte preservada
- /tmp/test_frustum — binário de test_frustum.cpp 47416 bytes — descartável, fonte preservada
- /tmp/test.ppm, /tmp/test_*.ppm — imagens PPM temporárias 1,4 MB cada — descartáveis, versões finais em docs/evidence/ciclo14_first_render_*.ppm já commitadas 0b471cb
- /tmp/arena-workspace/coding.patch, coding.diff, coding-numstat.txt — patches temporários do ambiente Arena — descartáveis, não são descoberta

Justificativa descarte categoria (d): binários temporários são descartáveis se código-fonte que os gerou já está no repositório, não precisam ser preservados, ocupam espaço desnecessário.

## Confirmação nenhum dado sensível/proprietário de grande volume foi commitado

- `git ls-files | xargs ls -lh` em tools/investigation/ mostra total ~50KB, nenhum arquivo >12KB exceto test.log 11554 B, todos são scripts de descoberta categoria (a), não contêm assets proprietários Data/, não contêm credenciais/segredos, não contêm dados pessoais, não contêm binários grandes.
- `git ls-files | grep -E "\.ppm|\.bmp|\.ozt|\.ozj|\.bmd"` mostra apenas 4 PPMs 1,4MB cada em docs/evidence/ciclo14_first_render_*.ppm que são renders gerados a partir de assets reais mas são imagens novas produzidas pelo rasterizador CPU-only, não são assets originais copiados, são evidência visual permitida (PPM trivial), tamanho total 5,6MB dentro do cap 128MB, não violam regra de não publicar assets proprietários em lote (são 4 imagens, não 2,4GB).
- Nenhum arquivo em /tmp/cliente_ref/Data/ (2,4GB) foi copiado para repo, confirmado via `find . -type f -size +10M` não retorna nada em repo, apenas em /tmp.
- Nenhum arquivo com credenciais: `grep -r "password\|secret\|token\|key" tools/investigation/` não retorna credenciais reais, apenas chaves de cifra BuxCrypt FC CF AB que são públicas e documentadas.
- Clones re-obteníveis /tmp/cliente_ref e /tmp/bmdref não commitados, apenas documentados URL/commit/hash em LICENSING.md e DEPENDENCIES.md.
- Confirmação com `git ls-files | grep investigation` e `git show --stat` do commit correspondente que arquivos realmente entraram no repositório com tamanho !=0 — será feito no commit Parte A.

## Comandos verificação

```
find /tmp -type f -newer /home/user/ClienteRecriado/.git/COMMIT_EDITMSG 2>/dev/null | head -20
git ls-files | grep investigation
git show --stat HEAD
ls -lh tools/investigation/
```

Evidência: docs/evidence/ciclo15_workspace_inventory.log lista completa.

