// SPDX-License-Identifier: MIT
// Frustum culling: reject whole objects before they reach the draw path.
//
// Why this exists and why it is here rather than in a renderer module. The
// project requires frustum culling from the first rendering version
// (docs/RENDERING.md section 12.1), and it is pure CPU mathematics with no GL
// dependency. That makes it implementable and testable in this environment,
// which no part of the actual draw path is — there is no GPU and no GL headers
// here. Putting it behind GL would have made an untestable requirement.
//
// The measurement this is meant to improve is draw call count, not frame rate
// directly. Fewer draw calls is a mechanism; whether it moves frame rate has to
// be measured on real hardware, which this environment cannot do. See
// docs/PERFORMANCE_BASELINE.md for what is and is not measured.
#ifndef MU_RENDER_CULL_HPP
#define MU_RENDER_CULL_HPP

#include "mu/assets/mesh.hpp"

namespace mu {

/// One plane of the view frustum, in the form dot(normal, p) + d >= 0 for points
/// inside. Normals point inward, toward the centre of the frustum.
struct Plane {
  Vec3 normal;
  float d = 0.0f;
};

/// The six planes that bound what the camera can see. Order is fixed so callers
/// and tests agree: left, right, bottom, top, near, far.
struct Frustum {
  Plane planes[6];
};

enum class CullResult {
  kOutside,   ///< Fully outside at least one plane. Safe to skip.
  kInside,    ///< Fully inside all planes.
  kIntersect  ///< Straddles at least one plane. Must be drawn.
};

/// Builds the six planes from a column-major view-projection matrix, the same
/// layout OpenGL uses, so no transposition is needed at the call site.
///
/// The Gribb-Hartmann method: each plane is a sum or difference of matrix rows.
/// Rows are extracted rather than assumed, because getting the row/column
/// convention wrong produces a frustum that culls the visible half of the scene
/// and does so silently.
Frustum FrustumFromViewProjection(const float m[16]);

/// Classifies an axis-aligned box against the frustum.
///
/// Uses the p-vertex test: for each plane, the corner furthest along the plane
/// normal is the last one to leave. If that corner is outside, the whole box is.
/// This is conservative in the safe direction — it can report kIntersect for a
/// box that is actually outside near a corner, which costs a draw call, but it
/// never reports kOutside for a box that is visible, which would drop geometry.
CullResult CullBox(const Frustum& f, const Bounds& box);

/// Convenience for the common case: is this object worth drawing? Returns true
/// for kInside and kIntersect, false only for kOutside.
///
/// An invalid box returns true. A mesh with no bounds cannot be proven outside,
/// and dropping it would make geometry disappear with no diagnostic.
bool IsVisible(const Frustum& f, const Bounds& box);

/// Signed distance from a point to a plane. Positive is inside. Exposed for
/// tests, which need it to build frustums whose expected answers are known by
/// construction rather than by running the same code twice.
float PlaneDistance(const Plane& p, const Vec3& v);

}  // namespace mu

#endif  // MU_RENDER_CULL_HPP
