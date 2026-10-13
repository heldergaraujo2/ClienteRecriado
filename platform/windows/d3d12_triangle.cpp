// SPDX-License-Identifier: MIT
// Isolated D3D12 smoke test. Not the production renderer or a benchmark.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <stdexcept>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib")

using Microsoft::WRL::ComPtr;
namespace {
constexpr UINT W = 1280, H = 720;
constexpr UINT FRAME_COUNT = 2;
struct Vertex { float p[3]; float c[4]; };

void Check(HRESULT hr, const char* what) {
  if (FAILED(hr)) {
    char text[256]{};
    std::snprintf(text, sizeof(text), "%s failed (HRESULT=0x%08lX)",
                  what, static_cast<unsigned long>(hr));
    throw std::runtime_error(text);
  }
}
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (m == WM_CLOSE) { DestroyWindow(h); return 0; }
  if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
  return DefWindowProcW(h, m, w, l);
}

class Renderer {
 public:
  void Init(HWND hwnd) {
    UINT flags = 0;
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
      debug->EnableDebugLayer(); flags |= DXGI_CREATE_FACTORY_DEBUG; debug_ = true;
    }
#endif
    Check(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory_)), "CreateDXGIFactory2");
    PickAdapter();
    Check(D3D12CreateDevice(adapter_.Get(), D3D_FEATURE_LEVEL_11_0,
                            IID_PPV_ARGS(&device_)), "D3D12CreateDevice");
    DXGI_ADAPTER_DESC1 ad{}; Check(adapter_->GetDesc1(&ad), "GetDesc1");
    std::wprintf(L"Adapter: %ls\n", ad.Description);
    std::printf("Dedicated video memory reported by DXGI: %llu MiB\n",
      static_cast<unsigned long long>(ad.DedicatedVideoMemory / (1024ULL * 1024ULL)));
    std::printf("D3D12 debug layer: %s\n", debug_ ? "enabled" : "unavailable/disabled");

    D3D12_COMMAND_QUEUE_DESC q{}; q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    Check(device_->CreateCommandQueue(&q, IID_PPV_ARGS(&queue_)), "CreateCommandQueue");
    DXGI_SWAP_CHAIN_DESC1 sd{}; sd.Width=W; sd.Height=H;
    sd.Format=DXGI_FORMAT_R8G8B8A8_UNORM; sd.SampleDesc.Count=1;
    sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.BufferCount=FRAME_COUNT;
    sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    ComPtr<IDXGISwapChain1> s1;
    Check(factory_->CreateSwapChainForHwnd(queue_.Get(), hwnd, &sd, nullptr, nullptr, &s1),
          "CreateSwapChainForHwnd");
    Check(s1.As(&swap_), "Query IDXGISwapChain3");
    Check(factory_->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");

    D3D12_DESCRIPTOR_HEAP_DESC hd{}; hd.NumDescriptors=FRAME_COUNT;
    hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    Check(device_->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtvHeap_)), "Create RTV heap");
    rtvStep_=device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto rh=rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT i=0; i<FRAME_COUNT; ++i) {
      Check(swap_->GetBuffer(i, IID_PPV_ARGS(&targets_[i])), "GetBuffer");
      device_->CreateRenderTargetView(targets_[i].Get(), nullptr, rh);
      rh.ptr += rtvStep_;
    }
    Check(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           IID_PPV_ARGS(&allocator_)), "CreateCommandAllocator");
    MakePipeline(); MakeVertices();
    Check(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator_.Get(),
                                     pso_.Get(), IID_PPV_ARGS(&list_)), "CreateCommandList");
    Check(list_->Close(), "Close initial command list");
    Check(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)), "CreateFence");
    event_=CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!event_) throw std::runtime_error("CreateEventW failed");
  }

  void Frame() {
    WaitGpu();
    Check(allocator_->Reset(), "Allocator Reset");
    Check(list_->Reset(allocator_.Get(), pso_.Get()), "List Reset");
    UINT i=swap_->GetCurrentBackBufferIndex();
    D3D12_RESOURCE_BARRIER b{}; b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource=targets_[i].Get();
    b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;
    b.Transition.StateAfter=D3D12_RESOURCE_STATE_RENDER_TARGET;
    list_->ResourceBarrier(1,&b);
    auto rtv=rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += static_cast<SIZE_T>(i)*rtvStep_;
    list_->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    const float clear[4]={0.035f,0.045f,0.075f,1.0f};
    list_->ClearRenderTargetView(rtv,clear,0,nullptr);
    D3D12_VIEWPORT vp{}; vp.Width=static_cast<float>(W); vp.Height=static_cast<float>(H); vp.MaxDepth=1;
    D3D12_RECT sc{0,0,static_cast<LONG>(W),static_cast<LONG>(H)};
    list_->RSSetViewports(1,&vp); list_->RSSetScissorRects(1,&sc);
    list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    list_->IASetVertexBuffers(0,1,&vbv_); list_->DrawInstanced(3,1,0,0);
    b.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;
    b.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;
    list_->ResourceBarrier(1,&b);
    Check(list_->Close(),"List Close");
    ID3D12CommandList* ls[]={list_.Get()}; queue_->ExecuteCommandLists(1,ls);
    Check(swap_->Present(1,0),"Present");
    ++fenceValue_; Check(queue_->Signal(fence_.Get(),fenceValue_),"Queue Signal");
  }

  void WaitGpu() {
    if (!queue_ || !fence_ || fence_->GetCompletedValue()>=fenceValue_) return;
    Check(fence_->SetEventOnCompletion(fenceValue_,event_),"SetEventOnCompletion");
    WaitForSingleObject(event_,INFINITE);
  }
  void Shutdown() {
    if (queue_ && fence_) WaitGpu();
    if (event_) { CloseHandle(event_); event_=nullptr; }
    std::puts("D3D12 smoke test: clean shutdown.");
  }
  ~Renderer() { Shutdown(); }

 private:
  void PickAdapter() {
    ComPtr<IDXGIFactory6> f6;
    if (SUCCEEDED(factory_.As(&f6))) {
      for (UINT i=0;;++i) {
        ComPtr<IDXGIAdapter1> a;
        if (f6->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                           IID_PPV_ARGS(&a))==DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC1 d{}; if (FAILED(a->GetDesc1(&d)) || (d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)) continue;
        if (SUCCEEDED(D3D12CreateDevice(a.Get(),D3D_FEATURE_LEVEL_11_0,__uuidof(ID3D12Device),nullptr))) { adapter_=a; return; }
      }
    }
    throw std::runtime_error("No compatible non-software D3D12 adapter found.");
  }

  void MakePipeline() {
    const char* shader=R"(
      struct VI { float3 p:POSITION; float4 c:COLOR; };
      struct VO { float4 p:SV_POSITION; float4 c:COLOR; };
      VO VSMain(VI v) { VO o; o.p=float4(v.p,1); o.c=v.c; return o; }
      float4 PSMain(VO v):SV_TARGET { return v.c; }
    )";
    ComPtr<ID3DBlob> vs,ps,err;
    HRESULT hr=D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"VSMain","vs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&vs,&err);
    if (FAILED(hr)) { if(err) std::fprintf(stderr,"%s\n",static_cast<const char*>(err->GetBufferPointer())); Check(hr,"Compile VS"); }
    err.Reset(); hr=D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"PSMain","ps_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&ps,&err);
    if (FAILED(hr)) { if(err) std::fprintf(stderr,"%s\n",static_cast<const char*>(err->GetBufferPointer())); Check(hr,"Compile PS"); }
    D3D12_ROOT_SIGNATURE_DESC rd{}; rd.Flags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> blob,errors;
    hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors);
    if (FAILED(hr)) { if(errors) std::fprintf(stderr,"%s\n",static_cast<const char*>(errors->GetBufferPointer())); Check(hr,"Serialize root signature"); }
    Check(device_->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root_)),"CreateRootSignature");
    D3D12_INPUT_ELEMENT_DESC il[]={
      {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
      {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC p{}; p.pRootSignature=root_.Get();
    p.VS={vs->GetBufferPointer(),vs->GetBufferSize()}; p.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    p.SampleMask=UINT_MAX; p.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;
    p.RasterizerState.CullMode=D3D12_CULL_MODE_NONE; p.RasterizerState.DepthClipEnable=TRUE;
    p.DepthStencilState.DepthEnable=FALSE; p.DepthStencilState.StencilEnable=FALSE;
    p.InputLayout={il,static_cast<UINT>(std::size(il))};
    p.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    p.NumRenderTargets=1; p.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM; p.SampleDesc.Count=1;
    p.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    Check(device_->CreateGraphicsPipelineState(&p,IID_PPV_ARGS(&pso_)),"CreateGraphicsPipelineState");
  }

  void MakeVertices() {
    const Vertex v[]={
      {{0.0f,0.55f,0.0f},{1.0f,0.18f,0.15f,1.0f}},
      {{0.55f,-0.45f,0.0f},{0.15f,0.95f,0.30f,1.0f}},
      {{-0.55f,-0.45f,0.0f},{0.15f,0.45f,1.0f,1.0f}}};
    D3D12_HEAP_PROPERTIES hp{}; hp.Type=D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC d{}; d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER; d.Width=sizeof(v);
    d.Height=1; d.DepthOrArraySize=1; d.MipLevels=1; d.SampleDesc.Count=1; d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Check(device_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&vb_)),"Create vertex buffer");
    void* p=nullptr; D3D12_RANGE rr{0,0}; Check(vb_->Map(0,&rr,&p),"Map vertex buffer");
    std::memcpy(p,v,sizeof(v)); vb_->Unmap(0,nullptr);
    vbv_.BufferLocation=vb_->GetGPUVirtualAddress(); vbv_.SizeInBytes=sizeof(v); vbv_.StrideInBytes=sizeof(Vertex);
  }

  ComPtr<IDXGIFactory4> factory_; ComPtr<IDXGIAdapter1> adapter_;
  ComPtr<ID3D12Device> device_; ComPtr<ID3D12CommandQueue> queue_;
  ComPtr<IDXGISwapChain3> swap_; ComPtr<ID3D12DescriptorHeap> rtvHeap_;
  ComPtr<ID3D12Resource> targets_[FRAME_COUNT],vb_;
  ComPtr<ID3D12CommandAllocator> allocator_; ComPtr<ID3D12GraphicsCommandList> list_;
  ComPtr<ID3D12RootSignature> root_; ComPtr<ID3D12PipelineState> pso_; ComPtr<ID3D12Fence> fence_;
  D3D12_VERTEX_BUFFER_VIEW vbv_{}; UINT rtvStep_=0; UINT64 fenceValue_=0;
  HANDLE event_=nullptr; bool debug_=false;
};

} // namespace

int wmain() {
  try {
    HINSTANCE inst=GetModuleHandleW(nullptr); const wchar_t* cls=L"ClienteRecriadoD3D12Smoke";
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.hInstance=inst; wc.lpfnWndProc=WndProc;
    wc.lpszClassName=cls; wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if (!RegisterClassExW(&wc)) throw std::runtime_error("RegisterClassExW failed");
    RECT r{0,0,static_cast<LONG>(W),static_cast<LONG>(H)};
    const DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    AdjustWindowRect(&r,style,FALSE);
    HWND hwnd=CreateWindowExW(0,cls,L"ClienteRecriado — D3D12 GPU Triangle",style,
      CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,nullptr,nullptr,inst,nullptr);
    if (!hwnd) throw std::runtime_error("CreateWindowExW failed");
    Renderer renderer; renderer.Init(hwnd); ShowWindow(hwnd,SW_SHOW); UpdateWindow(hwnd);
    std::puts("Rendering GPU triangle. Close the window to exit.");
    MSG msg{}; bool running=true;
    while (running) {
      while (PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
        if (msg.message==WM_QUIT) { running=false; break; }
        TranslateMessage(&msg); DispatchMessageW(&msg);
      }
      if (running) renderer.Frame();
    }
    renderer.Shutdown(); UnregisterClassW(cls,inst); return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr,"D3D12 smoke test FAILED: %s\n",e.what());
    MessageBoxA(nullptr,e.what(),"ClienteRecriado D3D12 smoke test failed",MB_OK|MB_ICONERROR);
    return 1;
  }
}
