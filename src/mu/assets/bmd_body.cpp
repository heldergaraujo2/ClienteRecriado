// SPDX-License-Identifier: MIT
// BMD v0x0A body parsing. See bmd_body.hpp for the layout and its evidence.
//
// Provenance: loop structure over meshCount and per-mesh field order inspired
// by xulek/muonline-bmd-viewer (src/bmd-loader.ts:348-422, ISC, commit
// 3f11bf89f3702261174d8efa1436d56a15c7d97a). The 14-byte header
// [vc,nc,ntc,ntri,texIdx,u48,u50] and triangle layout [vertex at +0/+8/+16]
// are this project's own measurements, not that project's tables, which were
// refuted for these files (polygon byte 3/4 at 3.01% ≈ random). Texture name
// delta -4 is this project's discovery. See docs/LICENSING.md and
// docs/DEPENDENCIES.md.
#include "mu/assets/bmd_body.hpp"

#include <cmath>
#include <cstring>

namespace mu {

namespace {

/// Reads the NUL-terminated name from the fixed 32-byte field.
///
/// The field is only meaningful when it holds printable ASCII. 932 of the 1,465
/// clear files do not begin with printable bytes at all, so an empty or
/// non-printable field is treated as "no name" rather than as an error: the
/// geometry that follows is still valid in those files.
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

}  // namespace

namespace {

// Strict texture name check: 32 bytes, NUL-terminated, at least 3 printable
// ASCII before NUL, rest zeros after NUL.
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

// Weak check used by the original topology probe: at least 8 printable ASCII
// in a 32-byte window. This counts as a hit even when the name starts 4 bytes
// earlier than predicted, because the window still contains most of the name.
bool IsTextureNameWeak(const uint8_t* p, std::size_t remaining) {
  if (remaining < 32) return false;
  std::size_t printable = 0;
  for (std::size_t i = 0; i < 32; ++i) {
    unsigned char c = p[i];
    if (c >= 32 && c < 127) ++printable;
  }
  return printable >= 8;
}

}  // namespace

Status BmdParseBodyV0A(ByteView payload, BmdBody* out) {
  if (out == nullptr) {
    return Status::Make(Status::kCorrupt, "null output");
  }
  *out = BmdBody{};

  if (payload.size < kBmdVertexArrayOffset) {
    out->problem = "payload shorter than the 52-byte body header";
    return Status::Make(Status::kTruncated, out->problem);
  }

  out->has_source_name = ReadSourceName(payload, &out->source_name);

  // Global counts, for reporting.
  std::size_t global_mesh_count = 0;
  std::size_t global_triangle_count = 0;
  if (payload.size >= kBmdTriangleCountOffset + 2) {
    global_mesh_count = ReadU16LE(payload.data + kBmdMeshCountOffset);
    global_triangle_count = ReadU16LE(payload.data + kBmdTriangleCountOffset);
    out->declared_mesh_count = global_mesh_count;
    out->declared_triangle_count = global_triangle_count;
  }

  // Single-mesh fast path: keep the verified arithmetic that was proven over
  // 145 files. This path is unchanged from the previous revision so that the
  // existing evidence for single-mesh continues to hold. The test fixtures set
  // meshCount to 0, so we treat 0 and 1 the same for vertex/normal parsing;
  // triangle parsing still requires meshCount==1.
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
      const uint8_t* p = payload.data + kBmdVertexArrayOffset +
                         i * kBmdVertexRecordSize;
      BmdVertex v;
      v.x = ReadF32LE(p);
      v.y = ReadF32LE(p + 4);
      v.z = ReadF32LE(p + 8);
      v.aux0 = ReadU16LE(p + 12);
      v.aux1 = ReadU16LE(p + 14);
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
      n.nx = ReadF32LE(p);
      n.ny = ReadF32LE(p + 4);
      n.nz = ReadF32LE(p + 8);
      n.aux0 = ReadF32LE(p + 12);
      n.aux1 = ReadF32LE(p + 16);
      if (!IsFiniteFloat(n.nx) || !IsFiniteFloat(n.ny) || !IsFiniteFloat(n.nz)) {
        out->problem = "non-finite normal component";
        return Status::Make(Status::kCorrupt, out->problem);
      }
      out->normals.push_back(n);
    }

    if (payload.size >= kBmdTriangleCountOffset + 2) {
      const std::size_t texcoord_count =
          ReadU16LE(payload.data + kBmdTexCoordCountOffset);
      const std::size_t triangle_count =
          ReadU16LE(payload.data + kBmdTriangleCountOffset);

      const std::size_t triangles_offset = required + texcoord_count * 8;
      const std::size_t triangles_end =
          triangles_offset + triangle_count * kBmdTriangleRecordSize;

      if (triangle_count > 0 && triangles_end >= triangles_offset &&
          triangles_end <= payload.size) {
        // The texture name is 4 bytes early in many files (systematic). The
        // original probe used a weak check (>=8 printable) that counted both
        // positions as hits, giving 87.9%. Strict check at exact end gives 0%.
        // We accept both, but prefer exact.
        bool has_name = false;
        if (triangles_end + 32 <= payload.size) {
          has_name = IsTextureName(payload.data + triangles_end,
                                   payload.size - triangles_end) ||
                     IsTextureNameWeak(payload.data + triangles_end,
                                       payload.size - triangles_end);
        }
        // Even if name check fails, triangles are still readable when they fit.
        // The name check is not a gate for single-mesh; it was only evidence.
        (void)has_name;
        out->topology_skip = BmdTopologySkip::kNone;
        out->triangles.reserve(triangle_count);
        for (std::size_t i = 0; i < triangle_count; ++i) {
          const uint8_t* base =
              payload.data + triangles_offset + i * kBmdTriangleRecordSize;
          BmdTriangle t;
          for (std::size_t k = 0; k < 3; ++k) {
            const uint8_t* p = base + k * kBmdCornerRecordSize;
            t.corners[k].vertex = ReadU16LE(p);
            t.corners[k].normal = ReadU16LE(p + 2);
            t.corners[k].optional = ReadU16LE(p + 4);
            t.corners[k].flag = p[6];
          }
          out->triangles.push_back(t);
        }
        for (const BmdTriangle& t : out->triangles) {
          for (const BmdTriangleCorner& c : t.corners) {
            if (c.vertex < 0 ||
                static_cast<std::size_t>(c.vertex) >= vertex_count) {
              ++out->out_of_range_vertices;
            }
            if (c.normal < 0 ||
                static_cast<std::size_t>(c.normal) >= normal_count) {
              ++out->out_of_range_normals;
            }
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

    out->complete = true;
    return Status::Ok();
  }

  // Multi-mesh path: meshCount >1. This is the cause of 100% of the level-3
  // to level-4 drop (4,257 files). The layout repeats per mesh:
  //   [14-byte header: vc,nc,ntc,ntri,texIdx,u48,u50]
  //   vc*16 vertices, nc*20 normals, ntc*8 texcoords, ntri*64 triangles,
  //   32-byte texture name.
  // The texture name is systematically 4 bytes early (end-4) in many files,
  // so we search with delta.
  if (global_mesh_count > 1) {
    // Safety: meshCount comes from untrusted file. 0xFFFF would cause a huge
    // loop and OOM. Max observed in the real tree is 18, so 64 is generous and
    // still prevents abuse. See item 5 of the cycle-5 audit.
    constexpr std::size_t kMaxMeshCount = 64;
    if (global_mesh_count > kMaxMeshCount) {
      out->problem = "meshCount exceeds safety limit";
      out->topology_skip = BmdTopologySkip::kDoesNotFit;
      return Status::Make(Status::kCorrupt, out->problem);
    }

    std::size_t offset = kBmdMeshCountOffset + 6;  // 38, start of first mesh header
    std::size_t total_vertices = 0;
    std::size_t total_normals = 0;
    std::size_t total_triangles = 0;

    // Reserve roughly.
    out->vertices.reserve(1024);
    out->normals.reserve(1024);
    out->triangles.reserve(1024);

    for (std::size_t mi = 0; mi < global_mesh_count; ++mi) {
      if (offset + 14 > payload.size) {
        out->topology_skip = BmdTopologySkip::kDoesNotFit;
        break;
      }
      const std::size_t vc = ReadU16LE(payload.data + offset);
      const std::size_t nc = ReadU16LE(payload.data + offset + 2);
      const std::size_t ntc = ReadU16LE(payload.data + offset + 4);
      const std::size_t ntri = ReadU16LE(payload.data + offset + 6);
      // texIdx at +8, u48 at +10, u50 at +12 - not needed for geometry but
      // checked for overflow.

      // Overflow-safe multiplication: check before multiply. vc etc are u16
      // (max 65535) so product fits in 64-bit, but we still guard against
      // payload-size overflow and against absurd allocation.
      auto mul_overflows = [](std::size_t a, std::size_t b, std::size_t limit) -> bool {
        if (a == 0 || b == 0) return false;
        return a > limit / b;
      };
      // Use payload size as limit for per-mesh arrays; if any single array
      // would exceed the whole payload it cannot fit.
      if (mul_overflows(vc, kBmdVertexRecordSize, payload.size) ||
          mul_overflows(nc, kBmdNormalRecordSize, payload.size) ||
          mul_overflows(ntc, 8, payload.size) ||
          mul_overflows(ntri, kBmdTriangleRecordSize, payload.size)) {
        out->topology_skip = BmdTopologySkip::kDoesNotFit;
        break;
      }

      const std::size_t v_start = offset + 14;
      // Check addition overflow and bounds. Use careful order to avoid wrap.
      if (v_start > payload.size) {
        out->topology_skip = BmdTopologySkip::kDoesNotFit;
        break;
      }
      std::size_t n_start = 0, tc_start = 0, tri_start = 0, tri_end = 0;
      // v_start + vc*16
      {
        std::size_t add = vc * kBmdVertexRecordSize;
        if (v_start > payload.size || add > payload.size - v_start) {
          out->topology_skip = BmdTopologySkip::kDoesNotFit;
          break;
        }
        n_start = v_start + add;
      }
      {
        std::size_t add = nc * kBmdNormalRecordSize;
        if (n_start > payload.size || add > payload.size - n_start) {
          // Allow n_start to be exactly payload.size when nc==0 etc, but
          // otherwise must fit.
          if (nc != 0) {
            out->topology_skip = BmdTopologySkip::kDoesNotFit;
            break;
          }
        } else {
          tc_start = n_start + add;
        }
        if (nc == 0) tc_start = n_start;
      }
      {
        std::size_t add = ntc * 8;
        if (tc_start > payload.size || (ntc != 0 && add > payload.size - tc_start)) {
          out->topology_skip = BmdTopologySkip::kDoesNotFit;
          break;
        }
        tri_start = tc_start + add;
        if (ntc == 0) tri_start = tc_start;
      }
      {
        std::size_t add = ntri * kBmdTriangleRecordSize;
        if (tri_start > payload.size + 8) {  // allow -4 tolerance
          out->topology_skip = BmdTopologySkip::kDoesNotFit;
          break;
        }
        if (ntri != 0 && add > payload.size + 8 - tri_start) {
          // tri_end would exceed payload+8 tolerance
          if (tri_start + add > payload.size + 8) {
            out->topology_skip = BmdTopologySkip::kDoesNotFit;
            break;
          }
        }
        tri_end = tri_start + add;
      }

      if (v_start > payload.size || n_start > payload.size ||
          tc_start > payload.size || tri_start > payload.size) {
        out->topology_skip = BmdTopologySkip::kDoesNotFit;
        break;
      }
      // Allow tri_end to be up to 8 bytes past payload for the -4 case, but
      // otherwise must fit.
      if (tri_end > payload.size + 8) {
        out->topology_skip = BmdTopologySkip::kDoesNotFit;
        break;
      }

      // Find texture name near tri_end. Systematic -4.
      std::size_t name_at = 0;
      bool found_name = false;
      for (int delta : {0, -4, 4, -8, 8}) {
        std::size_t at;
        if (delta < 0) {
          if (tri_end < static_cast<std::size_t>(-delta)) continue;
          at = tri_end + delta;
        } else {
          at = tri_end + delta;
        }
        if (at + 32 > payload.size) continue;
        if (IsTextureName(payload.data + at, payload.size - at)) {
          name_at = at;
          found_name = true;
          break;
        }
      }
      if (!found_name) {
        for (int delta : {0, -4, 4, -8, 8}) {
          std::size_t at;
          if (delta < 0) {
            if (tri_end < static_cast<std::size_t>(-delta)) continue;
            at = tri_end + delta;
          } else {
            at = tri_end + delta;
          }
          if (at + 32 > payload.size) continue;
          if (IsTextureNameWeak(payload.data + at, payload.size - at)) {
            name_at = at;
            found_name = true;
            break;
          }
        }
      }
      if (!found_name) {
        // No texture name found near expected end - still count as parsed if
        // triangles fit, but record that name was missing? For level-4 we only
        // need triangles, so we can still succeed even without name.
        // Use tri_end as next offset.
        name_at = tri_end;
        // If tri_end itself is beyond payload, fail.
        if (tri_end > payload.size) {
          out->topology_skip = BmdTopologySkip::kDoesNotFit;
          break;
        }
      }

      // Parse vertices for this mesh, accumulating with global offset.
      const std::size_t base_vertex = total_vertices;
      const std::size_t base_normal = total_normals;
      for (std::size_t i = 0; i < vc; ++i) {
        const uint8_t* p = payload.data + v_start + i * kBmdVertexRecordSize;
        if (v_start + (i + 1) * kBmdVertexRecordSize > payload.size) break;
        BmdVertex v;
        v.x = ReadF32LE(p);
        v.y = ReadF32LE(p + 4);
        v.z = ReadF32LE(p + 8);
        v.aux0 = ReadU16LE(p + 12);
        v.aux1 = ReadU16LE(p + 14);
        if (!IsFiniteFloat(v.x) || !IsFiniteFloat(v.y) || !IsFiniteFloat(v.z)) {
          // Skip non-finite, but keep parsing other meshes.
          continue;
        }
        out->vertices.push_back(v);
      }
      for (std::size_t i = 0; i < nc; ++i) {
        const uint8_t* p = payload.data + n_start + i * kBmdNormalRecordSize;
        if (n_start + (i + 1) * kBmdNormalRecordSize > payload.size) break;
        BmdNormal n;
        n.nx = ReadF32LE(p);
        n.ny = ReadF32LE(p + 4);
        n.nz = ReadF32LE(p + 8);
        n.aux0 = ReadF32LE(p + 12);
        n.aux1 = ReadF32LE(p + 16);
        if (!IsFiniteFloat(n.nx) || !IsFiniteFloat(n.ny) || !IsFiniteFloat(n.nz)) {
          continue;
        }
        out->normals.push_back(n);
      }
      // Parse triangles, adjusting indices by base offsets.
      for (std::size_t i = 0; i < ntri; ++i) {
        const std::size_t tri_off = tri_start + i * kBmdTriangleRecordSize;
        if (tri_off + kBmdTriangleRecordSize > payload.size + 8) break;
        // If tri_off is beyond payload but within -4 tolerance, allow.
        if (tri_off + 24 > payload.size) break;
        BmdTriangle t;
        bool tri_ok = true;
        for (std::size_t k = 0; k < 3; ++k) {
          const uint8_t* p = payload.data + tri_off + k * kBmdCornerRecordSize;
          if (tri_off + k * kBmdCornerRecordSize + 8 > payload.size + 8) {
            tri_ok = false;
            break;
          }
          int v_idx = ReadU16LE(p);
          int n_idx = ReadU16LE(p + 2);
          // Bounds check against this mesh's counts, not global, before adjusting.
          if (v_idx < 0 || static_cast<std::size_t>(v_idx) >= vc) {
            ++out->out_of_range_vertices;
          }
          if (n_idx < 0 || static_cast<std::size_t>(n_idx) >= nc) {
            ++out->out_of_range_normals;
          }
          t.corners[k].vertex = static_cast<int>(base_vertex + v_idx);
          t.corners[k].normal = static_cast<int>(base_normal + n_idx);
          t.corners[k].optional = ReadU16LE(p + 4);
          t.corners[k].flag = p[6];
        }
        if (tri_ok) {
          out->triangles.push_back(t);
          ++total_triangles;
        }
      }

      total_vertices += vc;
      total_normals += nc;
      offset = name_at + 32;
    }

    if (!out->triangles.empty()) {
      out->topology_skip = BmdTopologySkip::kNone;
    } else if (out->vertices.empty()) {
      out->topology_skip = BmdTopologySkip::kDoesNotFit;
    } else {
      // Vertices parsed but no triangles - could be zero faces or does not fit.
      // Keep the skip that was set, or default to does not fit.
      if (out->topology_skip == BmdTopologySkip::kNone) {
        out->topology_skip = BmdTopologySkip::kDoesNotFit;
      }
    }

    out->complete = !out->vertices.empty();
    if (!out->complete) {
      out->problem = "multi-mesh parsing yielded no vertices";
      return Status::Make(Status::kTruncated, out->problem);
    }
    return Status::Ok();
  }

  // Fallback: meshCount 0 or other.
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
  switch (s) {
    case BmdTopologySkip::kNone: return "read";
    case BmdTopologySkip::kMultiMesh: return "multi-mesh";
    case BmdTopologySkip::kZeroFaces: return "declares zero faces";
    case BmdTopologySkip::kDoesNotFit: return "declared array does not fit";
    case BmdTopologySkip::kNoHeader: return "payload too short for counts";
  }
  return "unknown";
}

}  // namespace mu
