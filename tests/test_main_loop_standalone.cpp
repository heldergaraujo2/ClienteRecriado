// SPDX-License-Identifier: MIT
// Standalone tests for callback-driven frame orchestration. Build into /tmp so
// these tests do not replace the repository's existing test executables.
#include "mu/platform/main_loop.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <thread>

namespace {

std::size_t g_checks = 0;
std::size_t g_failures = 0;

void Check(bool condition, const char* label) {
  ++g_checks;
  if (condition) return;
  ++g_failures;
  std::fprintf(stderr, "FAIL: %s\n", label);
}

void CheckNear(double actual, double expected, double tolerance,
               const char* label) {
  Check(std::isfinite(actual) && std::fabs(actual - expected) <= tolerance,
        label);
}

struct CallbackState {
  char sequence[3]{};
  std::size_t sequence_size = 0;
  bool render_received_update = false;
  bool quit_requested_seen = false;
};

void Append(CallbackState* state, char value) {
  if (state->sequence_size < 3) {
    state->sequence[state->sequence_size] = value;
    ++state->sequence_size;
  }
}

mu::platform::UpdateResult Update(const mu::platform::InputState& input,
                                  void* user_data) {
  auto* state = static_cast<CallbackState*>(user_data);
  Append(state, 'U');
  state->quit_requested_seen = input.quit_requested;
  return {7, 2};
}

mu::platform::RenderResult Render(const mu::platform::UpdateResult& update,
                                  void* user_data) {
  auto* state = static_cast<CallbackState*>(user_data);
  Append(state, 'R');
  state->render_received_update = update.visible_objects == 7 &&
                                  update.culled_objects == 2;
  return {3, 42};
}

void Present(void* user_data) {
  auto* state = static_cast<CallbackState*>(user_data);
  Append(state, 'P');
}

mu::platform::UpdateResult SlowUpdate(const mu::platform::InputState&,
                                      void*) {
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  return {};
}

mu::platform::RenderResult EmptyRender(const mu::platform::UpdateResult&,
                                       void*) {
  return {};
}

void EmptyPresent(void*) {}

void TestEmptyLoopDoesNotInventWorkOrTiming() {
  mu::platform::MainLoop loop;
  const mu::platform::FrameBudget frame = loop.Tick({});

  Check(!frame.timing_valid, "empty loop has no valid workload sample");
  Check(!frame.over_budget, "empty loop is not labelled over budget");
  Check(frame.update_ms == 0.0, "empty update reports zero, not a placeholder");
  Check(frame.render_ms == 0.0, "empty render reports zero, not a placeholder");
  Check(frame.present_ms == 0.0, "empty present reports zero, not a placeholder");
  Check(frame.total_ms == 0.0, "empty loop total reports zero workload time");
  Check(frame.visible_objects == 0 && frame.culled_objects == 0,
        "empty loop invents no scene counts");
  Check(frame.draw_calls_prepared == 0 && frame.triangles_submitted == 0,
        "empty loop invents no render counts");
}

void TestCallbacksRunInOrderAndPassResults() {
  CallbackState state;
  mu::platform::MainLoopCallbacks callbacks;
  callbacks.update = &Update;
  callbacks.render = &Render;
  callbacks.present = &Present;
  callbacks.user_data = &state;

  mu::platform::MainLoop loop;
  loop.SetCallbacks(callbacks);
  const mu::platform::InputState input{true};
  const mu::platform::FrameBudget frame = loop.Tick(input);

  Check(state.sequence_size == 3, "all three callbacks ran");
  Check(state.sequence[0] == 'U' && state.sequence[1] == 'R' &&
            state.sequence[2] == 'P',
        "callbacks run update, render, present");
  Check(state.quit_requested_seen, "update receives input state");
  Check(state.render_received_update, "render receives update result");
  Check(frame.timing_valid, "complete callback pipeline has valid timings");
  Check(frame.visible_objects == 7 && frame.culled_objects == 2,
        "frame records update counts");
  Check(frame.draw_calls_prepared == 3 && frame.triangles_submitted == 42,
        "frame records render counts");
  Check(std::isfinite(frame.update_ms) && frame.update_ms >= 0.0,
        "update callback time is finite and non-negative");
  Check(std::isfinite(frame.render_ms) && frame.render_ms >= 0.0,
        "render callback time is finite and non-negative");
  Check(std::isfinite(frame.present_ms) && frame.present_ms >= 0.0,
        "present callback time is finite and non-negative");
  CheckNear(frame.total_ms,
            frame.update_ms + frame.render_ms + frame.present_ms, 0.0,
            "total is exactly the sum of callback times");
  Check(frame.frame_index == 0, "first callback frame index is zero");
}

void TestIncompletePipelineCannotClaimBudgetPrediction() {
  mu::platform::MainLoopCallbacks callbacks;
  callbacks.update = &SlowUpdate;

  mu::platform::MainLoop loop(1000000.0);
  loop.SetCallbacks(callbacks);
  for (std::size_t index = 0; index < 12; ++index) {
    const mu::platform::FrameBudget frame = loop.Tick({});
    Check(!frame.timing_valid, "partial pipeline sample is invalid");
    Check(!frame.over_budget, "partial pipeline is not labelled over budget");
  }
  Check(!loop.PredictOverBudget(),
        "prediction ignores incomplete callback pipeline samples");
}

void TestPredictionUsesMeasuredCompleteCallbacks() {
  mu::platform::MainLoopCallbacks callbacks;
  callbacks.update = &SlowUpdate;
  callbacks.render = &EmptyRender;
  callbacks.present = &EmptyPresent;

  mu::platform::MainLoop loop(1000000.0);
  loop.SetCallbacks(callbacks);
  for (std::size_t index = 0; index < 10; ++index) {
    const mu::platform::FrameBudget frame = loop.Tick({});
    Check(frame.timing_valid, "complete pipeline sample is valid");
    Check(frame.over_budget, "measured slow callback exceeds frame budget");
  }
  Check(loop.PredictOverBudget(),
        "prediction uses the latest complete callback measurements");
}

void TestRatesAndHistoryBounds() {
  mu::platform::MainLoop loop(std::numeric_limits<double>::quiet_NaN());
  Check(loop.History().empty(), "new loop history is empty");
  const mu::platform::FrameBudget nan_frame = loop.Tick({});
  Check(nan_frame.target_fps == 60.0, "NaN target falls back to 60 FPS");
  Check(loop.History().size() == 1, "tick is recorded in history");

  loop.SetTargetFps(144.0);
  Check(loop.History().empty(), "rate change clears stale prediction samples");
  loop.SetTargetFps(std::numeric_limits<double>::infinity());
  Check(loop.Tick({}).target_fps == 60.0,
        "infinite target falls back to 60 FPS");
  loop.SetTargetFps(0.0);
  Check(loop.Tick({}).target_fps == 60.0,
        "zero target falls back to 60 FPS");
  loop.SetTargetFps(-1.0);
  Check(loop.Tick({}).target_fps == 60.0,
        "negative target falls back to 60 FPS");
  loop.SetTargetFps(std::numeric_limits<double>::denorm_min());
  Check(loop.Tick({}).target_fps == 60.0,
        "unrepresentable frame period falls back to 60 FPS");

  loop.SetTargetFps(144.0);
  Check(loop.History().empty(), "valid rate change clears old samples");
  const mu::platform::FrameBudget frame = loop.Tick({});
  Check(frame.target_fps == 144.0, "valid target FPS is retained");
  Check(frame.target_frame_ms > 6.9 && frame.target_frame_ms < 7.0,
        "frame budget follows configured FPS");

  mu::platform::MainLoop history_loop;
  for (std::size_t index = 0; index < 130; ++index) {
    history_loop.Tick({});
  }
  Check(history_loop.History().size() == 120,
        "history is bounded to 120 frames");
  Check(history_loop.History().front().frame_index == 10,
        "oldest history entry is evicted");
  Check(history_loop.History().back().frame_index == 129,
        "newest history entry is retained");
}

void TestReplacingCallbacksClearsHistory() {
  mu::platform::MainLoop loop;
  loop.Tick({});
  Check(loop.History().size() == 1, "initial no-work frame is stored");

  mu::platform::MainLoopCallbacks callbacks;
  callbacks.update = &Update;
  loop.SetCallbacks(callbacks);
  Check(loop.History().empty(), "callback replacement clears old samples");
}

}  // namespace

int main() {
  std::puts("MainLoop standalone tests (no assets, platform or GPU required)");
  TestEmptyLoopDoesNotInventWorkOrTiming();
  TestCallbacksRunInOrderAndPassResults();
  TestIncompletePipelineCannotClaimBudgetPrediction();
  TestPredictionUsesMeasuredCompleteCallbacks();
  TestRatesAndHistoryBounds();
  TestReplacingCallbacksClearsHistory();
  std::printf("%zu checks, %zu failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
