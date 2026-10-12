// SPDX-License-Identifier: MIT
#include "mu/render/cull.hpp"

#include <cmath>

namespace mu {

float PlaneDistance(const Plane& p, const Vec3& v) {
  return p.normal.x * v.x + p.normal.y * v.y + p.normal.z * v.z + p.d;
}

namespace {

// Homogeneous row of the matrix. Kept local to this translation unit: it is an
// intermediate of plane extraction, not something callers should see.
struct Vec4 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float w = 0.0f;
};

// Extracts row r of a column-major 4x4 matrix. Column-major means element
// (row, col) lives at m[col * 4 + row], so a row is four elements four apart.
// Getting this backwards yields a frustum that culls the visible half of the
// scene, and does it without any error being reported.
inline Vec4 Row(const float m[16], int r) {
  return Vec4{m[r], m[4 + r], m[8 + r], m[12 + r]};
}

inline Plane MakePlane(const Vec4& a, const Vec4& b, float sa, float sb) {
  Plane p;
  p.normal.x = sa * a.x + sb * b.x;
  p.normal.y = sa * a.y + sb * b.y;
  p.normal.z = sa * a.z + sb * b.z;
  p.d = sa * a.w + sb * b.w;
  // Normalising is what makes p.d a real distance, and more importantly makes
  // the sign test below independent of how large the matrix rows happened to be.
  const float len = std::sqrt(p.normal.x * p.normal.x +
                              p.normal.y * p.normal.y +
                              p.normal.z * p.normal.z);
  if (len > 0.0f) {
    const float inv = 1.0f / len;
    p.normal.x *= inv;
    p.normal.y *= inv;
    p.normal.z *= inv;
    p.d *= inv;
  }
  return p;
}

}  // namespace

Frustum FrustumFromViewProjection(const float m[16]) {
  const Vec4 r0 = Row(m, 0);
  const Vec4 r1 = Row(m, 1);
  const Vec4 r2 = Row(m, 2);
  const Vec4 r3 = Row(m, 3);

  Frustum f;
  // Gribb-Hartmann. Each plane is a sum or difference of two rows; the sign
  // picks which side of the clip volume becomes "inside".
  f.planes[0] = MakePlane(r3, r0, 1.0f, 1.0f);   // left
  f.planes[1] = MakePlane(r3, r0, 1.0f, -1.0f);  // right
  f.planes[2] = MakePlane(r3, r1, 1.0f, 1.0f);   // bottom
  f.planes[3] = MakePlane(r3, r1, 1.0f, -1.0f);  // top
  f.planes[4] = MakePlane(r3, r2, 1.0f, 1.0f);   // near
  f.planes[5] = MakePlane(r3, r2, 1.0f, -1.0f);  // far
  return f;
}

CullResult CullBox(const Frustum& f, const Bounds& box) {
  // An invalid box cannot be proven outside anything. Returning kOutside here
  // would make geometry with uncomputed bounds vanish silently, which is the
  // failure mode this whole module is meant to avoid.
  if (!box.valid) return CullResult::kIntersect;

  bool all_inside = true;
  for (const Plane& p : f.planes) {
    // The p-vertex: for each axis, take whichever end of the box the plane
    // normal points toward. That corner leaves the frustum last, so if it is
    // outside, nothing else can be inside.
    const Vec3 pv{p.normal.x >= 0.0f ? box.max.x : box.min.x,
                  p.normal.y >= 0.0f ? box.max.y : box.min.y,
                  p.normal.z >= 0.0f ? box.max.z : box.min.z};
    // And the n-vertex, the opposite corner, leaves first. If even that one is
    // inside, the box is entirely inside this plane.
    const Vec3 nv{p.normal.x >= 0.0f ? box.min.x : box.max.x,
                  p.normal.y >= 0.0f ? box.min.y : box.max.y,
                  p.normal.z >= 0.0f ? box.min.z : box.max.z};

    if (PlaneDistance(p, pv) < 0.0f) return CullResult::kOutside;
    if (PlaneDistance(p, nv) < 0.0f) all_inside = false;
  }
  return all_inside ? CullResult::kInside : CullResult::kIntersect;
}

bool IsVisible(const Frustum& f, const Bounds& box) {
  return CullBox(f, box) != CullResult::kOutside;
}

}  // namespace mu
