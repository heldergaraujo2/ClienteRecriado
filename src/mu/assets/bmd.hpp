// SPDX-License-Identifier: MIT
// Bux cipher and BMD container handling.
//
// Two separate findings live here, because they were established separately:
//
// 1) The "bux" stream cipher. A repeating 3-byte XOR with the key
//    { 0xFC, 0xCF, 0xAB }. Verified, not assumed: the reference client ships
//    both an encrypted and a plaintext terrain attribute file whose contents
//    are otherwise identical. Decrypting the encrypted one with this key
//    reproduces the plaintext one byte for byte over the compared prefix.
//    See docs/ASSET_FORMATS.md "Bux cipher" for the measurement.
//
// 2) The BMD model container. Header is the 3 ASCII bytes "BMD" plus a version
//    byte. Version 0x0C files carry a little-endian u32 payload length at
//    offset 4 and the payload starts at offset 8; file size equals 8 plus that
//    length. Version 0x0A files use an older layout with no length field.
//
//    The version 0x0C payload is NOT protected by the bux cipher. Its byte
//    entropy measures 7.99 bits/byte and a shift-coincidence scan finds no
//    repeating-XOR period, so it is a stream cipher or a compressed block whose
//    algorithm this project has not recovered. Recovering it would require the
//    loader in the original engine, which sits inside a VMProtect-protected
//    module. Status: BLOCKED, tracked in docs/TEST_RESULTS.md.
#ifndef MU_ASSETS_BMD_HPP
#define MU_ASSETS_BMD_HPP

#include <cstdint>

#include "mu/assets/format.hpp"

namespace mu {

/// The 3-byte repeating key. Exposed so the verification test can state which
/// key it used instead of hiding a magic constant inside the implementation.
constexpr uint8_t kBuxKey[3] = {0xFC, 0xCF, 0xAB};
constexpr std::size_t kBuxKeyLen = 3;

/// Applies the bux cipher. XOR with a repeating key is its own inverse, so
/// this one function both encrypts and decrypts.
void BuxCrypt(uint8_t* data, std::size_t size);
void BuxCrypt(uint8_t* data, std::size_t size, std::size_t phase_offset);

/// Shannon entropy in bits per byte. Used to tell an opaque encrypted payload
/// from a plaintext one without knowing either format.
double ByteEntropy(const uint8_t* data, std::size_t size);

enum class BmdKind {
  kUnknown,         ///< Not recognisable as BMD.
  kPlainV0C,        ///< "BMD" 0x0C + u32 length + payload.
  kPlainV0A,        ///< "BMD" 0x0A, legacy layout, no length field.
  kPlainOtherVer,   ///< "BMD" with some other version byte.
  kBuxContainer,    ///< Whole file bux-encrypted; not necessarily a model.
};

const char* BmdKindName(BmdKind k);

struct BmdInfo {
  BmdKind kind = BmdKind::kUnknown;
  uint8_t version = 0;
  uint32_t declared_payload_size = 0;
  std::size_t payload_offset = 0;
  std::size_t payload_size = 0;
  /// True when the size field is consistent with the file length. Only
  /// meaningful for kPlainV0C.
  bool length_consistent = false;
  /// Entropy of the payload. ~8.0 means encrypted or compressed.
  double payload_entropy = 0.0;
};

/// Classifies a file and validates the container, without decrypting anything.
Status BmdProbe(ByteView file, BmdInfo* out);

/// True when the first three bytes are the bux-encrypted form of "BMD",
/// i.e. the file is a whole-file bux container.
bool LooksLikeBuxContainer(ByteView file);

/// Measures how much decrypting with the bux key raises the fraction of zero
/// bytes. Plaintext that contains runs of zero padding encrypts to runs of the
/// key itself, so a real bux file shows a large positive gain while an
/// unrelated file shows none.
///
/// This is the test that catches files which are bux-encrypted but are *not*
/// models. Data/gate.bmd in the reference client is one: it opens with four
/// repetitions of the key (twelve plaintext zero bytes) and decrypts to a data
/// blob with no "BMD" magic anywhere in it. Keying detection on the magic
/// alone would have classified it as unknown.
double BuxDecryptGain(ByteView file, std::size_t sample = 4096);

}  // namespace mu

#endif  // MU_ASSETS_BMD_HPP
