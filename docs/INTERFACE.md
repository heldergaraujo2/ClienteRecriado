# Interface — fluxo de telas definido (E3 ciclo13)

## Fluxo login → seleção servidor → seleção personagem → mundo — definido

```
[Launcher.exe] (.NET mscoree.dll) — CONFIRMADO 15,6 MB, .NET launcher, 1 import mscoree.dll
  ↓
[Login] — Settings.ini [AutoLogin] User, Password, SaveData plaintext — CONFIRMADO 1.011 bytes, SECURITY.md alerta
  ↓
[Seleção Servidor] — Data/Local/Filter.bmd, ServerList.bmd etc — PROVÁVEL, mas não verificado sem executar cliente (BLOCKED sem Windows/GPU)
  ↓
[Seleção Personagem] — Data/Player/ 2.516 arquivos — CONFIRMADO inventário, Player/*.bmd modelos personagem
  ↓
[Mundo] — Data/ 29.060 arquivos 2.372 MB, Object1..Object110, Monster 945, NPC 335, Skill 369, Effect 353, Interface 835, etc — CONFIRMADO
```

- **Launcher:** `Launcher.exe` 15,6 MB .NET, `PS.dll` 11,9 MB Delphi packed, `PS/License.ps` — CONFIRMADO presença, papel launcher, mas fluxo interno DESCONHECIDO (sem executar)
- **Login:** `Settings.ini` [AutoLogin] User Password SaveData — CONFIRMADO plaintext credenciais, SECURITY.md, [Discord] DiscordShowPresence — CONFIRMADO, mu.ini 111 bytes binário não texto — CONFIRMADO
- **Seleção servidor:** `Scripts/*.reg` 16 arquivos 640x480..1920x1080 LANGUAGE ENGLISH/PORTUGUESE/SPANISH SOUND ON/OFF FULL SCREEN/WINDOW — CONFIRMADO resolução/idioma via registry, não config file, `Data/Local/` 366 arquivos — CONFIRMADO, mas lista servidores exata DESCONHECIDO sem executar
- **Seleção personagem:** `Data/Player/` 2.516 arquivos, `Item/` 3.585 arquivos — CONFIRMADO inventário, modelos Player/*.bmd com bones 63 máximo observado, skinning LBS D2 — CONFIRMADO
- **Mundo:** `Data/Object*` 100+ diretórios, `Monster/` 945, `NPC/` 335, `Skill/` 369, `Effect/` 353, `Interface/` 835, `Custom/` 1.609, `Map/` 79 .map 15,5 MB, `*.att` 105 11,3 MB terrain attributes, `*.ozb` 100 10,3 MB height/light maps, `*.obj` 81 7,2 MB object lists — CONFIRMADO inventário

## Como consome assets decodificados

| Tela | Assets consumidos | Como consome | Status |
|---|---|---|---|
| Login | `Data/Interface/` 835 arquivos, `Data/Local/` 366, `Settings.ini` [Font] Tahoma/Arial 13/11, `CESettings.ini` 749 bytes [RenderMesh] [EffectStaticDynamic] etc | GDI32 14 imports ChoosePixelFormat SetPixelFormat SwapBuffers CreateDIBSection CreateFontA TextOutW, USER32 83 imports ChangeDisplaySettingsA EnumDisplaySettingsA SetWindowsHookExA GetAsyncKeyState RegisterHotKey, wglUseFontBitmapsA GDI bitmap fonts — CONFIRMADO import tables, mas layout UI exato DESCONHECIDO | CONFIRMADO imports, DESCONHECIDO layout |
| Seleção Servidor | `Data/Local/Filter.bmd`, `ServerList.bmd`, `Data/Interface/` | Carrega BMD com BmdProbe, BmdBody, ComputeBounds, PackMeshForUpload D1, culling D3 — CONFIRMADO parsing 8.406 geometria, mas lista servidores exata BLOQUEADO sem executar | CONFIRMADO parsing, BLOQUEADO lista |
| Seleção Personagem | `Data/Player/` 2.516 Player/*.bmd com bones 63 max, `Data/Item/` 3.585, `Data/Interface/` | Mesh C1 representação interna única com bone_weights, Skeleton has_bones, skinning LBS D2 ApplySkinningLBS strict count identity/self-loop/zero-movement + rotação 90° não-trivial (0,1,0) esperado, buffers D1 PackMeshForUpload VBO/VAO sem GL, culling D3 — CONFIRMADO implementação, mas seleção personagem UI DESCONHECIDO | CONFIRMADO C1/D1/D2/D3, DESCONHECIDO UI |
| Mundo | `Data/Object*`, `Monster/`, `NPC/`, `Skill/`, `Effect/`, `Map/` 79 .map, `*.att` 105, `*.ozb` 100, `*.obj` 81, `Data/Custom/Script/` 22 Lua + ScriptEffect, `Data/Effect/` 6 Cg shaders glow_blur_fp.cg etc | Terrain 265 files classified 100% 97 .att still encrypted, BuxCrypt 1 MiB 880 MB/s gargalo 40ms map load, OztToRgbaTopDown 62,4us median p99 138,6us 2,2× tail, frustum culling 8406 bounds reais culled 2,77% a 7,88% escala real, streaming bounds primeiro depois geometria quando IsVisible true, LOD Caminho A bloqueado bloqueio #1 cifra 85% exige topologia Caminho B MSFT_lod glTF, draw calls 67,21 MiB per draw 2,9M corner sends 63,9% reenvios desnecessários sem glDrawElements, VBO/IBO batching DEC-008 — CONFIRMADO medido, mas mundo exato BLOQUEADO sem Windows/GPU | CONFIRMADO parsing e medição escala real, BLOQUEADO render real |

## Arquitetura pretendida reconstrução

```
mu/interface/
  screen.hpp — enum Screen { kLogin, kServerSelect, kCharSelect, kWorld }
  screen_manager.cpp — transição login→server→char→world, com FrameBudget
  login_screen.cpp — consome Settings.ini, Interface/*.ozj/.ozt, Font Tahoma/Arial atlas
  server_screen.cpp — consome Local/*.bmd, Interface
  char_screen.cpp — consome Player/*.bmd com skinning D2, Item/*.bmd, Interface
  world_screen.cpp — consome Object*, Monster, NPC, Map, att, ozb, obj, Custom/Script Lua, Effect Cg shaders (reimplementados GLSL sampler2D)
  ui/
    button.cpp, label.cpp, list.cpp — UI widgets, sem GDI, com atlas glyph cache
```

- **Integração:** `MainLoop::Tick` chama `ScreenManager::Update` (update) → `ScreenManager::Render` (render CPU side prepara draw calls via PackMeshForUpload) → `Present` (SwapBuffers)
- **Assets:** todos via `mu/assets/*` (ozt, ozj, bmd, terrain, tga, manifest), sem conversão em lote sem verificar compatibilidade e manter cópia referência (regra permanente), preferir carregadores compatíveis formatos existentes
- **Segurança:** não expor credenciais Settings.ini [AutoLogin] plaintext, não contornar DRM, não usar registry para resolução (usar config file), input via high-res clock QPC não timeGetTime 1ms (6% frame 60Hz 14% 144Hz), sem SetWindowsHookEx global hook

## Estado

- **CONCLUÍDO ciclo13:** fluxo telas definido login→server→char→world, como consome assets decodificados documentado com rótulos CONFIRMADO/PROVÁVEL/DESCONHECIDO/BLOQUEADO, integração com C1/D1/D2/D3/E2
- **PARCIAL:** implementação não iniciada, UI widgets não implementados, Lua ScriptEffect não integrado, Cg shaders não reimplementados GLSL
- **Próximo passo:** implementar screen_manager.hpp esqueleto com enum Screen e transição, sem GL, teste unitário transição login→server→char→world

E3 CONCLUÍDO ciclo13 como fluxo definido, implementação pendente.
