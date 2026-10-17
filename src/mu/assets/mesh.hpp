// SPDX-License-Identifier: MIT
// Internal mesh representation — the single format all importers produce.
//
// Why this module exists. The project has to serve two sources of geometry: the
// original client's BMD files, and new custom assets in a modern format. If the
// renderer read either format directly, every rendering concern would be written
// twice and would drift. So there is one representation, and importers convert
// into it. See docs/ARCHITECTURE.md section 9.
//
// The contract, and why each part is the way it is:
//
//   - Counts are 32-bit. The BMD format stores counts as u16, which caps a mesh
//     at 65,535 vertices and 65,535 triangles. That limit is a property of the
//     source format, not of the geometry, and this representation must not
//     inherit it — otherwise the "support significantly heavier assets"
//     requirement fails at the data layer before the renderer is even involved.
//
//   - Triangle indices are separate from the vertex list. A mesh whose vertices
//     are shared must not pay to expand them, which is what a non-indexed draw
//     does. This is also what makes VBO/VAO with glDrawElements worth having
//     (docs/DECISIONS.md, DEC-008).
//
//   - Bounding box is computed at import, not at draw. Frustum culling is a
//     project requirement from the start (docs/RENDERING.md section 12.1), and a
//     box recomputed every frame would cost more than the culling saves.
//
//   - Nothing is optional in a way that hides a gap. A mesh that has positions
//     but no texture coordinates says so explicitly, because the BMD importer
//     currently produces exactly that and the renderer must not silently
//     texturise it with garbage.
#ifndef MU_ASSETS_MESH_HPP
#define MU_ASSETS_MESH_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "mu/assets/format.hpp"

namespace mu {

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct Vec2 {
  float u = 0.0f;
  float v = 0.0f;
};

/// One bone influence on a vertex. Count per vertex is explicit rather than
/// fixed, because different formats and different assets use different numbers
/// of influences and padding a fixed four would waste both memory and bandwidth.
struct BoneWeight {
  uint32_t bone = 0;
  float weight = 0.0f;
};

/// Sentinel for a bone with no parent. Declared before use so it can serve as a
/// default member initialiser.
constexpr int32_t kNoBoneParent = -1;

struct Bone {
  std::string name;
  /// Index into the bone array, or kNoBoneParent for a root.
  int32_t parent = kNoBoneParent;
  Vec3 position;
  Vec3 rotation;
};

/// An axis-aligned box. Deliberately plain data: it is computed once at import
/// and read many times per frame during culling.
struct Bounds {
  Vec3 min;
  Vec3 max;
  bool valid = false;
};

struct Mesh {
  std::string name;

  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  /// Texture coordinates, parallel to positions when present.
  std::vector<Vec2> texcoords;
  /// Triangle indices into positions. Size is a multiple of 3 when non-empty.
  std::vector<uint32_t> indices;

  /// Bone weights per vertex, parallel to positions when present.
  /// Each vertex may have 0..N influences. Count per vertex is explicit
  /// rather than fixed 4, because different formats use different numbers
  /// and padding would waste memory and bandwidth. BMD importer leaves
  /// this empty (weights not yet decoded), glTF importer will populate.
  std::vector<std::vector<BoneWeight>> bone_weights;

  /// Set only when texcoords is populated AND a mapping from each triangle
  /// corner to a texcoord is known. The BMD importer leaves this false: neither
  /// the semantic coordinate array nor its corner mapping has been verified,
  /// so texturing would be guesswork. See docs/ASSET_FORMATS.md.
  bool has_texcoords = false;

  /// Set only when bone_weights is populated and each weight references
  /// a valid bone in the associated Skeleton and weights sum to ~1.0.
  /// BMD importer leaves this false (weights not decoded), glTF will set true.
  bool has_bone_weights = false;

  Bounds bounds;

  /// Vertices whose position could not be trusted, counted rather than dropped.
  /// A converter that silently skipped bad vertices would produce a mesh that
  /// looks fine and is wrong, which is worse than one that reports the problem.
  std::size_t skipped_vertices = 0;
  /// Triangles rejected because at least one corner has an out-of-range vertex
  /// or normal reference. Incremented once per rejected triangle, not once per
  /// invalid reference.
  std::size_t dropped_triangles = 0;
};

struct Skeleton {
  std::vector<Bone> bones;
  /// Set when the bone hierarchy was read. Transform values may still be
  /// unverified even when this is true — see the note on MeshFromBmd.
  bool has_bones = false;
};

/// Outcome of a conversion. Kept separate from the mesh so that a partially
/// converted mesh is never mistaken for a complete one.
struct MeshImportResult {
  Mesh mesh;
  Skeleton skeleton;
  bool ok = false;
  std::string problem;
};

/// Computes the axis-aligned bounds of a position list. Leaves |valid| false for
/// an empty list, because a zero-sized box at the origin is not the same thing as
/// no box and would cull incorrectly.
Bounds ComputeBounds(const std::vector<Vec3>& positions);

}  // namespace mu

#endif  // MU_ASSETS_MESH_HPP

// The BMD importer needs the body parser's output, included here rather than
// forward-declared because the converter takes it by value-free reference and
// callers already have it.
#include "mu/assets/bmd_body.hpp"

namespace mu {

/// Converts a parsed BMD body into the internal representation.
///
/// Path A of the dual-import architecture (docs/ARCHITECTURE.md section 9.2).
/// Status: PARTIAL, and the gaps are reported rather than papered over.
///
/// What this produces today:
///   - positions and normals from a body accepted by the BMD parser;
///   - triangle indices when the declared mesh arrays can be located and fit;
///   - a bounding box, computed here so culling does not pay for it per frame.
///
/// What it deliberately does not produce:
///   - verified texture coordinates. A candidate byte region is not semantically
///     validated as UV, and the index that binds each triangle corner to an
///     entry is unknown. Emitting guessed coordinates would let the renderer
///     silently use the wrong mapping, so has_texcoords stays false.
///   - skinning weights. The BMD weights have not been decoded/validated; bone
///     records alone do not establish usable skinning semantics.
///
/// Index handling. Triangles with any out-of-range vertex or normal reference
/// are dropped and counted once in |dropped_triangles| rather than clamped;
/// clamping could deform the mesh without any signal that it happened.
MeshImportResult MeshFromBmd(const BmdBody& body, const std::string& name);

}  // namespace mu
