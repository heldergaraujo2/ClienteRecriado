// SPDX-License-Identifier: MIT
// BMD body parsing: accepted geometry/topology/bone records and remaining gaps.
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
// Remaining limits (kept explicit so accepted bytes are not mistaken for
// complete format compatibility):
//
//   - The parser reads triangle records and some bone records, but it does not
//     establish UV semantics, corner-to-UV indices, texture/material
//     association, skinning weights, or animation meaning.
//   - The candidate coordinate region is not exposed as verified UV data. A
//     plausible float distribution is not semantic confirmation.
//   - The BMD v0x0C cipher remains BLOCKED by the repository policy in
//     AGENTS.md. Existing tool labels are classifier observations and must not
//     be treated as authorization to extend cipher work or claim full format
//     compatibility.
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
/// A historical restricted sample of 58,836 references in 319 single-mesh
/// files measured 97.66% / 97.20% in-range (limited to the first 80 triangles
/// per file). The cycle-17 full-tree report counts 523,717 out-of-range vertex
/// refs and 507,068 out-of-range normal refs. Consumers must bounds-check; see
/// docs/COMPATIBILITY.md and docs/ASSET_FORMATS.md.
struct BmdTriangleCorner {
  int32_t vertex = -1;
  int32_t normal = -1;
  /// The u16 at record offset 4. Observed as padding-like (64.95% zero and
  /// 18.18% 0xCDCD in 40,902 samples); it is not a validated UV index and must
  /// not be used for texturing. Retained as raw data so the parser does not
  /// silently discard it.
  uint16_t optional = 0;
  uint8_t flag = 0;
};

struct BmdTriangle {
  BmdTriangleCorner corners[3];
};

/// Why the triangle array was not read. Recorded because "triangles is empty"
/// on its own cannot be interpreted: the same empty vector means "this file has
/// no faces", "this file has several meshes and we only handle one" or "the
/// declared array does not fit", and those have different consequences for a
/// consumer. Measured over the whole asset tree in
/// docs/evidence/ciclo4_topology_skip_reasons.log.
enum class BmdTopologySkip : uint8_t {
  kNone = 0,      ///< Triangle records were read.
  kMultiMesh,     ///< A multi-mesh body could not be parsed by the current path.
  kZeroFaces,     ///< The file declares no triangles at all.
  kDoesNotFit,    ///< A declared array runs past the end of the payload.
  kNoHeader,      ///< Payload shorter than a required count field.
};

const char* BmdTopologySkipName(BmdTopologySkip s);

struct BmdBone {
  std::string name;          ///< 32-byte NUL-padded ASCII, may be truncated
  int parent = -1;           ///< -1 for root (65535 sentinel), else 0..boneCount-1
  float raw[6] = {0};        ///< 6 floats: inferred as position xyz + rotation xyz, not confirmed
  bool has_name = false;
  bool floats_finite = false;
};

struct BmdBody {
  std::string source_name;      ///< From the 32-byte field; may be empty.
  bool has_source_name = false;
  std::vector<BmdVertex> vertices;
  std::vector<BmdNormal> normals;
  /// Triangle records from parsed mesh arrays. May be empty when the declared
  /// body has no faces or a required count/array could not be validated; see
  /// |topology_skip| and |problem|.
  std::vector<BmdTriangle> triangles;
  /// Why `triangles` is empty. `kNone` when it is not. Only meaningful once the
  /// vertex and normal arrays parsed successfully, since the same arithmetic
  /// that fails here would already have failed there.
  BmdTopologySkip topology_skip = BmdTopologySkip::kNone;
  /// `meshCount` as declared, recorded for every file whether or not triangles
  /// were read, so the multi-mesh share can be measured instead of assumed.
  std::size_t declared_mesh_count = 0;
  /// `numTriangles` as declared, for the same reason.
  std::size_t declared_triangle_count = 0;
  /// `boneCount` as declared (payload offset 34)
  std::size_t declared_bone_count = 0;
  /// `actionCount` as declared (payload offset 36)
  std::size_t declared_action_count = 0;
  /// Texture coordinates per vertex of a triangle are NOT located. The u16 at
  /// corner offset 4 was the natural candidate and was ruled out. A mesh
  /// therefore cannot be textured correctly from this parser yet.
  bool has_texcoords = false;

  /// Bones parsed after triangles. Layout verified: each bone except last is
  /// 32 B name + 2 B parent (65535=root) + 24 B (6×f32) + 1 B pad 0 =59 B,
  /// last bone is 32+2+24=58 B, total bc*59-1. For multi-mesh, an extra u32
  /// (actionCount) precedes the bone block: total 4+bc*59-1. See
  /// docs/ASSET_FORMATS.md and evidence ciclo8_bones_*.log.
  std::vector<BmdBone> bones;
  bool has_bones = false;

  /// Triangle-corner references outside the vertex or normal array. Kept as
  /// separate reference counts; MeshFromBmd drops each affected triangle once.
  /// The cycle-17 full-tree totals are recorded in docs/COMPATIBILITY.md.
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
constexpr std::size_t kBmdBoneCountOffset = 34;
constexpr std::size_t kBmdActionCountOffset = 36;


/// Parses the body of a v0x0A BMD payload. The caller passes the payload only
/// (the 4-byte "BMD"+version header is not included).
///
/// Never throws. On any inconsistency it sets |problem| and leaves |complete|
/// false, because a body that half-parsed must not be mistaken for one that
/// parsed.
Status BmdParseBodyV0A(ByteView payload, BmdBody* out);

}  // namespace mu

#endif  // MU_ASSETS_BMD_BODY_HPP
