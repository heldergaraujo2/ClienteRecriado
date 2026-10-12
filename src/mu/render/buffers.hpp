// SPDX-License-Identifier: MIT
// CPU-side vertex/index buffer preparation (D1 — CONCLUÍDO ciclo12)
// Consistent with DEC-008: VBO/VAO indexed layout, no OpenGL headers.
// Transforms internal representation C1 (Mesh+Skeleton) into flat arrays ready for upload.
#ifndef MU_RENDER_BUFFERS_HPP
#define MU_RENDER_BUFFERS_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include "mu/assets/mesh.hpp"

namespace mu {

struct PackedVertex {
  float px, py, pz; // position
  float nx, ny, nz; // normal
  float u, v;       // texcoord (0,0 when not available)
  uint32_t bone_ids[4] = {0,0,0,0};
  float bone_weights[4] = {0,0,0,0};
  uint32_t weight_count = 0;
};

struct PackedBuffers {
  std::vector<PackedVertex> vertices;
  std::vector<uint32_t> indices;
  bool has_texcoords = false;
  bool has_bone_weights = false;
  Bounds bounds;
  size_t skipped_vertices = 0;
  size_t bad_indices = 0;
  bool ok = false;
  std::string problem;
};

PackedBuffers PackMeshForUpload(const Mesh& mesh);

struct PackedSkeleton {
  std::vector<int32_t> parents;
  std::vector<std::string> names;
  std::vector<Vec3> positions;
  std::vector<Vec3> rotations;
  bool has_bones = false;
  bool ok = false;
  std::string problem;
};

PackedSkeleton PackSkeletonForUpload(const Skeleton& skel);

} // namespace mu

#endif // MU_RENDER_BUFFERS_HPP
