# Áudio — arquitetura definida (E1 ciclo13)

## Formatos suportados — medido no cliente referência

| Formato | Arquivos | Bytes | Papel | Status |
|---|---|---|---|---|
| WAV | 1.020 | 211,9 MB | SFX efeitos sonoros | CONFIRMADO — `Data/Sound/` inventário `REFERENCE_AUDIT.md` §3, `mutool scan` |
| MP3 | 61 | 193,8 MB | Música fundo | CONFIRMADO — inventário, 61 arquivos |
| OGG | 3 | ~? | Música fundo alternativa | CONFIRMADO — 3 arquivos OGG, `vorbisfile.dll` + `ogg.dll` presentes |

- WAV: PCM, 1.020 arquivos, decodificação via `DSOUND` + `WINMM` no original (DirectSound legado) — CONFIRMADO import tables `wzAudio.dll` imports `vorbisfile`, `DSOUND`, `WINMM`, `Main.exe` imports `DSOUND` 2, `WINMM` 12
- OGG: Ogg Vorbis, `vorbisfile.dll` 999.424 B + `ogg.dll` 53.248 B, Xiph BSD-style, versão não extraída mas tamanho sugere 1.3.x — CONFIRMADO presença DLLs, papel playback via DirectSound
- MP3: 61 arquivos, decoder não identificado no cliente (nenhuma DLL mp3 decodifica, possivelmente `Audio.dll` ou `wzAudio.dll` interno) — PROVÁVEL decoder interno, DESCONHECIDO biblioteca exata

## Biblioteca pretendida para reconstrução

| Necessidade | Candidata | Licença | Por que |
|---|---|---|---|
| WAV decode | `miniaudio` single-header | MIT | Zero dep, suporta WAV PCM, já testado em outros projetos, sem DSOUND |
| OGG Vorbis decode | `stb_vorbis` single-header | Public domain / MIT | Single header, sem build integration, BSD compatível, substitui vorbisfile.dll+ogg.dll (BSD) |
| MP3 decode | `miniaudio` (inclui minimp3) ou `stb_vorbis` + `dr_mp3` | MIT | `dr_mp3` public domain, cobre 61 MP3 |
| Playback | `miniaudio` backend WASAPI/DirectSound/CoreAudio/ALSA | MIT | Abstrai DSOUND legado, sem dependência `DSOUND`/`WINMM` global timer, usa high-res clock |

- **Recomendação:** `miniaudio` + `stb_vorbis` + `dr_mp3` — todas single-header MIT/public domain, zero dep build, sem DLL hijack risco, sem HTTP em módulo áudio (original `Audio.dll` faz HTTP via WINHTTP/WININET — BLOQUEADO, não reproduzir)
- **Alternativa:** OpenAL Soft (LGPL) — mais pesado, requer build dep, não recomendado para zero-dep property

## Como se integra à estrutura geral

```
mu/audio/
  audio.hpp — interface AudioEngine (init, load, play, stop, setVolume, update)
  audio.cpp — implementação via miniaudio
  decoder.hpp — pluggable decoder interface (WAV/OGG/MP3)
  decoder_wav.cpp — via miniaudio
  decoder_ogg.cpp — via stb_vorbis
  decoder_mp3.cpp — via dr_mp3
  mixer.cpp — mixer CPU, 32 vozes, volume por categoria SFX/Music
```

- **Future integration:** a `MainLoop` update callback may call `AudioEngine::Update` once per frame without blocking rendering; the audio engine is not implemented or connected to the loop
- **Carregamento:** streaming de assets — SFX WAV carregados sob demanda quando objeto visível (IsVisible true), música MP3/OGG streaming com buffer duplo, orçamento VRAM não aplica mas RAM sim
- **Integração com culling:** SFX posicionais só tocam se objeto visível ou próximo (distância < threshold), reduz vozes ativas
- **Segurança:** sem HTTP, sem bcrypt, sem WININET, sem DLL carregada de diretório atual — carregar libs estáticas, validar TLS se precisar HTTP (não em áudio)
- **Testes:** `TestAudioEngine` — carrega WAV 1.020 arquivos reais, decodifica header, verifica duração >0, sem playback real (BLOCKED sem áudio device), 1.020 checks

## Estado

- **CONCLUÍDO ciclo13:** arquitetura definida, formatos suportados medidos, biblioteca pretendida escolhida (miniaudio MIT), integração documentada
- **PARCIAL:** implementação não iniciada (mu/audio NOT STARTED em ARCHITECTURE.md), decoder não implementado, playback BLOCKED sem áudio device neste ambiente
- **Próximo passo:** implementar decoder_wav via miniaudio, medir bench decode WAV 1.020 arquivos, registrar em OPTIMIZATION_LOG.md

E1 CONCLUÍDO ciclo13 como arquitetura definida, implementação pendente.
