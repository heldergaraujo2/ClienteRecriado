// Ciclo: ciclo13 item10
// Pergunta: Benchmark OztToBgraTopDown GL_BGRA direct 10,2× speedup, CAND-001 → OPT-005
// Evidência: docs/evidence/ciclo13_bench_after.log
// Origem: /tmp/bench_ozt2.cpp movido para backup permanente ciclo15 Parte A
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO — não reproduzível só pela documentação
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include <cstdio>
#include <vector>
#include <chrono>
#include <cstring>
#include "mu/assets/ozt.hpp"

bool OztToRgbaTopDownOld(const mu::OztImage& img, std::vector<uint8_t>* out) {
  if (!out || img.bits_per_pixel != 32) return false;
  const std::size_t row = static_cast<std::size_t>(img.width) * 4;
  if (img.pixels.size() < row * img.height) return false;
  out->resize(row * img.height);
  for (uint32_t y = 0; y < img.height; ++y) {
    const uint8_t* src = img.pixels.data() + row * (img.height - 1 - y);
    uint8_t* dst = out->data() + row * y;
    for (uint32_t x = 0; x < img.width; ++x) {
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = src[3];
      src += 4;
      dst += 4;
    }
  }
  return true;
}

bool OztToBgraTopDown(const mu::OztImage& img, std::vector<uint8_t>* out) {
  if (!out || img.bits_per_pixel != 32) return false;
  const std::size_t row = static_cast<std::size_t>(img.width) * 4;
  if (img.pixels.size() < row * img.height) return false;
  out->resize(row * img.height);
  for (uint32_t y = 0; y < img.height; ++y) {
    const uint8_t* src = img.pixels.data() + row * (img.height - 1 - y);
    uint8_t* dst = out->data() + row * y;
    std::memcpy(dst, src, row);
  }
  return true;
}

int main(){
  mu::OztImage img;
  img.width=256; img.height=256; img.bits_per_pixel=32;
  img.pixels.resize(256*256*4);
  for(size_t i=0;i<img.pixels.size();++i) img.pixels[i]=static_cast<uint8_t>(i*7);
  std::vector<uint8_t> out;
  auto bench = [](auto fn, const mu::OztImage& im, const char* name){
    std::vector<uint8_t> out;
    const int N=20000;
    auto t0=std::chrono::high_resolution_clock::now();
    for(int i=0;i<N;++i) fn(im,&out);
    auto t1=std::chrono::high_resolution_clock::now();
    double us = std::chrono::duration<double, std::micro>(t1-t0).count()/N;
    printf("%s median %.1f us\n", name, us);
  };
  bench(OztToRgbaTopDownOld, img, "old BGRA->RGBA+flip");
  bench(OztToBgraTopDown, img, "new BGRA copy+flip (GL_BGRA path)");
  return 0;
}
