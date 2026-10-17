// SPDX-License-Identifier: MIT
#include "mu/render/skinning.hpp"

#include <cmath>

namespace mu {

Mat4 Mat4::Identity() {
  Mat4 r{};
  for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
  r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
  return r;
}

Vec3 TransformPoint(const Mat4& mat, const Vec3& p) {
  // row-major: result = mat * vec4(p,1)
  float x = mat.m[0]*p.x + mat.m[1]*p.y + mat.m[2]*p.z + mat.m[3]*1.0f;
  float y = mat.m[4]*p.x + mat.m[5]*p.y + mat.m[6]*p.z + mat.m[7]*1.0f;
  float z = mat.m[8]*p.x + mat.m[9]*p.y + mat.m[10]*p.z + mat.m[11]*1.0f;
  float w = mat.m[12]*p.x + mat.m[13]*p.y + mat.m[14]*p.z + mat.m[15]*1.0f;
  if (std::fabs(w) > 1e-6f && std::fabs(w-1.0f) > 1e-6f) {
    x/=w; y/=w; z/=w;
  }
  return Vec3{x,y,z};
}

bool ValidateSkeletonForSkinning(const Skeleton& skel, std::string* problem_out) {
  if (!skel.has_bones) {
    if (problem_out) *problem_out = "no bones";
    return false;
  }
  if (skel.bones.empty()) {
    if (problem_out) *problem_out = "has_bones true but empty";
    return false;
  }
  for (size_t i=0;i<skel.bones.size();++i) {
    const auto& b = skel.bones[i];
    if (b.parent != kNoBoneParent) {
      if (b.parent < 0 || static_cast<size_t>(b.parent) >= skel.bones.size()) {
        if (problem_out) *problem_out = "parent out of range";
        return false;
      }
      if (b.parent == static_cast<int32_t>(i)) {
        if (problem_out) *problem_out = "self-loop";
        return false;
      }
    }
  }
  // cycle detection
  for (size_t i=0;i<skel.bones.size();++i) {
    bool visited[64]={false};
    int cur=static_cast<int>(i);
    size_t steps=0;
    while (cur!=kNoBoneParent && steps<=skel.bones.size()) {
      if (cur<0 || static_cast<size_t>(cur)>=skel.bones.size()) break;
      if (visited[cur]) {
        if (problem_out) *problem_out = "cycle";
        return false;
      }
      visited[cur]=true;
      cur=skel.bones[static_cast<size_t>(cur)].parent;
      ++steps;
    }
  }
  return true;
}

SkinnedResult ApplySkinningLBS(const Mesh& mesh, const Skeleton& skel, const std::vector<Mat4>& boneTransforms) {
  SkinnedResult out;
  // Strict count checks
  if (mesh.positions.empty()) {
    out.problem = "no positions";
    return out;
  }
  if (mesh.normals.size()!=mesh.positions.size()) {
    out.problem = "normals mismatch";
    return out;
  }
  // If no bone weights: zero movement (original)
  if (!mesh.has_bone_weights) {
    out.positions = mesh.positions;
    out.ok = true;
    return out;
  }
  if (mesh.bone_weights.size()!=mesh.positions.size()) {
    out.problem = "bone_weights size mismatch strict";
    return out;
  }
  // Skeleton validation strict
  std::string prob;
  if (!ValidateSkeletonForSkinning(skel,&prob)) {
    out.problem = prob;
    return out;
  }
  if (boneTransforms.size()!=skel.bones.size()) {
    out.problem = "boneTransforms size mismatch strict";
    return out;
  }
  out.positions.resize(mesh.positions.size());
  for (size_t vi=0; vi<mesh.positions.size(); ++vi) {
    const auto& wlist = mesh.bone_weights[vi];
    if (wlist.empty()) {
      out.positions[vi]=mesh.positions[vi];
      continue;
    }
    Vec3 acc{0,0,0};
    float wsum=0.0f;
    for (const auto& bw : wlist) {
      if (bw.bone>=boneTransforms.size()) continue; // skip invalid bone id, fail safe
      Vec3 tp = TransformPoint(boneTransforms[bw.bone], mesh.positions[vi]);
      acc.x += tp.x * bw.weight;
      acc.y += tp.y * bw.weight;
      acc.z += tp.z * bw.weight;
      wsum += bw.weight;
    }
    if (wsum < 1e-6f) {
      out.positions[vi]=mesh.positions[vi];
    } else {
      // If weights sum ~1, use acc directly, else normalize
      if (std::fabs(wsum-1.0f) > 1e-3f) {
        acc.x/=wsum; acc.y/=wsum; acc.z/=wsum;
      }
      out.positions[vi]=acc;
    }
  }
  out.ok = true;
  return out;
}

} // namespace mu
