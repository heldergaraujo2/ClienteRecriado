// Ciclo: ciclo14 B
// Pergunta: Debug Beer01.bmd triângulos degenerados v1,1,1 area 0, por que rasterizador só 6/30 tris
// Evidência: docs/evidence/ciclo14_first_render_validation.md
// Origem: /tmp/debug_beer.cpp movido para backup permanente ciclo15 Parte A
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO — não reproduzível só pela documentação
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/mesh.hpp"
#include "mu/fs.hpp"
#include <iostream>
int main(){
  std::string path="/tmp/cliente_ref/Data/Object1/Beer01.bmd";
  std::vector<uint8_t> raw;
  mu::ReadFile(path,&raw);
  mu::ByteView view1(raw.data(), raw.size());
  mu::BmdInfo info;
  mu::BmdProbe(view1,&info);
  mu::ByteView payload=view1.sub(info.payload_offset, info.payload_size);
  mu::BmdBody body;
  mu::BmdParseBodyV0A(payload,&body);
  std::cout<<"vertices "<<body.vertices.size()<<" normals "<<body.normals.size()<<" tris "<<body.triangles.size()<<" out_of_range_v "<<body.out_of_range_vertices<<" out_of_range_n "<<body.out_of_range_normals<<"\n";
  for(size_t i=0;i<std::min<size_t>(10, body.triangles.size());++i){
    auto &t=body.triangles[i];
    std::cout<<"tri "<<i<<" corners ";
    for(int k=0;k<3;++k) std::cout<<t.corners[k].vertex<<","<<t.corners[k].normal<<" ";
    std::cout<<"\n";
  }
  auto r=mu::MeshFromBmd(body,path);
  std::cout<<"mesh pos "<<r.mesh.positions.size()<<" indices "<<r.mesh.indices.size()<<" dropped "<<r.mesh.dropped_triangles<<"\n";
  for(size_t i=0;i<std::min<size_t>(10, r.mesh.indices.size()/3);++i){
    std::cout<<"mesh tri "<<i<<" "<<r.mesh.indices[i*3]<<","<<r.mesh.indices[i*3+1]<<","<<r.mesh.indices[i*3+2]<<"\n";
  }
  return 0;
}
