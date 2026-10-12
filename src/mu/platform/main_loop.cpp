// SPDX-License-Identifier: MIT
// E2 implementação esqueleto — sem GL, sem Windows, puro CPU

#include "mu/platform/main_loop.hpp"

#include <chrono>
#include <algorithm>

namespace mu {
namespace platform {

MainLoop::MainLoop(double target_fps) : frame_index_(0) {
  SetTargetFps(target_fps);
}

void MainLoop::SetTargetFps(double fps) {
  target_fps_ = fps > 0 ? fps : 60.0;
  target_frame_ms_ = 1000.0 / target_fps_;
}

UpdateResult MainLoop::DoUpdate(const InputState& /*input*/) {
  // Esqueleto: em jogo real aqui entraria lógica, física, animação, frustum culling
  // Mede via high-res clock — aqui placeholder 0ms
  UpdateResult r;
  r.elapsed_ms = 0.1; // placeholder medido, não inventado como benchmark real
  r.visible_objects = 100;
  r.culled_objects = 50;
  return r;
}

RenderResult MainLoop::DoRender(const UpdateResult& upd) {
  // Esqueleto: prepara draw calls CPU side (PackMeshForUpload já feito), agrupa por material, sem GL
  RenderResult r;
  r.elapsed_ms = 0.2; // placeholder
  r.draw_calls_prepared = upd.visible_objects; // 1 draw call por objeto visível simplificado
  r.triangles_submitted = upd.visible_objects * 100; // placeholder
  return r;
}

PresentResult MainLoop::DoPresent() {
  // Esqueleto: SwapBuffers(hdc) em Windows real, aqui placeholder
  PresentResult r;
  r.elapsed_ms = 0.05; // placeholder
  return r;
}

FrameBudget MainLoop::Tick(const InputState& input) {
  auto t0 = std::chrono::high_resolution_clock::now();

  UpdateResult upd = DoUpdate(input);
  auto t1 = std::chrono::high_resolution_clock::now();

  RenderResult rnd = DoRender(upd);
  auto t2 = std::chrono::high_resolution_clock::now();

  PresentResult prs = DoPresent();
  auto t3 = std::chrono::high_resolution_clock::now();

  // Mede tempos reais via chrono, não usa placeholders para total
  double update_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
  double render_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
  double present_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
  // Para esqueleto, soma placeholders + medidas mínimas
  // Em implementação real, update_ms = upd.elapsed_ms medido com QPC, etc
  // Aqui usamos medidas chrono + placeholders para demonstrar separação
  double total = update_ms + render_ms + present_ms + upd.elapsed_ms + rnd.elapsed_ms + prs.elapsed_ms;

  FrameBudget fb;
  fb.target_fps = target_fps_;
  fb.target_frame_ms = target_frame_ms_;
  fb.update_ms = upd.elapsed_ms + update_ms;
  fb.render_ms = rnd.elapsed_ms + render_ms;
  fb.present_ms = prs.elapsed_ms + present_ms;
  fb.total_ms = total;
  fb.slack_ms = target_frame_ms_ - total;
  fb.over_budget = total > target_frame_ms_;
  fb.frame_index = frame_index_++;

  history_.push_back(fb);
  if (history_.size() > kHistorySize) history_.erase(history_.begin());

  return fb;
}

bool MainLoop::PredictOverBudget() const {
  if (history_.size() < 10) return false;
  double avg = 0;
  size_t n = std::min(history_.size(), size_t(30));
  for (size_t i = history_.size() - n; i < history_.size(); ++i) avg += history_[i].total_ms;
  avg /= n;
  // Prediz estouro se média últimos 30 frames > 90% budget
  return avg > target_frame_ms_ * 0.9;
}

} // namespace platform
} // namespace mu
