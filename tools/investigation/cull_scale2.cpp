// Ciclo: ciclo13 item5
// Pergunta: Medição culling escala real identity VP -1..1 clip cube with_bounds 8406 inside 2 outside 662 intersect 7742 culled 7,88%
// Evidência: docs/evidence/ciclo13_cull_scale.log
// Origem: /tmp/cull_scale2.cpp movido para backup permanente ciclo15 Parte A
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO — não reproduzível só pela documentação
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include <cstdio>
#include <vector>
#include <string>
#include "mu/fs.hpp"
#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/mesh.hpp"
#include "mu/render/cull.hpp"
bool Printable(mu::ByteView p){ if(p.size<32) return false; for(size_t i=0;i<32;++i){ uint8_t c=p.data[i]; if(c!=0 && (c<32||c>126)) return false; } return true; }
int main(){
  std::string root="/tmp/cliente_ref/Data";
  std::vector<mu::FileEntry> files;
  mu::WalkFiles(root,&files);
  size_t with_bounds=0, inside=0, outside=0, intersect=0;
  float ident[16]={0}; ident[0]=ident[5]=ident[10]=ident[15]=1.0f;
  mu::Frustum f = mu::FrustumFromViewProjection(ident);
  for(auto& fe: files){
    if(mu::ExtensionOf(fe.path)!=".bmd") continue;
    std::vector<uint8_t> raw; if(!mu::ReadFile(fe.path,&raw)) continue;
    mu::ByteView view(raw.data(), raw.size());
    mu::BmdInfo info; if(!mu::BmdProbe(view,&info) || !info.length_consistent) continue;
    mu::ByteView cipher=view.sub(info.payload_offset, info.payload_size);
    std::vector<uint8_t> plain; mu::ByteView payload=cipher;
    if(info.kind==mu::BmdKind::kPlainV0C){
      plain.assign(cipher.data, cipher.data+cipher.size);
      mu::BmdV0CDecrypt(plain.data(), plain.size());
      if(!Printable(mu::ByteView(plain.data(), plain.size()))) continue;
      payload=mu::ByteView(plain.data(), plain.size());
    }
    mu::BmdBody body;
    if(!mu::BmdParseBodyV0A(payload,&body) || !body.complete) continue;
    std::vector<mu::Vec3> positions;
    for(auto& v: body.vertices) positions.push_back({v.x, v.y, v.z});
    mu::Bounds b = mu::ComputeBounds(positions);
    if(!b.valid) continue;
    ++with_bounds;
    auto cr = mu::CullBox(f,b);
    if(cr==mu::CullResult::kInside) ++inside;
    else if(cr==mu::CullResult::kOutside) ++outside;
    else ++intersect;
  }
  printf("identity VP frustum -1..1 clip cube: with_bounds %zu inside %zu outside %zu intersect %zu culled_ratio %.2f%%\n", with_bounds, inside, outside, intersect, with_bounds? (100.0*outside/with_bounds):0);
  return 0;
}
