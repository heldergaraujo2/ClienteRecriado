# Renderer API spike — first Windows/GPU vertical slice

## Purpose and scope

This spike proves only that a real Windows process can create a Win32 window, select a non-software DXGI adapter, create a D3D12 device/swap chain, submit a triangle to the GPU, present frames, and shut down cleanly. It is deliberately separate from the asset library and is **not** a playable client or a performance benchmark.

## Criteria-based comparison

| Criterion | Direct3D 12 | Vulkan | Direct3D 11 | OpenGL 3.x+ |
|---|---|---|---|---|
| Primary Windows fit | Native Windows API and SDK | Supported by many vendors on Windows, with loader/SDK | Native Windows API and SDK | Driver-provided implementation |
| Explicit control / heavy-scene headroom | High; explicit queues, command lists, descriptors and resource lifetime | High; explicit low-level control and multi-vendor ecosystem | Moderate; runtime manages more synchronization/resource details | More driver-managed |
| CPU submission scaling | Designed to reduce CPU overhead when used correctly | Designed for explicit/multithreaded command generation | Simpler, but can carry more driver/runtime overhead in CPU-heavy submission | Depends heavily on driver and implementation |
| Implementation complexity | Highest in this shortlist | High | Lowest of these modern choices | Low to medium |
| Debugging/validation | D3D12 debug layer and GPU-based validation | Khronos validation layers | Mature debug layer/tooling | Driver-dependent diagnostics |
| Portability beyond Windows | No | Yes | No | Yes, with implementation differences |
| Decision for this project | **First prototype** | Conditional candidate | **Comparison baseline** after D3D12 works | Not first path; retain only if evidence supports it |

This table is a design comparison, not a benchmark. None of these APIs guarantees higher FPS by itself. D3D12 and Vulkan shift more synchronization and resource-lifetime responsibility to the application; D3D11 is simpler and helps distinguish API complexity from renderer/asset problems. Official documentation:
- [Direct3D 12 programming guide](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [Direct3D 12 changes from Direct3D 11](https://learn.microsoft.com/en-us/windows/win32/direct3d12/important-changes-from-directx-11-to-directx-12)
- [Direct3D 11 graphics](https://learn.microsoft.com/en-us/windows/win32/direct3d11/)
- [Vulkan specification introduction](https://docs.vulkan.org/spec/latest/chapters/introduction.html)

## Build on Windows

1. Install Visual Studio Build Tools or Visual Studio with **Desktop development with C++** and a Windows 10/11 SDK. Do not download random DLLs.
2. Open **Developer PowerShell for Visual Studio** so cl.exe and SDK libraries are on PATH.
3. From the repository root run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_d3d12_smoke.ps1
```

4. If build succeeds, run:

```powershell
.\build\d3d12_triangle.exe
```

A window should show a colored triangle. The console reports the selected adapter name and dedicated-memory value. Close the window normally. If compilation fails, save the complete first error and toolchain versions; do not claim the test passed.

## What the smoke test proves—and does not

If it builds and runs successfully, it proves basic Win32 window creation, D3D12 device/swap-chain creation, GPU draw submission and presentation on that adapter. It does **not** prove BMD decoding, texture/material/UV correctness, gameplay, network compatibility, frame-rate gains, or that D3D12 is the final API choice. The single-allocator frame loop waits for correctness and simplicity, so it must not be used as a performance benchmark.

## Evidence checklist for the target PC

Save output in docs/evidence/d3d12_smoke_<date>.log and D3D11 output in docs/evidence/d3d11_smoke_<date>.log (do not commit proprietary assets or secrets):
- Windows edition/build and architecture;
- MSVC version and Windows SDK version;
- adapter name, dedicated-memory report, driver version/date and D3D feature level;
- whether the debug layer was available/enabled;
- build exit code and complete compiler output;
- successful window/triangle observation, present-loop duration and clean shutdown;
- any device-removal/debug-layer errors.

After the D3D12 smoke test passed, an isolated D3D11 triangle baseline was added at `platform/windows/d3d11_triangle.cpp`, built by `scripts/build_d3d11_smoke.ps1`. Both tests have now run on the same target PC (AMD Radeon RX 6600): the triangle was visibly rendered and each executable reported clean shutdown. Logs are stored under `docs/evidence/`. The D3D11 result also reports feature level `0xB100`.

The two smoke tests validate basic API/device/window/present paths only. They are not representative game FPS tests and do not establish that one API is faster. The production API decision remains open. For a meaningful comparison, use a real, semantically verified asset scene, equivalent build/settings, controlled resolution and present behavior, repeatable runs, and recorded median/p95/p99 frame times where feasible.


## Target-PC D3D12 result (2026-10-13)

The user ran the existing D3D12 executable on an AMD Radeon RX 6600 and visually confirmed the triangle. Console output reported 8147 MiB dedicated video memory via DXGI, debug layer unavailable/disabled, and `D3D12 smoke test: clean shutdown.` after closing the window normally. This confirms the isolated smoke path only. No FPS, frame-time distribution, original-asset rendering, or production API choice is established. D3D11 baseline build/run remains NOT EXECUTED.


## Target-PC D3D11 result (2026-10-13)

The user built and ran the isolated D3D11 smoke executable on an AMD Radeon RX 6600. The triangle was visually confirmed and normal window close printed `D3D11 smoke test: clean shutdown.`. Console output reported feature level `0xB100` and 8147 MiB dedicated-memory metadata from DXGI (not measured runtime VRAM). Evidence: `docs/evidence/d3d11_smoke_20261013.log`. D3D12 also passed the isolated triangle/clean-shutdown check on this PC. Neither test establishes original-asset correctness, playable-client status, FPS, or a final production API choice.
