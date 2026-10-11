# Reference audit — `heldergaraujo2/Cliente`

Everything in this document was obtained by inspecting the repository and its
binaries directly. Commands are given where the result is non-obvious, so any
claim can be re-derived. Nothing here is inferred from what a MU Online client
"is usually like".

## 1. Repository state

| Property | Value | How obtained |
|---|---|---|
| Branches | `main` only | `git branch -a` |
| Commits | 1 — `88edfc73` "Upload inicial do cliente MU Online sem video de selecao" | `git log --all --oneline` |
| Tags | none | `git tag` |
| Files tracked | 29,164 | `git ls-tree -r -l main` |
| Uncompressed size | 2,590,397,883 bytes (2470.4 MB) | sum of the size column |
| `.git` pack size | 1.3 GB | `du -sh .git` |

A single initial commit means there is **no history to learn from**: no record
of what was changed, why, or what regressed. Any change-tracking capability has
to be created here, in the reconstruction repository.

## 2. Top-level layout

The root holds exactly 30 files. Everything else is in four directories.

```
Data/          29,060 files   2372.0 MB   game assets
ScreenShots/       47 files     51.7 MB   captured images, includes Full.bmp (7.9 MB)
Scripts/           16 files      0.0 MB   .reg files for resolution / language / sound / window mode
PS/                 2 entries    0.0 MB   License.ps and an empty Logs/ directory
```

Root files, largest first:

```
15,612,416  Launcher.exe
11,878,664  PS.dll
 7,758,336  Main.dll
 3,615,232  Main.exe
 2,775,700  main.premium
 1,455,616  3DGuard.dll
   999,424  vorbisfile.dll
   770,384  msvcr100.dll
   719,360  MHPClient.dll
   521,216  3DPlugin.dll
   520,192  hinetworks.dll
   421,200  msvcp100.dll
   381,142  Main.ico
   304,128  AutoPotion.dll
   270,336  arcadia.dll
   212,992  wzAudio.dll
   207,360  Camera.dll
   155,136  CEffect.dll
   142,688  3D.Premium
    55,296  MHPVerify.dll
    53,248  ogg.dll
    35,328  Audio.dll
     8,428  ListaPatch.txt
     2,108  Checksum.premium
     1,216  License.hinet
     1,011  Settings.ini
       749  CESettings.ini
       111  mu.ini
       100  AutoPotion.pcd
        74  .gitignore
```

**Correction to an early mistake in this audit.** An initial inventory built by
splitting `git ls-tree` output on whitespace reported `.reg` files at the
repository root. They are not. All 16 `.reg` files live in `Scripts/`, and their
names contain spaces (`1024 x 768.reg`), which is what broke the naive split.
Every path in this document comes from a NUL-delimited listing
(`git ls-tree -r -z -l`).

## 3. Asset inventory by format

| Extension | Files | Bytes | Notes |
|---|---|---|---|
| `.ozj` | 13,928 | 971.7 MB | JPEG textures |
| `.bmd` | 10,090 | 585.5 MB | models |
| `.ozt` | 3,470 | 442.0 MB | 32-bit BGRA textures |
| `.wav` | 1,020 | 211.9 MB | sound effects |
| `.att` | 105 | 11.3 MB | terrain attributes |
| `.ozb` | 100 | 10.3 MB | binary height/light maps |
| `.obj` | 81 | 7.2 MB | terrain object lists |
| `.map` | 79 | 15.5 MB | terrain maps |
| `.smd` | 10 | 6.9 MB | Valve StudioModelData, text format |
| `.tga` | 15 | 7.85 MB | loose truevision images |
| `.jpg` | 22 | 1.2 MB | |
| `.mp3` | 61 | 193.8 MB | music |
| `.lua` | 22 | 0.1 MB | client-side scripts under `Data/Custom/Script` |
| `.cg` | 6 | 3.5 KB | Cg shaders, glow post-process |
| `.db` | 12 | 0.65 MB | `Thumbs.db` — Windows Explorer cache, junk |
| `.bmp` | 2 | 0.26 MB | |
| `.dat` | 2 | 108 B | |
| `.att1` | 1 | 131 KB | almost certainly a typo for `.att` |
| `.rar`, `.zip` | 2 | 14.3 MB | archives shipped inside the asset tree |
| `.ozp`, `.cws`, `.sfk`, `.png`, `.ini`, `.txt` | misc | | |

`Data/` subdirectory families, by file count: `Item` 3,585, `Player` 2,516,
`Custom` 1,609, `Sound` 1,020, `Monster` 945, `Interface` 835, then 100+
`Object*` directories (`Object1` … `Object110`), `Skill` 369, `Local` 366,
`Effect` 353, `NPC` 335.

## 4. Binary analysis

All modules are PE `i386` — 32-bit x86. Verified with `objdump -f` on each one.
There is no 64-bit binary in the client.

### 4.1 Protection — the finding that shapes everything

`objdump -h Main.exe` shows:

```
  0 .text     size 0   vma 00401000
  1 .rdata    size 0   vma 00d22000
  2 .data     size 0   vma 00e5f000
  3 .vmp0     size 0   vma 0987b000
  4 .vmp1     size 3604352  vma 0991f000
  5 .reloc    size e4
  6 .rsrc     size 228a
```

`.vmp0` and `.vmp1` are VMProtect section names, and `.text`, `.rdata` and
`.data` have zero size on disk — their contents live inside `.vmp1`. **`Main.exe`
is VMProtect-protected.** Its entry point (`0x09c8ef6d`) is inside the packed
region.

Consequences, and they are the reason this project exists in the form it does:

- The render loop is in the protected module (see 4.2). It cannot be read,
  patched or replaced by legitimate means.
- No unpacking, key generation or licence check removal will be attempted here.
  This is a hard rule, stated again in `AGENTS.md`.

Further licensing / anti-cheat components: `License.hinet`, `main.premium`,
`3D.Premium`, `Checksum.premium`, `MHPClient.dll`, `MHPVerify.dll`,
`3DGuard.dll`, `PS/License.ps`. `ListaPatch.txt` carries checksums for
`main.premium`, `3D.Premium` and `License.hinet`, so the protection files are
integrity-checked by the client itself.

`hinetworks.dll` and `3DPlugin.dll` have byte-identical `.data` (316,652) and
`.rsrc` (10,548) sizes and near-identical `.reloc`, and identical import sets.
They are the same product built under two names.

### 4.2 Where the renderer actually is

Import tables, extracted by parsing the PE import directory directly:

**`Main.exe` — 51 `OPENGL32` + 2 `GLU32` functions. This is the render loop.**

```
glAlphaFunc glBegin glBindTexture glBlendFunc glClear glClearColor
glColor3f glColor3fv glColor3ub glColor4f glColor4ub glColorMask
glDeleteTextures glDepthFunc glDepthMask glDisable glEnable glEnd glFlush
glFogf glFogfv glFogi glFrontFace glGenTextures glGetFloatv glGetIntegerv
glGetString glLoadIdentity glMatrixMode glNormal3f glPolygonMode glPopMatrix
glPushMatrix glReadPixels glRotatef glScalef glStencilFunc glStencilOp
glTexCoord2f glTexEnvf glTexEnvi glTexImage2D glTexParameteri glTranslatef
glVertex2f glVertex3f glVertex3fv glViewport
wglCreateContext wglDeleteContext wglMakeCurrent
GLU: gluOrtho2D gluPerspective
```

**`Main.dll` — 37 `OPENGL32` functions, and it is *not* packed.** Its sections
are ordinary: `.text` 2,003,941 bytes, `.rdata` 222,916, `.data` 21,702,016,
`.fptable`, `.rsrc`, `.reloc`. This is statically analysable game logic.

```
glBegin glBindTexture glCallLists glClear glClearColor glColor3f glColor3fv
glColor4f glColor4fv glColorPointer glDisable glDisableClientState glDrawArrays
glEnable glEnableClientState glEnd glFogf glFogfv glFogi glGenLists
glGetFloatv glGetIntegerv glHint glListBase glLoadIdentity glMatrixMode
glPopMatrix glPushMatrix glRasterPos2f glTexCoord2fv glTexCoordPointer
glTexEnvf glTexImage2D glTexParameteri glVertex4fv glVertexPointer
wglUseFontBitmapsA
```

The two lists together tell the story:

- `glBegin` / `glEnd` in `Main.exe` — **immediate-mode submission** is on the
  hot path.
- `glVertexPointer`, `glColorPointer`, `glTexCoordPointer`,
  `glEnableClientState`, `glDrawArrays` in `Main.dll` — **client-side vertex
  arrays**, meaning the driver re-marshals vertex data on every draw.
- `glGenLists`, `glCallLists`, `glListBase` — **display lists**, deprecated on
  modern drivers where they are frequently emulated in software.
- `wglUseFontBitmapsA` — text is rendered through GDI bitmap fonts, which
  matches the `[Font]` section of `Settings.ini` naming Tahoma and Arial.
- `glReadPixels` — a framebuffer readback on the presentation path. This is a
  classic full pipeline stall.
- **Absent everywhere:** `glGenBuffers`, `glBufferData`, `glDrawElements`,
  `glGenFramebuffers`, `wglSwapIntervalEXT`, `glGetStringi`. No vertex buffer
  objects, no index buffers, no render targets, no explicit vsync control.

Other imports of `Main.exe`, which pin down the rest of the architecture:

| DLL | Count | What it implies |
|---|---|---|
| `GDI32` | 14 | `ChoosePixelFormat`, `SetPixelFormat`, `SwapBuffers`, `CreateDIBSection`, `CreateFontA`, `TextOutW` — a WGL context on a GDI window, GDI text |
| `WINMM` | 12 | `timeGetTime`, `timeBeginPeriod`, `timeEndPeriod`, `timeSetEvent`, `timeKillEvent` — millisecond timer driven loop |
| `WS2_32` | 32 | network, with `WSASend` explicitly imported |
| `DSOUND` | 2 | DirectSound audio |
| `USER32` | 83 | `ChangeDisplaySettingsA`, `EnumDisplaySettingsA`, `SetWindowsHookExA`, `GetAsyncKeyState`, `RegisterHotKey` |
| `WININET` / `CRYPT32` / `urlmon` | 15 / 33 / 2 | HTTP and crypto, consistent with patching and licence checks |
| `dbghelp` | 7 | crash dump capture |
| `IPHLPAPI` | 1 | MAC address retrieval, typical of hardware fingerprinting for licensing |
| `msvcr100` / `msvcp100` | | Visual Studio 2010 runtime |

### 4.3 The other modules

| Module | Evidence | Role |
|---|---|---|
| `Launcher.exe` 15.6 MB | single import, `mscoree.dll` | .NET launcher |
| `PS.dll` 11.9 MB | sections `.itext` `.didata` `.edata` `.pr0` `.pr1`; imports `comctl32`, `winspool.drv`, `imagehlp` | Delphi application, packed; ships with `PS/License.ps` |
| `arcadia.dll` | `.gfids` section (MSVC control-flow guard, VS2019+), 11 `OPENGL32` including `glTexSubImage2D`, no `USER32` | standalone GL effects / texture-upload plugin |
| `CEffect.dll` | 8 `OPENGL32`, 58 `MSVCP100` | effect rendering |
| `Camera.dll` | no OpenGL, 56 `KERNEL32` | camera math |
| `Audio.dll` | `MSVCP140`, `VCRUNTIME140`, `WINHTTP`, `bcrypt`, `WININET` | VS2015+ audio module that also makes HTTP calls |
| `wzAudio.dll` | `vorbisfile`, `DSOUND`, `WINMM` | Ogg Vorbis playback through DirectSound |
| `AutoPotion.dll` + `AutoPotion.pcd` | | third-party auto-potion helper |

Two runtime generations are present: VS2010 (`msvcr100`/`msvcp100`) for the
core and most plugins, VS2015+ (`MSVCP140`/`VCRUNTIME140`) for `Audio.dll`, and
MSVC with control-flow guard for `arcadia.dll`. The client has been extended
over many years by different authors.

## 5. Configuration surface

`Settings.ini` (1,011 bytes) exposes the tuning surface the client already has:

```ini
[AntiLag]
DisableStaticEffect = 0     DisableDynamicEffect = 0
DisableSkillEffect  = 0     DisableGlowEffect    = 0
DisableWings        = 0     ReduceMemory         = 0

[Graphics]
Anisotropy = 1     MaxAnisotropy = 32     Linear = 1
Fog = 1            TronEffects  = 1       MiniMap = 1
DisableNewFPS = 0

[Other]
CustomInterface = 0   ; 0 Season 6 default / 1 Season 1 / 2 Season 2 /
                      ; 3 Season 3 / 4 Season 8 / 5 Navi Blue / 6 Mu Legend
ServerTime = 1        DisablePing = 0     CloseMessageBox = 0
DisableAutoCtrl = 0   WideScreenMode = 1  Debug = 0

[Font]   FontEnable, FontName1..5 (Tahoma), FontNameMini (Arial), sizes 13/11
[Options] HealthBarPlayer, HealthBarMonster, HudNumberFormat, MoveFavorite
[Configuracion] SkyBox = 1, DayNight = 1
[AutoLogin] User, Password, SaveData          <-- see SECURITY.md
[Discord] DiscordShowPresence = 1
```

Performance-relevant observations:

- `Anisotropy = 1` with `MaxAnisotropy = 32` requests up to 16x anisotropic
  filtering. On the hardware this client targets that is expensive and is a
  candidate for a measurable quality/perf trade-off.
- `DisableNewFPS = 0` proves a distinct "new FPS" code path exists. The string
  `DisableNewFPS` is present in `Main.dll` (`strings -n 6 Main.dll | grep -i
  fps` returns exactly `DisableNewFPS`, `SwapBuffers`, `timeBeginPeriod`).
- A complete `[AntiLag]` block already exists. It is a supported, documented
  way to reduce load without touching protected code.

`CESettings.ini` (749 bytes) is a second, independent feature-switch file:
`[RenderMesh]`, `[EffectStaticDynamic]`, `[Animation]`, `[Interface]`,
`[MapEx]`, `[Extra]`, `[HotKeys]`. Comments are in Spanish.

`mu.ini` is 111 bytes of binary, not text.

`Scripts/*.reg` (16 files) write resolution, language, sound and window mode
into the registry: `640 x 480`, `800 x 600`, `1024 x 768`, `1280 x 1024`,
`1366 x 768`, `1440 x 900`, `1600x900`, `1680 x 1050`, `1920 x 1080`, plus
`LANGUAGE ENGLISH/PORTUGUESE/SPANISH`, `SOUND ON/OFF`, `_ FULL SCREEN MODE` /
`_ WINDOW MODE`. **Resolution and window mode are registry-driven, not
config-file driven** — that is why they are shipped as `.reg` files.

## 6. Patch manifest

`ListaPatch.txt`, 188 lines, format `relative/path CRC32HEX SIZE`:

```
3D.Premium 4108e84d 142688
License.hinet 086214e9 1241
main.premium 96913ae6 2479888
Data/gate.bmd 05d2a7b8 7168
Data/InGameShopScript/512.2011.006/IBSCategory.txt bab7f199 281
```

The `512.xxxx` in-game-shop paths indicate client version 512, i.e. the Season
6 line, with builds dated 2011 and 2025 present simultaneously.

Verified with `build/mutool verify` against the checked-out tree:

```
total 188   match 116   size mismatch 9   crc mismatch 11   missing 52
```

**The repository does not match its own patch manifest.** 52 entries are absent
from the tree (for example `Data/Item/axe10.ozj`, `Data/Item/Axe12.bmd`,
`Data/Item/DarkHammer.bmd`), and 20 more differ in size or checksum, including
`Data/gate.bmd`, `main.premium` and `3D.Premium`. A client installed from this
repository is not the client the manifest describes.

## 7. Shader and scripting layer

Six Cg shaders under `Data/Effect/`:

```
glow_blur_fp.cg    glow_blur_vp.cg
glow_combine_fp.cg glow_combine_vp.cg
glow_darken_fp.cg  glow_darken_vp.cg
```

`glow_blur_fp.cg` uses `samplerRECT` and `texRECT` — the
`NV_texture_rectangle` extension — and blends four samples with weights
0.105 / 0.86 / 0.085 / 0.86. This is a **four-tap bloom/glow post-process**,
which explains the `DisableGlowEffect` and `TronEffects` settings. `rect`
textures are deprecated in every modern API, so a reconstruction must
reimplement this with `sampler2D` and normalised coordinates.

22 Lua files under `Data/Custom/Script/` implement UI and protocol extensions:
`ScriptMain.lua`, `System/InterfaceCore.lua`, `System/ProtocolCore.lua`,
`System/LoadImages.lua`, `Util/Json.lua`, plus feature folders `Musaffe`,
`NewWindow`, `UpgradeMachine`, `WindowTitle`, `TesteProtocol`. There is also a
`Data/Custom/ScriptEffect/` tree. So the client has a **runtime Lua layer that
registers custom network protocols** — relevant to any compatibility work.

## 8. Things that should not ship

| Item | Evidence | Impact |
|---|---|---|
| 12 × `Thumbs.db` | 677,376 bytes total | Explorer cache; leaks directory listings, wastes space |
| `Data/*.rar`, `Data/*.zip` | 14.3 MB | archives nested inside the asset tree |
| `Data/**/*.att1` | 1 file, 131,076 bytes | almost certainly a typo |
| `ScreenShots/` | 51.7 MB | 47 captured screenshots, not game content |
| Plaintext credentials | `Settings.ini` `[AutoLogin]` | see `docs/SECURITY.md` |

## 9. What can and cannot be reused

**Can be used (formats, not bytes).** The layout of every asset container is
documented in `docs/ASSET_FORMATS.md` from byte-level measurement. Writing an
independent reader for a format is interoperability work and is what this
project does.

**Cannot be used.** The engine. `Main.exe` is VMProtect-protected and the render
loop is inside it. The licensing and anti-cheat modules are commercial products.
Neither will be modified.

**Rights unconfirmed.** Every asset under `Data/` — models, textures, maps,
sounds, music, UI, Lua scripts. These are third-party intellectual property.
This repository redistributes none of them. `docs/LICENSING.md` tracks the
status.
