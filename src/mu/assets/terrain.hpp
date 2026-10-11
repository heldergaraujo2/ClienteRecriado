// SPDX-License-Identifier: MIT
// Terrain file handling: the .att / .map / .obj files under Data/World*.
//
// Measurements over the reference client's 264 terrain files:
//
//   .map  196610 bytes  (77 files)   = 2 + 256*256*3
//   .att  131076 bytes  (68 files)   = 4 + 256*256*2
//   .att   65540 bytes  (27 files)   = 4 + 256*256
//   .att   65539 bytes  ( 9 files)   = 3 + 256*256
//   .map  196609 bytes  ( 2 files)   = 1 + 256*256*3
//
// The constant 256*256 term is the terrain grid: every map is a 256x256 tile
// lattice, and the per-tile payload is 1 byte for .att, 3 bytes for .map. The
// small leading prefix differs per family and is treated as an opaque header
// rather than guessed at.
//
// Encryption state, measured rather than assumed:
//   - Some files are plaintext. Data/World1/Lorencia.att opens with
//     00 FF FF 04 04 04 ...
//   - Some files are bux-encrypted (see mu/assets/bmd.hpp). Data/World1/
//     Terrain1.att opens with FC 30 54 F8 CB AF F8 CB AF ..., which decrypts
//     to 00 FF FF 04 04 04 04 04 ..., matching the plaintext file above.
//   - The "Enc*" family uses a different scheme. Decrypting 60 of them with the
//     bux key improved the low-byte density of only 1, so bux is NOT their
//     cipher. Their algorithm is not recovered: BLOCKED, see docs/TEST_RESULTS.md.
#ifndef MU_ASSETS_TERRAIN_HPP
#define MU_ASSETS_TERRAIN_HPP

#include <cstdint>

#include "mu/assets/format.hpp"

namespace mu {

constexpr uint32_t kTerrainGridSize = 256;

enum class TerrainKind {
  kAttribute1,   ///< .att, 1 byte per tile
  kAttribute2,   ///< .att, 2 bytes per tile
  kMap3,         ///< .map, 3 bytes per tile
  kObject,       ///< .obj, variable-length object list
  kUnknown,
};

const char* TerrainKindName(TerrainKind k);

struct TerrainInfo {
  TerrainKind kind = TerrainKind::kUnknown;
  std::size_t header_size = 0;
  std::size_t payload_size = 0;
  /// True when the payload length is an exact multiple of the grid area.
  bool grid_consistent = false;

  /// Best guess at whether the payload is already plaintext. Computed from the
  /// density of small byte values, which is high for terrain attributes and
  /// near 1/4 for random bytes.
  bool looks_plaintext = false;
  double small_byte_density = 0.0;
};

/// Classifies a terrain file by length and inspects the payload statistics.
Status TerrainProbe(ByteView file, TerrainInfo* out);

/// Decrypts in place with the bux key and reports whether that made the data
/// look *more* like terrain, so callers can auto-detect without a flag.
/// Returns true when decryption was applied and accepted.
bool TerrainTryBuxDecrypt(std::vector<uint8_t>* data);

/// Fraction of bytes below 0x40 in the first n bytes. Terrain attribute and map
/// payloads are dominated by small values; ciphertext is not.
double SmallByteDensity(const uint8_t* data, std::size_t n);

}  // namespace mu

#endif  // MU_ASSETS_TERRAIN_HPP
