# Ciclo: ciclo13 item10
# Pergunta: Atualiza ozt.cpp com otimização 2px + BGRA direct OztToBgraTopDown, implementar OPT-005
# Evidência: docs/evidence/ciclo13_bench_after.log
# Origem: /tmp/update_ozt.py movido para backup permanente ciclo15 Parte A
# Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
# Data backup: 2026-10-12
# Branch: arena/519f8fc9-clienterecriado

path="src/mu/assets/ozt.cpp"
text=open(path,encoding='utf-8').read()
old="""bool OztToRgbaTopDown(const OztImage& img, std::vector<uint8_t>* out) {
  if (!out || img.bits_per_pixel != 32) return false;
  const std::size_t row = static_cast<std::size_t>(img.width) * 4;
  if (img.pixels.size() < row * img.height) return false;
  out->resize(row * img.height);
  for (uint32_t y = 0; y < img.height; ++y) {
    // Flip vertically: TGA stores the bottom row first, GL expects the top row
    // first when uploading with GL_UNPACK_ROW_LENGTH defaults.
    const uint8_t* src = img.pixels.data() + row * (img.height - 1 - y);
    uint8_t* dst = out->data() + row * y;
    for (uint32_t x = 0; x < img.width; ++x) {
      dst[0] = src[2];  // R <- B
      dst[1] = src[1];  // G <- G
      dst[2] = src[0];  // B <- R
      dst[3] = src[3];  // A <- A
      src += 4;
      dst += 4;
    }
  }
  return true;
}"""
new="""bool OztToRgbaTopDown(const OztImage& img, std::vector<uint8_t>* out) {
  if (!out || img.bits_per_pixel != 32) return false;
  const std::size_t row = static_cast<std::size_t>(img.width) * 4;
  if (img.pixels.size() < row * img.height) return false;
  out->resize(row * img.height);
  // Optimized: process 2 pixels (8B) at a time via uint64_t with BGRA->RGBA swap via bit ops
  // BGRA little-endian uint32_t = B | G<<8 | R<<16 | A<<24, desired RGBA = R | G<<8 | B<<16 | A<<24
  // = (pixel & 0xFF00FF00) | ((pixel & 0xFF)<<16) | ((pixel>>16)&0xFF)
  for (uint32_t y = 0; y < img.height; ++y) {
    const uint8_t* src = img.pixels.data() + row * (img.height - 1 - y);
    uint8_t* dst = out->data() + row * y;
    uint32_t x=0;
    for (; x+1 < img.width; x+=2) {
      uint64_t chunk;
      std::memcpy(&chunk, src, 8);
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

bool OztToBgraTopDown(const OztImage& img, std::vector<uint8_t>* out) {
  // CAND-001 / OPT-005: BGRA upload direct via GL_BGRA (GL_EXT_bgra universal desktop GL)
  // Elimina swap R<->B, apenas flip vertical via memcpy — 10,2× speedup medido 66,4us->6,5us 256x256
  // Usar com glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, data)
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
}"""
text=text.replace(old,new)
open(path,'w',encoding='utf-8').write(text)
print("updated ozt")
