# Rede — estrutura de pacotes documentada (E2 ciclo13)

## Import tables — CONFIRMADO

| DLL | Count | Função | Status |
|---|---|---|---|
| WS2_32 | 32 | `WSASend` explicitamente importado, `socket`, `connect`, `send`, `recv` etc | CONFIRMADO — PE import directory Main.exe |
| WININET | 15 | HTTP | CONFIRMADO — patching + licence checks |
| CRYPT32 | 33 | crypto | CONFIRMADO |
| urlmon | 2 | URL | CONFIRMADO |
| IPHLPAPI | 1 | MAC retrieval | CONFIRMADO — fingerprint hardware licensing |
| WINHTTP | ? | HTTP em Audio.dll | CONFIRMADO — Audio.dll imports WINHTTP |

- **Protocolo jogo:** WS2_32 direto, TCP/UDP — CONFIRMADO import WSASend, mas estrutura pacote NÃO decodificada (dentro Main.exe VMProtect-protected .vmp0/.vmp1)
- **Patch:** WININET + ListaPatch.txt 188 linhas path CRC32 SIZE — CONFIRMADO formato `relative/path CRC32HEX SIZE`, verificado `mutool verify` total 188 match 116 size mismatch 9 crc mismatch 11 missing 52
- **Licensing/anti-cheat:** hinetworks.dll 520KB + 3DGuard.dll 1.455MB + License.hinet 1.216B + main.premium 2.775MB + 3D.Premium 142KB + Checksum.premium 2KB — CONFIRMADO presença, byte-identical .data/.rsrc entre hinetworks e 3DPlugin, MHPClient.dll 719KB MHPVerify.dll 55KB — CONFIRMADO, mas protocolo interno DESCONHECIDO e BLOQUEADO (comercial, não redistribuível, não contornar DRM regra permanente)

## Estrutura de pacotes — rotulada

| Conclusão | Detalhe | Rótulo |
|---|---|---|
| Cliente usa WS2_32 para jogo, não usa HTTP para jogo (HTTP só patching/licence) | WSASend importado, WININET separado | CONFIRMADO |
| ListaPatch.txt formato `path CRC32 SIZE` | 188 linhas, verificado mutool verify 116 match | CONFIRMADO |
| Protocolo jogo tem handshake + login + map load | Inferido de strings e Lua ProtocolCore.lua | PROVÁVEL |
| Lua `Data/Custom/Script/System/ProtocolCore.lua` registra protocolos custom | 22 Lua files, System/ProtocolCore.lua + Util/Json.lua + folders Musaffe NewWindow etc | CONFIRMADO — existência arquivo, mas estrutura pacote interna DESCONHECIDO |
| Pacotes têm header com size + opcode + payload | Padrão MU Online (0xC1 0xC2 etc) — comum em servidores privados, mas não verificado neste cliente específico sem engenharia reversa proibida | PROVÁVEL — baseado em conhecimento público MU Online, não em medição deste binário protegido |
| Criptografia de pacotes (XOR ou similar) | Main.exe protegido, não pode ser lido sem contornar VMProtect — regra permanente proíbe | BLOQUEADO — não tentar desembrulhar, não remover verificações licença |
| Anti-cheat envia heartbeat | MHPClient.dll + MHPVerify.dll presentes, mas sem análise | DESCONHECIDO — presença CONFIRMADA, comportamento BLOQUEADO |
| Servidor valida movimento e ação | Necessário para compatibilidade, mas não medido aqui | PROVÁVEL — sem evidência direta, mas padrão MU |

## Arquitetura pretendida reconstrução

```
mu/network/
  socket.hpp — wrapper WS2_32 (Windows) / POSIX (Linux), sem dependência externa
  packet.hpp — struct Packet { uint8_t header, size, opcode, payload[] } — pluggable, sem hardcode opcode
  crypto.hpp — interface PacketCrypto (XOR, etc) — implementação limpa sem DRM
  protocol.hpp — enum opcodes, mas sem valores hardcoded até medição real
  client.hpp — NetworkClient (connect, send, recv, update)
```

- **Segurança:** não enfraquecer validações servidor para melhorar benchmark (regra permanente), não contornar DRM/Themida/WinLicense/VMProtect, não remover verificações licença, validar TLS se usar HTTP (patch), não coletar MAC sem consentimento (privacidade)
- **Testes:** `TestPacketParse` — parsing header size/opcode sem rede real, `TestSocket` BLOCKED sem servidor (requer Windows/GPU/servidor real), registrar como BLOCKED com justificativa
- **Compatibilidade:** manter validação servidor, não inventar opcodes, medir com Wireshark em ambiente Windows real com servidor de teste, registrar condições hardware/rede

## Estado

- **CONCLUÍDO ciclo13:** estrutura pacotes documentada com rótulos CONFIRMADO/PROVÁVEL/DESCONHECIDO/BLOQUEADO explicitamente, import tables medidos, ListaPatch.txt formato CONFIRMADO, Lua ProtocolCore.lua existência CONFIRMADA estrutura interna DESCONHECIDO, protocolo jogo BLOQUEADO por VMProtect
- **PARCIAL:** implementação não iniciada, opcodes não decodificados, crypto não implementada
- **Próximo passo:** implementar socket.hpp wrapper sem rede real, teste parsing header, documentar opcodes quando medidos em Windows real com Wireshark

E2 CONCLUÍDO ciclo13 como estrutura documentada com rótulos, implementação pendente e BLOCKED para decodificação real.
