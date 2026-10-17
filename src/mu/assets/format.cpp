// SPDX-License-Identifier: MIT
#include "mu/assets/format.hpp"

#include <cstdio>

namespace mu {

const char* StatusName(Status::Code c) {
  switch (c) {
    case Status::kOk: return "OK";
    case Status::kNotThisFormat: return "NOT_THIS_FORMAT";
    case Status::kTruncated: return "TRUNCATED";
    case Status::kCorrupt: return "CORRUPT";
    case Status::kUnsupported: return "UNSUPPORTED";
    case Status::kIoError: return "IO_ERROR";
  }
  return "UNKNOWN";
}

namespace {
// Slice-by-8 CRC-32 tables (reflected polynomial 0xEDB88320).
//
// The byte-at-a-time version measures 391 MB/s on the development machine,
// which dominates `mutool verify` on a 2.4 GB asset tree: the checksum pass
// alone would take around six seconds. Slice-by-8 processes eight bytes per
// iteration from eight 256-entry tables, and measures roughly 3 GB/s on the
// same machine, so verification becomes I/O bound instead of CPU bound.
// See docs/OPTIMIZATION_LOG.md for the measurement.
struct CrcTables {
  uint32_t t[8][256];
  CrcTables() {
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = i;
      for (int k = 0; k < 8; ++k)
        c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
      t[0][i] = c;
    }
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = t[0][i];
      for (int s = 1; s < 8; ++s) {
        c = t[0][c & 0xFF] ^ (c >> 8);
        t[s][i] = c;
      }
    }
  }
};

const CrcTables& tables() {
  static const CrcTables kTables;
  return kTables;
}
}  // namespace

uint32_t Crc32Update(uint32_t crc, const void* data, std::size_t size) {
  const uint8_t* p = static_cast<const uint8_t*>(data);
  const CrcTables& T = tables();
  uint32_t c = crc ^ 0xFFFFFFFFu;

  // Eight bytes at a time while enough input remains.
  std::size_t i = 0;
  for (; i + 8 <= size; i += 8) {
    uint32_t lo, hi;
    std::memcpy(&lo, p + i, 4);
    std::memcpy(&hi, p + i + 4, 4);
    lo ^= c;
    c = T.t[7][lo & 0xFF] ^ T.t[6][(lo >> 8) & 0xFF] ^
        T.t[5][(lo >> 16) & 0xFF] ^ T.t[4][(lo >> 24) & 0xFF] ^
        T.t[3][hi & 0xFF] ^ T.t[2][(hi >> 8) & 0xFF] ^
        T.t[1][(hi >> 16) & 0xFF] ^ T.t[0][(hi >> 24) & 0xFF];
  }
  for (; i < size; ++i) c = T.t[0][(c ^ p[i]) & 0xFF] ^ (c >> 8);

  return c ^ 0xFFFFFFFFu;
}

uint32_t Crc32(const void* data, std::size_t size) {
  return Crc32Update(0, data, size);
}

bool ReadFile(const std::string& path, std::vector<uint8_t>* out) {
  out->clear();
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  if (std::fseek(f, 0, SEEK_END) != 0) {
    std::fclose(f);
    return false;
  }
  long n = std::ftell(f);
  if (n < 0) {
    std::fclose(f);
    return false;
  }
  std::rewind(f);
  out->resize(static_cast<std::size_t>(n));
  if (n > 0) {
    std::size_t got = std::fread(out->data(), 1, out->size(), f);
    if (got != out->size()) {
      std::fclose(f);
      out->clear();
      return false;
    }
  }
  std::fclose(f);
  return true;
}

}  // namespace mu
