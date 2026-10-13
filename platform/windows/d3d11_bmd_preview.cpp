// SPDX-License-Identifier: MIT
// Experimental, untextured D3D11 preview for plaintext BMD v0x0A only.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/mesh.hpp"
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"user32.lib")
using Microsoft::WRL::ComPtr;
namespace {
struct V{float p[3];float c[4];};
void Check(HRESULT h,const char* op){if(FAILED(h)){char s[200]{};std::snprintf(s,sizeof(s),"%s failed: 0x%08lX",op,static_cast<unsigned long>(h));throw std::runtime_error(s);}}
LRESULT CALLBACK Proc(HWND h,UINT m,WPARAM w,LPARAM l){if(m==WM_CLOSE){DestroyWindow(h);return 0;}if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);}
class R{
 public:
 void Init(HWND w,const mu::Mesh&m){
  if(m.positions.empty()||m.indices.empty()||m.indices.size()%3)throw std::runtime_error("No validated triangles.");
  DXGI_SWAP_CHAIN_DESC sd{};sd.BufferCount=2;sd.BufferDesc.Width=1200;sd.BufferDesc.Height=800;sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.OutputWindow=w;sd.SampleDesc.Count=1;sd.Windowed=TRUE;sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
  D3D_FEATURE_LEVEL ls[]={D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0},got{};HRESULT h=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,ls,4,D3D11_SDK_VERSION,&sd,&sc_,&d_,&got,&c_);if(h==E_INVALIDARG)Check(D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,ls+1,3,D3D11_SDK_VERSION,&sd,&sc_,&d_,&got,&c_),"D3D11CreateDeviceAndSwapChain");else Check(h,"D3D11CreateDeviceAndSwapChain");
  ComPtr<ID3D11Texture2D>b;Check(sc_->GetBuffer(0,IID_PPV_ARGS(&b)),"GetBuffer");Check(d_->CreateRenderTargetView(b.Get(),nullptr,&rt_),"CreateRenderTargetView");Pipeline();Buffers(m);std::wprintf(L"D3D feature level 0x%X\n",unsigned(got));
 }
 void Frame(){float bg[]={.035f,.045f,.075f,1};c_->OMSetRenderTargets(1,rt_.GetAddressOf(),nullptr);c_->ClearRenderTargetView(rt_.Get(),bg);D3D11_VIEWPORT p{};p.Width=1200;p.Height=800;p.MinDepth=0;p.MaxDepth=1;c_->RSSetViewports(1,&p);UINT stride=sizeof(V),off=0;ID3D11Buffer*v=vb_.Get();c_->IASetInputLayout(il_.Get());c_->IASetVertexBuffers(0,1,&v,&stride,&off);c_->IASetIndexBuffer(ib_.Get(),DXGI_FORMAT_R32_UINT,0);c_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);c_->VSSetShader(vs_.Get(),nullptr,0);c_->PSSetShader(ps_.Get(),nullptr,0);c_->RSSetState(rs_.Get());c_->DrawIndexed(count_,0,0);Check(sc_->Present(1,0),"Present");}
 private:
 void Pipeline(){const char*s="struct I{float3 p:POSITION;float4 c:COLOR;};struct O{float4 p:SV_POSITION;float4 c:COLOR;};O VSMain(I i){O o;o.p=float4(i.p,1);o.c=i.c;return o;}float4 PSMain(O i):SV_TARGET{return i.c;}";ComPtr<ID3DBlob>v,p,e;HRESULT h=D3DCompile(s,strlen(s),nullptr,nullptr,nullptr,"VSMain","vs_4_0",0,0,&v,&e);if(FAILED(h))Check(h,"Compile VS");h=D3DCompile(s,strlen(s),nullptr,nullptr,nullptr,"PSMain","ps_4_0",0,0,&p,&e);if(FAILED(h))Check(h,"Compile PS");D3D11_INPUT_ELEMENT_DESC a[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};Check(d_->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs_),"CreateVertexShader");Check(d_->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps_),"CreatePixelShader");Check(d_->CreateInputLayout(a,2,v->GetBufferPointer(),v->GetBufferSize(),&il_),"CreateInputLayout");D3D11_RASTERIZER_DESC q{};q.FillMode=D3D11_FILL_SOLID;q.CullMode=D3D11_CULL_NONE;q.DepthClipEnable=TRUE;Check(d_->CreateRasterizerState(&q,&rs_),"CreateRasterizerState");}
 void Buffers(const mu::Mesh&m){auto b=m.bounds.valid?m.bounds:mu::ComputeBounds(m.positions);if(!b.valid)throw std::runtime_error("Invalid bounds.");float lo[]={b.min.x,b.min.y,b.min.z},hi[]={b.max.x,b.max.y,b.max.z},e[]={hi[0]-lo[0],hi[1]-lo[1],hi[2]-lo[2]};int y=int(std::max_element(e,e+3)-e),x=(y+1)%3;if(e[(y+2)%3]>e[x])x=(y+2)%3;int z=3-x-y;float mid[]={(lo[0]+hi[0])*.5f,(lo[1]+hi[1])*.5f,(lo[2]+hi[2])*.5f};float s=1.6f/std::max({e[0],e[1],e[2]});if(!std::isfinite(s)||s<=0)throw std::runtime_error("Invalid extent.");std::vector<V>v;for(auto a:m.positions){float q[]={a.x,a.y,a.z};v.push_back({{(q[x]-mid[x])*s,(q[y]-mid[y])*s,(q[z]-mid[z])*s},{.3f,.78f,.94f,1}});}D3D11_BUFFER_DESC d{};d.ByteWidth=UINT(v.size()*sizeof(V));d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;D3D11_SUBRESOURCE_DATA i{};i.pSysMem=v.data();Check(d_->CreateBuffer(&d,&i,&vb_),"Vertex buffer");d={};d.ByteWidth=UINT(m.indices.size()*sizeof(uint32_t));d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_INDEX_BUFFER;i={};i.pSysMem=m.indices.data();Check(d_->CreateBuffer(&d,&i,&ib_),"Index buffer");count_=UINT(m.indices.size());std::wprintf(L"GPU geometry: %llu vertices, %u indices, %u triangles\n",(unsigned long long)v.size(),count_,count_/3);}
 ComPtr<ID3D11Device>d_;ComPtr<ID3D11DeviceContext>c_;ComPtr<IDXGISwapChain>sc_;ComPtr<ID3D11RenderTargetView>rt_;ComPtr<ID3D11Buffer>vb_,ib_;ComPtr<ID3D11InputLayout>il_;ComPtr<ID3D11RasterizerState>rs_;ComPtr<ID3D11VertexShader>vs_;ComPtr<ID3D11PixelShader>ps_;UINT count_=0;
};
}
int wmain(int argc,wchar_t**argv){
 if(argc!=2){std::fwprintf(stderr,L"Usage: d3d11_bmd_preview.exe <plaintext-BMD-v0A>\n");return 2;}
 try{
  HANDLE f=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot open BMD file.");LARGE_INTEGER n{};if(!GetFileSizeEx(f,&n)||n.QuadPart<=0||n.QuadPart>0x7fffffff){CloseHandle(f);throw std::runtime_error("Unsupported BMD size.");}std::vector<uint8_t>b(size_t(n.QuadPart));size_t pos=0;while(pos<b.size()){DWORD got=0;if(!ReadFile(f,b.data()+pos,DWORD(std::min<size_t>(b.size()-pos,0x40000000)),&got,nullptr)||!got){CloseHandle(f);throw std::runtime_error("BMD read failed.");}pos+=got;}CloseHandle(f);
  mu::BmdInfo info;auto st=mu::BmdProbe(mu::ByteView(b.data(),b.size()),&info);if(!st)throw std::runtime_error("BMD probe: "+st.message);if(info.kind!=mu::BmdKind::kPlainV0A)throw std::runtime_error("Only plaintext BMD v0x0A is supported; other versions and encrypted containers are rejected.");mu::BmdBody body;st=mu::BmdParseBodyV0A(mu::ByteView(b.data()+info.payload_offset,info.payload_size),&body);if(!st||!body.complete)throw std::runtime_error("BMD parse: "+(body.problem.empty()?st.message:body.problem));auto m=mu::MeshFromBmd(body,"BMD preview");if(!m.ok)throw std::runtime_error("Mesh import: "+m.problem);std::wprintf(L"Parsed %llu vertices, %llu normals, %llu triangle records; dropped %llu triangles. UV/material mapping is unverified.\n",(unsigned long long)body.vertices.size(),(unsigned long long)body.normals.size(),(unsigned long long)body.triangles.size(),(unsigned long long)m.mesh.dropped_triangles);if(m.mesh.indices.empty())throw std::runtime_error("No triangles survived existing index validation.");
  HINSTANCE i=GetModuleHandleW(nullptr);const wchar_t*name=L"NexoBmdPreviewWindow";WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.hInstance=i;wc.lpfnWndProc=Proc;wc.lpszClassName=name;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);if(!RegisterClassExW(&wc))throw std::runtime_error("RegisterClassExW failed.");RECT r{0,0,1200,800};DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;AdjustWindowRect(&r,style,FALSE);HWND h=CreateWindowExW(0,name,L"Experimental D3D11 BMD Preview (untextured)",style,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,nullptr,nullptr,i,nullptr);if(!h){UnregisterClassW(name,i);throw std::runtime_error("CreateWindowExW failed.");}R renderer;renderer.Init(h,m.mesh);ShowWindow(h,SW_SHOW);MSG msg{};bool run=true;while(run){while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT){run=false;break;}TranslateMessage(&msg);DispatchMessageW(&msg);}if(run)renderer.Frame();}UnregisterClassW(name,i);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"BMD preview failed: %s\n",e.what());MessageBoxA(nullptr,e.what(),"BMD preview failed",MB_OK|MB_ICONERROR);return 1;}
}
