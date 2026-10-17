// SPDX-License-Identifier: MIT
#include "mu/assets/ozt.hpp"

#include <cstring>
#include <iterator>
#include <limits>

namespace mu {
namespace {

constexpr std::size_t kSignatureSize = 4;
constexpr std::size_t kTgaHeaderSize = 18;
constexpr std::size_t kHeaderSize = kSignatureSize + kTgaHeaderSize;  // 22
constexpr std::size_t kTgaFooterSize = 26;
constexpr uint32_t kMaxDimension = 8192;  // sanity cap, well above any asset
// Largest trailer length accepted. Observed values top out at 34 bytes among
// well-formed files; this leaves room for variants without trusting a corrupt
// file that claims megabytes of metadata.
constexpr std::size_t kMaxTrailerSize = 4096;
constexpr std::size_t kNoSignature = static_cast<std::size_t>(-1);

bool CheckedImageBytes(uint32_t width, uint32_t height, std::size_t channels,
                      std::size_t* row_bytes, std::size_t* total_bytes) {
  if (!row_bytes || !total_bytes || width == 0 || height == 0 || channels == 0)
    return false;
  const std::size_t max_size = std::numeric_limits<std::size_t>::max();
  const std::size_t w = static_cast<std::size_t>(width);
  const std::size_t h = static_cast<std::size_t>(height);
  if (w > max_size / channels) return false;
  const std::size_t row = w * channels;
  if (h > max_size / row) return false;
  *row_bytes = row;
  *total_bytes = row * h;
  return true;
}

/// Locates the 4-byte OZT signature. It normally sits at offset 0, but a group
/// of files in the reference client (Data/Custom/Interface/Rank*.OZT, Next.OZT,
/// Main_IE.ozt and others) carries an extra 4-byte variant marker 00 4B 6D 4B
/// in front of it. Returning the offset of the signature keeps the rest of the
/// parser unchanged.
std::size_t FindSignature(ByteView file) {
  static const uint8_t kSig[4] = {0x00, 0x00, 0x02, 0x00};
  static const uint8_t kVariant[4] = {0x00, 0x4B, 0x6D, 0x4B};

  if (file.size < kHeaderSize) return kNoSignature;
  if (std::memcmp(file.data, kSig, 4) == 0) return 0;

  // The variant form carries an extra 4-byte marker in front of the ordinary
  // signature. It is matched explicitly rather than by searching, because the
  // embedded TGA header itself contains the byte sequence 00 00 02 00 (colour
  // map type 0, image type 2), so a blind scan would match at offset 4 in
  // *every* file and make the check meaningless.
  if (file.size >= kHeaderSize + kSignatureSize &&
      std::memcmp(file.data, kVariant, 4) == 0 &&
      std::memcmp(file.data + kSignatureSize, kSig, 4) == 0)
    return kSignatureSize;

  return kNoSignature;
}

}  // namespace

Status OztProbe(ByteView file, OztImage* out) {
  if (!out) return Status::Make(Status::kIoError, "null output");
  *out = OztImage{};

  const std::size_t sig = FindSignature(file);
  if (sig == kNoSignature || sig + kHeaderSize > file.size)
    return Status::Make(Status::kNotThisFormat, "OZT signature not present");

  // The 00 4B 6D 4B variant is recognised but not decoded. Measured over all
  // 43 such files in the reference client: image type at the TGA offset reads 0
  // rather than 2, the dimensions sit 4 bytes later than in the standard form
  // (Rank01.OZT is 78x78 at file offset 20), and the file is exactly 4 bytes
  // shorter than header-plus-payload for any consistent pixel offset. Those
  // three facts cannot all be satisfied by one layout, so guessing an offset
  // that happens to make the arithmetic work would produce silently wrong
  // pixels. Reported as unsupported instead. See docs/ASSET_FORMATS.md.
  if (sig != 0)
    return Status::Make(Status::kUnsupported,
                        "OZT variant with a 00 4B 6D 4B marker: layout not "
                        "resolved");

  const uint8_t* h = file.data + sig + kSignatureSize;
  const uint32_t image_type = h[2];
  const uint32_t width = ReadU16LE(h + 12);
  const uint32_t height = ReadU16LE(h + 14);
  const uint32_t bpp = h[16];

  if (image_type != 2)
    return Status::Make(Status::kUnsupported,
                        "OZT image type != 2 (only uncompressed truecolour "
                        "is implemented)");
  if (width == 0 || height == 0 || width > kMaxDimension ||
      height > kMaxDimension)
    return Status::Make(Status::kCorrupt, "OZT dimensions out of range");
  if (bpp != 32 && bpp != 24)
    return Status::Make(Status::kUnsupported,
                        "OZT bits per pixel is neither 32 nor 24");

  const std::size_t bytes_per_pixel = bpp / 8;
  const std::size_t need =
      static_cast<std::size_t>(width) * height * bytes_per_pixel;

  // Guard the multiplication before it can wrap on a corrupt dimension pair.
  if (height != 0 && need / height != width * bytes_per_pixel)
    return Status::Make(Status::kCorrupt, "OZT pixel size overflows");

  const std::size_t pixel_offset = sig + kHeaderSize;
  if (pixel_offset + need > file.size)
    return Status::Make(Status::kTruncated,
                        "OZT declares more pixels than the file contains");

  const std::size_t remainder = file.size - pixel_offset - need;

  out->width = width;
  out->pixel_offset = pixel_offset;
  out->height = height;
  out->bits_per_pixel = bpp;
  out->image_type = static_cast<uint8_t>(image_type);
  out->has_footer = (remainder == kTgaFooterSize);
  out->trailer_size = remainder;

  // Measured trailer lengths across 400 reference files: 26 bytes (362, the
  // standard TGA footer), 0 (22), 30 (12), 34 (1), plus a few large outliers.
  // Rejecting anything that is not 0 or 26 would fail to load around 3% of the
  // asset base for no benefit: the pixel payload is already fully determined by
  // the header, so trailing metadata can be ignored safely. It is reported
  // through trailer_size so an inventory can still flag the unusual cases.
  if (remainder > kMaxTrailerSize)
    return Status::Make(Status::kCorrupt,
                        "OZT trailer is larger than any observed variant");

  return Status::Ok();
}

Status OztRead(ByteView file, OztImage* out) {
  Status s = OztProbe(file, out);
  if (!s) return s;
  const std::size_t n = out->pixel_bytes();
  out->pixels.assign(file.data + out->pixel_offset,
                     file.data + out->pixel_offset + n);
  return Status::Ok();
}

bool OztToRgbaTopDown(const OztImage& img, std::vector<uint8_t>* out) {
  if (!out || (img.bits_per_pixel != 24 && img.bits_per_pixel != 32))
    return false;

  std::size_t source_row = 0;
  std::size_t source_bytes = 0;
  if (!CheckedImageBytes(img.width, img.height, img.bits_per_pixel / 8,
                         &source_row, &source_bytes) ||
      img.pixels.size() < source_bytes)
    return false;

  std::size_t output_row = 0;
  std::size_t output_bytes = 0;
  if (!CheckedImageBytes(img.width, img.height, 4, &output_row,
                         &output_bytes))
    return false;
  out->resize(output_bytes);

  if (img.bits_per_pixel == 24) {
    for (uint32_t y = 0; y < img.height; ++y) {
      const std::size_t source_y =
          static_cast<std::size_t>(img.height - 1 - y);
      const uint8_t* src = img.pixels.data() + source_row * source_y;
      uint8_t* dst = out->data() + output_row * static_cast<std::size_t>(y);
      for (uint32_t x = 0; x < img.width; ++x) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst[3] = 255;
        src += 3;
        dst += 4;
      }
    }
    return true;
  }

  // Optimized 32-bit path: process two pixels (8 bytes) at a time while
  // swapping BGRA to RGBA. The row order is flipped from bottom-up to top-down.
  // BGRA little-endian = B | G<<8 | R<<16 | A<<24; RGBA swaps R and B.
  const std::size_t row = output_row;
  for (uint32_t y = 0; y < img.height; ++y) {
    const std::size_t source_y =
        static_cast<std::size_t>(img.height - 1 - y);
    const std::size_t output_y = static_cast<std::size_t>(y);
    const uint8_t* src = img.pixels.data() + row * source_y;
    uint8_t* dst = out->data() + row * output_y;
    uint32_t x = 0;
    for (; x + 1 < img.width; x += 2) {
      uint64_t chunk;
      std::memcpy(&chunk, src, 8);
      const uint32_t p0 = static_cast<uint32_t>(chunk & 0xFFFFFFFFULL);
      const uint32_t p1 = static_cast<uint32_t>((chunk >> 32) & 0xFFFFFFFFULL);
      const uint32_t r0 = (p0 & 0xFF00FF00) | ((p0 & 0xFF) << 16) |
                          ((p0 >> 16) & 0xFF);
      const uint32_t r1 = (p1 & 0xFF00FF00) | ((p1 & 0xFF) << 16) |
                          ((p1 >> 16) & 0xFF);
      const uint64_t out_chunk = (static_cast<uint64_t>(r1) << 32) | r0;
      std::memcpy(dst, &out_chunk, 8);
      src += 8;
      dst += 8;
    }
    for (; x < img.width; ++x) {
      uint32_t pixel;
      std::memcpy(&pixel, src, 4);
      const uint32_t rgba = (pixel & 0xFF00FF00) | ((pixel & 0xFF) << 16) |
                            ((pixel >> 16) & 0xFF);
      std::memcpy(dst, &rgba, 4);
      src += 4;
      dst += 4;
    }
  }
  return true;
}

bool OztToBgraTopDown(const OztImage& img, std::vector<uint8_t>* out) {
  if (!out || (img.bits_per_pixel != 24 && img.bits_per_pixel != 32))
    return false;

  std::size_t source_row = 0;
  std::size_t source_bytes = 0;
  if (!CheckedImageBytes(img.width, img.height, img.bits_per_pixel / 8,
                         &source_row, &source_bytes) ||
      img.pixels.size() < source_bytes)
    return false;

  std::size_t output_row = 0;
  std::size_t output_bytes = 0;
  if (!CheckedImageBytes(img.width, img.height, 4, &output_row,
                         &output_bytes))
    return false;
  out->resize(output_bytes);

  if (img.bits_per_pixel == 32) {
    // Preserve the measured direct-BGRA path: only reverse the row order.
    for (uint32_t y = 0; y < img.height; ++y) {
      const std::size_t source_y =
          static_cast<std::size_t>(img.height - 1 - y);
      const std::size_t output_y = static_cast<std::size_t>(y);
      const uint8_t* src = img.pixels.data() + source_row * source_y;
      uint8_t* dst = out->data() + output_row * output_y;
      std::memcpy(dst, src, output_row);
    }
    return true;
  }

  // Convert 24-bit BGR to opaque BGRA, also flipping bottom-up rows to top-down.
  for (uint32_t y = 0; y < img.height; ++y) {
    const std::size_t source_y =
        static_cast<std::size_t>(img.height - 1 - y);
    const std::size_t output_y = static_cast<std::size_t>(y);
    const uint8_t* src = img.pixels.data() + source_row * source_y;
    uint8_t* dst = out->data() + output_row * output_y;
    for (uint32_t x = 0; x < img.width; ++x) {
      dst[0] = src[0];
      dst[1] = src[1];
      dst[2] = src[2];
      dst[3] = 255;
      src += 3;
      dst += 4;
    }
  }
  return true;
}

bool OztWriteRgba(uint32_t width, uint32_t height, const uint8_t* rgba,
                  bool with_footer, std::vector<uint8_t>* out) {
  if (!out || !rgba || width == 0 || height == 0 || width > kMaxDimension ||
      height > kMaxDimension)
    return false;
  const std::size_t row = static_cast<std::size_t>(width) * 4;
  out->clear();
  out->reserve(kHeaderSize + row * height + kTgaFooterSize);

  // 4-byte OZT signature. Appended byte by byte: handing vector::insert a
  // pointer pair taken from a 4-byte array makes GCC's bounds analysis report
  // a false overflow at -O2, and a warning-free build is worth four lines.
  out->push_back(0x00);
  out->push_back(0x00);
  out->push_back(0x02);
  out->push_back(0x00);

  // 18-byte TGA header.
  uint8_t h[kTgaHeaderSize] = {0};
  h[2] = 2;  // uncompressed truecolour
  h[12] = static_cast<uint8_t>(width & 0xFF);
  h[13] = static_cast<uint8_t>((width >> 8) & 0xFF);
  h[14] = static_cast<uint8_t>(height & 0xFF);
  h[15] = static_cast<uint8_t>((height >> 8) & 0xFF);
  h[16] = 32;  // bits per pixel
  h[17] = 8;   // 8 alpha bits
  out->insert(out->end(), std::begin(h), std::end(h));

  // Pixels, bottom row first, BGRA.
  std::vector<uint8_t> line(row);
  for (uint32_t y = 0; y < height; ++y) {
    const uint8_t* src = rgba + row * (height - 1 - y);
    uint8_t* dst = line.data();
    for (uint32_t x = 0; x < width; ++x) {
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = src[3];
      src += 4;
      dst += 4;
    }
    out->insert(out->end(), line.begin(), line.end());
  }

  if (with_footer) {
    // TGA 2.0 footer: two u32 offsets (zero = no such area), then the 18-byte
    // signature "TRUEVISION-XFILE." and a NUL.
    uint8_t f[kTgaFooterSize] = {0};
    static const char kId[] = "TRUEVISION-XFILE.";
    std::memcpy(f + 8, kId, sizeof(kId));  // includes the terminating NUL
    out->insert(out->end(), std::begin(f), std::end(f));
  }
  return true;
}

}  // namespace mu
