// SPDX-License-Identifier: MIT
// Isolated D3D11 smoke test. Not the production renderer or a benchmark.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib")

using Microsoft::WRL::ComPtr;
namespace {
constexpr UINT W=1280, H=720;
struct Vertex { float p[3]; float c[4]; };
void Check(HRESULT hr, const char* what) {
  if (FAILED(hr)) {
    char msg[256]{};
    std::snprintf(msg,sizeof(msg),"%s failed (HRESULT=0x%08lX)",what,
                  static_cast<unsigned long>(hr));
    throw std::runtime_error(msg);
  }
}
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  if(m==WM_CLOSE){DestroyWindow(h);return 0;}
  if(m==WM_DESTROY){PostQuitMessage(0);return 0;}
  return DefWindowProcW(h,m,w,l);
}
class Renderer {
 public:
  void Init(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount=2; sd.BufferDesc.Width=W; sd.BufferDesc.Height=H;
    sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow=hwnd;
    sd.SampleDesc.Count=1; sd.Windowed=TRUE; sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    UINT flags=0;
#if defined(_DEBUG)
    flags|=D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0,
                                D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL selected{};
    HRESULT hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,
      flags,levels,static_cast<UINT>(sizeof(levels)/sizeof(levels[0])),
      D3D11_SDK_VERSION,&sd,&swap_,&device_,&selected,&context_);
    if(hr==E_INVALIDARG) {
      Check(D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,
        flags,levels+1,static_cast<UINT>(sizeof(levels)/sizeof(levels[0])-1),
        D3D11_SDK_VERSION,&sd,&swap_,&device_,&selected,&context_),
        "D3D11CreateDeviceAndSwapChain");
    } else Check(hr,"D3D11CreateDeviceAndSwapChain");
    ComPtr<IDXGIDevice> dxgiDevice; ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC desc{};
    Check(device_.As(&dxgiDevice),"Query IDXGIDevice");
    Check(dxgiDevice->GetAdapter(&adapter),"GetAdapter");
    Check(adapter->GetDesc(&desc),"GetDesc");
    std::wprintf(L"Adapter: %ls\n",desc.Description);
    std::wprintf(L"Dedicated video memory reported by DXGI: %llu MiB\n",
      static_cast<unsigned long long>(desc.DedicatedVideoMemory/(1024ULL*1024ULL)));
    std::wprintf(L"D3D feature level: 0x%X\n",static_cast<unsigned>(selected));
#if defined(_DEBUG)
    std::wprintf(L"D3D11 debug device requested. Availability depends on SDK layers.\n");
#else
    std::wprintf(L"D3D11 debug device: not requested (release smoke test).\n");
#endif
    ComPtr<ID3D11Texture2D> back;
    Check(swap_->GetBuffer(0,IID_PPV_ARGS(&back)),"GetBuffer");
    Check(device_->CreateRenderTargetView(back.Get(),nullptr,&rtv_),"CreateRenderTargetView");
    MakePipeline(); MakeVertices();
  }
  void Frame() {
    const float clear[4]={0.035f,0.045f,0.075f,1.0f};
    context_->OMSetRenderTargets(1,rtv_.GetAddressOf(),nullptr);
    context_->ClearRenderTargetView(rtv_.Get(),clear);
    D3D11_VIEWPORT vp{}; vp.Width=static_cast<float>(W); vp.Height=static_cast<float>(H);
    vp.MinDepth=0; vp.MaxDepth=1;
    context_->RSSetViewports(1,&vp);
    const UINT stride=sizeof(Vertex), offset=0;
    ID3D11Buffer* vb=vertices_.Get();
    context_->IASetInputLayout(layout_.Get());
    context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vs_.Get(),nullptr,0);
    context_->PSSetShader(ps_.Get(),nullptr,0);
    context_->Draw(3,0);
    Check(swap_->Present(1,0),"Present");
  }
  void Shutdown() {
    if(shutdown_) return;
    shutdown_=true;
    if(context_) context_->ClearState();
    if(swap_) swap_->SetFullscreenState(FALSE,nullptr);
    context_.Reset(); rtv_.Reset(); vertices_.Reset(); layout_.Reset();
    vs_.Reset(); ps_.Reset(); swap_.Reset(); device_.Reset();
    std::wprintf(L"D3D11 smoke test: clean shutdown.\n");
  }
  ~Renderer(){Shutdown();}
 private:
  void MakePipeline() {
    const char* shader=R"(
      struct VI { float3 p:POSITION; float4 c:COLOR; };
      struct VO { float4 p:SV_POSITION; float4 c:COLOR; };
      VO VSMain(VI v) { VO o; o.p=float4(v.p,1); o.c=v.c; return o; }
      float4 PSMain(VO v):SV_TARGET { return v.c; }
    )";
    ComPtr<ID3DBlob> vs,ps,err;
    HRESULT hr=D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,
      "VSMain","vs_4_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&vs,&err);
    if(FAILED(hr)){if(err)std::fprintf(stderr,"%s\n",static_cast<const char*>(err->GetBufferPointer()));Check(hr,"Compile VS");}
    err.Reset(); hr=D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,
      "PSMain","ps_4_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&ps,&err);
    if(FAILED(hr)){if(err)std::fprintf(stderr,"%s\n",static_cast<const char*>(err->GetBufferPointer()));Check(hr,"Compile PS");}
    D3D11_INPUT_ELEMENT_DESC il[]={
      {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
      {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
    Check(device_->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vs_),"CreateVertexShader");
    Check(device_->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&ps_),"CreatePixelShader");
    Check(device_->CreateInputLayout(il,2,vs->GetBufferPointer(),vs->GetBufferSize(),&layout_),"CreateInputLayout");
  }
  void MakeVertices() {
    const Vertex v[]={
      {{0.0f,0.55f,0.0f},{1.0f,0.18f,0.15f,1.0f}},
      {{0.55f,-0.45f,0.0f},{0.15f,0.95f,0.30f,1.0f}},
      {{-0.55f,-0.45f,0.0f},{0.15f,0.45f,1.0f,1.0f}}};
    D3D11_BUFFER_DESC bd{}; bd.ByteWidth=sizeof(v); bd.Usage=D3D11_USAGE_DEFAULT;
    bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA init{}; init.pSysMem=v;
    Check(device_->CreateBuffer(&bd,&init,&vertices_),"CreateBuffer");
  }
  ComPtr<ID3D11Device> device_; ComPtr<ID3D11DeviceContext> context_;
  ComPtr<IDXGISwapChain> swap_; ComPtr<ID3D11RenderTargetView> rtv_;
  ComPtr<ID3D11Buffer> vertices_; ComPtr<ID3D11InputLayout> layout_;
  ComPtr<ID3D11VertexShader> vs_; ComPtr<ID3D11PixelShader> ps_;
  bool shutdown_=false;
};
}
int wmain() {
  try {
    HINSTANCE inst=GetModuleHandleW(nullptr); const wchar_t* cls=L"ClienteRecriadoD3D11Smoke";
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.hInstance=inst; wc.lpfnWndProc=WndProc;
    wc.lpszClassName=cls; wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!RegisterClassExW(&wc)) throw std::runtime_error("RegisterClassExW failed");
    RECT r{0,0,static_cast<LONG>(W),static_cast<LONG>(H)};
    const DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    AdjustWindowRect(&r,style,FALSE);
    HWND hwnd=CreateWindowExW(0,cls,L"ClienteRecriado - D3D11 GPU Triangle",style,
      CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,nullptr,nullptr,inst,nullptr);
    if(!hwnd) throw std::runtime_error("CreateWindowExW failed");
    Renderer renderer; renderer.Init(hwnd); ShowWindow(hwnd,SW_SHOW); UpdateWindow(hwnd);
    std::wprintf(L"Rendering GPU triangle. Close the window to exit.\n");
    MSG msg{}; bool running=true;
    while(running) {
      while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
        if(msg.message==WM_QUIT){running=false;break;}
        TranslateMessage(&msg); DispatchMessageW(&msg);
      }
      if(running) renderer.Frame();
    }
    renderer.Shutdown(); UnregisterClassW(cls,inst); return 0;
  } catch(const std::exception& e) {
    std::fprintf(stderr,"D3D11 smoke test FAILED: %s\n",e.what());
    MessageBoxA(nullptr,e.what(),"ClienteRecriado D3D11 smoke test failed",MB_OK|MB_ICONERROR);
    return 1;
  }
}
