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
