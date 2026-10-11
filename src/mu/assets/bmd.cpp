// SPDX-License-Identifier: MIT
#include "mu/assets/bmd.hpp"

#include <cmath>
#include <cstring>
#include <vector>

namespace mu {

void BuxCrypt(uint8_t* data, std::size_t size, std::size_t phase_offset) {
  for (std::size_t i = 0; i < size; ++i)
    data[i] = static_cast<uint8_t>(data[i] ^ kBuxKey[(i + phase_offset) % 3]);
}

void BuxCrypt(uint8_t* data, std::size_t size) { BuxCrypt(data, size, 0); }

double ByteEntropy(const uint8_t* data, std::size_t size) {
  if (size == 0) return 0.0;
  std::size_t hist[256] = {0};
  for (std::size_t i = 0; i < size; ++i) ++hist[data[i]];
  double h = 0.0;
  const double n = static_cast<double>(size);
  for (int i = 0; i < 256; ++i) {
    if (hist[i] == 0) continue;
    const double p = static_cast<double>(hist[i]) / n;
    h -= p * std::log2(p);
  }
  return h;
}

const char* BmdKindName(BmdKind k) {
  switch (k) {
    case BmdKind::kUnknown: return "UNKNOWN";
    case BmdKind::kPlainV0C: return "BMD_V0C";
    case BmdKind::kPlainV0A: return "BMD_V0A";
    case BmdKind::kPlainOtherVer: return "BMD_OTHER_VERSION";
    case BmdKind::kBuxContainer: return "BUX_CONTAINER";
  }
  return "UNKNOWN";
}

bool LooksLikeBuxContainer(ByteView file) {
  if (file.size < 4) return false;
  // "BMD" under the bux cipher. Byte 3 is not checked because it encodes the
  // version byte, which legitimately varies between files.
  static const uint8_t kEnc[3] = {
      static_cast<uint8_t>('B' ^ kBuxKey[0]),
      static_cast<uint8_t>('M' ^ kBuxKey[1]),
      static_cast<uint8_t>('D' ^ kBuxKey[2]),
  };
  return std::memcmp(file.data, kEnc, 3) == 0;
}

namespace {
double ZeroFraction(const uint8_t* p, std::size_t n) {
  if (n == 0) return 0.0;
  std::size_t z = 0;
  for (std::size_t i = 0; i < n; ++i)
    if (p[i] == 0) ++z;
  return static_cast<double>(z) / static_cast<double>(n);
}
}  // namespace

double BuxDecryptGain(ByteView file, std::size_t sample) {
  const std::size_t n = file.size < sample ? file.size : sample;
  if (n < 16) return 0.0;
  const double before = ZeroFraction(file.data, n);
  std::vector<uint8_t> trial(file.data, file.data + n);
  BuxCrypt(trial.data(), trial.size());
  return ZeroFraction(trial.data(), trial.size()) - before;
}

Status BmdProbe(ByteView file, BmdInfo* out) {
  if (!out) return Status::Make(Status::kIoError, "null output");
  *out = BmdInfo{};

  if (file.size < 4)
    return Status::Make(Status::kTruncated, "file smaller than 4 bytes");

  if (std::memcmp(file.data, "BMD", 3) == 0) {
    out->version = file.data[3];
    switch (out->version) {
      case 0x0C: {
        out->kind = BmdKind::kPlainV0C;
        if (file.size < 8)
          return Status::Make(Status::kTruncated,
                              "BMD v0x0C header is incomplete");
        out->declared_payload_size = ReadU32LE(file.data + 4);
        out->payload_offset = 8;
        // Trust the smaller of the declared length and the bytes actually
        // present, so a truncated download degrades instead of overflowing.
        const std::size_t avail = file.size - 8;
        out->payload_size =
            (out->declared_payload_size <= avail)
                ? out->declared_payload_size
                : avail;
        out->length_consistent =
            (static_cast<std::size_t>(out->declared_payload_size) + 8 ==
             file.size);
        break;
      }
      case 0x0A:
        out->kind = BmdKind::kPlainV0A;
        // Legacy layout: measurements show the u32 at offset 4 is not a length
        // (it decodes to implausible values such as 1635017028 on a 21141-byte
        // file). Do not interpret it; expose the rest of the file as payload.
        out->payload_offset = 4;
        out->payload_size = file.size - 4;
        out->length_consistent = true;  // no length field to be inconsistent
        break;
      default:
        out->kind = BmdKind::kPlainOtherVer;
        out->payload_offset = 4;
        out->payload_size = file.size - 4;
        break;
    }
  } else if (LooksLikeBuxContainer(file) || BuxDecryptGain(file) > 0.05) {
    // Either the magic decrypts to "BMD", or decrypting measurably reveals
    // zero padding. The second case covers bux-encrypted data files that are
    // not models at all but are shipped with a .bmd extension.
    out->kind = BmdKind::kBuxContainer;
    out->payload_offset = 0;
    out->payload_size = file.size;
    out->length_consistent = true;
  } else {
    return Status::Make(Status::kNotThisFormat,
                        "neither a BMD magic nor a bux-encrypted container");
  }

  // Sample the payload for entropy. Cap the sample so a 1 MB model does not
  // cost more than a 64 KB histogram; entropy is stable well before that.
  const std::size_t cap = out->payload_size < 65536 ? out->payload_size : 65536;
  if (cap > 0)
    out->payload_entropy = ByteEntropy(file.data + out->payload_offset, cap);

  return Status::Ok();
}


void BmdV0CDecrypt(uint8_t* data, std::size_t size) {
  if (data == nullptr) return;
  // The running key is derived from the previous CIPHERTEXT byte, so the value
  // must be read before it is overwritten. That ordering is the whole reason
  // this is not self-inverse the way the bux XOR is.
  uint8_t running = kBmdV0CInitialKey;
  for (std::size_t i = 0; i < size; ++i) {
    const uint8_t cipher = data[i];
    data[i] = static_cast<uint8_t>(
        static_cast<uint8_t>(cipher ^ kBmdV0CKey[i & (kBmdV0CKeyLen - 1)]) -
        running);
    running = static_cast<uint8_t>(cipher + kBmdV0CKeyStep);
  }
}

}  // namespace mu
