// Ciclo: ciclo16 B
// Question: List dropped-triangle rates by file and identify outliers.
// Evidência: docs/evidence/ciclo15_render_validation.log
// Origem: /tmp/list_bad.cpp movido para backup permanente ciclo16 para garantir que todo trabalho está salvo no repositório
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/mesh.hpp"
#include "mu/fs.hpp"
#include <filesystem>
int main(int argc,char**argv){
  std::string dir=argc>1?argv[1]:"/tmp/cliente_ref/Data/Object1";
  for(auto &p: std::filesystem::recursive_directory_iterator(dir)){
    if(!p.is_regular_file()) continue;
    if(p.path().extension()!=".bmd") continue;
    std::vector<uint8_t> raw;
    if(!mu::ReadFile(p.path().string(), &raw)) continue;
    mu::ByteView view(raw.data(), raw.size());
    mu::BmdInfo info;
    if(!mu::BmdProbe(view,&info) || !info.length_consistent) continue;
    mu::ByteView payload=view.sub(info.payload_offset, info.payload_size);
    std::vector<uint8_t> plain;
    if(info.kind==mu::BmdKind::kPlainV0C){
      plain.assign(payload.data,payload.data+payload.size);
      mu::BmdV0CDecrypt(plain.data(), plain.size());
      payload=mu::ByteView(plain.data(), plain.size());
    } else if(info.kind==mu::BmdKind::kBuxContainer){
      std::vector<uint8_t> tmp=raw;
      mu::BuxCrypt(tmp.data(), tmp.size());
      mu::ByteView view2(tmp.data(), tmp.size());
      mu::BmdInfo info2;
      if(!mu::BmdProbe(view2,&info2) || !info2.length_consistent) continue;
      raw.swap(tmp);
      view=mu::ByteView(raw.data(), raw.size());
      info=info2;
      payload=view.sub(info.payload_offset, info.payload_size);
      if(info.kind==mu::BmdKind::kPlainV0C){
        plain.assign(payload.data,payload.data+payload.size);
        mu::BmdV0CDecrypt(plain.data(), plain.size());
        payload=mu::ByteView(plain.data(), plain.size());
      }
    }
    mu::BmdBody body;
    if(!mu::BmdParseBodyV0A(payload,&body).ok() || !body.complete) continue;
    auto r=mu::MeshFromBmd(body, p.path().string());
    if(!r.ok) continue;
    size_t total_triangles = body.triangles.size();
    size_t dropped = r.mesh.dropped_triangles;
    double rate = total_triangles ? static_cast<double>(dropped) * 100.0 / static_cast<double>(total_triangles) : 0.0;
    if(rate>10) printf("%s dropped %zu total_tri %zu rate %.2f%% verts %zu tris_prod %zu\n", p.path().c_str(), dropped, total_triangles, rate, r.mesh.positions.size(), r.mesh.indices.size()/3);
  }
  return 0;
}
