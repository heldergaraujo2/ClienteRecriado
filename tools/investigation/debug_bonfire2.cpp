// Ciclo: ciclo14 B
// Pergunta: Debug Bonfire áreas degeneradas idx 1,1,1 area 0 w, por que rasterizado 0
// Evidência: docs/evidence/ciclo14_first_render_validation.md
// Origem: /tmp/debug_bonfire2.cpp movido para backup permanente ciclo15 Parte A
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
  cam.aspect=800.0f/600.0f;
  cam.fov_deg=45;
  cam.near_plane=0.1f;
  cam.far_plane=std::max(100.0f,radius*10.0f);
  cam.target=center;
  cam.eye={center.x, center.y+radius*0.3f, center.z+radius*2.2f};
  cam.up={0,1,0};
  mu::Mat4 viewM=cam.GetViewMatrix();
  mu::Mat4 proj=cam.GetProjectionMatrix();
  mu::Mat4 vp=mu::Multiply(proj,viewM);
  // Transform first 10 vertices
  struct VOut{float x,y,z,w;};
  std::vector<VOut> vout(mesh.positions.size());
  for(size_t i=0;i<mesh.positions.size();++i){
    mu::Vec3 pos=mesh.positions[i];
    float x = vp.m[0]*pos.x + vp.m[1]*pos.y + vp.m[2]*pos.z + vp.m[3];
    float y = vp.m[4]*pos.x + vp.m[5]*pos.y + vp.m[6]*pos.z + vp.m[7];
    float z = vp.m[8]*pos.x + vp.m[9]*pos.y + vp.m[10]*pos.z + vp.m[11];
    float w = vp.m[12]*pos.x + vp.m[13]*pos.y + vp.m[14]*pos.z + vp.m[15];
    if(w<=0.001f){ vout[i]={0,0,0,0}; continue; }
    float ndc_x=x/w, ndc_y=y/w, ndc_z=z/w;
    float sx=(ndc_x*0.5f+0.5f)*800;
    float sy=(1.0f-(ndc_y*0.5f+0.5f))*600;
    float depth=ndc_z*0.5f+0.5f;
    vout[i]={sx,sy,depth,w};
  }
  // Check first 10 triangles
  for(size_t ti=0; ti<10 && ti*3+2 < mesh.indices.size(); ++ti){
    uint32_t i0=mesh.indices[ti*3], i1=mesh.indices[ti*3+1], i2=mesh.indices[ti*3+2];
    auto v0=vout[i0], v1=vout[i1], v2=vout[i2];
    float area = (v2.x - v0.x)*(v1.y - v0.y) - (v2.y - v0.y)*(v1.x - v0.x);
    // Actually edge func: (cx-ax)*(by-ay)-(cy-ay)*(bx-ax) with a=v0,b=v1,c=v2
    float area2 = (v2.x - v0.x)*(v1.y - v0.y) - (v2.y - v0.y)*(v1.x - v0.x);
    // Let's compute using our EdgeFunc
    auto edge=[&](float ax,float ay,float bx,float by,float cx,float cy){return (cx-ax)*(by-ay)-(cy-ay)*(bx-ax);};
    float area3=edge(v0.x,v0.y,v1.x,v1.y,v2.x,v2.y);
    std::cout<<"tri "<<ti<<" idx "<<i0<<","<<i1<<","<<i2<<" v0 "<<v0.x<<","<<v0.y<<" v1 "<<v1.x<<","<<v1.y<<" v2 "<<v2.x<<","<<v2.y<<" area "<<area3<<" w "<<v0.w<<","<<v1.w<<","<<v2.w<<"\n";
  }
  return 0;
}
