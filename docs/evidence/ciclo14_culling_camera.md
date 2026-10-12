# Culling escala real — posição/orientação câmera sintética e justificativa 2,77%/7,88%

## Configurações usadas ciclo13

### Config A — frustum manual x>=0 x<=10 unbounded Y/Z (ciclo13_cull_scale.log)
- Construção: 6 planos, normals inward
  - left: normal (1,0,0) d=0  => x >=0
  - right: normal (-1,0,0) d=10 => x <=10
  - bottom/top/near/far: normal (0,1,0) d=1e6 => efetivamente unbounded (1e6)
- Tipo: ortográfico slab 10 unidades largura em X, infinito Y/Z
- Posição/orientação câmera sintética implícita: câmera ortográfica alinhada eixos, olhando -Z, centrada, volume visível slab 0..10 X
- Não é perspectiva, não tem posição olho/alvo, é frustum sintético para teste unitário

### Config B — frustum identity view-projection -1..1 clip cube (FrustumFromViewProjection identity)
- Matriz identidade 4x4 col-major [1 0 0 0; 0 1 0 0; 0 0 1 0; 0 0 0 1]
- Extração Gribb-Hartmann: 6 planos do clip cube -1..1 em X,Y,Z
- Tipo: clip cube canônico OpenGL, equivale a câmera perspectiva com FOV 90°? Na verdade identidade VP não é perspectiva real, é sem transformação
- Posição câmera implícita: origem, olhando -Z, near -1 far 1, FOV não definido, volume cubo 2x2x2 centrado origem

## Por que 2,77% e 7,88% são plausíveis para essa configuração — ou não representativas

**Bounds reais usados:** 8406 bounding boxes extraídas de vértices decodificados de arquivos BMD reais via BmdParseBodyV0A, mas em **model space**, não world space. Cada modelo está centrado próximo origem local (ex.: Object40 0..1, Player modelos etc), não distribuídos em mundo como em jogo real onde objetos teriam posições world diferentes (mapa, terreno).

**Config A slab 0..10 X:**
- total_bmd 10090 with_bounds 8406 inside 7 outside 233 intersect 8166 culled 2,77%
- Justificativa: Como todos os bounds estão em model space centrados perto origem (0..alguns unidades), quase todos intersectam slab 0..10 que contém origem e é infinito Y/Z. Só 233 estão completamente fora à esquerda x<0 (ex.: modelos com min/max negativos). 7 totalmente dentro 0..10, 8166 intersectam porque têm parte fora slab mas parte dentro (ex.: min -1 max 1 straddles plano x=0). Baixo culling 2,77% é **plausível** para esta configuração artificial, mas **não representativa** de jogo real onde objetos world estariam espalhados 0..255 mapa e frustum perspectiva teria near/far e culling muito maior (ex.: 50-90% culled).

**Config B identity VP -1..1 cube:**
- with_bounds 8406 inside 2 outside 662 intersect 7742 culled 7,88%
- Justificativa: Clip cube -1..1 muito pequeno, contém apenas modelos minúsculos dentro -1..1. Como muitos modelos têm tamanho >2 (ex.: Ozarugold max_vc 22987), quase todos intersectam bordas cube, só 2 totalmente dentro, 662 totalmente fora (>1 ou <-1), 7742 intersectam. 7,88% culled também plausível para cube pequeno, mas também não representativa: perspectiva real teria far plane 1000, FOV 60°, posição câmera elevada olhando terreno, culling baseado em world positions.

**Ajuste necessário para cenário jogo real:**
- Para medir culling realista, precisaríamos:
  1. Carregar posições world de objetos de mapa (terrain .att + Object/*.bmd placements)
  2. Construir view matrix com posição câmera (ex.: eye (128,128,50) alvo (128,128,0) up (0,1,0))
  3. Projeção perspectiva fov 60° aspect 800/600 near 0.1 far 1000
  4. Aplicar transform world * view * projection para cada bounds
  5. Então medir culled ratio — esperado 50-90% dependendo densidade mapa, não 2,77%
- Atual medição prova que culling CPU funciona e é safe (invalid never culled, p-vertex conservative), mas **não prova ganho performance real** — é validação funcional em escala real 8406 bounds reais, não benchmark de cenário jogo.

**Conclusão:** 2,77%/7,88% são plausíveis para configurações sintéticas usadas (slab 0..10 e cube -1..1) dado que bounds são model space centrados origem, mas **não representativas** de cenário jogo real. Para próxima tarefa, ajustar teste com câmera perspectiva real eye (0,0,10) target (0,0,0) up (0,1,0) fov 45° near 0.1 far 100 e world positions variadas, ou documentar limitação explicitamente em docs/RENDERING.md §12.6.

**Evidência:** docs/evidence/ciclo13_cull_scale.log, src/mu/render/cull.cpp p-vertex test, tests/test_assets.cpp TestFrustumCull e TestFrustumCullRealBounds.

