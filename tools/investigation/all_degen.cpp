// Ciclo: ciclo16 B
// Pergunta: Análise degenerados e inflação bounds para tolerância a priori 15-67% Beer01 Object03 Shield Axe11, quantifica ok% degen% usedVerts% inflation X Y Z expectedTol
// Evidência: docs/evidence/ciclo16_axe11_tolerance_analysis.log
// Origem: /tmp/all_degen.cpp movido para backup permanente ciclo16 para garantir que todo trabalho está salvo no repositório
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/mesh.hpp"
#include "mu/fs.hpp"
#include "mu/render/software_rasterizer.hpp"
#include <set>
#include <iostream>
#include <cmath>
struct Stats{ std::string name; size_t totalTris, okTris, degenTris; size_t totalVerts, usedVerts; float inflX, inflY, inflZ; float expectedTol; };

Stats analyze(const std::string& path, const std::string& name){
  std::vector<uint8_t> raw;
  mu::ReadFile(path,&raw);
  mu::ByteView view(raw.data(),raw.size());
  mu::BmdInfo info;
  mu::BmdProbe(view,&info);
  mu::ByteView payload=view.sub(info.payload_offset, info.payload_size);
  std::vector<uint8_t> plain;
  if(info.kind==mu::BmdKind::kPlainV0C){ plain.assign(payload.data,payload.data+payload.size); mu::BmdV0CDecrypt(plain.data(),plain.size()); payload=mu::ByteView(plain.data(),plain.size()); }
  else if(info.kind==mu::BmdKind::kBuxContainer){
    std::vector<uint8_t> tmp=raw;
    mu::BuxCrypt(tmp.data(),tmp.size());
    mu::ByteView view2(tmp.data(),tmp.size());
    mu::BmdInfo info2;
    mu::BmdProbe(view2,&info2);
    raw.swap(tmp);
    view=mu::ByteView(raw.data(),raw.size());
    info=info2;
    payload=view.sub(info.payload_offset, info.payload_size);
    if(info.kind==mu::BmdKind::kPlainV0C){ plain.assign(payload.data,payload.data+payload.size); mu::BmdV0CDecrypt(plain.data(),plain.size()); payload=mu::ByteView(plain.data(),plain.size()); }
  }
  mu::BmdBody body;
  mu::BmdParseBodyV0A(payload,&body);
  auto r=mu::MeshFromBmd(body,path);
  mu::Mesh mesh=r.mesh;
  mu::Bounds b_all = mu::ComputeBounds(mesh.positions);
  mu::Vec3 center{0,0,0};
  float radius=1;
  if(mesh.bounds.valid){
    center.x=(mesh.bounds.min.x+mesh.bounds.max.x)*0.5f;
    center.y=(mesh.bounds.min.y+mesh.bounds.max.y)*0.5f;
    center.z=(mesh.bounds.min.z+mesh.bounds.max.z)*0.5f;
    float dx=mesh.bounds.max.x - mesh.bounds.min.x;
    float dy=mesh.bounds.max.y - mesh.bounds.min.y;
    float dz=mesh.bounds.max.z - mesh.bounds.min.z;
    radius=std::sqrt(dx*dx+dy*dy+dz*dz)*0.5f;
  }
  mu::Camera cam;
  cam.aspect=800.0f/600.0f;
  cam.fov_deg=45.0f;
  cam.near_plane=0.1f;
  cam.far_plane=std::max(100.0f, radius*10.0f);
  cam.target=center;
  cam.eye={center.x, center.y+radius*0.3f, center.z+radius*2.2f};
  mu::Mat4 viewM=cam.GetViewMatrix();
  mu::Mat4 proj=cam.GetProjectionMatrix();
  mu::Mat4 vp=mu::Multiply(proj, viewM);
  struct VOut{float x,y,w; bool clipped;};
  std::vector<VOut> vout(mesh.positions.size());
  for(size_t vi=0; vi<mesh.positions.size(); ++vi){
    mu::Vec3 pos=mesh.positions[vi];
    float x = vp.m[0]*pos.x + vp.m[1]*pos.y + vp.m[2]*pos.z + vp.m[3];
    float y = vp.m[4]*pos.x + vp.m[5]*pos.y + vp.m[6]*pos.z + vp.m[7];
    float w = vp.m[12]*pos.x + vp.m[13]*pos.y + vp.m[14]*pos.z + vp.m[15];
    if(w<=0.001f){ vout[vi].clipped=true; continue; }
    float ndc_x=x/w, ndc_y=y/w;
    float sx=(ndc_x*0.5f+0.5f)*800;
    float sy=(1.0f-(ndc_y*0.5f+0.5f))*600;
    vout[vi].x=sx; vout[vi].y=sy; vout[vi].w=w; vout[vi].clipped=false;
  }
  size_t degenerate=0, clipped=0, ok=0;
  std::set<uint32_t> usedVerts;
  for(size_t ti=0; ti+2<mesh.indices.size(); ti+=3){
    uint32_t i0=mesh.indices[ti], i1=mesh.indices[ti+1], i2=mesh.indices[ti+2];
    if(i0>=vout.size()||i1>=vout.size()||i2>=vout.size()){ ++degenerate; continue; }
    if(vout[i0].clipped||vout[i1].clipped||vout[i2].clipped){ ++clipped; continue; }
    float area = (vout[i1].x - vout[i0].x)*(vout[i2].y - vout[i0].y) - (vout[i2].x - vout[i0].x)*(vout[i1].y - vout[i0].y);
    if(std::fabs(area)<1e-6f){ ++degenerate; continue; }
    ++ok;
    usedVerts.insert(i0); usedVerts.insert(i1); usedVerts.insert(i2);
  }
  std::vector<mu::Vec3> usedPos;
  for(auto idx: usedVerts) usedPos.push_back(mesh.positions[idx]);
  mu::Bounds b_used = mu::ComputeBounds(usedPos);
  float inflX=1, inflY=1, inflZ=1;
  if(b_all.valid && b_used.valid){
    float dx_all=b_all.max.x-b_all.min.x, dy_all=b_all.max.y-b_all.min.y, dz_all=b_all.max.z-b_all.min.z;
    float dx_used=b_used.max.x-b_used.min.x, dy_used=b_used.max.y-b_used.min.y, dz_used=b_used.max.z-b_used.min.z;
    if(dx_used>1e-6f) inflX=dx_all/dx_used;
    if(dy_used>1e-6f) inflY=dy_all/dy_used;
    if(dz_used>1e-6f) inflZ=dz_all/dz_used;
  }
  float degenRate = mesh.indices.size()/3 ? (float)degenerate*100.0f/(mesh.indices.size()/3) : 0;
  float unusedRate = mesh.positions.size() ? (float)(mesh.positions.size()-usedVerts.size())*100.0f/mesh.positions.size() : 0;
  // Expected tolerance = max inflation -1 + degenerate contribution: e.g., if inflX=1.57 => 57% error expected, plus some margin
  float maxInfl = std::max(inflX, std::max(inflY, inflZ));
  float expectedTol = (maxInfl-1.0f)*100.0f + 10.0f; // +10% margin for perspective
  if(expectedTol<15.0f) expectedTol=15.0f; // minimum 15%
  Stats s;
  s.name=name;
  s.totalTris=mesh.indices.size()/3;
  s.okTris=ok;
  s.degenTris=degenerate;
  s.totalVerts=mesh.positions.size();
  s.usedVerts=usedVerts.size();
  s.inflX=inflX; s.inflY=inflY; s.inflZ=inflZ;
  s.expectedTol=expectedTol;
  std::cout<<"Model "<<name<<" path "<<path<<"\n";
  std::cout<<"  totalTris mesh "<<s.totalTris<<" ok "<<ok<<" degen "<<degenerate<<" clipped "<<clipped<<" ok% "<<(ok*100.0/s.totalTris)<<"% degen% "<<degenRate<<"%\n";
  std::cout<<"  totalVerts "<<s.totalVerts<<" usedVerts "<<s.usedVerts<<" used% "<<(usedVerts.size()*100.0/mesh.positions.size())<<"% unused% "<<unusedRate<<"%\n";
  std::cout<<"  bounds all dx "<<(b_all.max.x-b_all.min.x)<<" dy "<<(b_all.max.y-b_all.min.y)<<" dz "<<(b_all.max.z-b_all.min.z)<<"\n";
  std::cout<<"  bounds used dx "<<(b_used.max.x-b_used.min.x)<<" dy "<<(b_used.max.y-b_used.min.y)<<" dz "<<(b_used.max.z-b_used.min.z)<<"\n";
  std::cout<<"  inflation X "<<inflX<<" Y "<<inflY<<" Z "<<inflZ<<" max "<<maxInfl<<" expectedTol "<<expectedTol<<"% (a priori)\n";
  return s;
}
int main(){
  analyze("/tmp/cliente_ref/Data/Object1/Beer01.bmd","Beer01");
  analyze("/tmp/cliente_ref/Data/Object2/Object03.bmd","Object03");
  analyze("/tmp/cliente_ref/Data/Player/AmbitionShield.bmd","AmbitionShield");
  analyze("/tmp/cliente_ref/Data/Player/Axe11.bmd","Axe11");
  return 0;
}
