# Compatibility

Visual and functional compatibility status. Separated into what is verified,
what is partial and what is not started, because "compatible" without a
qualification is not a meaningful claim.

## 1. Asset format compatibility

This is the layer that actually works, and it is verified against the whole
reference tree rather than a sample.

| Format | Files | Loadable | Level |
|---|---|---|---|
| OZJ texture | 13,928 | 13,832 (99.31%) | **Header + geometry verified.** Pixels not decoded — no JPEG decoder in the library. |
| OZT texture | 3,470 | 3,404 (98.10%) | **Full pixel load verified**, round-tripped through a writer. |
| TGA | 15 | 15 (100%) | **Full pixel load** for uncompressed 24/32-bit. RLE detected, not decoded. |
| BMD model | 10,090 | 10,066 classified (99.76%) | **Container only.** Payload is encrypted and the cipher is not recovered. |
| `.att` terrain | 105 | 105 (100%) | **Container + grid geometry.** 8 plaintext, 97 encrypted. |
| `.map` terrain | 79 | 79 (100%) | **Container + grid geometry.** 2 plaintext, 77 encrypted. |
| `.obj` terrain | 81 | 81 (100%) | **Container only.** Record layout not resolved. |
| `.ozb` | 100 | 0 | Not started. |
| `.smd` | 10 | 0 | Not started (plain text, straightforward). |
| `.lua` | 22 | 0 | Not started (plain text). |
| `.cg` | 6 | 0 | Read and analysed; not compiled. |

### What "loadable" does not mean here

For OZT and TGA, "full pixel load" means the bytes are read, dimensions
validated, and the pixel payload round-trips correctly through a writer and back.
It does **not** mean the image has been displayed and compared against the
original, because there is no renderer. Visual verification is `NOT EXECUTED`.

For BMD, "container only" means the magic, version and payload length are parsed
and validated. There is no vertex data, no bone hierarchy, no texture
references. **No model can be rendered.**

## 2. Blocking incompatibilities

### 2.1 BMD payload cipher — blocks all geometry

8,570 of 10,090 BMD files have payload entropy above 7.5 bits/byte. The cipher
is not recovered; six hypotheses were tested and ruled out
([`ASSET_FORMATS.md`](ASSET_FORMATS.md)).

Consequences, stated plainly: no characters, no monsters, no items, no world
objects, no skeletal animation. This is the largest single functional gap and it
gates the renderer work — there is nothing to render until it is resolved.

### 2.2 `Enc*` terrain encryption — blocks most terrain

219 of 264 terrain files. Not the bux cipher (1 of 60 improved when decrypted
with it). 45 terrain files can be read.

## 3. Renderer compatibility

The original renderer is fully characterised from its import tables
([`RENDERING.md`](RENDERING.md)): fixed-function OpenGL 1.1, immediate mode plus
client-side vertex arrays plus display lists, GDI bitmap fonts, `glReadPixels`
on the presentation path, Cg/`NV_texture_rectangle` glow post-process.

No replacement renderer exists. Every item below is `NOT STARTED`:

| Capability | Status |
|---|---|
| Window creation and GL context | NOT STARTED |
| Terrain rendering | BLOCKED (2.2) |
| Model rendering | BLOCKED (2.1) |
| Skeletal animation | BLOCKED (2.1) |
| Texture upload | Library converts to RGBA; no GL upload path |
| Particle and skill effects | NOT STARTED |
| UI rendering | NOT STARTED |
| Text rendering | NOT STARTED |
| Glow post-process | Analysed, not reimplemented |
| Fog, sky box, day/night | NOT STARTED |
| Camera | NOT STARTED |

## 4. Configuration compatibility

The client's configuration surface is documented in
[`REFERENCE_AUDIT.md`](REFERENCE_AUDIT.md) section 5. Two files drive it:
`Settings.ini` (`[AntiLag]`, `[Graphics]`, `[Other]`, `[Font]`, `[Options]`,
`[Configuracion]`, `[AutoLogin]`, `[Discord]`) and `CESettings.ini`
(`[RenderMesh]`, `[EffectStaticDynamic]`, `[Animation]`, `[Interface]`,
`[MapEx]`, `[Extra]`, `[HotKeys]`).

A reconstruction that wants drop-in compatibility should read both files with
the same key names and the same defaults. No reader for either file exists yet.
`NOT STARTED`.

Note that resolution and window mode are **not** in these files — they are in
the registry, written by the 16 `.reg` files in `Scripts/`.

## 5. Network and protocol compatibility

`NOT EXECUTED`. No authorised test server is available.

What is known from the reference client: `Main.exe` imports 32 `WS2_32`
functions including `WSASend`, and there is a Lua layer under
`Data/Custom/Script/` containing `System/ProtocolCore.lua` and a
`TesteProtocol` feature folder, which indicates the client registers custom
protocol handlers at runtime. Client version markers in `ListaPatch.txt` are
`512.2011.006` and `512.2025.005`, i.e. the Season 6 line.

Any claim of protocol compatibility would be a guess. None is made.

## 6. Audio compatibility

`NOT STARTED`. The reference client plays 1,020 WAV files (211.9 MB) and 61 MP3
plus 3 OGG music tracks (198 MB) through `wzAudio.dll` using `vorbisfile` and
DirectSound.

Note that MP3 is the dominant music format and would need its own decoder; the
Ogg library the client ships does not cover it.

## 7. Platform compatibility

| Target | Status |
|---|---|
| Windows x86 (32-bit) | `NOT EXECUTED` — never compiled for. This is the actual target. |
| Windows x64 | NOT APPLICABLE — the reference client is entirely 32-bit |
| Linux | Library and tools build and pass all tests. Not a shipping target. |

The Windows build gap is significant and is called out in
[`BUILD.md`](BUILD.md): **the target platform has never been compiled for.**

## 8. Visual comparison methodology

Not yet possible, since there is no renderer. When it becomes possible, the
method should be:

1. Identical scene, camera position, resolution and graphics settings on both
   clients.
2. Captured frames compared pixel-by-pixel, with a reported difference metric
   rather than a visual impression.
3. Expected differences listed in advance — the glow pass reimplemented in GLSL
   will not be bit-identical, and saying so beforehand keeps the comparison
   honest.
4. **Never modify an asset to hide a renderer bug.** If the reconstruction looks
   wrong, the fix belongs in the renderer.
