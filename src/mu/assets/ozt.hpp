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
  uint8_t image_type = 0;
  bool has_footer = false;
  std::size_t trailer_size = 0;
  std::size_t pixel_offset = 0;
  std::vector<uint8_t> pixels;
  std::size_t pixel_bytes() const {
    return static_cast<std::size_t>(width) * height * (bits_per_pixel / 8);
  }
};

Status OztProbe(ByteView file, OztImage* out);
/// Copies the declared, bottom-up pixel payload after a successful probe.
Status OztRead(ByteView file, OztImage* out);
/// Converts 24-bit BGR or 32-bit BGRA pixels to top-down RGBA8.
bool OztToRgbaTopDown(const OztImage& img, std::vector<uint8_t>* out);
/// Converts 24-bit BGR or 32-bit BGRA pixels to top-down BGRA8.
/// 24-bit inputs receive an opaque alpha byte; 32-bit alpha is preserved.
bool OztToBgraTopDown(const OztImage& img, std::vector<uint8_t>* out);
bool OztWriteRgba(uint32_t width, uint32_t height, const uint8_t* rgba,
                  bool with_footer, std::vector<uint8_t>* out);

}  // namespace mu

#endif  // MU_ASSETS_OZT_HPP
