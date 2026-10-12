// SPDX-License-Identifier: MIT
// E2 — Esqueleto main loop update/render/present com predição frame budget, sem GL
// Não chama OpenGL, não inclui <windows.h>, puro CPU
#ifndef MU_PLATFORM_MAIN_LOOP_HPP
#define MU_PLATFORM_MAIN_LOOP_HPP

#include <cstdint>
#include <vector>
#include <string>

namespace mu {
namespace platform {

// Frame timing
struct FrameBudget {
  double target_fps = 60.0; // alvo
  double target_frame_ms = 16.666; // 1000/target_fps
  double update_ms = 0.0; // medido update
  double render_ms = 0.0; // medido render (CPU side, sem GPU)
  double present_ms = 0.0; // medido present (SwapBuffers)
  double total_ms = 0.0;
  double slack_ms = 0.0; // target - total
  bool over_budget = false;
  uint64_t frame_index = 0;
};

// Input state (simplificado)
struct InputState {
  bool quit_requested = false;
  // teclas, mouse etc — placeholder
};

// Update: lógica jogo, física, animação, culling
// Retorna tempo gasto ms (medido via high-res clock em implementação real)
struct UpdateResult {
  double elapsed_ms = 0.0;
  size_t visible_objects = 0;
  size_t culled_objects = 0;
};

// Render: prepara draw calls (CPU side, sem GL)
// Retorna tempo gasto ms + número draw calls preparadas
struct RenderResult {
  double elapsed_ms = 0.0;
  size_t draw_calls_prepared = 0;
  size_t triangles_submitted = 0;
};

// Present: SwapBuffers + timing
struct PresentResult {
  double elapsed_ms = 0.0;
};

// Main loop esqueleto — separa update/render/present, prediz budget
class MainLoop {
public:
  explicit MainLoop(double target_fps = 60.0);

  // Chamado uma vez por frame, na ordem update -> render -> present
  // Sem GL, sem Windows — puro CPU, mede tempos via std::chrono em implementação real
  FrameBudget Tick(const InputState& input);

  // Prediz se próximo frame estourará budget baseado em média móvel últimos N frames
  bool PredictOverBudget() const;

  // Histórico para análise
  const std::vector<FrameBudget>& History() const { return history_; }

  // Config
  void SetTargetFps(double fps);

private:
  double target_fps_;
  double target_frame_ms_;
  uint64_t frame_index_;
  std::vector<FrameBudget> history_;
  static constexpr size_t kHistorySize = 120; // 2s a 60fps

  UpdateResult DoUpdate(const InputState& input);
  RenderResult DoRender(const UpdateResult& upd);
  PresentResult DoPresent();
};

} // namespace platform
} // namespace mu

#endif // MU_PLATFORM_MAIN_LOOP_HPP
