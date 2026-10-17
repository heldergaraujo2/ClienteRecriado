// Ciclo: ciclo14 B
// Pergunta: Encontra arquivos com bones has_bones true, identifica modelos com ossos para render
// Evidência: docs/evidence/ciclo14_first_render_*.ppm
// Origem: /tmp/find_bones.cpp movido para backup permanente ciclo15 Parte A
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO — não reproduzível só pela documentação
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/fs.hpp"
#include <iostream>
int main(int argc, char** argv){
  std::string root = argc>1 ? argv[1] : "/tmp/cliente_ref/Data/Object1";
  std::vector<mu::FileEntry> files;
  mu::WalkFiles(root,&files);
  for(auto &f: files){
    if(mu::ExtensionOf(f.path)!=".bmd") continue;
    std::vector<uint8_t> raw;
    if(!mu::ReadFile(f.path,&raw)) continue;
    mu::ByteView view(raw.data(), raw.size());
    mu::BmdInfo info;
    if(!mu::BmdProbe(view,&info) || !info.length_consistent) continue;
    mu::ByteView payload=view.sub(info.payload_offset, info.payload_size);
    std::vector<uint8_t> plain;
    if(info.kind==mu::BmdKind::kPlainV0C){
      plain.assign(payload.data,payload.data+payload.size);
      mu::BmdV0CDecrypt(plain.data(),plain.size());
      bool printable=true;
      for(size_t i=0;i<32 && printable;++i){ uint8_t c=plain[i]; if(c!=0 && (c<32||c>126)) printable=false; }
      if(!printable) continue;
      payload=mu::ByteView(plain.data(),plain.size());
    }
    mu::BmdBody body;
    if(!mu::BmdParseBodyV0A(payload,&body).ok() || !body.complete) continue;
    if(body.has_bones){
      std::cout<<f.path<<" bones "<<body.bones.size()<<" verts "<<body.vertices.size()<<" tris "<<body.triangles.size()<<"\n";
    }
  }
  return 0;
}
