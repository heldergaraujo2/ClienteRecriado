#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Localiza o array de triângulos num corpo BMD e mede a confiança da hipótese.

Gerador de evidência versionado. A saída deste script é o que está citado em
docs/ASSET_FORMATS.md; rode-o de novo para reproduzir os números em vez de
confiar no texto.

    python3 tools/bmd_topology_probe.py /caminho/para/Data

O que ele testa, e por quê cada parte existe:

1. Hipótese. O corpo de uma malha é vértices, normais, UV, triângulos, nessa
   ordem, com tamanhos vindos do cabeçalho:

       tstart = 52 + numVertices*16 + numNormals*20 + numTexCoords*8
       cada triângulo ocupa 64 bytes
       índices de vértice em +2, +6 e +10 dentro do registro

2. Confirmação independente da localização. Se o stride de 64 estiver certo, o
   nome da textura cai logo depois do último triângulo. Esse teste não depende
   de interpretar nenhum índice.

3. Controle. Uma taxa alta de índices "válidos" só significa algo comparada a
   uma linha de base. São duas:
     - acaso: vc/65536, a chance de um u16 aleatório cair abaixo da contagem;
     - controle limpo: a região de coordenadas f32 dos vértices, onde não há
       índice nenhum. Um controle deslocado dentro do próprio array de
       triângulos NÃO serve, porque o registro inteiro é feito de índices e o
       controle fica contaminado — medido, deu 81,20%, o que não discrimina
       posição nenhuma.

A base do payload difere por versão: 4 bytes em v0x0A, 8 em v0x0C. Usar 4 para
v0x0C desalinha o corpo e o fluxo da cifra ao mesmo tempo, e o resultado parece
uma falha do layout quando é uma falha da medição.
"""

import collections
import os
import struct
import sys

# Tabela da cifra v0x0C. Proveniência: xulek/muonline-bmd-viewer (ISC),
# reimplementada em src/mu/assets/bmd.cpp. Ver docs/LICENSING.md.
V0C_KEY = bytes([0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
                 0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2])

PAYLOAD_BASE = {0x0A: 4, 0x0C: 8}


def decrypt_v0c(data: bytes) -> bytes:
    out = bytearray(len(data))
    running = 0x5E
    for i, c in enumerate(data):
        out[i] = ((c ^ V0C_KEY[i & 15]) - running) & 0xFF
        running = (c + 0x3D) & 0xFF
    return bytes(out)


def payload_of(data: bytes):
    """Retorna o payload decifrado, ou None se a versão não for suportada."""
    if len(data) < 4 or data[:3] != b'BMD':
        return None
    base = PAYLOAD_BASE.get(data[3])
    if base is None or len(data) <= base:
        return None
    pay = data[base:]
    return decrypt_v0c(pay) if data[3] == 0x0C else pay


def main(root: str, limit: int = 1500) -> int:
    paths = []
    for dirpath, _, filenames in os.walk(root):
        for name in filenames:
            if name.lower().endswith('.bmd'):
                paths.append(os.path.join(dirpath, name))

    tri_ok = tri_n = 0
    ctl_ok = ctl_n = 0
    chance = 0.0
    nfiles = 0
    texname_hit = texname_n = 0
    residue = collections.Counter()
    examined = 0

    for path in paths:
        with open(path, 'rb') as fh:
            data = fh.read()
        pay = payload_of(data)
        if pay is None or len(pay) < 52:
            continue
        examined += 1
        if examined > limit:
            break

        mesh_count = struct.unpack_from('<H', pay, 32)[0]
        n_vert = struct.unpack_from('<H', pay, 38)[0]
        n_norm = struct.unpack_from('<H', pay, 40)[0]
        n_uv = struct.unpack_from('<H', pay, 42)[0]
        n_tri = struct.unpack_from('<H', pay, 44)[0]

        # Só arquivos de malha única: com várias malhas o corpo se repete e a
        # aritmética abaixo não se aplica a um bloco só.
        if mesh_count != 1 or n_vert == 0 or n_tri == 0:
            continue

        tstart = 52 + n_vert * 16 + n_norm * 20 + n_uv * 8
        if tstart + n_tri * 64 > len(pay):
            continue

        nfiles += 1
        chance += n_vert / 65536.0

        for i in range(min(n_tri, 100)):
            base = tstart + i * 64
            for k in range(3):
                idx = struct.unpack_from('<H', pay, base + 2 + k * 4)[0]
                tri_n += 1
                if idx < n_vert:
                    tri_ok += 1

        # Controle limpo: coordenadas f32 dos vértices, onde não há índice.
        for i in range(min(n_vert, 300)):
            base = 52 + i * 16
            for k in range(3):
                val = struct.unpack_from('<H', pay, base + k * 4)[0]
                ctl_n += 1
                if val < n_vert:
                    ctl_ok += 1

        # Confirmação independente: nome de textura logo após o último triângulo.
        after = tstart + n_tri * 64
        if after + 32 <= len(pay):
            texname_n += 1
            window = pay[after:after + 32]
            if sum(1 for c in window if 32 <= c < 127) >= 8:
                texname_hit += 1
        residue[len(pay) - after] += 1

    if nfiles == 0:
        print("nenhum arquivo de malha única com array completo encontrado")
        return 1

    hyp = tri_ok / tri_n
    ctl = ctl_ok / ctl_n
    rnd = chance / nfiles

    print("raiz analisada                 : %s" % root)
    print("arquivos de malha única usados : %d" % nfiles)
    print()
    print("HIPÓTESE  índices em +2/+6/+10 : %6d/%6d = %6.2f%%"
          % (tri_ok, tri_n, 100.0 * hyp))
    print("CONTROLE  região f32 de vértice: %6d/%6d = %6.2f%%"
          % (ctl_ok, ctl_n, 100.0 * ctl))
    print("ACASO     vc/65536 médio       : %20.2f%%" % (100.0 * rnd))
    print()
    print("razão hipótese/acaso           : %.1fx" % (hyp / rnd))
    print("razão hipótese/controle        : %.2fx" % (hyp / ctl))
    print()
    print("nome de textura no ponto previsto: %d/%d = %.1f%%"
          % (texname_hit, texname_n, 100.0 * texname_hit / max(1, texname_n)))
    print("resíduos mais comuns após o array: %s"
          % ", ".join("%d (%dx)" % kv for kv in residue.most_common(5)))
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
