// Ciclo: ciclo14 B
// Pergunta: Debug Bonfire01.bmd clip ndc x/w y/w z/w, por que culled_frustum 72
// Evidência: docs/evidence/ciclo14_first_render_validation.md
// Origem: /tmp/debug_bonfire.cpp movido para backup permanente ciclo15 Parte A
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO — não reproduzível só pela documentação
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/mesh.hpp"
#include "mu/render/software_rasterizer.hpp"
#include "mu/fs.hpp"
#include <iostream>
#include <cmath>
int main(){
  std::string path="/tmp/cliente_ref/Data/Object1/Bonfire01.bmd";
  std::vector<uint8_t> raw;
  mu::ReadFile(path,&raw);
  mu::ByteView view1(raw.data(), raw.size());
  mu::BmdInfo info;
  mu::BmdProbe(view1,&info);
  mu::ByteView payload=view1.sub(info.payload_offset, info.payload_size);
  std::vector<uint8_t> plain;
  if(info.kind==mu::BmdKind::kPlainV0C){
    plain.assign(payload.data,payload.data+payload.size);
    mu::BmdV0CDecrypt(plain.data(),plain.size());
    payload=mu::ByteView(plain.data(),plain.size());
  }
  mu::BmdBody body;
  mu::BmdParseBodyV0A(payload,&body);
  auto r=mu::MeshFromBmd(body,path);
  mu::Mesh mesh=r.mesh;
  std::cout<<"vertices "<<mesh.positions.size()<<" tris "<<mesh.indices.size()/3<<"\n";
  mu::Camera cam;
  mu::Vec3 center{0,0,0};
  float radius=1;
  if(mesh.bounds.valid){
    center.x=(mesh.bounds.min.x+mesh.bounds.max.x)*0.5f;
    center.y=(mesh.bounds.min.y+mesh.bounds.max.y)*0.5f;
    center.z=(mesh.bounds.min.z+mesh.bounds.max.z)*0.5f;
    float dx=mesh.bounds.max.x-mesh.bounds.min.x;
    float dy=mesh.bounds.max.y-mesh.bounds.min.y;
    float dz=mesh.bounds.max.z-mesh.bounds.min.z;
    radius=std::sqrt(dx*dx+dy*dy+dz*dz)*0.5f;
  }
  std::cout<<"center "<<center.x<<","<<center.y<<","<<center.z<<" radius "<<radius<<"\n";
  cam.aspect=800.0f/600.0f;
  cam.fov_deg=45;
  cam.near_plane=0.1f;
  cam.far_plane=std::max(100.0f,radius*10.0f);
  cam.target=center;
  cam.eye={center.x, center.y+radius*0.3f, center.z+radius*2.2f};
  cam.up={0,1,0};
  std::cout<<"eye "<<cam.eye.x<<","<<cam.eye.y<<","<<cam.eye.z<<" target "<<cam.target.x<<","<<cam.target.y<<","<<cam.target.z<<" far "<<cam.far_plane<<"\n";
  mu::Mat4 viewM=cam.GetViewMatrix();
  mu::Mat4 proj=cam.GetProjectionMatrix();
  mu::Mat4 vp=mu::Multiply(proj,viewM);
  for(size_t i=0;i<std::min<size_t>(5, mesh.positions.size());++i){
    mu::Vec3 pos=mesh.positions[i];
    float x = vp.m[0]*pos.x + vp.m[1]*pos.y + vp.m[2]*pos.z + vp.m[3];
    float y = vp.m[4]*pos.x + vp.m[5]*pos.y + vp.m[6]*pos.z + vp.m[7];
    float z = vp.m[8]*pos.x + vp.m[9]*pos.y + vp.m[10]*pos.z + vp.m[11];
    float w = vp.m[12]*pos.x + vp.m[13]*pos.y + vp.m[14]*pos.z + vp.m[15];
    std::cout<<"v"<<i<<" pos "<<pos.x<<","<<pos.y<<","<<pos.z<<" clip "<<x<<","<<y<<","<<z<<","<<w<<" ndc "<<x/w<<","<<y/w<<","<<z/w<<"\n";
  }
  return 0;
}
