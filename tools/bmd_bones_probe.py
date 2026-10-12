#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Full-scan measurement of the BMD bone hierarchy.

Why this exists as a committed tool rather than a throwaway script: this project
published "98.0%" for the bone hierarchy based on a script that capped itself at
4,000 files walked and did not say so. The audit caught it. A measurement that
lives in a script with an invisible cap will be re-run by the next person who
cannot see the cap.

This script has NO cap. It walks the whole tree and prints the population size
next to every rate, so a rate cannot be read without its denominator.

Usage: python3 tools/bmd_bones_probe.py <Data directory>
"""
import os
import struct
import sys

KEY = [0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
       0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2]


def decrypt_v0c(data: bytes) -> bytes:
    out = bytearray(len(data))
    running = 0x5E
    for i, c in enumerate(data):
        out[i] = ((c ^ KEY[i & 15]) - running) & 0xFF
        running = (c + 0x3D) & 0xFF
    return bytes(out)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: bmd_bones_probe.py <Data directory>", file=sys.stderr)
        return 1
    root = sys.argv[1]

    seen = 0
    eligible = 0
    size_ok = 0
    names_ok = 0
    transform_ok = 0

    for dirpath, _, filenames in os.walk(root):
        for name in filenames:
            if not name.lower().endswith(".bmd"):
                continue
            path = os.path.join(dirpath, name)
            try:
                raw = open(path, "rb").read()
            except OSError:
                continue
            seen += 1
            if raw[:3] != b"BMD":
                continue
            version = raw[3]
            if version == 0x0A:
                payload = raw[4:]
            elif version == 0x0C:
                payload = decrypt_v0c(raw[8:])
            else:
                continue
            if len(payload) < 52:
                continue

            def u16(off: int) -> int:
                return struct.unpack_from("<H", payload, off)[0]

            mesh_count = u16(32)
            bone_count = u16(34)
            num_vertices = u16(38)
            num_normals = u16(40)
            num_texcoords = u16(42)
            num_triangles = u16(44)

            # Single mesh only. The multi-mesh layout is not confirmed, so
            # including those would measure an assumption, not the format.
            if mesh_count != 1 or num_vertices == 0 or num_triangles == 0:
                continue
            if bone_count == 0 or bone_count > 8:
                continue
            eligible += 1

            base = (52 + num_vertices * 16 + num_normals * 20 +
                    num_texcoords * 8 + num_triangles * 64 + 32)
            # Interleaved record: 32 B name + 27 B transform, total 59 B each,
            # and the whole block is boneCount * 59 - 1 bytes. The -1 is not
            # explained; it is measured, not derived.
            if len(payload) - base != bone_count * 59 - 1:
                continue
            size_ok += 1

            names_valid = True
            floats_finite = True
            for i in range(bone_count):
                rec = base + i * 59
                block = payload[rec:rec + 32]
                if len(block) < 32 or sum(1 for c in block if 32 <= c < 127) < 3:
                    names_valid = False
                    break
                # The transform: u16 parent, then 6 floats, then 1 pad byte.
                for k in range(6):
                    value = struct.unpack_from("<f", payload, rec + 34 + k * 4)[0]
                    if value != value or value in (float("inf"), float("-inf")):
                        floats_finite = False
            if names_valid:
                names_ok += 1
                if floats_finite:
                    transform_ok += 1

    def pct(part: int, whole: int) -> str:
        return "%.2f%%" % (100.0 * part / whole) if whole else "n/a"

    print("BMD bone hierarchy - FULL SCAN, no cap")
    print("  tree                          : %s" % root)
    print()
    print("  .bmd files seen               : %6d" % seen)
    print("  eligible (single mesh, bones) : %6d  (%s of seen)"
          % (eligible, pct(eligible, seen)))
    print("  size matches bc*59-1          : %6d  (%s of eligible)"
          % (size_ok, pct(size_ok, eligible)))
    print("  names ASCII valid             : %6d  (%s of size-ok)"
          % (names_ok, pct(names_ok, size_ok)))
    print("  names valid AND floats finite : %6d  (%s of size-ok)"
          % (transform_ok, pct(transform_ok, size_ok)))
    print()
    print("  Read the denominator. A rate over 'size-ok' is NOT a rate over")
    print("  the tree: %s of size-ok is %s of eligible."
          % (pct(names_ok, size_ok), pct(names_ok, eligible)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
