# BMD v0x0A triangle-layout observation — 2026-10-13

## Provenance

Observation supplied by the user from a local Windows PowerShell inspection of
`agnecklace.bmd` in their authorized local client tree. The asset itself and
its raw record bytes are intentionally not stored in this repository.

## Decoded observations

- File size: 10,220 bytes.
- Global mesh count: 2.
- First mesh header begins at file offset 42 and is 10 bytes.
- First mesh counts: 24 vertices, 24 normals, 16 texture-coordinate entries,
  and 48 polygon records; texture index 0.
- Vertex records begin at file offset 52; normal records at 436; candidate UV
  entries at 916; first triangle record at 1044.
- The first triangle record decodes to polygon size 3; vertex indices 0, 1, 2;
  normal indices 0, 1, 2; candidate UV indices 0, 1, 2.
- The file offset and payload offset differ by the 4-byte `BMD` + version
  header. The first record is therefore at file offset 1044 / payload offset
  1040.

## Interpretation

The parser's coordinate cursors point four bytes into the vertex/normal
records. Reusing those cursors as array boundaries displaced the triangle array
by four bytes. The prior interleaved 8-byte-corner interpretation was reading
the wrong starting address. The implementation now subtracts four bytes when
calculating the triangle array start, reads the separate index tables in the
64-byte polygon record, and triangulates quads.

This is evidence from one real model and synthetic fixtures only. It does not
establish that every v0x0A file follows this layout, validate UV semantics,
prove texture/material association, or demonstrate a successful GPU render.
The whole-tree parser report, automated tests, documentation checker, and
Windows D3D11 preview must be rerun against the updated branch.
