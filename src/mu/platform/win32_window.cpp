// SPDX-License-Identifier: MIT
// D5 — Esqueleto implementação BLOCKED
// Este arquivo NÃO compila em Windows ainda — é esqueleto documentado, retorna BLOCKED neste ambiente
// Quando portar para Windows, descomentar includes e implementar sequência real

#include "mu/platform/win32_window.hpp"

// NÃO incluir <windows.h> aqui para não quebrar build Linux
// Em Windows real:
// #include <windows.h>
// #include <GL/gl.h>
// #pragma comment(lib, "opengl32.lib")

namespace mu {
namespace platform {

bool CreateGLWindow(const Win32GLConfig& /*cfg*/, Win32GLWindow* out) {
  // Sequência real Windows (documentada em header):
  // RegisterClassExW(CS_OWNDC)
  // CreateWindowExW
  // GetDC
  // ChoosePixelFormat
  // SetPixelFormat (1× por janela)
  // wglCreateContext
  // wglMakeCurrent
  // Neste ambiente Linux: BLOCKED
  if (out) out->created = false;
  return false; // BLOCKED
}

void DestroyGLWindow(Win32GLWindow* win) {
  // Sequência real:
  // wglMakeCurrent(nullptr,nullptr)
  // wglDeleteContext
  // ReleaseDC
  // DestroyWindow
  // UnregisterClass
  if (win) win->created = false;
}

bool SwapGLWindow(Win32GLWindow* /*win*/) {
  // SwapBuffers(hdc)
  return false; // BLOCKED
}

bool PollEvents(Win32GLWindow* /*win*/) {
  // PeekMessage/TranslateMessage/DispatchMessage
  // Retorna false se WM_QUIT
  return true; // BLOCKED mas não quit
}

Win32GLStatus GetWin32GLStatus() {
  // Neste ambiente: sem Windows SDK, sem GPU, sem display
  return Win32GLStatus::kBlockedNoWindowsSDK;
}

} // namespace platform
} // namespace mu
