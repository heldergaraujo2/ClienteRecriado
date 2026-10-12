// Ciclo: ciclo16 B
// Pergunta: Lista bad_indices por arquivo, encontra máximas 29.49% Furniture06 etc, mede taxas Object1 14.22% Player 3.47%
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
    size_t total_refs = body.triangles.size()*3;
    size_t bad = r.mesh.bad_indices;
    double rate = total_refs? (double)bad*100.0/total_refs : 0;
    if(rate>10) printf("%s bad %zu total_tri %zu rate %.2f%% verts %zu tris_prod %zu\n", p.path().c_str(), bad, body.triangles.size(), rate, r.mesh.positions.size(), r.mesh.indices.size()/3);
  }
  return 0;
}
