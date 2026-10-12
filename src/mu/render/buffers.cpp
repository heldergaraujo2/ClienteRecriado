// SPDX-License-Identifier: MIT
#include "mu/render/buffers.hpp"

#include <cmath>

namespace mu {

PackedBuffers PackMeshForUpload(const Mesh& mesh) {
  PackedBuffers out;
  out.skipped_vertices = mesh.skipped_vertices;
  out.bad_indices = mesh.bad_indices;
  out.has_texcoords = mesh.has_texcoords;
  out.has_bone_weights = mesh.has_bone_weights;

  if (mesh.positions.empty()) {
    out.problem = "no positions";
    return out;
  }
  if (mesh.normals.size() != mesh.positions.size()) {
    out.problem = "normals size mismatch";
    return out;
  }
  if (mesh.has_texcoords && mesh.texcoords.size() != mesh.positions.size()) {
    out.problem = "texcoords size mismatch despite has_texcoords true";
    return out;
  }
  if (mesh.has_bone_weights && mesh.bone_weights.size() != mesh.positions.size()) {
    out.problem = "bone_weights size mismatch despite has_bone_weights true";
    return out;
  }

  out.vertices.reserve(mesh.positions.size());
  for (size_t i = 0; i < mesh.positions.size(); ++i) {
    PackedVertex pv;
    pv.px = mesh.positions[i].x;
    pv.py = mesh.positions[i].y;
    pv.pz = mesh.positions[i].z;
    pv.nx = mesh.normals[i].x;
    pv.ny = mesh.normals[i].y;
    pv.nz = mesh.normals[i].z;
    if (mesh.has_texcoords) {
      pv.u = mesh.texcoords[i].u;
      pv.v = mesh.texcoords[i].v;
    } else {
      pv.u = 0.0f;
      pv.v = 0.0f;
    }
    if (mesh.has_bone_weights) {
      const auto& w = mesh.bone_weights[i];
      pv.weight_count = static_cast<uint32_t>(w.size() > 4 ? 4 : w.size());
      for (uint32_t k = 0; k < pv.weight_count; ++k) {
        pv.bone_ids[k] = w[k].bone;
        pv.bone_weights[k] = w[k].weight;
      }
      for (uint32_t k = pv.weight_count; k < 4; ++k) {
        pv.bone_ids[k] = 0;
        pv.bone_weights[k] = 0.0f;
      }
    } else {
      pv.weight_count = 0;
      for (int k = 0; k < 4; ++k) {
        pv.bone_ids[k] = 0;
        pv.bone_weights[k] = 0.0f;
      }
    }
    out.vertices.push_back(pv);
  }

  if (!mesh.indices.empty()) {
    if (mesh.indices.size() % 3 != 0) {
      out.problem = "indices not multiple of 3";
      return out;
    }
    // Validate indices against vertex count
    size_t bad = 0;
    for (uint32_t idx : mesh.indices) {
      if (idx >= mesh.positions.size()) ++bad;
    }
    out.bad_indices += bad;
    // Copy only valid indices? For testability, we copy all but count bad
    // For upload, we should drop triangles with bad indices (same as MeshFromBmd)
    out.indices.reserve(mesh.indices.size());
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
      uint32_t a = mesh.indices[i];
      uint32_t b = mesh.indices[i+1];
      uint32_t c = mesh.indices[i+2];
      if (a >= mesh.positions.size() || b >= mesh.positions.size() || c >= mesh.positions.size()) {
        continue; // drop triangle with bad index
      }
      out.indices.push_back(a);
      out.indices.push_back(b);
      out.indices.push_back(c);
    }
  } else {
    // No indices: sequential 0..N-1
    out.indices.reserve(mesh.positions.size());
    for (uint32_t i = 0; i < mesh.positions.size(); ++i) out.indices.push_back(i);
  }

  out.bounds = ComputeBounds(mesh.positions);
  out.ok = true;
  return out;
}

PackedSkeleton PackSkeletonForUpload(const Skeleton& skel) {
  PackedSkeleton out;
  out.has_bones = skel.has_bones;
  if (!skel.has_bones) {
    out.problem = "no bones";
    out.ok = true; // not an error, just empty
    return out;
  }
  if (skel.bones.empty()) {
    out.problem = "has_bones true but bones empty";
    return out;
  }
  out.parents.reserve(skel.bones.size());
  out.names.reserve(skel.bones.size());
  out.positions.reserve(skel.bones.size());
  out.rotations.reserve(skel.bones.size());
  for (size_t i = 0; i < skel.bones.size(); ++i) {
    const auto& b = skel.bones[i];
    if (b.parent != kNoBoneParent && (b.parent < 0 || static_cast<size_t>(b.parent) >= skel.bones.size())) {
      out.problem = "bone parent out of range";
      return out;
    }
    if (b.parent == static_cast<int32_t>(i)) {
      out.problem = "bone self-loop parent==i";
      return out;
    }
    out.parents.push_back(b.parent);
    out.names.push_back(b.name);
    out.positions.push_back(b.position);
    out.rotations.push_back(b.rotation);
  }
  // Cycle detection same as BmdBody
  for (size_t i = 0; i < skel.bones.size(); ++i) {
    bool visited[64] = {false};
    int cur = static_cast<int>(i);
    size_t steps = 0;
    while (cur != kNoBoneParent && steps <= skel.bones.size()) {
      if (cur < 0 || static_cast<size_t>(cur) >= skel.bones.size()) break;
      if (visited[cur]) {
        out.problem = "bone cycle detected";
        return out;
      }
      visited[cur] = true;
      cur = out.parents[static_cast<size_t>(cur)];
      ++steps;
    }
  }
  out.ok = true;
  return out;
}

} // namespace mu
