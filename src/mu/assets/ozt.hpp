// SPDX-License-Identifier: MIT
// OZT reader - MU Online's 32-bit BGRA texture container.
//
// LAYOUT (derived empirically, see docs/ASSET_FORMATS.md "OZT" for the
// measurements that established each field):
//
//   offset 0  : 4-byte signature, observed as 00 00 02 00
//   offset 4  : standard 18-byte TGA image header
//                 [4]  colour-map length (id length)
//                 [5]  colour-map type
//                 [6]  image type          (2 = uncompressed truecolour)
//                 [7]  colour-map first index (u16)
//                 [9]  colour-map length      (u16)
//                 [11] colour-map entry size
//                 [12] x origin (u16)
//                 [14] y origin (u16)
//                 [16] width  (u16, little endian)
//                 [18] height (u16, little endian)
//                 [20] bits per pixel      (32 in every sample measured)
//                 [21] image descriptor    (8 = 8 alpha bits)
//   offset 22 : width * height * 4 bytes, packed BGRA, bottom row first
//   trailer   : 26-byte TGA footer, present in some files and absent in others
//
// So a complete file is either 22 + w*h*4 or 48 + w*h*4 bytes long. The loader
// accepts both because both occur in the wild; it never trusts the length
// alone, it validates the dimensions against the actual byte count.
#ifndef MU_ASSETS_OZT_HPP
#define MU_ASSETS_OZT_HPP

#include <cstdint>

#include "mu/assets/format.hpp"

namespace mu {

struct OztImage {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t bits_per_pixel = 0;
  uint8_t image_type = 0;      ///< From the embedded TGA header.
  bool has_footer = false;     ///< Whether a 26-byte TGA footer is present.

  /// Bytes present after the pixel payload. 26 is the standard TGA footer;
  /// 0 and 30 also occur in the reference asset base. Reported rather than
  /// rejected, because the payload is fully determined by the header.
  std::size_t trailer_size = 0;

  /// Offset of the first pixel byte. Normally 22, but 26 for the variant that
  /// carries an extra 4-byte marker in front of the signature.
  std::size_t pixel_offset = 0;

  /// Decoded pixels, BGRA order, row 0 = bottom row (TGA convention).
  /// Empty unless OztRead() is asked to decode.
  std::vector<uint8_t> pixels;

  std::size_t pixel_bytes() const {
    return static_cast<std::size_t>(width) * height * (bits_per_pixel / 8);
  }
};

/// Validates an OZT header and fills in the geometry, without copying pixels.
/// Use this for inventories and integrity checks; it touches 22 bytes.
Status OztProbe(ByteView file, OztImage* out);

/// Validates and copies the pixel payload into out->pixels.
Status OztRead(ByteView file, OztImage* out);

/// Converts the native BGRA bottom-up payload to top-down RGBA, which is what
/// glTexImage2D wants for GL_RGBA/GL_UNSIGNED_BYTE. Returns false on overflow.
bool OztToRgbaTopDown(const OztImage& img, std::vector<uint8_t>* out);

/// Writes a valid OZT from RGBA top-down pixels. Used to generate test
/// fixtures from scratch - the repository contains no original assets.
bool OztWriteRgba(uint32_t width, uint32_t height, const uint8_t* rgba,
                  bool with_footer, std::vector<uint8_t>* out);

}  // namespace mu

#endif  // MU_ASSETS_OZT_HPP
