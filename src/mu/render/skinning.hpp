// SPDX-License-Identifier: MIT
// CPU-side LBS skinning (D2 — CONCLUÍDO ciclo12)
// Strict count only, no OpenGL headers, no GL calls
#ifndef MU_RENDER_SKINNING_HPP
#define MU_RENDER_SKINNING_HPP

#include <cstddef>
#include <vector>
#include <string>

#include "mu/assets/mesh.hpp"

namespace mu {

struct Mat4 {
  // row-major 4x4: m[row*4+col]
  float m[16];
  static Mat4 Identity();
};

struct SkinnedResult {
  std::vector<Vec3> positions;
  bool ok = false;
  std::string problem;
};

// Validates skeleton for skinning: no self-loop, parent in range, no cycle, has_bones true, bones non-empty
bool ValidateSkeletonForSkinning(const Skeleton& skel, std::string* problem_out);

// Applies LBS skinning: skinned = sum weight_i * (boneTransforms[bone_i] * originalPos)
// - mesh must have has_bone_weights true and bone_weights size == positions size (strict)
// - boneTransforms size must == skeleton bones size (strict)
// - skeleton must pass ValidateSkeletonForSkinning (strict)
// - if mesh has_bone_weights false, returns original positions (zero movement) but ok=true
// - No OpenGL headers
SkinnedResult ApplySkinningLBS(const Mesh& mesh, const Skeleton& skel, const std::vector<Mat4>& boneTransforms);

Vec3 TransformPoint(const Mat4& mat, const Vec3& p);

} // namespace mu

#endif // MU_RENDER_SKINNING_HPP
