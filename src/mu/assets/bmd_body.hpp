// SPDX-License-Identifier: MIT
// BMD v0x0A body parsing - the two verified sections.
//
// This module exists because the container header and the body are different
// problems, and only the body yields geometry. The layout below was derived
// from the reference assets and verified, not guessed:
//
//   offset  size   field
//   0       32     source model name, ASCII, NUL-padded (not always present)
//   32       2     u16, undecoded
//   34       2     u16, undecoded
//   36       2     u16, undecoded
//   38       2     u16 VERTEX COUNT
//   40       2     u16 NORMAL COUNT
//   42      ...    undecoded
//   52      vc*16  vertex array, 16 bytes each:
//                    [f32 x][f32 y][f32 z][i16 node][i16 bindVertex]
//   52+vc*16 nc*20 normal array, 20 bytes each:
//                    [f32 nx][f32 ny][f32 nz][f32 aux0][f32 aux1]
//
// The array start and stride were confirmed on 145 of 145 testable files with
// zero exceptions, by deriving the normals position geometrically first (runs
// of unit-length float triples) and only then comparing it against the header.
// See docs/evidence/ciclo1_bmd_layout.log.
//
// Field position inside the record was settled by measurement over all 1,194
// v0x0A files, and it does NOT match the ISC-licensed xulek/muonline-bmd-viewer
// layout tables, which lead each record with a node index and a padding
// halfword. In these files the coordinates come first:
//
//   - Normals read at record offset 0 are unit-length 99.91% of the time
//     (260,030 of 260,268). At offset 4, only 32.51%. Offset 0 wins outright.
//   - In the vertex record, the u16 at offset 12 is below 200 in 99.94% of
//     records and the one at offset 14 in 95.53%; at offsets 0 and 2 the
//     figures are 2.63% and 0.82%. The small integers trail the coordinates.
//
// An earlier revision of this file applied the reference layout instead. That
// was wrong, and the reason is worth keeping: the measurements that appeared to
// support it were taken against file offsets while the parser works on payload
// offsets, and the two differ by the 4-byte "BMD"+version header. Every number
// in that revision was shifted by 4 bytes. The corrected measurements are the
// ones above.
//
// What this module deliberately does NOT do:
//
//   - It does not read face indices, texture coordinates or bones. The section
//     after the normals is not decoded; several plausible layouts were tested
//     and none held. Emitting guessed indices would produce silently wrong
//     geometry, which is worse than emitting none.
//   - It does not read the per-mesh texture coordinate array, triangle array or
//     texture name. The full per-mesh layout is documented in
//     docs/ASSET_FORMATS.md; porting it is pending.
//   - It handles only the f32 arrays used by v0x0A. The v0x0C variant stores
//     these arrays as half-floats, so this parser must not be pointed at a
//     v0x0C payload; doing so reads the wrong width and yields garbage.
#ifndef MU_ASSETS_BMD_BODY_HPP
#define MU_ASSETS_BMD_BODY_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "mu/assets/format.hpp"

namespace mu {

struct BmdVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  /// Trailing small integers, unidentified. They were briefly named node and
  /// bind_vertex on the strength of being small, and that was wrong: aux0 has
  /// median 1, p99 68 and maximum 88, while boneCount in the same files is
  /// typically 1 to 8. A bone index does not reach 88. They are not texture
  /// coordinate indices either — that test passes at 99.89% but does not
  /// discriminate, because the values are almost always below 200 and so is
  /// numTexCoords. Kept unnamed so the name does not become a false fact that
  /// later code assumes.
  uint16_t aux0 = 0;
  uint16_t aux1 = 0;
};

struct BmdNormal {
  float nx = 0.0f;
  float ny = 0.0f;
  float nz = 0.0f;
  /// Trailing pair. The reference implementation calls these texture
  /// coordinates; this project has not confirmed that reading, so they are
  /// carried unnamed rather than labelled with a guess.
  float aux0 = 0.0f;
  float aux1 = 0.0f;
};

/// One corner of a triangle. The record holds three of these, 8 bytes each, at
/// record offsets 0, 8 and 16.
///
/// Measured over 58,836 references in 319 single-mesh files: |vertex| is below
/// the mesh vertex count in 97.66% of cases and |normal| below the normal count
/// in 97.20%. Those are not 100%, so a consumer must bounds-check rather than
/// trust the file. See docs/ASSET_FORMATS.md.
struct BmdTriangleCorner {
  int32_t vertex = -1;
  int32_t normal = -1;
  /// The u16 at record offset 4. This is PADDING, not an index. Measured over
  /// 40,902 samples: 64.95% zero and 18.18% exactly 0xCDCD, which is the MSVC
  /// uninitialised-memory fill pattern. Excluding those, it is below the
  /// texcoord count in only 5.33% of cases. Carried so nothing is silently
  /// dropped, but it must not be used as an index.
  uint16_t optional = 0;
  uint8_t flag = 0;
};

struct BmdTriangle {
  BmdTriangleCorner corners[3];
};

struct BmdBody {
  std::string source_name;      ///< From the 32-byte field; may be empty.
  bool has_source_name = false;
  std::vector<BmdVertex> vertices;
  std::vector<BmdNormal> normals;
  /// Triangles, when the mesh counts allowed locating the array. Empty when the
  /// body was not laid out as a single mesh, which is not an error.
  std::vector<BmdTriangle> triangles;
  /// Texture coordinates per vertex of a triangle are NOT located. The u16 at
  /// corner offset 4 was the natural candidate and was ruled out. A mesh
  /// therefore cannot be textured correctly from this parser yet.
  bool has_texcoords = false;

  /// Corner indices that fall outside the vertex or normal array. Measured at
  /// roughly 2.4% of all references, cause not established. Reported rather than
  /// rejected so a caller can bounds-check instead of reading out of bounds.
  std::size_t out_of_range_vertices = 0;
  std::size_t out_of_range_normals = 0;

  /// Set when parsing stopped, with the reason. Empty on success.
  std::string problem;
  /// True only when both arrays were read completely and their sizes match the
  /// header counts. A partial read is not reported as success.
  bool complete = false;
};

/// Byte offsets and record sizes, exposed so tests can state the layout they
/// are checking instead of repeating literals.
constexpr std::size_t kBmdNameSize = 32;
constexpr std::size_t kBmdVertexCountOffset = 38;
constexpr std::size_t kBmdNormalCountOffset = 40;
constexpr std::size_t kBmdVertexArrayOffset = 52;
constexpr std::size_t kBmdVertexRecordSize = 16;
constexpr std::size_t kBmdNormalRecordSize = 20;
constexpr std::size_t kBmdTriangleRecordSize = 64;
constexpr std::size_t kBmdCornerRecordSize = 8;
constexpr std::size_t kBmdTexCoordCountOffset = 42;
constexpr std::size_t kBmdTriangleCountOffset = 44;
constexpr std::size_t kBmdMeshCountOffset = 32;


/// Parses the body of a v0x0A BMD payload. The caller passes the payload only
/// (the 4-byte "BMD"+version header is not included).
///
/// Never throws. On any inconsistency it sets |problem| and leaves |complete|
/// false, because a body that half-parsed must not be mistaken for one that
/// parsed.
Status BmdParseBodyV0A(ByteView payload, BmdBody* out);

}  // namespace mu

#endif  // MU_ASSETS_BMD_BODY_HPP
