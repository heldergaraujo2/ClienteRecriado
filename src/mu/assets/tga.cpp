// SPDX-License-Identifier: MIT
#include "mu/assets/tga.hpp"

namespace mu {
namespace {
constexpr std::size_t kHeaderSize = 18;
constexpr uint32_t kMaxDimension = 8192;
}  // namespace

Status TgaProbe(ByteView file, TgaImage* out) {
  if (!out) return Status::Make(Status::kIoError, "null output");
  *out = TgaImage{};

  if (file.size < kHeaderSize)
    return Status::Make(Status::kTruncated, "file smaller than an 18-byte TGA "
                                            "header");

  out->id_length = file.data[0];
  out->image_type = file.data[2];
  out->width = ReadU16LE(file.data + 12);
  out->height = ReadU16LE(file.data + 14);
  out->bits_per_pixel = file.data[16];
  out->descriptor = file.data[17];
  // Bit 5 of the descriptor is the origin flag: set means the first row in the
  // file is the top row, which is the opposite of the usual TGA layout.
  out->top_to_bottom = (out->descriptor & 0x20) != 0;

  if (out->image_type != 2 && out->image_type != 10)
    return Status::Make(Status::kUnsupported,
                        "TGA image type is neither uncompressed truecolour "
                        "nor RLE truecolour");
  if (out->width == 0 || out->height == 0 || out->width > kMaxDimension ||
      out->height > kMaxDimension)
    return Status::Make(Status::kCorrupt, "TGA dimensions out of range");
  if (out->bits_per_pixel != 24 && out->bits_per_pixel != 32)
    return Status::Make(Status::kUnsupported,
                        "TGA bits per pixel is neither 24 nor 32");

  out->pixel_offset = kHeaderSize + out->id_length;
  if (out->pixel_offset > file.size)
    return Status::Make(Status::kTruncated, "TGA image ID runs past the file");

  if (out->image_type == 2) {
    out->pixel_bytes = static_cast<std::size_t>(out->width) * out->height *
                       (out->bits_per_pixel / 8);
    if (out->pixel_offset + out->pixel_bytes > file.size)
      return Status::Make(Status::kTruncated,
                          "TGA declares more pixels than the file contains");
  }
  return Status::Ok();
}

Status TgaRead(ByteView file, TgaImage* out) {
  Status s = TgaProbe(file, out);
  if (!s) return s;
  if (out->image_type != 2)
    return Status::Make(Status::kUnsupported,
                        "TGA RLE decoding is not implemented");
  out->pixels.assign(file.data + out->pixel_offset,
                     file.data + out->pixel_offset + out->pixel_bytes);
  return Status::Ok();
}

}  // namespace mu
