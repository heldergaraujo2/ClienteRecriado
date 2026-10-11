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
//   52      vc*16  vertex array, 16 bytes each: [f32 x][f32 y][f32 z][u16][u16]
//   52+vc*16 nc*20 normal array, 20 bytes each: [f32 nx][f32 ny][f32 nz][f32][f32]
//
// Both offsets were confirmed on 145 of 145 testable files with zero
// exceptions, by deriving the normals position geometrically first (runs of
// unit-length float triples) and only then comparing it against the header.
// See docs/evidence/ciclo1_bmd_layout.log.
//
// What this module deliberately does NOT do:
//
//   - It does not read face indices, texture coordinates or bones. The section
//     after the normals is not decoded; several plausible layouts were tested
//     and none held. Emitting guessed indices would produce silently wrong
//     geometry, which is worse than emitting none.
//   - It does not interpret the two trailing u16 of a vertex record or the two
//     trailing f32 of a normal record. Both are unidentified. The (u,v) reading
//     of the normal tail was tested and rejected on a control measurement.
//   - It does not handle v0x0C, whose payload is encrypted.
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
  /// Trailing fields of the 16-byte record. Meaning not established; carried
  /// so that callers can inspect them rather than silently losing them.
  uint16_t aux0 = 0;
  uint16_t aux1 = 0;
};

struct BmdNormal {
  float nx = 0.0f;
  float ny = 0.0f;
  float nz = 0.0f;
  /// Trailing fields of the 20-byte record. Meaning not established. Observed
  /// as 0.0 in the files examined.
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
