// SPDX-License-Identifier: MIT
#include "mu/assets/ozj.hpp"

#include <cstring>

namespace mu {
namespace {

constexpr std::size_t kMaxHeaderSearch = 4096;

bool IsSoiAt(ByteView file, std::size_t off) {
  return off + 2 < file.size && file.data[off] == 0xFF &&
         file.data[off + 1] == 0xD8 && file.data[off + 2] == 0xFF;
}

/// Walks the marker chain assuming the image starts at |start|.
Status ProbeFrom(ByteView file, std::size_t start, OzjImage* out) {
  if (start + 4 > file.size)
    return Status::Make(Status::kTruncated, "not enough data for a JPEG");
  if (file.data[start] != 0xFF || file.data[start + 1] != 0xD8)
    return Status::Make(Status::kNotThisFormat, "missing JPEG SOI (FF D8)");

  std::size_t i = start + 2;
  bool found_sof = false;

  while (i + 1 < file.size) {
    // Skip padding FF bytes, which encoders are allowed to emit between
    // markers. Bailing out here would produce false corruption reports.
    if (file.data[i] != 0xFF)
      return Status::Make(Status::kCorrupt, "expected a marker prefix 0xFF");
    while (i < file.size && file.data[i] == 0xFF) ++i;
    if (i >= file.size)
      return Status::Make(Status::kTruncated, "marker prefix at end of file");

    const uint8_t marker = file.data[i++];

    if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7))
      continue;  // Standalone markers carry no payload.

    if (marker == 0xD9) {  // End of image.
      out->has_end_of_image = true;
      break;
    }

    if (i + 1 >= file.size)
      return Status::Make(Status::kTruncated, "segment length missing");
    // JPEG stores segment lengths big-endian.
    const uint32_t seg_len = ReadU16BE(file.data + i);
    if (seg_len < 2)
      return Status::Make(Status::kCorrupt, "segment length below 2");
    if (i + seg_len > file.size)
      return Status::Make(Status::kTruncated,
                          "segment runs past the end of the file");

    // SOS is followed by entropy-coded data of *unspecified* length, which is
    // free to contain any byte value including 0x00 and 0xFF. The only way
    // past it is to scan for the next marker, honouring the two escape rules:
    // FF 00 is a stuffed literal FF, and FF D0..D7 are restart markers that
    // belong to the scan rather than ending it. Treating SOS like any other
    // length-prefixed segment stops the walk inside the scan data.
    if (marker == 0xDA) {
      i += seg_len;
      bool found_marker = false;
      while (i + 1 < file.size) {
        if (file.data[i] != 0xFF) {
          ++i;
          continue;
        }
        const uint8_t next = file.data[i + 1];
        if (next == 0x00 || (next >= 0xD0 && next <= 0xD7)) {
          i += 2;  // stuffed byte or restart marker: still inside the scan
          continue;
        }
        found_marker = true;
        break;
      }
      if (!found_marker) {
        // A truncated scan is common in partially downloaded files. Report it
        // only if we never saw the geometry, otherwise keep what we learned.
        if (!found_sof)
          return Status::Make(Status::kTruncated,
                              "entropy-coded data not terminated");
        break;
      }
      continue;
    }

    // Start-of-frame family. C4/C8/CC are the DHT/JPG/DAC markers, not SOF,
    // so they must be excluded even though they sit in the same range.
    const bool is_sof = (marker >= 0xC0 && marker <= 0xCF) && marker != 0xC4 &&
                        marker != 0xC8 && marker != 0xCC;
    if (is_sof && !found_sof) {
      if (seg_len < 8)
        return Status::Make(Status::kCorrupt, "SOF segment too short");
      const uint8_t* p = file.data + i + 2;
      out->precision = p[0];
      out->height = ReadU16BE(p + 1);
      out->width = ReadU16BE(p + 3);
      out->components = p[5];
      out->is_progressive = (marker >= 0xC1 && marker <= 0xC3) ||
                            (marker >= 0xC9 && marker <= 0xCB) ||
                            (marker >= 0xCD && marker <= 0xCF);
      found_sof = true;
      if (out->width == 0 || out->height == 0)
        return Status::Make(Status::kCorrupt, "SOF declares a zero dimension");
      if (out->components == 0 || out->components > 4)
        return Status::Make(Status::kCorrupt, "SOF component count invalid");
    }

    i += seg_len;
  }

  if (!found_sof)
    return Status::Make(Status::kCorrupt, "no start-of-frame marker found");
  return Status::Ok();
}

}  // namespace

bool OzjLooksLikeJpeg(ByteView file) {
  return file.size >= 3 && file.data[0] == 0xFF && file.data[1] == 0xD8 &&
         file.data[2] == 0xFF;
}

std::size_t OzjFindImageStart(ByteView file, bool* found) {
  bool dummy = false;
  bool* ok = found ? found : &dummy;
  *ok = false;
  if (!OzjLooksLikeJpeg(file)) return 0;

  // A file whose markers walk cleanly from offset 0 is an ordinary JPEG.
  OzjImage trial;
  if (ProbeFrom(file, 0, &trial).ok()) {
    *ok = true;
    return 0;
  }

  // Otherwise look for a second start-of-image marker: the OZJ container
  // duplicates the leading header and restarts the image after it.
  const std::size_t limit =
      file.size < kMaxHeaderSearch ? file.size : kMaxHeaderSearch;
  for (std::size_t off = 3; off + 2 < limit; ++off) {
    if (!IsSoiAt(file, off)) continue;
    OzjImage probe;
    if (ProbeFrom(file, off, &probe).ok()) {
      *ok = true;
      return off;
    }
  }
  return 0;
}

Status OzjProbe(ByteView file, OzjImage* out) {
  if (!out) return Status::Make(Status::kIoError, "null output");
  *out = OzjImage{};

  if (file.size < 4)
    return Status::Make(Status::kTruncated, "file smaller than 4 bytes");
  if (!OzjLooksLikeJpeg(file))
    return Status::Make(Status::kNotThisFormat, "missing JPEG SOI (FF D8)");

  // Report the outcome of the first attempt when nothing works, because that
  // message is the useful one for diagnosing a genuinely broken file.
  Status first = ProbeFrom(file, 0, out);
  if (first.ok()) return first;

  bool found = false;
  const std::size_t start = OzjFindImageStart(file, &found);
  if (found && start != 0) {
    *out = OzjImage{};
    return ProbeFrom(file, start, out);
  }
  *out = OzjImage{};
  return first;
}

}  // namespace mu
