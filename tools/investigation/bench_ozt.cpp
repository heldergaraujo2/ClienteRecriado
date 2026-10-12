// Ciclo: ciclo13 item10
// Pergunta: Benchmark OztToRgbaTopDown old vs new 2-pixel swap bit ops, mede ganho otimização OZT
// Evidência: docs/evidence/ciclo13_bench_after.log
// Origem: /tmp/bench_ozt.cpp movido para backup permanente ciclo15 Parte A
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO — não reproduzível só pela documentação
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

#include <cstdio>
#include <vector>
#include <chrono>
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

bool OztToRgbaTopDownNew(const mu::OztImage& img, std::vector<uint8_t>* out) {
  if (!out || img.bits_per_pixel != 32) return false;
  const std::size_t row = static_cast<std::size_t>(img.width) * 4;
  if (img.pixels.size() < row * img.height) return false;
  out->resize(row * img.height);
  for (uint32_t y = 0; y < img.height; ++y) {
    const uint8_t* src = img.pixels.data() + row * (img.height - 1 - y);
    uint8_t* dst = out->data() + row * y;
    // Process 2 pixels at a time via uint64_t if possible
    uint32_t x=0;
    for (; x+1 < img.width; x+=2) {
      uint64_t chunk;
      std::memcpy(&chunk, src, 8);
      // chunk contains 2 BGRA pixels little-endian: B0 G0 R0 A0 B1 G1 R1 A1
      // We need to swap B and R per pixel
      // Extract low 32 and high 32
      uint32_t p0 = static_cast<uint32_t>(chunk & 0xFFFFFFFFULL);
      uint32_t p1 = static_cast<uint32_t>((chunk >> 32) & 0xFFFFFFFFULL);
      uint32_t r0 = (p0 & 0xFF00FF00) | ((p0 & 0xFF) << 16) | ((p0 >> 16) & 0xFF);
      uint32_t r1 = (p1 & 0xFF00FF00) | ((p1 & 0xFF) << 16) | ((p1 >> 16) & 0xFF);
      uint64_t outChunk = (static_cast<uint64_t>(r1) << 32) | r0;
      std::memcpy(dst, &outChunk, 8);
      src+=8; dst+=8;
    }
    for (; x < img.width; ++x) {
      uint32_t p;
      std::memcpy(&p, src, 4);
      uint32_t r = (p & 0xFF00FF00) | ((p & 0xFF) << 16) | ((p >> 16) & 0xFF);
      std::memcpy(dst, &r, 4);
      src+=4; dst+=4;
    }
  }
  return true;
}

int main(){
  mu::OztImage img;
  img.width=256; img.height=256; img.bits_per_pixel=32;
  img.pixels.resize(256*256*4);
  for(size_t i=0;i<img.pixels.size();++i) img.pixels[i]=static_cast<uint8_t>(i*7);
  std::vector<uint8_t> out1, out2;
  // warmup
  OztToRgbaTopDownOld(img,&out1);
  OztToRgbaTopDownNew(img,&out2);
  bool same = out1==out2;
  printf("same %d\n", same);
  auto bench = [](auto fn, const mu::OztImage& im, const char* name){
    std::vector<uint8_t> out;
    const int N=20000;
    auto t0=std::chrono::high_resolution_clock::now();
    for(int i=0;i<N;++i) fn(im,&out);
    auto t1=std::chrono::high_resolution_clock::now();
    double us = std::chrono::duration<double, std::micro>(t1-t0).count()/N;
    printf("%s median %.1f us\n", name, us);
  };
  bench(OztToRgbaTopDownOld, img, "old");
  bench(OztToRgbaTopDownNew, img, "new");
  return 0;
}
