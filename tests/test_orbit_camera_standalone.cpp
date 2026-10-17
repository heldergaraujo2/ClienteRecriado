// SPDX-License-Identifier: MIT
// Standalone, asset-free camera tests. Build directly to /tmp so this test does
// not touch the repository's active build/ executables or parser test binary.
#include "mu/render/orbit_camera.hpp"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>

namespace {

constexpr float kPi = 3.14159265358979323846f;
std::size_t g_checks = 0;
std::size_t g_failures = 0;

void Check(bool condition, const char* label) {
  ++g_checks;
  if (condition) return;
  ++g_failures;
  std::fprintf(stderr, "FAIL: %s\n", label);
}

void CheckNear(float actual, float expected, float tolerance, const char* label) {
  Check(std::isfinite(actual) && std::fabs(actual - expected) <= tolerance,
        label);
}

void CheckFinite(mu::render::OrbitCameraPose pose, const char* label) {
  Check(std::isfinite(pose.eye.x) && std::isfinite(pose.eye.y) &&
            std::isfinite(pose.eye.z) && std::isfinite(pose.target.x) &&
            std::isfinite(pose.target.y) && std::isfinite(pose.target.z),
        label);
}

void TestDefaultPose() {
  const mu::render::OrbitCamera camera;
  const auto pose = camera.Pose();
  CheckNear(camera.distance(), 3.0f, 1.0e-6f, "default distance");
  CheckNear(pose.eye.x, 0.0f, 1.0e-6f, "default eye x");
  CheckNear(pose.eye.y, 0.0f, 1.0e-6f, "default eye y");
  CheckNear(pose.eye.z, -3.0f, 1.0e-6f, "default eye z");
  CheckNear(pose.target.x, 0.0f, 1.0e-6f, "default target x");
  CheckNear(pose.up.y, 1.0f, 1.0e-6f, "world-up y");
}

void TestYawAndFullRevolution() {
  mu::render::OrbitCamera camera;
  Check(camera.SetAngles(kPi * 0.5f, 0.0f), "set quarter-turn yaw");
  auto pose = camera.Pose();
  CheckNear(pose.eye.x, 3.0f, 2.0e-6f, "quarter-turn eye x");
  CheckNear(pose.eye.z, 0.0f, 2.0e-6f, "quarter-turn eye z");

  Check(camera.SetAngles(-kPi + 0.25f, 0.0f), "set near-wrap yaw");
  const auto before = camera.Pose();
  Check(camera.Orbit(2.0f * kPi, 0.0f), "orbit one full revolution");
  pose = camera.Pose();
  CheckNear(camera.yaw_radians(), -kPi + 0.25f, 2.0e-6f,
            "yaw wraps after 360 degrees");
  CheckNear(pose.eye.x, before.eye.x, 2.0e-6f, "full-turn eye x");
  CheckNear(pose.eye.z, before.eye.z, 2.0e-6f, "full-turn eye z");

  Check(camera.SetAngles(kPi, 0.0f), "set positive-pi boundary yaw");
  Check(camera.yaw_radians() < kPi, "yaw stays below positive-pi boundary");
  CheckNear(camera.yaw_radians(), -kPi, 3.0e-7f,
            "positive-pi boundary wraps to negative-pi side");
}

void TestPitchClampAndOrbit() {
  mu::render::OrbitCamera camera;
  Check(camera.SetAngles(0.0f, 4.0f), "set over-limit positive pitch");
  CheckNear(camera.pitch_radians(), mu::render::OrbitCamera::kMaximumPitchRadians,
            1.0e-6f, "positive pitch is clamped");
  Check(camera.Orbit(0.0f, -8.0f), "orbit over negative pitch limit");
  CheckNear(camera.pitch_radians(), -mu::render::OrbitCamera::kMaximumPitchRadians,
            1.0e-6f, "negative pitch is clamped");
  CheckFinite(camera.Pose(), "clamped pose remains finite");
}

void TestZoomAndDistanceLimits() {
  mu::render::OrbitCamera camera;
  Check(camera.Zoom(1.0f), "zoom in one wheel step");
  Check(camera.distance() < mu::render::OrbitCamera::kDefaultDistance,
        "positive wheel steps zoom in");
  Check(camera.Zoom(-1.0f), "zoom back one wheel step");
  CheckNear(camera.distance(), mu::render::OrbitCamera::kDefaultDistance,
            2.0e-6f, "opposite wheel steps reverse zoom");

  Check(camera.Zoom(1.0e30f), "large zoom-in input is bounded");
  CheckNear(camera.distance(), mu::render::OrbitCamera::kMinimumDistance,
            1.0e-6f, "zoom-in clamps to minimum distance");
  Check(camera.Zoom(-1.0e30f), "large zoom-out input is bounded");
  CheckNear(camera.distance(), mu::render::OrbitCamera::kMaximumDistance,
            1.0e-2f, "zoom-out clamps to maximum distance");

  Check(camera.SetDistance(0.001f), "small positive distance accepted");
  CheckNear(camera.distance(), mu::render::OrbitCamera::kMinimumDistance,
            1.0e-6f, "distance setter clamps to minimum");
  Check(camera.SetDistance(200000.0f), "large positive distance accepted");
  CheckNear(camera.distance(), mu::render::OrbitCamera::kMaximumDistance,
            1.0e-2f, "distance setter clamps to maximum");
}

void TestTargetTranslation() {
  mu::render::OrbitCamera camera;
  Check(camera.SetAngles(0.7f, 0.2f), "set target test angles");
  const auto before = camera.Pose();
  Check(camera.SetTarget(mu::render::OrbitVec3{1.0f, -2.0f, 4.0f}),
        "set finite target");
  const auto after = camera.Pose();
  CheckNear(after.target.x, 1.0f, 1.0e-6f, "target x");
  CheckNear(after.target.y, -2.0f, 1.0e-6f, "target y");
  CheckNear(after.target.z, 4.0f, 1.0e-6f, "target z");
  CheckNear(after.eye.x - before.eye.x, 1.0f, 2.0e-6f,
            "eye follows target x");
  CheckNear(after.eye.y - before.eye.y, -2.0f, 2.0e-6f,
            "eye follows target y");
  CheckNear(after.eye.z - before.eye.z, 4.0f, 2.0e-6f,
            "eye follows target z");
}

void TestInvalidInputsDoNotMutateCamera() {
  mu::render::OrbitCamera camera(mu::render::OrbitVec3{2.0f, 3.0f, 4.0f});
  Check(camera.SetAngles(0.3f, -0.2f), "initialize invalid-input test");
  Check(camera.SetDistance(5.0f), "initialize test distance");
  const auto before = camera.Pose();
  const float yaw_before = camera.yaw_radians();
  const float pitch_before = camera.pitch_radians();
  const float distance_before = camera.distance();
  const float inf = std::numeric_limits<float>::infinity();
  const float nan = std::numeric_limits<float>::quiet_NaN();

  Check(!camera.SetTarget(mu::render::OrbitVec3{inf, 0.0f, 0.0f}),
        "reject non-finite target");
  Check(!camera.SetAngles(nan, 0.0f), "reject non-finite angle");
  Check(!camera.SetDistance(inf), "reject non-finite distance");
  Check(!camera.SetDistance(0.0f), "reject zero distance");
  Check(!camera.SetDistance(-1.0f), "reject negative distance");
  Check(!camera.Orbit(inf, 0.0f), "reject non-finite orbit delta");
  Check(!camera.Zoom(nan), "reject non-finite zoom input");
  CheckNear(camera.yaw_radians(), yaw_before, 1.0e-7f,
            "invalid input preserves yaw");
  CheckNear(camera.pitch_radians(), pitch_before, 1.0e-7f,
            "invalid input preserves pitch");
  CheckNear(camera.distance(), distance_before, 1.0e-7f,
            "invalid input preserves distance");
  const auto after = camera.Pose();
  CheckNear(after.eye.x, before.eye.x, 1.0e-6f,
            "invalid input preserves eye x");
  CheckNear(after.eye.y, before.eye.y, 1.0e-6f,
            "invalid input preserves eye y");
  CheckNear(after.eye.z, before.eye.z, 1.0e-6f,
            "invalid input preserves eye z");
}

}  // namespace

int main() {
  std::puts("OrbitCamera standalone tests (no assets or D3D11 dependencies)");
  TestDefaultPose();
  TestYawAndFullRevolution();
  TestPitchClampAndOrbit();
  TestZoomAndDistanceLimits();
  TestTargetTranslation();
  TestInvalidInputsDoNotMutateCamera();
  std::printf("%zu checks, %zu failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
