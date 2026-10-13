// SPDX-License-Identifier: MIT
#include "mu/render/orbit_camera.hpp"

#include <algorithm>
#include <cmath>

namespace mu::render {
namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kTwoPi = 2.0 * kPi;

bool IsFinite(OrbitVec3 v) noexcept {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

double WrapYaw(double yaw) noexcept {
  double wrapped = std::remainder(yaw, kTwoPi);
  if (wrapped >= kPi) wrapped -= kTwoPi;
  return wrapped;
}

float WrapYawToFloat(double yaw) noexcept {
  constexpr float kFloatPi = 3.14159265358979323846f;
  const float wrapped = static_cast<float>(WrapYaw(yaw));
  // Rounding a double just below +pi can produce the float above +pi. Keep the
  // stored angle in the documented half-open interval after conversion.
  return wrapped >= kFloatPi ? -kFloatPi : wrapped;
}

}  // namespace

OrbitCamera::OrbitCamera(OrbitVec3 target) noexcept { (void)SetTarget(target); }

bool OrbitCamera::SetTarget(OrbitVec3 target) noexcept {
  if (!IsFinite(target)) return false;
  target_ = target;
  return true;
}

bool OrbitCamera::SetAngles(float yaw_radians, float pitch_radians) noexcept {
  if (!std::isfinite(yaw_radians) || !std::isfinite(pitch_radians)) return false;
  yaw_radians_ = WrapYawToFloat(static_cast<double>(yaw_radians));
  pitch_radians_ = std::clamp(pitch_radians, -kMaximumPitchRadians,
                              kMaximumPitchRadians);
  return true;
}

bool OrbitCamera::SetDistance(float distance) noexcept {
  if (!std::isfinite(distance) || distance <= 0.0f) return false;
  distance_ = std::clamp(distance, kMinimumDistance, kMaximumDistance);
  return true;
}

bool OrbitCamera::Orbit(float yaw_delta_radians,
                        float pitch_delta_radians) noexcept {
  if (!std::isfinite(yaw_delta_radians) ||
      !std::isfinite(pitch_delta_radians)) {
    return false;
  }

  const double next_yaw = static_cast<double>(yaw_radians_) +
                          static_cast<double>(yaw_delta_radians);
  const double next_pitch = static_cast<double>(pitch_radians_) +
                            static_cast<double>(pitch_delta_radians);
  yaw_radians_ = WrapYawToFloat(next_yaw);
  pitch_radians_ = static_cast<float>(std::clamp(
      next_pitch, -static_cast<double>(kMaximumPitchRadians),
      static_cast<double>(kMaximumPitchRadians)));
  return true;
}

bool OrbitCamera::Zoom(float wheel_steps) noexcept {
  if (!std::isfinite(wheel_steps)) return false;

  const double log_distance = std::log(static_cast<double>(distance_)) -
      static_cast<double>(wheel_steps) * static_cast<double>(kZoomLogRate);
  const double min_log = std::log(static_cast<double>(kMinimumDistance));
  const double max_log = std::log(static_cast<double>(kMaximumDistance));
  const double clamped_log = std::clamp(log_distance, min_log, max_log);
  distance_ = static_cast<float>(std::exp(clamped_log));
  return true;
}

OrbitCameraPose OrbitCamera::Pose() const noexcept {
  const double yaw = static_cast<double>(yaw_radians_);
  const double pitch = static_cast<double>(pitch_radians_);
  const double radius = static_cast<double>(distance_);
  const double horizontal = radius * std::cos(pitch);

  OrbitCameraPose pose;
  pose.target = target_;
  pose.eye = OrbitVec3{
      static_cast<float>(static_cast<double>(target_.x) +
                         horizontal * std::sin(yaw)),
      static_cast<float>(static_cast<double>(target_.y) +
                         radius * std::sin(pitch)),
      static_cast<float>(static_cast<double>(target_.z) -
                         horizontal * std::cos(yaw))};
  pose.up = OrbitVec3{0.0f, 1.0f, 0.0f};
  return pose;
}

}  // namespace mu::render
