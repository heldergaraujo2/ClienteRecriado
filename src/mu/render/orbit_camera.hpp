// SPDX-License-Identifier: MIT
// Platform-independent orbit-camera state for diagnostic model viewers.
#ifndef MU_RENDER_ORBIT_CAMERA_HPP
#define MU_RENDER_ORBIT_CAMERA_HPP

namespace mu::render {

/// Minimal vector owned by the camera component, avoiding an asset/parser
/// dependency. A platform adapter can convert it to its native math type.
struct OrbitVec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

/// Camera pose in model/world coordinates. A D3D11 caller can pass converted
/// vectors to DirectXMath's XMMatrixLookAtLH; this type does not choose a GPU
/// API or matrix memory convention.
struct OrbitCameraPose {
  OrbitVec3 eye{};
  OrbitVec3 target{};
  OrbitVec3 up{0.0f, 1.0f, 0.0f};
};

/// Small, testable orbit controller. Angles are radians; positive yaw moves the
/// eye from the -Z axis toward +X. Positive wheel steps zoom in. Mouse-pixel
/// sensitivity and window/message handling belong to the platform adapter.
class OrbitCamera final {
 public:
  static constexpr float kDefaultDistance = 3.0f;
  static constexpr float kMinimumDistance = 0.05f;
  static constexpr float kMaximumDistance = 100000.0f;
  /// Keep the view direction away from the world-up singularity at +/-90 deg.
  static constexpr float kMaximumPitchRadians = 1.55334303427f;  // 89 degrees
  /// Multiplicative zoom factor is exp(-wheel_steps * this value).
  static constexpr float kZoomLogRate = 0.12f;

  OrbitCamera() noexcept = default;
  explicit OrbitCamera(OrbitVec3 target) noexcept;

  /// Rejects a non-finite target without changing the current camera.
  bool SetTarget(OrbitVec3 target) noexcept;
  /// Wraps yaw to [-pi, pi), clamps pitch, and rejects non-finite inputs.
  bool SetAngles(float yaw_radians, float pitch_radians) noexcept;
  /// Clamps positive finite values to the supported distance range.
  bool SetDistance(float distance) noexcept;

  /// Applies angular deltas in radians. Returns false for non-finite input.
  bool Orbit(float yaw_delta_radians, float pitch_delta_radians) noexcept;
  /// Applies mouse-wheel detents; positive values zoom in exponentially.
  /// Returns false for non-finite input.
  bool Zoom(float wheel_steps) noexcept;

  OrbitVec3 target() const noexcept { return target_; }
  float yaw_radians() const noexcept { return yaw_radians_; }
  float pitch_radians() const noexcept { return pitch_radians_; }
  float distance() const noexcept { return distance_; }
  OrbitCameraPose Pose() const noexcept;

 private:
  OrbitVec3 target_{};
  float yaw_radians_ = 0.0f;
  float pitch_radians_ = 0.0f;
  float distance_ = kDefaultDistance;
};

}  // namespace mu::render

#endif  // MU_RENDER_ORBIT_CAMERA_HPP
