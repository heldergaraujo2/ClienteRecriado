// SPDX-License-Identifier: MIT
#include "mu/assets/terrain.hpp"

#include <cstring>

#include "mu/assets/bmd.hpp"

namespace mu {
namespace {
constexpr std::size_t kGridArea =
    static_cast<std::size_t>(kTerrainGridSize) * kTerrainGridSize;
constexpr std::size_t kProbeBytes = 4096;
}  // namespace

const char* TerrainKindName(TerrainKind k) {
  switch (k) {
    case TerrainKind::kAttribute1: return "ATTR_1B";
    case TerrainKind::kAttribute2: return "ATTR_2B";
    case TerrainKind::kMap3: return "MAP_3B";
    case TerrainKind::kObject: return "OBJECT";
    case TerrainKind::kUnknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

double SmallByteDensity(const uint8_t* data, std::size_t n) {
  if (n == 0) return 0.0;
  std::size_t small = 0;
  for (std::size_t i = 0; i < n; ++i)
    if (data[i] < 0x40) ++small;
  return static_cast<double>(small) / static_cast<double>(n);
}

Status TerrainProbe(ByteView file, TerrainInfo* out) {
  if (!out) return Status::Make(Status::kIoError, "null output");
  *out = TerrainInfo{};

  if (file.size < 16)
    return Status::Make(Status::kTruncated, "file too small to be terrain");

  // Match the measured size families. The header length is derived from the
  // residue of the payload length modulo the grid area, so an unseen map size
  // still classifies as long as it is a whole number of grids.
  const std::size_t size = file.size;
  struct Candidate {
    std::size_t bytes_per_tile;
    TerrainKind kind;
  };
  static const Candidate kCandidates[] = {
      {1, TerrainKind::kAttribute1},
      {2, TerrainKind::kAttribute2},
      {3, TerrainKind::kMap3},
  };

  for (const Candidate& c : kCandidates) {
    const std::size_t payload = kGridArea * c.bytes_per_tile;
    if (size <= payload) continue;
    const std::size_t header = size - payload;
    // All observed headers are 4 bytes or fewer. Anything larger means this is
    // not the family we think it is.
    if (header > 4) continue;
    out->kind = c.kind;
    out->header_size = header;
    out->payload_size = payload;
    out->grid_consistent = true;
    break;
  }

  if (out->kind == TerrainKind::kUnknown) {
    // .obj files are variable-length object lists; they have no fixed grid.
    out->kind = TerrainKind::kObject;
    out->grid_consistent = false;
    out->payload_size = size;
  }

  const std::size_t n = size < kProbeBytes ? size : kProbeBytes;
  out->small_byte_density = SmallByteDensity(file.data, n);
  // Measured separation: plaintext terrain probes around 0.70-0.99, ciphertext
  // sits near 0.25 (a quarter of the byte range is below 0x40). 0.55 sits
  // between them with margin on both sides.
  out->looks_plaintext = out->small_byte_density > 0.55;
  return Status::Ok();
}

bool TerrainTryBuxDecrypt(std::vector<uint8_t>* data) {
  if (!data || data->empty()) return false;
  const std::size_t n = data->size() < kProbeBytes ? data->size() : kProbeBytes;
  const double before = SmallByteDensity(data->data(), n);
  if (before > 0.55) return false;  // already plaintext

  std::vector<uint8_t> trial(data->data(), data->data() + n);
  BuxCrypt(trial.data(), trial.size());
  const double after = SmallByteDensity(trial.data(), trial.size());

  // Require a decisive improvement, not a marginal one, so that the Enc*
  // family (where bux does nothing useful) is left untouched.
  if (after <= before + 0.10) return false;

  BuxCrypt(data->data(), data->size());
  return true;
}

}  // namespace mu

namespace mu {

Status TerrainParseHeight(ByteView file, TerrainHeight* out) {
  *out = TerrainHeight{};
  const std::size_t body =
      static_cast<std::size_t>(kTerrainGridSize) * kTerrainGridSize;
  if (file.size < body) {
    return Status::Make(Status::kTruncated,
                        "terrain height file shorter than one grid");
  }
  // All 76 TerrainHeight files in the reference tree are the same format: an
  // 8-bit BMP whose pixel data IS the height grid, behind a four-byte duplicated
  // prefix. This replaced an earlier description that split them into "47 BMP"
  // and "29 raw grid", which measurement refuted: the 29 are 4 + 1078 + 65536
  // bytes with an entirely zero prefix, i.e. the same BMP with its header
  // cleared. 1078 is exactly 14 (file header) + 40 (DIB) + 1024 (256x4 palette).
  //
  // The duplicated prefix means the real header starts at offset 4, not 0.
  // Measured on the 47 files whose header is legible, offset 4 gives a size
  // matching the remaining bytes in 44 of them, with pixoff 1078, DIB 40,
  // 256x256, 8 bpp. This is the same duplicated-prefix pattern the OZJ format in
  // this client uses.
  //
  // An earlier version of this function read the header at offset 0, got bpp 0
  // and pixoff 0, and fell back to reading the trailing bytes. That fallback
  // happened to produce a smooth field, so the bug was invisible until the
  // header was actually located.
  if (file.size >= 58 && file.data[0] == 'B' && file.data[1] == 'M') {
    auto rd32 = [&file](std::size_t o) {
      return static_cast<std::size_t>(file.data[o]) |
             (static_cast<std::size_t>(file.data[o + 1]) << 8) |
             (static_cast<std::size_t>(file.data[o + 2]) << 16) |
             (static_cast<std::size_t>(file.data[o + 3]) << 24);
    };
    // Try offset 4 first, which is where the real header was measured to be,
    // then offset 0 for the one file that has no duplicated prefix.
    for (std::size_t off : {std::size_t{4}, std::size_t{0}}) {
      if (off + 54 > file.size) continue;
      if (file.data[off] != 'B' || file.data[off + 1] != 'M') continue;
      const std::size_t declared = rd32(off + 2);
      const std::size_t pixoff = rd32(off + 10);
      const uint16_t bpp = static_cast<uint16_t>(file.data[off + 28]) |
                           (static_cast<uint16_t>(file.data[off + 29]) << 8);
      // Validate against the file rather than trusting the header: a header
      // whose declared size does not match the bytes actually present is the
      // symptom that sent the earlier version down the wrong path.
      if (declared != file.size - off) continue;
      if (bpp != 8) continue;
      if (pixoff + body > file.size) continue;
      out->prefix_size = off + pixoff;
      out->cells.assign(file.data + off + pixoff,
                        file.data + off + pixoff + body);
      out->complete = true;
      return Status::Ok();
    }
    out->header_ignored = true;
  }
  // No legible header, so take the body from the end of the file. In a BMP of
  // this shape the pixel data is the trailing 65,536 bytes anyway, so both paths
  // land on the same bytes -- which is why the two branches survived as long as
  // they did without producing a wrong answer.
  //
  // This covers the 29 files whose header is zeroed. header_ignored stays false
  // for them, correctly: the flag means "looked like a BMP but no header could be
  // read", and these do not look like a BMP at offset 0 at all.
  out->prefix_size = file.size - body;
  out->cells.assign(file.data + out->prefix_size, file.data + file.size);
  out->complete = out->cells.size() == body;
  return out->complete ? Status::Ok()
                       : Status::Make(Status::kTruncated,
                                      "terrain height grid incomplete");
}

double TerrainHeightSmoothness(const TerrainHeight& h) {
  const std::size_t n = h.cells.size();
  if (n <= kTerrainGridSize) return 0.0;
  // Horizontal neighbours only. Including vertical ones would mix two different
  // strides into one number and make the result harder to compare against the
  // ~85.3 expected for independent random bytes.
  unsigned long long sum = 0;
  std::size_t count = 0;
  for (std::size_t i = 1; i < n; ++i) {
    const int a = h.cells[i];
    const int b = h.cells[i - 1];
    sum += static_cast<unsigned long long>(a > b ? a - b : b - a);
    ++count;
  }
  return count ? static_cast<double>(sum) / static_cast<double>(count) : 0.0;
}

}  // namespace mu
