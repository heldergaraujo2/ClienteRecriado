// SPDX-License-Identifier: MIT
// OZJ reader - MU Online's JPEG texture container.
//
// Finding: an OZJ is a *plain* JPEG. In a sample of 120 files drawn from the
// reference client, 119 began with the JPEG start-of-image marker FF D8 FF.
// There is no OZJ-specific wrapper header to strip.
//
// This module deliberately does NOT contain a JPEG decoder. It walks the JPEG
// marker structure to extract geometry, component count and colour precision,
// which is everything the inventory / VRAM-budget tooling needs, and it
// validates that the file is well formed. Actual pixel decoding is delegated
// to a pluggable decoder (see docs/ASSET_FORMATS.md "OZJ decoding").
#ifndef MU_ASSETS_OZJ_HPP
#define MU_ASSETS_OZJ_HPP

#include <cstdint>

#include "mu/assets/format.hpp"

namespace mu {

struct OzjImage {
  uint32_t width = 0;
  uint32_t height = 0;
  uint8_t precision = 0;     ///< Bits per sample, normally 8.
  uint8_t components = 0;    ///< 1 = grey, 3 = YCbCr.
  bool is_progressive = false;
  bool has_end_of_image = false;
};

/// Walks the JPEG marker chain. Returns kCorrupt with a precise reason when the
/// chain is malformed, which is exactly what an integrity check wants.
///
/// OZJ files carry a duplicated leading header: in 200 of 200 reference files
/// sampled, a start-of-image marker appears at offset 0 *and* at offset 24, and
/// the first 24 bytes are a byte-for-byte copy of bytes 24..47. The decodable
/// image is the one at offset 24; walking the markers from offset 0 runs into
/// the middle of a segment length and reports a healthy file as corrupt. This
/// function therefore locates the real image start first.
Status OzjProbe(ByteView file, OzjImage* out);

/// Returns the offset at which the decodable JPEG begins, or 0 when the file
/// starts with the image directly. Sets |found| to false when no valid start
/// could be located.
std::size_t OzjFindImageStart(ByteView file, bool* found);

/// True when the buffer starts with a JPEG start-of-image marker.
bool OzjLooksLikeJpeg(ByteView file);

}  // namespace mu

#endif  // MU_ASSETS_OZJ_HPP
