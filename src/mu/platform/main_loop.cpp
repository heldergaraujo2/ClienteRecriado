// SPDX-License-Identifier: MIT
#include "mu/platform/main_loop.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace mu {
namespace platform {
namespace {

using SteadyClock = std::chrono::steady_clock;

[[nodiscard]] double Milliseconds(SteadyClock::duration duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

}  // namespace

MainLoop::MainLoop(double target_fps)
    : target_fps_(60.0),
      target_frame_ms_(1000.0 / 60.0),
      frame_index_(0),
      callbacks_{},
      history_{} {
  SetTargetFps(target_fps);
}

void MainLoop::SetCallbacks(MainLoopCallbacks callbacks) {
  callbacks_ = callbacks;
  history_.clear();
}

void MainLoop::SetTargetFps(double fps) {
  double next_fps = fps;
  if (!std::isfinite(next_fps) || next_fps <= 0.0) {
    next_fps = 60.0;
  }

  double next_frame_ms = 1000.0 / next_fps;
  if (!std::isfinite(next_frame_ms) || next_frame_ms <= 0.0) {
    next_fps = 60.0;
    next_frame_ms = 1000.0 / next_fps;
  }

  if (next_fps != target_fps_) {
    history_.clear();
  }
  target_fps_ = next_fps;
  target_frame_ms_ = next_frame_ms;
}

UpdateResult MainLoop::DoUpdate(const InputState& input) const {
  if (callbacks_.update == nullptr) {
    return {};
  }
  return callbacks_.update(input, callbacks_.user_data);
}

RenderResult MainLoop::DoRender(const UpdateResult& update) const {
  if (callbacks_.render == nullptr) {
    return {};
  }
  return callbacks_.render(update, callbacks_.user_data);
}

void MainLoop::DoPresent() const {
  if (callbacks_.present != nullptr) {
    callbacks_.present(callbacks_.user_data);
  }
}

FrameBudget MainLoop::Tick(const InputState& input) {
  UpdateResult update{};
  RenderResult render{};
  double update_ms = 0.0;
  double render_ms = 0.0;
  double present_ms = 0.0;

  if (callbacks_.update != nullptr) {
    const auto begin = SteadyClock::now();
    update = DoUpdate(input);
    update_ms = Milliseconds(SteadyClock::now() - begin);
  }

  if (callbacks_.render != nullptr) {
    const auto begin = SteadyClock::now();
    render = DoRender(update);
    render_ms = Milliseconds(SteadyClock::now() - begin);
  }

  if (callbacks_.present != nullptr) {
    const auto begin = SteadyClock::now();
    DoPresent();
    present_ms = Milliseconds(SteadyClock::now() - begin);
  }

  FrameBudget frame{};
  frame.target_fps = target_fps_;
  frame.target_frame_ms = target_frame_ms_;
  frame.update_ms = update_ms;
  frame.render_ms = render_ms;
  frame.present_ms = present_ms;
  frame.total_ms = update_ms + render_ms + present_ms;
  frame.slack_ms = target_frame_ms_ - frame.total_ms;
  frame.visible_objects = update.visible_objects;
  frame.culled_objects = update.culled_objects;
  frame.draw_calls_prepared = render.draw_calls_prepared;
  frame.triangles_submitted = render.triangles_submitted;
  frame.timing_valid = callbacks_.update != nullptr &&
                       callbacks_.render != nullptr &&
                       callbacks_.present != nullptr;
  frame.over_budget = frame.timing_valid &&
                      frame.total_ms > frame.target_frame_ms;
  frame.frame_index = frame_index_++;

  history_.push_back(frame);
  if (history_.size() > kHistorySize) {
    history_.erase(history_.begin());
  }

  return frame;
}

bool MainLoop::PredictOverBudget() const {
  const std::size_t lookback = std::min(history_.size(), std::size_t{30});
  const std::size_t first = history_.size() - lookback;
  double total_ms = 0.0;
  std::size_t valid_samples = 0;

  for (std::size_t index = first; index < history_.size(); ++index) {
    const FrameBudget& frame = history_[index];
    if (!frame.timing_valid || frame.target_fps != target_fps_) {
      continue;
    }
    total_ms += frame.total_ms;
    ++valid_samples;
  }

  if (valid_samples < 10) {
    return false;
  }

  const double average_ms = total_ms / static_cast<double>(valid_samples);
  return average_ms > target_frame_ms_ * 0.9;
}

}  // namespace platform
}  // namespace mu
