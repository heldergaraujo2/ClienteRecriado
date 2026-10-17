// Ciclo: ciclo13 item5
// Pergunta: Medição culling escala real 8406 bounds reais total_bmd 10090 with_bounds 8406 inside 7 outside 233 intersect 8166 culled 2,77% frustum 0..10 X
// Evidência: docs/evidence/ciclo13_cull_scale.log
// Origem: /tmp/cull_scale.cpp movido para backup permanente ciclo15 Parte A
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

int main(int argc, char** argv){
  std::string root = argc>1 ? argv[1] : "/tmp/cliente_ref/Data";
  std::vector<mu::FileEntry> files;
  mu::WalkFiles(root,&files);
  size_t total=0, with_bounds=0;
  size_t inside=0, outside=0, intersect=0, invalid=0;
  // Frustum that contains 0..10 in X, unbounded Y/Z (same as unit test)
  mu::Frustum f{};
  f.planes[0].normal = {1,0,0}; f.planes[0].d=0; // x>=0
  f.planes[1].normal = {-1,0,0}; f.planes[1].d=10; // x<=10
  for(int i=2;i<6;++i){ f.planes[i].normal={0,1,0}; f.planes[i].d=1e6f; }

  for(auto& fe: files){
    if(mu::ExtensionOf(fe.path)!=".bmd") continue;
    ++total;
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
    // Build mesh positions from body.vertices
    std::vector<mu::Vec3> positions;
    positions.reserve(body.vertices.size());
    for(auto& v: body.vertices) positions.push_back({v.x, v.y, v.z});
    mu::Bounds b = mu::ComputeBounds(positions);
    if(!b.valid){ ++invalid; continue; }
    ++with_bounds;
    auto cr = mu::CullBox(f, b);
    if(cr==mu::CullResult::kInside) ++inside;
    else if(cr==mu::CullResult::kOutside) ++outside;
    else ++intersect;
  }
  printf("total_bmd %zu with_bounds %zu invalid %zu inside %zu outside %zu intersect %zu\n", total, with_bounds, invalid, inside, outside, intersect);
  printf("culled %zu total %zu culled_ratio %.2f%%\n", outside, with_bounds, with_bounds? (100.0*outside/with_bounds):0);
  return 0;
}
