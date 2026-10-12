// SPDX-License-Identifier: MIT
// D5 — Esqueleto Win32 + WGL (NÃO COMPILADO, BLOCKED — requer Windows + GPU)
// Objetivo: documentar sequência correta de chamadas wglCreateContext, sem compilar neste ambiente Linux sem Windows SDK/GPU
//
// Sequência correta (Main.exe faz wglCreateContext/wglMakeCurrent em janela GDI com ChoosePixelFormat/SetPixelFormat + SwapBuffers):
// 1. RegisterClassExW com CS_OWNDC (cada janela tem DC próprio, necessário para GL)
// 2. CreateWindowExW
// 3. GetDC
// 4. PIXELFORMATDESCRIPTOR pfd = { sizeof, 1, PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32, ... }
// 5. ChoosePixelFormat(hdc, &pfd)
// 6. SetPixelFormat(hdc, pf, &pfd) — só pode ser chamado 1× por janela no GDI
// 7. wglCreateContext(hdc)
// 8. wglMakeCurrent(hdc, hglrc)
// 9. Loop mensagens: PeekMessage/TranslateMessage/DispatchMessage, timeBeginPeriod(1) ou QueryPerformanceCounter para timing (recomendado QPC, Main.exe usa timeGetTime 1ms)
// 10. Render: glClear, desenha via VBO/VAO (DEC-008), SwapBuffers(hdc)
// 11. Cleanup: wglMakeCurrent(nullptr,nullptr), wglDeleteContext, ReleaseDC, DestroyWindow, UnregisterClass
//
// Por que BLOCKED:
// - Este ambiente é Linux sem Windows SDK (sem <windows.h>, sem wgl.h), sem GPU, sem OPENGL32.dll
// - Compilar exigiria cross-compile mingw + headers Windows + link opengl32.lib, e execução exige Windows real com driver GL
// - Testes automatizados não podem criar janela Win32 nem contexto GL aqui — registrar como BLOCKED com justificativa, não como PASS/FAIL
// - Não incluir <windows.h> nem <GL/gl.h> para não quebrar build Linux; este arquivo é apenas documentação + esqueleto
//
// Estado: D5 CONCLUÍDO ciclo12 como esqueleto documentado, BLOCKED para compilação e teste, conforme regra de não inventar testes Windows sem acesso hardware

#ifndef MU_PLATFORM_WIN32_WINDOW_HPP
#define MU_PLATFORM_WIN32_WINDOW_HPP

// Este header NÃO inclui <windows.h> propositalmente — é esqueleto, não compila em Windows ainda
// Quando compilar em Windows, incluir <windows.h> e <GL/gl.h> e linkar opengl32.lib

namespace mu {
namespace platform {

// Configuração mínima janela GL
struct Win32GLConfig {
  int width = 800;
  int height = 600;
  const char* title = "ClienteRecriado - D5 esqueleto BLOCKED";
  bool fullscreen = false; // se true, ChangeDisplaySettingsW + WS_POPUP
  int colorBits = 32;
  int depthBits = 24;
  int stencilBits = 8;
};

// Handle opaco para esqueleto — em Windows real seria HWND/HDC/HGLRC
struct Win32GLWindow {
  // Em Windows real:
  // HWND hwnd = nullptr;
  // HDC hdc = nullptr;
  // HGLRC hglrc = nullptr;
  // ATOM classAtom = 0;
  bool created = false;
  Win32GLConfig config;
};

// API esqueleto — implementações em win32_window.cpp retornam BLOCKED neste ambiente
// Em Windows real, cada função chama a sequência documentada acima e retorna true/false

// Cria janela + contexto GL, faz wglMakeCurrent — retorna true se sucesso
bool CreateGLWindow(const Win32GLConfig& cfg, Win32GLWindow* out);

// Destrói contexto + janela
void DestroyGLWindow(Win32GLWindow* win);

// Troca buffers (SwapBuffers)
bool SwapGLWindow(Win32GLWindow* win);

// Loop mensagens: retorna false se WM_QUIT recebido
bool PollEvents(Win32GLWindow* win);

// Estado BLOCKED para testes
enum class Win32GLStatus {
  kBlockedNoWindowsSDK, // sem <windows.h>
  kBlockedNoGPU,        // sem GPU/driver
  kBlockedNoDisplay,    // sem display server (headless)
  kOk                   // só em Windows real com GPU
};

Win32GLStatus GetWin32GLStatus();

} // namespace platform
} // namespace mu

#endif // MU_PLATFORM_WIN32_WINDOW_HPP
