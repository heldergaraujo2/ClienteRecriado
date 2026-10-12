# Próxima tarefa

> **Ciclo 8 fechado** com parsing completo de ossos nível 5. Base `c29f473` → HEAD `1e9f74d` (ciclo7) → agora ciclo8 com 3800 arquivos (37,66%).
> Todos os bloqueios de ossos resolvidos, checker 53→56 verificações 0 falhas, build limpo 384/0 ASan 0.

## O que mudou neste ciclo (ciclo 8)

**Layout de osso final confirmado e implementado:**

- Single `bc*59-1 = (bc-1)*59+58` (32 nome + u16 parent 0xFFFF=root + 6×f32 + 1 pad exceto último)
- Multi `4 + bc*59-1` onde 4B extra = u32 actionCount (ac)
- Hex prova `PrickleGlove.bmd` remain 62 = `01 00 00 00` + `PrickleGlo\0...` + `FFFF` + 6 floats
- Evidência `ciclo8_bones_multi.log`, `ciclo8_bones_layout.log`, `ciclo8_bones_bmdreport.log`

**Implementação:**

- `src/mu/assets/bmd_body.hpp` com `BmdBone {string name; int parent; float raw[6]; bool has_name; bool floats_finite;}` + `declared_bone_count/action_count`, `bones/has_bones`
- `bmd_body.cpp` com `TryParseBonesAt()` validando nome ASCII NUL-padded/loose >=3 printable, parent, 6 floats finitos
- Single base `triangles_end+32`, multi base offset após último nome textura +4 se `extra==ac`
- `bmd_report.hpp/cpp` estendidos com `has_bones`, `bone_count`, `bones_parsed`, `total_bones`
- `mutool bmdreport` agora imprime LEVEL 5

**Medição:**

- Nível 5: 3.505 (34,74%) → **3.800 (37,66%)**, +295 arquivos, 8.001 ossos, 100% parent válido
- 45,21% dos arquivos com geometria têm ossos (3800/8406)
- Custom bc51 remain 2698 vs expected 3008 diff -310 → filtrado, não segue 59B

**Testes:** 384 checks, 0 failures · 56 verificações doc↔código 0 falhas · ASan 0

## Próxima tarefa: UV por canto e animação (nível 6 + skinning)

**Por que esta, e não outra.** Nível 4 é 83,30% (8.405), nível 3 83,31% (8.406), nível 5 37,66% (3.800). Multi-mesh 100% parseado, ossos 37,66% com layout final confirmado. Próximos bloqueios para modelo renderizável texturizado e animado:

- Nível 6 (UV) é **0%** — sem índice ligando canto de triângulo a coordenada de textura, modelo não pode ser texturizado
- Animação após ossos: 500 arquivos com bloco extra, 385 divisível por `12*boneCount` (77%), mas 3 floats por keyframe não decodificados

Já temos:

- Vértices, normais, faces, ossos (hierarquia) em 3.800 arquivos
- Texturas OZT/OZJ/TGA parseáveis (98-100%)
- Altura de terreno decodificada (TerrainHeight.OZB)
- `mutool bmdanim` mede `boneCount*keyframes*12` bytes

O que falta:

- Índice de UV por canto: 9 hipóteses descartadas no ciclo 3, critério de parada definido
- Ordem dos 6 floats de osso: posição vs rotação, sistema coordenadas, graus vs radianos
- Seção de animação: o que 3 floats por keyframe representam, actionCount
- Validação de skinning: pesos por vértice, ordem dos floats, teste sem GPU

É testável sem GPU: parsing puro CPU, evidência automática como em ciclo 6-8.

**Critério de aceite:**

1. `mutool bmdreport` nível 6 >0 ou animação decodificada com evidência bruta versionada, OU relatório conclusivo de que UV está em outro arquivo (ex: `.o3d`, `.bmd` extra) com evidência negativa
2. `make test` e `make test-san` limpos (384 checks)
3. `check_docs_consistency.py` 0 falhas (56 verificações após ciclo 8)
4. Documentação de UV/animação com evidência, não inferência, em `ASSET_FORMATS.md`
5. Zero regressão em nível 3 (8.406), nível 4 (8.405), nível 5 (3.800)

**O que isso NÃO destrava:** backend VBO/VAO continua BLOCKED sem GPU/contexto, mas parsing completo é pré-requisito.

## Depois disso

| Tarefa | Por quê | Bloqueio |
|---|---|---|
| Backend VBO/VAO com skinning | decisão DEC-008 registrada | GPU/contexto BLOCKED |
| Escala vertical altura | 3 caminhos fechados | testável aqui |
| Parte B DEC-008 | benchmark real | GPU/contexto BLOCKED |
| Baseline FPS/VRAM | objetivo do projeto | GPU BLOCKED |
| Validação visual Windows | compatibilidade | Windows/GPU BLOCKED |

## Verificação de conformidade, permanente

```
python3 tools/check_docs_consistency.py   # 56 verificações, 0 falhas (era 53)
make clean && make test                    # 384/0 + logs ciclo8_test.log
make clean && make test-san                # 384/0 ASan 0 + logs ciclo8_test_san.log
grep -rn "glGenBuffers\|wglCreateContext\|glDrawElements\|GL/gl.h" src/
```

Deve retornar apenas comentários. Verificado ciclo 8: 0 includes gráficos.
