// SPDX-License-Identifier: MIT
// Plain TGA reader. The reference client ships 15 loose .tga files, and the OZT
// container is itself a 4-byte signature wrapped around a TGA header, so this
// is needed in its own right rather than as a convenience.
#ifndef MU_ASSETS_TGA_HPP
#define MU_ASSETS_TGA_HPP

#include <cstdint>

#include "mu/assets/format.hpp"

namespace mu {

struct TgaImage {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t bits_per_pixel = 0;
  uint8_t image_type = 0;   ///< 2 = uncompressed truecolour, 10 = RLE.
  uint8_t id_length = 0;    ///< Length of the image ID field that follows.
  uint8_t descriptor = 0;   ///< Bit 5 is the top-to-bottom origin flag.
  bool top_to_bottom = false;

  /// Offset of the first pixel byte.
  std::size_t pixel_offset = 0;
  /// Uncompressed payload size implied by the header.
  std::size_t pixel_bytes = 0;

  std::vector<uint8_t> pixels;
};

/// Reads the 18-byte header and validates the geometry against the file size.
/// Does not copy pixels.
Status TgaProbe(ByteView file, TgaImage* out);

/// Probes and, for uncompressed images, copies the pixels. RLE images return
/// kUnsupported: none of the 15 .tga files in the reference client use it, and
/// adding an untested decoder would be worse than reporting the gap honestly.
Status TgaRead(ByteView file, TgaImage* out);

}  // namespace mu

#endif  // MU_ASSETS_TGA_HPP
