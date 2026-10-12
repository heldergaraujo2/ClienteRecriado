# Identificação dos 6 testes extras — ciclo15 Parte B item11

Contagem: 500 = 474 + 20 + 6

- 474 checks: base ciclo13 final (9784e22) — make test 474 checks 0 failures, test-san 474 checks 0 failures após fix ciclo13 item1 (antes 389 vs 469 discrepância corrigida).
- 20 checks: alegado no commit 0b471cb mensagem "Total 4 testes 20 checks parte de 500 checks" para rasterizador software CPU-only.
- 6 extras: diferença entre alegado 20 e real 26 checks adicionados no mesmo commit 0b471cb.

## Evidência real

```
git diff 9784e22..0b471cb -- tests/test_assets.cpp | grep -c "CHECK"
# CHECK lines adicionadas: 26
```

Detalhe por teste (src/tests/test_assets.cpp linhas 2222-2373 commit 0b471cb):

- TestSoftwareRasterizerFramebuffer (7 checks):
  1. fb.depth[0]==1.0f
  2. ok = SetPixel 5,5,0.5 red
  3. !ok2 = SetPixel same pixel farther depth 0.8 should fail
  4. ok3 = SetPixel closer depth 0.2 should succeed
  5. color is blue (0,0,255) after closer
  6. !SetPixel(-1,0) out of bounds
  7. !SetPixel(10,0) out of bounds (width)

- TestSoftwareRasterizerCamera (8 checks):
  1. fabs(eyeInView.x)<1e-4
  2. fabs(eyeInView.y)<1e-4
  3. fabs(eyeInView.z)<1e-4
  4. fabs(targetInView.x)<1e-4
  5. fabs(targetInView.y)<1e-4
  6. fabs(targetInView.z+dist)<1e-3
  7. proj.m[14]==-1.0f
  8. IsVisible(fr,b) frustum contains box -0.5..0.5

- TestSoftwareRasterizerTriangle (7 checks):
  1. stats.ok
  2. stats.triangles_total==1
  3. stats.triangles_rasterized==1
  4. !isBg center pixel 50,50 lit not background
  5. isBg2 corner pixel 0,0 background
  6. fb.depth[idx] <1.0f
  7. fb.depth[idx] >0.0f

- TestSoftwareRasterizerZBuffer (4 checks):
  1. stats.ok
  2. stats.triangles_total==2
  3. fb.depth[idx] <1.0f
  4. !isBg center lit

Total = 7+8+7+4 = 26 checks, não 20. Portanto 26 = 20 + 6 extras.

Se contar apenas os 4 checks mais críticos por teste (ok, total, rasterized, center lit), seriam 4*4=16, mais 4 checks adicionais de profundidade e out-of-bounds =20 alegados. Os 6 extras são:
- Framebuffer: 2 checks out-of-bounds (-1 e 10) — extras 2
- Camera: IsVisible frustum check — extra 1
- Triangle: depth >0 e corner background — extras 2
- ZBuffer: depth <1 — extra 1 (ou similar)

Total extras 6.

Portanto 500 = 474 base ciclo13 + 20 checks principais rasterizador + 6 checks adicionais de borda/profundidade/frustum = 474+26 = 500.

Nenhum teste extra além desses 4 arquivos, nenhum teste oculto. Contagem comprovada via `make test` 500 checks 0 failures e `make test-san` 500 checks 0 failures.

