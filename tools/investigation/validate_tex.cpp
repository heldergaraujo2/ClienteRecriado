// Ciclo: ciclo16 D
// Pergunta: Validação objetiva textura placeholder checkerboard bimodal bright/dark hasBoth, prova amostragem textura correta por código não subjetiva
// Evidência: docs/evidence/ciclo16_textured_validation.log
// Origem: /tmp/validate_tex.cpp movido para backup permanente ciclo16 para garantir que todo trabalho está salvo no repositório
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/mesh.hpp"
#include "mu/fs.hpp"
#include "mu/render/software_rasterizer.hpp"
#include <cstdio>
#include <vector>
#include <cmath>
#include <set>

int main(){
  std::string bmdPath="/tmp/cliente_ref/Data/Object1/Beer01.bmd";
  std::string ppmPath="/tmp/ciclo16_beer_textured.ppm";
  // Load mesh to get bounds for placeholder UV calculation (same as rasterizer)
  std::vector<uint8_t> raw;
  mu::ReadFile(bmdPath,&raw);
  mu::ByteView view(raw.data(),raw.size());
  mu::BmdInfo info;
  mu::BmdProbe(view,&info);
  mu::ByteView payload=view.sub(info.payload_offset, info.payload_size);
  std::vector<uint8_t> plain;
  if(info.kind==mu::BmdKind::kPlainV0C){ plain.assign(payload.data,payload.data+payload.size); mu::BmdV0CDecrypt(plain.data(),plain.size()); payload=mu::ByteView(plain.data(),plain.size()); }
  mu::BmdBody body;
  mu::BmdParseBodyV0A(payload,&body);
  auto r=mu::MeshFromBmd(body,bmdPath);
  mu::Mesh mesh=r.mesh;
  // Load PPM
  FILE* f=fopen(ppmPath.c_str(),"rb");
  if(!f){ printf("cannot open %s\n", ppmPath.c_str()); return 1; }
  char magic[3]={0};
  fscanf(f,"%2s",magic);
  int w,h,maxval;
  fscanf(f,"%d %d",&w,&h);
  fscanf(f,"%d",&maxval);
  fgetc(f);
  std::vector<uint8_t> data((size_t)w*h*3);
  fread(data.data(),1,data.size(),f);
  fclose(f);
  uint8_t bg_r=30, bg_g=30, bg_b=45;
  int thresh=10;
  int nonBg=0;
  int bright=0, dark=0;
  // For checkerboard validation: texture is 256x256 checkerSize 32, so each checker 32px, 8x8 checkers
  // UV placeholder: u = (pos.x - bmin.x)/dx *4, v = (pos.y - bmin.y)/dy *4 (or Z if dy small)
  // In rendered image, we can't directly get UV per pixel without re-rasterizing, but we can check that non-bg pixels have bimodal distribution (bright vs dark) due to checkerboard
  // Count bright pixels >150 and dark pixels <100 (after lighting modulation, but still bimodal)
  for(int y=0;y<h;++y){
    for(int x=0;x<w;++x){
      size_t idx=(y*w+x)*3;
      uint8_t rr=data[idx], gg=data[idx+1], bb=data[idx+2];
      if(std::abs((int)rr-bg_r)>thresh || std::abs((int)gg-bg_g)>thresh || std::abs((int)bb-bg_b)>thresh){
        ++nonBg;
        int avg=(rr+gg+bb)/3;
        if(avg>120) ++bright;
        else if(avg<80) ++dark;
      }
    }
  }
  printf("Beer01 textured: w %d h %d nonBg %d bright %d dark %d\n", w,h,nonBg,bright,dark);
  // Objective validation: texture placeholder should produce both bright and dark pixels in non-bg area
  // If texture sampling works, we expect both bright and dark >0 and ratio not extreme
  // For checkerboard 50% white 50% black, after lighting modulation, we expect bright and dark both significant
  bool hasBoth = bright>0 && dark>0;
  double brightRatio = nonBg? (double)bright*100.0/nonBg : 0;
  double darkRatio = nonBg? (double)dark*100.0/nonBg : 0;
  printf("brightRatio %.1f%% darkRatio %.1f%% hasBoth %d\n", brightRatio, darkRatio, hasBoth?1:0);
  // Also check that non-textured version had only grayscale (R==G==B) and textured has same but bimodal? Actually both grayscale because checkerboard is grayscale, but textured should have variation
  // For objective proof that texture is sampled: compare with non-textured PPM
  std::string ppmPlain="/tmp/ciclo16_beer_a.ppm";
  FILE* f2=fopen(ppmPlain.c_str(),"rb");
  if(f2){
    char mg2[3]={0};
    fscanf(f2,"%2s",mg2);
    int w2,h2,mv2;
    fscanf(f2,"%d %d",&w2,&h2);
    fscanf(f2,"%d",&mv2);
    fgetc(f2);
    std::vector<uint8_t> data2((size_t)w2*h2*3);
    fread(data2.data(),1,data2.size(),f2);
    fclose(f2);
    int nonBg2=0;
    int uniqueColors=0;
    std::set<int> colors;
    for(int y=0;y<h2;++y) for(int x=0;x<w2;++x){
      size_t idx=(y*w2+x)*3;
      uint8_t rr=data2[idx], gg=data2[idx+1], bb=data2[idx+2];
      if(std::abs((int)rr-bg_r)>thresh || std::abs((int)gg-bg_g)>thresh || std::abs((int)bb-bg_b)>thresh){
        ++nonBg2;
        colors.insert(rr); // grayscale so R is intensity
      }
    }
    printf("Beer01 plain: nonBg %d unique intensity values %zu\n", nonBg2, colors.size());
    // Textured should have at least as many unique intensities due to checkerboard modulating
    // Actually plain also has varying intensity due to lambert shading, but textured should have more variation due to checkerboard
    // Let's compute unique colors in textured
    std::set<int> colorsTex;
    for(int y=0;y<h;++y) for(int x=0;x<w;++x){
      size_t idx=(y*w+x)*3;
      uint8_t rr=data[idx], gg=data[idx+1], bb=data[idx+2];
      if(std::abs((int)rr-bg_r)>thresh || std::abs((int)gg-bg_g)>thresh || std::abs((int)bb-bg_b)>thresh){
        colorsTex.insert(rr);
      }
    }
    printf("Beer01 textured: unique intensity values %zu\n", colorsTex.size());
  }
  // Final objective check: PASS if hasBoth and brightRatio and darkRatio both >10% and <90%
  bool pass = hasBoth && brightRatio>10 && brightRatio<90 && darkRatio>10 && darkRatio<90;
  printf("Validation textured sampling: %s (expected bimodal checkerboard, objective, not subjective)\n", pass?"PASS":"FAIL");
  return pass?0:1;
}
