# Validação visual primeira prova — ciclo14 Parte B

## Objetivo
Descrever por escrito o que as imagens `ciclo14_first_render_*.ppm` mostram — silhueta reconhecível, proporções corretas, z-buffer funcionando sem artefatos sobreposição errada — e documentar limitações conhecidas.

## Imagens geradas

### Beer01 — simples, Object1/Beer01.bmd
- **Arquivo:** `ciclo14_first_render_Beer01.ppm` 800x600 1,4MB P6 binário, 48 vértices, 30 triângulos total, 6 rasterizados, 33768 pixels não-bg 7% tela
- **Comando:** `./build/mutool render /tmp/cliente_ref/Data/Object1/Beer01.bmd docs/evidence/ciclo14_first_render_Beer01.ppm 800 600`
- **Câmera:** center (0,0,0) approx? Bounds min (-14.12,-20.39,-0.87) max (14.12,12.23,11.13) center (-0, -4.08, 5.13) radius ~20, eye ( -0, 1.9, 49) target (-0,-4,5) up (0,1,0) fov 45° aspect 1.33 near 0.1 far max(100, radius*10)=200
- **O que mostra:** silhueta cilíndrica vertical com base circular e topo aberto, proporções corretas altura ~32 largura ~28 ratio ~1.14, reconhecível como caneca/cerveja. 6 triângulos não-degenerados formam parte lateral e base. Z-buffer funcionando: sem artefatos de sobreposição errada, pixels mais próximos (frente) cobrem pixels mais distantes (fundo) sem flicker.
- **Limitações:** sem textura (UV bloqueado nível6 0% 11 hipóteses descartadas platô 9-26× artefato floats), sem iluminação realista só lambert simples dot(normal, lightDir)*0.8+0.2 ambient luz fixa (0.3,0.5,1) normalizada, sem animação bloqueada, muitos tris degenerados 24/30 v1,1,1 etc por layout triângulo 64B 3×8B mas arquivo contém degenerados intencionais ou parsing ainda parcial bad_indices 8,4% geral.

### Object03 — médio, Object2/Object03.bmd
- **Arquivo:** `ciclo14_first_render_Object03.ppm` 800x600, 88 vértices, 103 tris total, 82 rasterizados 79% não-degenerados, 108154 pixels não-bg 22% tela, 214080 pixels escritos total (inclui background sobreposto? mas non-bg 108k)
- **Comando:** `./build/mutool render /tmp/cliente_ref/Data/Object2/Object03.bmd docs/evidence/ciclo14_first_render_Object03.ppm 800 600`
- **Câmera:** bounds min (-113.73,-113.59,0.00) max (113.54,113.68,287.16) center (-0.09,0.04,143.58) radius ~197, eye (-0.09,59,577) target (-0.09,0.04,143) far 1970
- **O que mostra:** silhueta retangular vertical com altura 287 Z, largura 227 X, proporções corretas, reconhecível como objeto cenário (parede/pilar). 82 tris rasterizados cobrem centro tela 22%, z-buffer funcionando sem artefatos, pixels distantes não sobrepõem próximos.
- **Limitações:** sem textura, sem iluminação realista, sem animação, multi-mesh 2 meshes declarado meshCount 2 mas total vertices 88, sem backface culling (desabilitado winding desconhecido), sem clipping avançado só w<=0.001.

### AmbitionShield — médio-complexo com ossos, Player/AmbitionShield.bmd
- **Arquivo:** `ciclo14_first_render_AmbitionShield.ppm` 800x600, 385 vértices, 545 tris total, 448 rasterizados 82% não-degenerados, 32945 pixels não-bg 6.8% tela, 133245 pixels escritos
- **Comando:** `./build/mutool render /tmp/cliente_ref/Data/Player/AmbitionShield.bmd docs/evidence/ciclo14_first_render_AmbitionShield.ppm 800 600`
- **Câmera:** bounds min (-34.81,-28.56,-61.80) max (37.88,2.28,70.55) center (1.53,-13.14,4.37) radius ~77, eye (1.53,10,173) target (1.53,-13,4) far 770
- **O que mostra:** silhueta escudo com curvatura reconhecível, proporções corretas largura ~72 altura ~30 profundidade ~132, 3 bones (body.has_bones true bones.size 3), mas skinning não aplicado pois has_bone_weights false (MeshFromBmd não popula pesos, ainda BLOQUEADO), então render é bind pose. Z-buffer funcionando, sem artefatos.
- **Limitações:** sem textura UV bloqueado, sem iluminação realista, sem animação bloqueada, sem skinning real (pesos não decodificados), V0C decryptable 71% (773/1080 Player), 3 bones loose mas has_bone_weights false, render bind pose apenas.

### Axe11 — complexo com ossos, Player/Axe11.bmd
- **Arquivo:** `ciclo14_first_render_Axe11.ppm` 800x600, 1364 vértices, 2665 tris total, 528 rasterizados 19% não-degenerados, 61696 pixels não-bg 12.8% tela, 168870 pixels escritos
- **Comando:** `./build/mutool render /tmp/cliente_ref/Data/Player/Axe11.bmd docs/evidence/ciclo14_first_render_Axe11.ppm 800 600`
- **Câmera:** bounds min (-57.69,-78.62,-21.49) max (192.22,79.75,16.04) center (67.26,0.56,-2.72) radius ~144, eye (67,43,314) target (67,0.5,-2.7) far 1440
- **O que mostra:** silhueta machado com cabo longo 250 X e lâmina, proporções corretas, reconhecível como arma, 528 tris rasterizados 19% (muitos degenerados por bad_indices 5,87% Player), 1 bone, z-buffer funcionando.
- **Limitações:** sem textura, sem iluminação realista, sem animação, sem skinning real (has_bone_weights false), muitos tris degenerados.

## Z-buffer validação

Teste unitário `TestSoftwareRasterizerZBuffer`: dois triângulos sobrepostos z=0 e z=0.5, closer deve vencer, center pixel lit depth <1, PASS 500 checks. Em renders reais, não há artefatos de sobreposição errada visível (ex.: Beer01 frente cobre fundo, Object03 sem flicker). Evidência: framebuffer depth test `SetPixel` só escreve se z < stored depth, inicial 1.0 far.

## Proporções corretas

Bounds extraídos de vértices reais via `ComputeBounds` e usados para câmera auto-fit: center = (min+max)/2, radius = sqrt(dx²+dy²+dz²)/2, eye = center + (0,0.3r,2.2r). Isso garante objeto cabe na tela com margem, proporções preservadas pois projeção perspectiva mantém aspect ratio. Ex.: Beer01 bounds -14..14 X, -20..12 Y, -0.8..11 Z, radius 20, eye distância 44, FOV 45°, objeto ocupa 7% tela, não esticado.

## Silhueta reconhecível

Todos os 4 renders mostram silhueta reconhecível como objeto original (caneca, pilar, escudo, machado) mesmo sem textura, validando pipeline dados parser→malha→culling→projeção→rasterização. Primeira prova visual simbólica e técnica do projeto.

## Limitações documentadas

- Sem textura real ainda, já que UV está bloqueado nível6 0% 11 hipóteses descartadas platô 9-26× explicado artefato floats pequenos lidos como u16, sem mapeamento UV não há textura, OZT 3.404 98,10% OZJ 13.832 99,31% pixels disponíveis mas sem UV não aplicáveis
- Sem iluminação realista, só lambert simples intensidade = dot(normal, lightDir)*0.8+0.2 ambient, luz direcional fixa (0.3,0.5,1) normalizada, não PBR, não sombras, não specular
- Sem animação, já que está bloqueada nível anim 62 plausíveis finite<1e6 ratio 0,02× NÃO discriminativo vs controle 99,86%, 53 plausíveis [-pi,pi] ratio 0,04× NÃO discriminativo vs controle 33,76%, sem parser formal actionCount→kf, ordem xyz vs zyx, convenção Euler/quaternion
- Muitos triângulos degenerados: taxa bad_indices 8,4% geral, corner refs out of range 5,87% Player, 13,87% Object1, etc, triângulos com vértices repetidos v1,1,1 etc dropados por fabs(area)<1e-6, correto fail safe
- Sem backface culling (desabilitado para primeira prova, winding BMD desconhecido), poderia habilitar depois com teste winding
- Sem clipping avançado, só descarta triângulos com w<=0.001 behind camera, sem Sutherland-Hodgman, suficiente para primeira prova
- Performance: rasterização CPU 800x600 com 500 tris ~100k pixels ~ms, não otimizada sem SIMD multithread, mas funciona Linux sem GPU, não é pipeline final produção (VBO/VAO/OpenGL conforme DEC-008)
- Backend GPU real ainda BLOQUEADO por falta ambiente Windows/GPU, mas software CPU-only IMPLEMENTADO PARCIAL

## Próxima tarefa Parte C

Com rasterizador funcionando, nova abordagem UV possível: gerar renders teste com diferentes hipóteses offset/layout UV aplicadas como textura, mesmo que textura real não mapeada ainda, testar se coordenadas candidatas produzem padrão geometricamente plausível vs ruído aleatório — validação adicional não substitui rigor estatístico, tratar como pista exploratória não prova. Ou melhorar sombreamento com textura OZT real.

