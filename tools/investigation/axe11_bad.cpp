// Ciclo: ciclo16 B
// Pergunta: Mede bad_indices Axe11 bad 71 total 2736 rate 2.59% bounds min/max
// Evidência: docs/evidence/ciclo15_render_validation.log
// Origem: /tmp/axe11_bad.cpp movido para backup permanente ciclo16 para garantir que todo trabalho está salvo no repositório
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/mesh.hpp"
#include "mu/fs.hpp"
#include <iostream>
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
  std::cout<<"verts "<<r.mesh.positions.size()<<" tris body "<<body.triangles.size()<<" tris mesh "<<r.mesh.indices.size()/3<<" bad "<<r.mesh.bad_indices<<" skipped "<<r.mesh.skipped_vertices<<"\n";
  size_t total=body.triangles.size();
  size_t bad=r.mesh.bad_indices;
  double rate = total? (double)bad*100.0/total:0;
  std::cout<<"bad rate vs body tris "<<rate<<"%\n";
  // also count degenerate area <1e-6 after projection? Need to compute degenerate in mesh
  // For Axe11, also compute bounds inflation due to bad triangles? Bad triangles are dropped, so they don't affect rendered bbox, but they DO affect 3D bounds if their vertices are extreme? Actually bounds computed from positions, not triangles, so bad_indices doesn't affect bounds directly.
  // But degenerate triangles (area 0) are still in mesh.indices but dropped at rasterization fabs(area)<1e-6, so they don't affect 2D bbox but their vertices still affect 3D bounds.
  std::cout<<"bounds min "<<r.mesh.bounds.min.x<<","<<r.mesh.bounds.min.y<<","<<r.mesh.bounds.min.z<<" max "<<r.mesh.bounds.max.x<<","<<r.mesh.bounds.max.y<<","<<r.mesh.bounds.max.z<<"\n";
  return 0;
}
