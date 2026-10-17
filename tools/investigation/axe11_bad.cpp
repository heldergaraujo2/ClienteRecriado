// Ciclo: ciclo16 B
// Question: Measure dropped-triangle counts for Axe11 and compare bounds.
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
  std::cout<<"verts "<<r.mesh.positions.size()<<" tris body "<<body.triangles.size()<<" tris mesh "<<r.mesh.indices.size()/3<<" dropped "<<r.mesh.dropped_triangles<<" skipped "<<r.mesh.skipped_vertices<<"\n";
  size_t total=body.triangles.size();
  size_t dropped=r.mesh.dropped_triangles;
  double rate = total ? static_cast<double>(dropped) * 100.0 / static_cast<double>(total) : 0.0;
  std::cout<<"dropped rate vs body tris "<<rate<<"%\n";
  // also count degenerate area <1e-6 after projection? Need to compute degenerate in mesh
  // Bounds are computed from positions, not triangles, so dropping a triangle
  // does not change the bounds directly.
  // But degenerate triangles (area 0) are still in mesh.indices but dropped at rasterization fabs(area)<1e-6, so they don't affect 2D bbox but their vertices still affect 3D bounds.
  std::cout<<"bounds min "<<r.mesh.bounds.min.x<<","<<r.mesh.bounds.min.y<<","<<r.mesh.bounds.min.z<<" max "<<r.mesh.bounds.max.x<<","<<r.mesh.bounds.max.y<<","<<r.mesh.bounds.max.z<<"\n";
  return 0;
}
