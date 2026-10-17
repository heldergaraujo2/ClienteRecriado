// SPDX-License-Identifier: MIT
// ClienteRecriado - independent MU Online client reconstruction.
//
// Common types shared by the asset readers.
//
// IMPORTANT: the *format knowledge* encoded here was derived by inspecting the
// byte layout of the reference client's asset files (see docs/ASSET_FORMATS.md
// for the evidence). No code was copied from the original client, and no
// original asset bytes are stored in this repository.
#ifndef MU_ASSETS_FORMAT_HPP
#define MU_ASSETS_FORMAT_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace mu {

/// Result of a parse/decode attempt. Readers never throw; they return this.
struct Status {
  enum Code : int {
    kOk = 0,
    kNotThisFormat,  ///< File does not match the format signature.
    kTruncated,      ///< Declared size exceeds available bytes.
    kCorrupt,        ///< Signature matched but the body is inconsistent.
    kUnsupported,    ///< Signature matched, variant not implemented.
    kIoError,        ///< Underlying read failure.
  };

  Code code = kOk;
  std::string message;

  bool ok() const { return code == kOk; }
  explicit operator bool() const { return ok(); }

  static Status Ok() { return {}; }
  static Status Make(Code c, std::string m) { return {c, std::move(m)}; }
};

const char* StatusName(Status::Code c);

/// An in-memory view over a file. Kept non-owning so callers control lifetime
/// and can map files instead of copying them (matters for 2.4 GB of assets).
struct ByteView {
  const uint8_t* data = nullptr;
  std::size_t size = 0;

  ByteView() = default;
  ByteView(const uint8_t* d, std::size_t s) : data(d), size(s) {}

  bool empty() const { return data == nullptr || size == 0; }

  /// Safe slicing: never returns a view past the end.
  ByteView sub(std::size_t off, std::size_t len) const {
    if (off >= size) return {};
    if (off + len > size || off + len < off) len = size - off;
    return {data + off, len};
  }
};

/// Little-endian unaligned readers. The original files are x86 Windows
/// products, so little-endian is correct, but we do not assume the *host* is.
inline uint16_t ReadU16LE(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(p[1] << 8);
}
inline uint32_t ReadU32LE(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}
inline int16_t ReadI16LE(const uint8_t* p) {
  return static_cast<int16_t>(ReadU16LE(p));
}

/// Big-endian readers. JPEG is big-endian end to end, unlike every other
/// format in this client, so it needs its own accessors. Mixing the two is an
/// easy and very quiet bug: reading a JPEG segment length as little-endian
/// turns 0x0043 into 17152 and reports a healthy file as truncated.
inline uint16_t ReadU16BE(const uint8_t* p) {
  return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}
inline uint32_t ReadU32BE(const uint8_t* p) {
  return (static_cast<uint32_t>(p[0]) << 24) |
         (static_cast<uint32_t>(p[1]) << 16) |
         (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}
inline int32_t ReadI32LE(const uint8_t* p) {
  return static_cast<int32_t>(ReadU32LE(p));
}
inline float ReadF32LE(const uint8_t* p) {
  uint32_t u = ReadU32LE(p);
  float f;
  static_assert(sizeof(f) == sizeof(u), "float must be 32 bit");
  // memcpy, not a cast: type-punning through a union/cast is UB and MSVC +
  // GCC + Clang all fold this memcpy away at any optimisation level.
  std::memcpy(&f, &u, sizeof(f));
  return f;
}

/// CRC-32 (IEEE 802.3, polynomial 0xEDB88320, init 0xFFFFFFFF, final xor).
/// This is the checksum used by the reference client's patch manifest
/// (ListaPatch.txt), so we need the exact same polynomial to verify patches.
uint32_t Crc32(const void* data, std::size_t size);
uint32_t Crc32Update(uint32_t crc, const void* data, std::size_t size);

/// Reads a whole file into memory. Returns false on failure.
bool ReadFile(const std::string& path, std::vector<uint8_t>* out);

}  // namespace mu

#endif  // MU_ASSETS_FORMAT_HPP
