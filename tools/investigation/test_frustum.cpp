// Ciclo: ciclo14 B
// Pergunta: Teste frustum culling com câmera 0,0,5 olhando origem, por que Bonfire culled, far plane 100 vs radius*10
// Evidência: docs/evidence/ciclo14_culling_camera.md
// Origem: /tmp/test_frustum.cpp movido para backup permanente ciclo15 Parte A
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO — não reproduzível só pela documentação
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include "mu/render/software_rasterizer.hpp"
#include "mu/render/cull.hpp"
#include <iostream>
int main(){
  mu::Camera cam;
  cam.eye={0,0,5};
  cam.target={0,0,0};
  cam.up={0,1,0};
  cam.fov_deg=45;
  cam.aspect=1.333f;
  cam.near_plane=0.1f;
  cam.far_plane=100.0f;
  auto frustum=cam.GetFrustum();
  mu::Bounds b;
  b.min={-1,-1,-1}; b.max={1,1,1}; b.valid=true;
  auto res=mu::CullBox(frustum,b);
  std::cout<<"CullBox result "<<(int)res<<" 0=Outside 1=Inside 2=Intersect IsVisible "<<mu::IsVisible(frustum,b)<<"\n";
  for(int i=0;i<6;++i){
    std::cout<<"plane "<<i<<" normal "<<frustum.planes[i].normal.x<<","<<frustum.planes[i].normal.y<<","<<frustum.planes[i].normal.z<<" d "<<frustum.planes[i].d<<"\n";
  }
  // Test with Bonfire bounds
  mu::Bounds b2;
  b2.min={-59.27f,-57.32f,-24.51f}; b2.max={60.87f,48.30f,35.97f}; b2.valid=true;
  auto res2=mu::CullBox(frustum,b2);
  std::cout<<"Bonfire bounds with cam at 0,0,5 result "<<(int)res2<<" visible "<<mu::IsVisible(frustum,b2)<<"\n";

  // Camera for Bonfire: center approx (0.8,-4.5,5.7) radius ~84, eye (0.8, 20.7, 190)
  mu::Camera cam2;
  cam2.eye={0.8f, 20.7f, 190.0f};
  cam2.target={0.8f, -4.5f, 5.7f};
  cam2.up={0,1,0};
  cam2.fov_deg=45; cam2.aspect=1.333f; cam2.near_plane=0.1f; cam2.far_plane=1000.0f;
  auto frustum2=cam2.GetFrustum();
  auto res3=mu::CullBox(frustum2,b2);
  std::cout<<"Bonfire bounds with cam2 result "<<(int)res3<<" visible "<<mu::IsVisible(frustum2,b2)<<"\n";
  for(int i=0;i<6;++i){
    std::cout<<"plane2 "<<i<<" normal "<<frustum2.planes[i].normal.x<<","<<frustum2.planes[i].normal.y<<","<<frustum2.planes[i].normal.z<<" d "<<frustum2.planes[i].d<<"\n";
  }
  return 0;
}
