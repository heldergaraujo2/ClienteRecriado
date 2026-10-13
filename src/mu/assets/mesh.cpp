// SPDX-License-Identifier: MIT
#include "mu/assets/mesh.hpp"

#include <cmath>
#include <limits>

namespace mu {

Bounds ComputeBounds(const std::vector<Vec3>& positions) {
  Bounds b;
  if (positions.empty()) {
    // Left invalid on purpose. Returning a box from -inf to +inf would make
    // every cull test pass, and returning a zero box at the origin would make
    // geometry elsewhere in the world cull away. Neither is what "no box"
    // should mean.
    return b;
  }

  const float inf = std::numeric_limits<float>::infinity();
  Vec3 lo{inf, inf, inf};
  Vec3 hi{-inf, -inf, -inf};

  for (const Vec3& p : positions) {
    // Non-finite components are skipped rather than folded in: one NaN would
    // poison every comparison and silently produce a box that culls nothing.
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
      continue;
    }
    if (p.x < lo.x) lo.x = p.x;
    if (p.y < lo.y) lo.y = p.y;
    if (p.z < lo.z) lo.z = p.z;
    if (p.x > hi.x) hi.x = p.x;
    if (p.y > hi.y) hi.y = p.y;
    if (p.z > hi.z) hi.z = p.z;
  }

  // Every component non-finite means nothing was folded in, so there is still no
  // usable box even though the list was not empty.
  if (!std::isfinite(lo.x) || !std::isfinite(hi.x) || !std::isfinite(lo.y) ||
      !std::isfinite(hi.y) || !std::isfinite(lo.z) || !std::isfinite(hi.z)) {
    return Bounds{};
  }

  b.min = lo;
  b.max = hi;
  b.valid = true;
  return b;
}

}  // namespace mu

namespace mu {

MeshImportResult MeshFromBmd(const BmdBody& body, const std::string& name) {
  MeshImportResult r;
  r.mesh.name = name.empty() ? body.source_name : name;

  r.mesh.positions.reserve(body.vertices.size());
  for (const BmdVertex& v : body.vertices) {
    // Non-finite positions are counted and skipped. Keeping one would poison the
    // bounding box and put a vertex at infinity in the draw.
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) {
      ++r.mesh.skipped_vertices;
      continue;
    }
    r.mesh.positions.push_back(Vec3{v.x, v.y, v.z});
  }

  r.mesh.normals.reserve(body.normals.size());
  for (const BmdNormal& n : body.normals) {
    r.mesh.normals.push_back(Vec3{n.nx, n.ny, n.nz});
  }

  // Skipped vertices shift every later index, so indices are only usable when
  // nothing was skipped. Reporting that is better than silently re-indexing,
  // which would need a mapping this function does not build.
  const bool indices_usable = r.mesh.skipped_vertices == 0;
  const std::size_t vertex_count = r.mesh.positions.size();
  const std::size_t normal_count = r.mesh.normals.size();

  if (indices_usable) {
    r.mesh.indices.reserve(body.triangles.size() * 3);
    for (const BmdTriangle& t : body.triangles) {
      bool good = true;
      for (const BmdTriangleCorner& c : t.corners) {
        const bool vertex_ok = c.vertex >= 0 &&
                               static_cast<std::size_t>(c.vertex) < vertex_count;
        const bool normal_ok = c.normal >= 0 &&
                               static_cast<std::size_t>(c.normal) < normal_count;
        if (!vertex_ok || !normal_ok) {
          good = false;
          ++r.mesh.dropped_triangles;
          break;
        }
      }
      if (!good) continue;
      for (const BmdTriangleCorner& c : t.corners) {
        r.mesh.indices.push_back(static_cast<uint32_t>(c.vertex));
      }
    }
  }

  // Texture coordinates are intentionally left empty. See the header comment:
  // the array exists in the file but the corner-to-entry binding does not, and
  // guessing it would produce a mesh that renders and looks wrong.
  r.mesh.has_texcoords = false;

  r.mesh.bounds = ComputeBounds(r.mesh.positions);

  r.ok = !r.mesh.positions.empty();
  if (!r.ok) r.problem = "body produced no usable vertex positions";
  return r;
}

}  // namespace mu
