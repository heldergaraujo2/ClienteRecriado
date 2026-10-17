// SPDX-License-Identifier: MIT
// Platform-independent frame orchestration; no Win32 or graphics headers.
#ifndef MU_PLATFORM_MAIN_LOOP_HPP
#define MU_PLATFORM_MAIN_LOOP_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mu {
namespace platform {

struct FrameBudget {
  double target_fps = 60.0;
  double target_frame_ms = 1000.0 / 60.0;
  double update_ms = 0.0;
  double render_ms = 0.0;
  double present_ms = 0.0;
  double total_ms = 0.0;
  double slack_ms = 1000.0 / 60.0;
  std::size_t visible_objects = 0;
  std::size_t culled_objects = 0;
  std::size_t draw_calls_prepared = 0;
  std::size_t triangles_submitted = 0;
  // Valid means all hooks ran; these durations are not full-frame, FPS, or GPU
  // measurements and exclude event waiting and frame pacing.
  bool timing_valid = false;
  bool over_budget = false;
  std::uint64_t frame_index = 0;
};

struct InputState {
  bool quit_requested = false;
};

struct UpdateResult {
  std::size_t visible_objects = 0;
  std::size_t culled_objects = 0;
};

struct RenderResult {
  std::size_t draw_calls_prepared = 0;
  std::size_t triangles_submitted = 0;
};

// Synchronous hooks supplied by the future application. user_data is owned by
// the caller and must remain valid while the callbacks are installed.
struct MainLoopCallbacks {
  using UpdateCallback = UpdateResult (*)(const InputState&, void* user_data);
  using RenderCallback = RenderResult (*)(const UpdateResult&, void* user_data);
  using PresentCallback = void (*)(void* user_data);

  UpdateCallback update = nullptr;
  RenderCallback render = nullptr;
  PresentCallback present = nullptr;
  void* user_data = nullptr;
};

// Calls the supplied update -> render -> present hooks and records their wall
// times. A timing sample is valid only when all three hooks are installed; the
// loop never fabricates workload, object counts, draw calls, or elapsed time.
class MainLoop {
public:
  explicit MainLoop(double target_fps = 60.0);

  // Install or replace hooks between frames. Clearing history prevents samples
  // from a previous callback set from influencing budget prediction.
  void SetCallbacks(MainLoopCallbacks callbacks);

  // Invoke one frame synchronously in update -> render -> present order.
  FrameBudget Tick(const InputState& input);

  // Predict from at least ten valid samples in the most recent thirty frames.
  bool PredictOverBudget() const;

  const std::vector<FrameBudget>& History() const { return history_; }

  // Invalid, non-positive, or unrepresentable rates fall back to 60 FPS.
  void SetTargetFps(double fps);

private:
  double target_fps_;
  double target_frame_ms_;
  std::uint64_t frame_index_;
  MainLoopCallbacks callbacks_;
  std::vector<FrameBudget> history_;
  static constexpr std::size_t kHistorySize = 120;

  UpdateResult DoUpdate(const InputState& input) const;
  RenderResult DoRender(const UpdateResult& update) const;
  void DoPresent() const;
};

}  // namespace platform
}  // namespace mu

#endif  // MU_PLATFORM_MAIN_LOOP_HPP
