// Ciclo: ciclo16 B
// Pergunta: Análise específica Axe11 degenerados 80% ok 19.8% usedVerts 26% inflation X1.57, fonte erro quantificada para tolerância a priori 67.3%
// Evidência: docs/evidence/ciclo16_axe11_tolerance_analysis.log
// Origem: /tmp/axe11_degen.cpp movido para backup permanente ciclo16 para garantir que todo trabalho está salvo no repositório
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
int main(){
  std::string path="/tmp/cliente_ref/Data/Player/Axe11.bmd";
  std::vector<uint8_t> raw;
  mu::ReadFile(path,&raw);
  mu::ByteView view(raw.data(),raw.size());
  mu::BmdInfo info;
  mu::BmdProbe(view,&info);
  mu::ByteView payload=view.sub(info.payload_offset, info.payload_size);
  std::vector<uint8_t> plain;
  if(info.kind==mu::BmdKind::kPlainV0C){ plain.assign(payload.data,payload.data+payload.size); mu::BmdV0CDecrypt(plain.data(),plain.size()); payload=mu::ByteView(plain.data(),plain.size()); }
  mu::BmdBody body;
  mu::BmdParseBodyV0A(payload,&body);
  auto r=mu::MeshFromBmd(body,path);
  mu::Mesh mesh=r.mesh;
  std::cout<<"total verts "<<mesh.positions.size()<<" total tris mesh "<<mesh.indices.size()/3<<" body tris "<<body.triangles.size()<<" bad "<<r.mesh.bad_indices<<"\n";
  mu::Bounds b_all = mu::ComputeBounds(mesh.positions);
  std::cout<<"bounds all valid "<<b_all.valid<<" min "<<b_all.min.x<<","<<b_all.min.y<<","<<b_all.min.z<<" max "<<b_all.max.x<<","<<b_all.max.y<<","<<b_all.max.z<<"\n";
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
  std::cout<<"tri stats: total "<<mesh.indices.size()/3<<" ok "<<ok<<" degenerate area<1e-6 "<<degenerate<<" clipped w<=0.001 "<<clipped<<" ok% "<<(double)ok*100.0/(mesh.indices.size()/3)<<"%\n";
  std::cout<<"used verts in ok tris "<<usedVerts.size()<<" vs total verts "<<mesh.positions.size()<<" used% "<<(double)usedVerts.size()*100.0/mesh.positions.size()<<"%\n";
  std::vector<mu::Vec3> usedPos;
  for(auto idx: usedVerts) usedPos.push_back(mesh.positions[idx]);
  mu::Bounds b_used = mu::ComputeBounds(usedPos);
  std::cout<<"bounds used valid "<<b_used.valid<<" min "<<b_used.min.x<<","<<b_used.min.y<<","<<b_used.min.z<<" max "<<b_used.max.x<<","<<b_used.max.y<<","<<b_used.max.z<<"\n";
  if(b_all.valid && b_used.valid){
    float dx_all=b_all.max.x-b_all.min.x, dy_all=b_all.max.y-b_all.min.y, dz_all=b_all.max.z-b_all.min.z;
    float dx_used=b_used.max.x-b_used.min.x, dy_used=b_used.max.y-b_used.min.y, dz_used=b_used.max.z-b_used.min.z;
    std::cout<<"size all dx "<<dx_all<<" dy "<<dy_all<<" dz "<<dz_all<<"\n";
    std::cout<<"size used dx "<<dx_used<<" dy "<<dy_used<<" dz "<<dz_used<<"\n";
    std::cout<<"inflation dx all/used "<<dx_all/dx_used<<" dy "<<dy_all/dy_used<<" dz "<<dz_all/dz_used<<"\n";
  }
  return 0;
}
