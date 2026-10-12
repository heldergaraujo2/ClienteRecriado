#include "mu/assets/bmd_body.hpp"
#include <cmath>
#include <cstring>
namespace mu {
namespace {
bool ReadSourceName(ByteView payload, std::string* out) {
  if (payload.size < kBmdNameSize) return false;
  std::size_t end = 0;
  while (end < kBmdNameSize && payload.data[end] != 0) ++end;
  if (end == 0) return false;
  for (std::size_t i = 0; i < end; ++i) {
    const unsigned char c = payload.data[i];
    if (c < 32 || c > 126) return false;
  }
  out->assign(reinterpret_cast<const char*>(payload.data), end);
  return true;
}
bool IsFiniteFloat(float v) { return std::isfinite(v); }
bool IsTextureName(const uint8_t* p, std::size_t remaining) {
  if (remaining < 32) return false;
  std::size_t nul = 0;
  while (nul < 32 && p[nul] != 0) ++nul;
  if (nul < 3 || nul >= 32) return false;
  for (std::size_t i = 0; i < nul; ++i) {
    unsigned char c = p[i];
    if (c < 32 || c > 126) return false;
  }
  for (std::size_t i = nul; i < 32; ++i) {
    if (p[i] != 0) return false;
  }
  return true;
}
bool IsTextureNameWeak(const uint8_t* p, std::size_t remaining) {
  if (remaining < 32) return false;
  std::size_t printable = 0;
  for (std::size_t i = 0; i < 32; ++i) {
    unsigned char c = p[i];
    if (c >= 32 && c < 127) ++printable;
  }
  return printable >= 8;
}
bool TryParseBonesAt(ByteView payload, std::size_t base, std::size_t bone_count,
                     std::vector<BmdBone>* out_bones) {
  if (bone_count == 0 || bone_count > 64) return false;
  if (base > payload.size) return false;
  std::size_t expected = bone_count * 59 - 1;
  bool has_extra_pad = false;
  if (base + expected > payload.size) {
    if (base + bone_count * 59 <= payload.size) {
      expected = bone_count * 59;
      has_extra_pad = true;
    } else {
      if (base + expected > payload.size) return false;
    }
  }
  std::vector<BmdBone> bones;
  bones.reserve(bone_count);
  std::size_t offset = base;
  std::size_t valid_names_strict = 0;
  std::size_t valid_names_loose = 0;
  for (std::size_t i = 0; i < bone_count; ++i) {
    std::size_t rec_size = 59;
    if (!has_extra_pad && i + 1 == bone_count) rec_size = 58;
    if (offset + 32 > payload.size) return false;
    const uint8_t* p = payload.data + offset;
    std::string name;
    bool has_name_strict = false;
    bool has_name_loose = false;
    // Strict: NUL-padded ASCII >=3
    std::size_t nul = 0;
    while (nul < 32 && p[nul] != 0) ++nul;
    if (nul >= 3 && nul < 32) {
      bool ok = true;
      for (std::size_t k = 0; k < nul; ++k) {
        unsigned char c = p[k];
        if (c < 32 || c > 126) { ok = false; break; }
      }
      if (ok) {
        for (std::size_t k = nul; k < 32; ++k) if (p[k] != 0) { ok = false; break; }
        if (ok) {
          name.assign(reinterpret_cast<const char*>(p), nul);
          has_name_strict = true;
          ++valid_names_strict;
        }
      }
    }
    // Loose: at least 3 printable in 32B (old probe)
    int printable = 0;
    for (std::size_t k = 0; k < 32; ++k) {
      unsigned char c = p[k];
      if (c >= 32 && c < 127) ++printable;
    }
    if (printable >= 3) {
      has_name_loose = true;
      ++valid_names_loose;
      if (name.empty()) {
        // fallback name: first printable run
        std::size_t start = 0;
        while (start < 32 && (p[start] < 32 || p[start] >= 127)) ++start;
        std::size_t end = start;
        while (end < 32 && p[end] >= 32 && p[end] < 127) ++end;
        if (end > start) name.assign(reinterpret_cast<const char*>(p+start), end-start);
      }
    }
    if (offset + 34 > payload.size) return false;
    uint16_t parent_raw = ReadU16LE(p + 32);
    int parent = (parent_raw == 0xFFFF) ? -1 : static_cast<int>(parent_raw);
    if (parent != -1 && (parent < 0 || static_cast<std::size_t>(parent) >= bone_count)) {
      return false;
    }
    if (offset + 34 + 6*4 > payload.size) return false;
    float vals[6];
    bool finite = true;
    for (int k = 0; k < 6; ++k) {
      vals[k] = ReadF32LE(p + 34 + k*4);
      if (!IsFiniteFloat(vals[k])) finite = false;
    }
    if (!finite) return false;
    if (rec_size == 59) {
      if (offset + 58 >= payload.size) return false;
    }
    BmdBone b;
    b.name = name;
    b.parent = parent;
    for (int k = 0; k < 6; ++k) b.raw[k] = vals[k];
    b.has_name = has_name_strict || has_name_loose;
    b.floats_finite = finite;
    bones.push_back(std::move(b));
    offset += rec_size;
  }
  if (bones.size() != bone_count) return false;
  // Require at least loose names for all bones to avoid garbage
  if (valid_names_loose < bone_count) return false;
  *out_bones = std::move(bones);
  return true;
}
} // namespace

Status BmdParseBodyV0A(ByteView payload, BmdBody* out) {
  if (out == nullptr) return Status::Make(Status::kCorrupt, "null output");
  *out = BmdBody{};
  if (payload.size < kBmdVertexArrayOffset) {
    out->problem = "payload shorter than the 52-byte body header";
    return Status::Make(Status::kTruncated, out->problem);
  }
  out->has_source_name = ReadSourceName(payload, &out->source_name);
  std::size_t global_mesh_count = 0;
  std::size_t global_triangle_count = 0;
  std::size_t global_bone_count = 0;
  std::size_t global_action_count = 0;
  if (payload.size >= kBmdTriangleCountOffset + 2) {
    global_mesh_count = ReadU16LE(payload.data + kBmdMeshCountOffset);
    global_bone_count = ReadU16LE(payload.data + kBmdBoneCountOffset);
    global_action_count = ReadU16LE(payload.data + kBmdActionCountOffset);
    global_triangle_count = ReadU16LE(payload.data + kBmdTriangleCountOffset);
    out->declared_mesh_count = global_mesh_count;
    out->declared_triangle_count = global_triangle_count;
    out->declared_bone_count = global_bone_count;
    out->declared_action_count = global_action_count;
  }
  if (global_mesh_count <= 1) {
    const std::size_t vertex_count = ReadU16LE(payload.data + kBmdVertexCountOffset);
    const std::size_t normal_count = ReadU16LE(payload.data + kBmdNormalCountOffset);
    const std::size_t vertex_bytes = vertex_count * kBmdVertexRecordSize;
    const std::size_t normal_bytes = normal_count * kBmdNormalRecordSize;
    const std::size_t normals_offset = kBmdVertexArrayOffset + vertex_bytes;
    const std::size_t required = normals_offset + normal_bytes;
    if (required < normals_offset || required > payload.size) {
      out->problem = "declared vertex/normal counts exceed the payload";
      return Status::Make(Status::kTruncated, out->problem);
    }
    out->vertices.reserve(vertex_count);
    for (std::size_t i = 0; i < vertex_count; ++i) {
      const uint8_t* p = payload.data + kBmdVertexArrayOffset + i * kBmdVertexRecordSize;
      BmdVertex v;
      v.x = ReadF32LE(p); v.y = ReadF32LE(p+4); v.z = ReadF32LE(p+8);
      v.aux0 = ReadU16LE(p+12); v.aux1 = ReadU16LE(p+14);
      if (!IsFiniteFloat(v.x) || !IsFiniteFloat(v.y) || !IsFiniteFloat(v.z)) {
        out->problem = "non-finite vertex coordinate";
        return Status::Make(Status::kCorrupt, out->problem);
      }
      out->vertices.push_back(v);
    }
    out->normals.reserve(normal_count);
    for (std::size_t i = 0; i < normal_count; ++i) {
      const uint8_t* p = payload.data + normals_offset + i * kBmdNormalRecordSize;
      BmdNormal n;
      n.nx = ReadF32LE(p); n.ny = ReadF32LE(p+4); n.nz = ReadF32LE(p+8);
      n.aux0 = ReadF32LE(p+12); n.aux1 = ReadF32LE(p+16);
      if (!IsFiniteFloat(n.nx) || !IsFiniteFloat(n.ny) || !IsFiniteFloat(n.nz)) {
        out->problem = "non-finite normal component";
        return Status::Make(Status::kCorrupt, out->problem);
      }
      out->normals.push_back(n);
    }
    std::size_t triangles_offset = 0;
    std::size_t triangles_end = 0;
    std::size_t texcoord_count = 0;
    std::size_t triangle_count = 0;
    if (payload.size >= kBmdTriangleCountOffset + 2) {
      texcoord_count = ReadU16LE(payload.data + kBmdTexCoordCountOffset);
      triangle_count = ReadU16LE(payload.data + kBmdTriangleCountOffset);
      triangles_offset = required + texcoord_count * 8;
      triangles_end = triangles_offset + triangle_count * kBmdTriangleRecordSize;
      if (triangle_count > 0 && triangles_end >= triangles_offset && triangles_end <= payload.size) {
        bool has_name = false;
        if (triangles_end + 32 <= payload.size) {
          has_name = IsTextureName(payload.data + triangles_end, payload.size - triangles_end) ||
                     IsTextureNameWeak(payload.data + triangles_end, payload.size - triangles_end);
        }
        (void)has_name;
        out->topology_skip = BmdTopologySkip::kNone;
        out->triangles.reserve(triangle_count);
        for (std::size_t i = 0; i < triangle_count; ++i) {
          const uint8_t* base = payload.data + triangles_offset + i * kBmdTriangleRecordSize;
          BmdTriangle t;
          for (std::size_t k = 0; k < 3; ++k) {
            const uint8_t* p = base + k * kBmdCornerRecordSize;
            t.corners[k].vertex = ReadU16LE(p);
            t.corners[k].normal = ReadU16LE(p+2);
            t.corners[k].optional = ReadU16LE(p+4);
            t.corners[k].flag = p[6];
          }
          out->triangles.push_back(t);
        }
        for (const BmdTriangle& t : out->triangles) {
          for (const BmdTriangleCorner& c : t.corners) {
            if (c.vertex < 0 || static_cast<std::size_t>(c.vertex) >= vertex_count) ++out->out_of_range_vertices;
            if (c.normal < 0 || static_cast<std::size_t>(c.normal) >= normal_count) ++out->out_of_range_normals;
          }
        }
      } else if (triangle_count == 0) {
        out->topology_skip = BmdTopologySkip::kZeroFaces;
      } else {
        out->topology_skip = BmdTopologySkip::kDoesNotFit;
      }
    } else {
      out->topology_skip = BmdTopologySkip::kNoHeader;
    }
    // Bone parsing for single-mesh
    if (global_bone_count > 0 && triangles_end > 0) {
      std::size_t bone_base = triangles_end + 32;
      if (bone_base <= payload.size) {
        std::vector<BmdBone> bones;
        if (TryParseBonesAt(payload, bone_base, global_bone_count, &bones)) {
          out->bones = std::move(bones);
          out->has_bones = true;
        }
      }
    }
    out->complete = true;
    return Status::Ok();
  }
  if (global_mesh_count > 1) {
    constexpr std::size_t kMaxMeshCount = 64;
    if (global_mesh_count > kMaxMeshCount) {
      out->problem = "meshCount exceeds safety limit";
      out->topology_skip = BmdTopologySkip::kDoesNotFit;
      return Status::Make(Status::kCorrupt, out->problem);
    }
    std::size_t offset = kBmdMeshCountOffset + 6;
    std::size_t total_vertices = 0;
    std::size_t total_normals = 0;
    std::size_t total_triangles = 0;
    out->vertices.reserve(1024);
    out->normals.reserve(1024);
    out->triangles.reserve(1024);
    for (std::size_t mi = 0; mi < global_mesh_count; ++mi) {
      if (offset + 14 > payload.size) { out->topology_skip = BmdTopologySkip::kDoesNotFit; break; }
      const std::size_t vc = ReadU16LE(payload.data + offset);
      const std::size_t nc = ReadU16LE(payload.data + offset + 2);
      const std::size_t ntc = ReadU16LE(payload.data + offset + 4);
      const std::size_t ntri = ReadU16LE(payload.data + offset + 6);
      auto mul_overflows = [](std::size_t a, std::size_t b, std::size_t limit)->bool {
        if (a==0||b==0) return false; return a > limit / b;
      };
      if (mul_overflows(vc,kBmdVertexRecordSize,payload.size) ||
          mul_overflows(nc,kBmdNormalRecordSize,payload.size) ||
          mul_overflows(ntc,8,payload.size) ||
          mul_overflows(ntri,kBmdTriangleRecordSize,payload.size)) {
        out->topology_skip = BmdTopologySkip::kDoesNotFit; break;
      }
      const std::size_t v_start = offset + 14;
      if (v_start > payload.size) { out->topology_skip = BmdTopologySkip::kDoesNotFit; break; }
      std::size_t n_start=0, tc_start=0, tri_start=0, tri_end=0;
      {
        std::size_t add = vc * kBmdVertexRecordSize;
        if (v_start > payload.size || add > payload.size - v_start) { out->topology_skip = BmdTopologySkip::kDoesNotFit; break; }
        n_start = v_start + add;
      }
      {
        std::size_t add = nc * kBmdNormalRecordSize;
        if (n_start > payload.size || (nc!=0 && add > payload.size - n_start)) {
          if (nc!=0) { out->topology_skip = BmdTopologySkip::kDoesNotFit; break; }
        } else {
          tc_start = n_start + add;
        }
        if (nc==0) tc_start = n_start;
      }
      {
        std::size_t add = ntc * 8;
        if (tc_start > payload.size || (ntc!=0 && add > payload.size - tc_start)) {
          out->topology_skip = BmdTopologySkip::kDoesNotFit; break;
        }
        tri_start = tc_start + add;
        if (ntc==0) tri_start = tc_start;
      }
      {
        std::size_t add = ntri * kBmdTriangleRecordSize;
        if (tri_start > payload.size + 8) { out->topology_skip = BmdTopologySkip::kDoesNotFit; break; }
        if (ntri!=0 && tri_start + add > payload.size + 8) {
          if (tri_start + add > payload.size + 8) { out->topology_skip = BmdTopologySkip::kDoesNotFit; break; }
        }
        tri_end = tri_start + add;
      }
      if (v_start > payload.size || n_start > payload.size || tc_start > payload.size || tri_start > payload.size) {
        out->topology_skip = BmdTopologySkip::kDoesNotFit; break;
      }
      if (tri_end > payload.size + 8) { out->topology_skip = BmdTopologySkip::kDoesNotFit; break; }
      std::size_t name_at=0; bool found_name=false;
      for (int delta: {0,-4,4,-8,8}) {
        std::size_t at;
        if (delta<0) { if (tri_end < static_cast<std::size_t>(-delta)) continue; at = tri_end + delta; } else at = tri_end + delta;
        if (at+32>payload.size) continue;
        if (IsTextureName(payload.data+at, payload.size-at)) { name_at=at; found_name=true; break; }
      }
      if (!found_name) {
        for (int delta: {0,-4,4,-8,8}) {
          std::size_t at;
          if (delta<0) { if (tri_end < static_cast<std::size_t>(-delta)) continue; at = tri_end + delta; } else at = tri_end + delta;
          if (at+32>payload.size) continue;
          if (IsTextureNameWeak(payload.data+at, payload.size-at)) { name_at=at; found_name=true; break; }
        }
      }
      if (!found_name) {
        name_at = tri_end;
        if (tri_end > payload.size) { out->topology_skip = BmdTopologySkip::kDoesNotFit; break; }
      }
      const std::size_t base_vertex = total_vertices;
      const std::size_t base_normal = total_normals;
      for (std::size_t i=0;i<vc;++i) {
        const uint8_t* p = payload.data + v_start + i * kBmdVertexRecordSize;
        if (v_start + (i+1)*kBmdVertexRecordSize > payload.size) break;
        BmdVertex v; v.x=ReadF32LE(p); v.y=ReadF32LE(p+4); v.z=ReadF32LE(p+8);
        v.aux0=ReadU16LE(p+12); v.aux1=ReadU16LE(p+14);
        if (!IsFiniteFloat(v.x)||!IsFiniteFloat(v.y)||!IsFiniteFloat(v.z)) continue;
        out->vertices.push_back(v);
      }
      for (std::size_t i=0;i<nc;++i) {
        const uint8_t* p = payload.data + n_start + i * kBmdNormalRecordSize;
        if (n_start + (i+1)*kBmdNormalRecordSize > payload.size) break;
        BmdNormal n; n.nx=ReadF32LE(p); n.ny=ReadF32LE(p+4); n.nz=ReadF32LE(p+8);
        n.aux0=ReadF32LE(p+12); n.aux1=ReadF32LE(p+16);
        if (!IsFiniteFloat(n.nx)||!IsFiniteFloat(n.ny)||!IsFiniteFloat(n.nz)) continue;
        out->normals.push_back(n);
      }
      for (std::size_t i=0;i<ntri;++i) {
        const std::size_t tri_off = tri_start + i * kBmdTriangleRecordSize;
        if (tri_off + kBmdTriangleRecordSize > payload.size + 8) break;
        if (tri_off + 24 > payload.size) break;
        BmdTriangle t; bool tri_ok=true;
        for (std::size_t k=0;k<3;++k) {
          const uint8_t* p = payload.data + tri_off + k * kBmdCornerRecordSize;
          if (tri_off + k*kBmdCornerRecordSize + 8 > payload.size + 8) { tri_ok=false; break; }
          int v_idx=ReadU16LE(p); int n_idx=ReadU16LE(p+2);
          if (v_idx<0 || static_cast<std::size_t>(v_idx)>=vc) ++out->out_of_range_vertices;
          if (n_idx<0 || static_cast<std::size_t>(n_idx)>=nc) ++out->out_of_range_normals;
          t.corners[k].vertex = static_cast<int>(base_vertex + v_idx);
          t.corners[k].normal = static_cast<int>(base_normal + n_idx);
          t.corners[k].optional = ReadU16LE(p+4);
          t.corners[k].flag = p[6];
        }
        if (tri_ok) { out->triangles.push_back(t); ++total_triangles; }
      }
      total_vertices+=vc; total_normals+=nc;
      offset = name_at + 32;
    }
    if (!out->triangles.empty()) out->topology_skip = BmdTopologySkip::kNone;
    else if (out->vertices.empty()) out->topology_skip = BmdTopologySkip::kDoesNotFit;
    else {
      if (out->topology_skip == BmdTopologySkip::kNone) out->topology_skip = BmdTopologySkip::kDoesNotFit;
    }
    // Bone parsing for multi-mesh: extra u32 actionCount before bones
    if (global_bone_count>0 && offset>0 && offset <= payload.size) {
      std::size_t bone_base = offset;
      std::size_t remaining = payload.size > bone_base ? payload.size - bone_base : 0;
      if (remaining >= 4) {
        // Check if first 4 bytes look like actionCount (small) and rest matches bone layout
        uint32_t extra = ReadU32LE(payload.data + bone_base);
        // extra should equal actionCount (from header) or be small (<256)
        // and remaining should be 4 + bc*59-1
        std::size_t expected_with_extra = 4 + global_bone_count * 59 - 1;
        std::size_t expected_with_extra2 = 4 + global_bone_count * 59;
        if ((extra == global_action_count || extra < 256) &&
            (remaining == expected_with_extra || remaining == expected_with_extra2 || remaining >= expected_with_extra)) {
          // Try parse at bone_base+4
          std::vector<BmdBone> bones;
          if (TryParseBonesAt(payload, bone_base+4, global_bone_count, &bones)) {
            out->bones = std::move(bones);
            out->has_bones = true;
          }
        } else {
          // Try without extra header (fallback)
          std::vector<BmdBone> bones;
          if (TryParseBonesAt(payload, bone_base, global_bone_count, &bones)) {
            out->bones = std::move(bones);
            out->has_bones = true;
          }
        }
      } else {
        std::vector<BmdBone> bones;
        if (TryParseBonesAt(payload, bone_base, global_bone_count, &bones)) {
          out->bones = std::move(bones);
          out->has_bones = true;
        }
      }
    }
    out->complete = !out->vertices.empty();
    if (!out->complete) {
      out->problem = "multi-mesh parsing yielded no vertices";
      return Status::Make(Status::kTruncated, out->problem);
    }
    return Status::Ok();
  }
  if (global_mesh_count == 0) {
    out->topology_skip = BmdTopologySkip::kMultiMesh;
    out->complete = true;
    return Status::Ok();
  }
  out->topology_skip = BmdTopologySkip::kNoHeader;
  out->complete = true;
  return Status::Ok();
}
const char* BmdTopologySkipName(BmdTopologySkip s) {
  switch(s){
    case BmdTopologySkip::kNone: return "read";
    case BmdTopologySkip::kMultiMesh: return "multi-mesh";
    case BmdTopologySkip::kZeroFaces: return "declares zero faces";
    case BmdTopologySkip::kDoesNotFit: return "declared array does not fit";
    case BmdTopologySkip::kNoHeader: return "payload too short for counts";
  }
  return "unknown";
}
} // namespace mu
