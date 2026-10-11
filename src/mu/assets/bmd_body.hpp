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
  /// Trailing small integers. Measured below 200 in 99.94% and 95.53% of
  /// records respectively, consistent with a bone index and a vertex binding.
  int16_t node = 0;
  int16_t bind_vertex = 0;
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

struct BmdBody {
  std::string source_name;      ///< From the 32-byte field; may be empty.
  bool has_source_name = false;
  std::vector<BmdVertex> vertices;
  std::vector<BmdNormal> normals;

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


/// Parses the body of a v0x0A BMD payload. The caller passes the payload only
/// (the 4-byte "BMD"+version header is not included).
///
/// Never throws. On any inconsistency it sets |problem| and leaves |complete|
/// false, because a body that half-parsed must not be mistaken for one that
/// parsed.
Status BmdParseBodyV0A(ByteView payload, BmdBody* out);

}  // namespace mu

#endif  // MU_ASSETS_BMD_BODY_HPP
