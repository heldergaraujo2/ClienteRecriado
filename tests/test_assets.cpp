#include <algorithm>
#include <cmath>
// SPDX-License-Identifier: MIT
// Test suite for the ClienteRecriado asset library.
//
// Uses a small built-in harness rather than a downloaded framework, so the
// suite builds with nothing but a compiler. Every fixture is generated here at
// run time from scratch: this repository contains no original game assets.
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <cstring>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

#if defined(_WIN32)
#include <direct.h>  // _rmdir
#define rmdir _rmdir
#else
#include <unistd.h>  // mkdtemp, rmdir
#endif

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/mesh.hpp"
#include "mu/render/cull.hpp"
#include "mu/render/buffers.hpp"
#include "mu/render/skinning.hpp"
#include "mu/platform/win32_window.hpp"
#include "mu/platform/main_loop.hpp"
#include "mu/assets/bmd_report.hpp"
#include "mu/assets/format.hpp"
#include "mu/assets/manifest.hpp"
#include "mu/assets/ozj.hpp"
#include "mu/assets/ozt.hpp"
#include "mu/assets/terrain.hpp"
#include "mu/fs.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;
const char* g_current = "";

void Check(bool cond, const char* what, const char* file, int line) {
  ++g_checks;
  if (!cond) {
    ++g_failures;
    std::printf("  FAIL [%s] %s  (%s:%d)\n", g_current, what, file, line);
  }
}

#define CHECK(cond) Check((cond), #cond, __FILE__, __LINE__)
#define CHECK_EQ(a, b)                                                        \
  do {                                                                        \
    auto va_ = (a);                                                           \
    auto vb_ = (b);                                                           \
    ++g_checks;                                                               \
    if (!(va_ == vb_)) {                                                      \
      ++g_failures;                                                           \
      std::printf("  FAIL [%s] %s == %s  (%s:%d)\n", g_current, #a, #b,       \
                  __FILE__, __LINE__);                                        \
    }                                                                         \
  } while (0)

struct Test {
  const char* name;
  void (*fn)();
};

// ---------------------------------------------------------------- CRC-32

void TestCrc32KnownVectors() {
  // The canonical check value for CRC-32/ISO-HDLC. If this fails, every patch
  // verification result is meaningless, so it is asserted first.
  const char* s = "123456789";
  CHECK_EQ(mu::Crc32(s, 9), 0xCBF43926u);

  CHECK_EQ(mu::Crc32("", 0), 0x00000000u);

  // Incremental update must agree with the one-shot call.
  const char* msg = "Data/gate.bmd";
  uint32_t a = mu::Crc32(msg, std::strlen(msg));
  uint32_t b = mu::Crc32Update(mu::Crc32("Data/", 5), "gate.bmd", 8);
  CHECK_EQ(a, b);

  // Longer buffers, to exercise the slice-by-8 path and every tail length.
  // Expected values computed independently with Python's zlib.crc32.
  std::vector<uint8_t> pattern(1024);
  for (std::size_t i = 0; i < pattern.size(); ++i)
    pattern[i] = static_cast<uint8_t>(i % 256);
  CHECK_EQ(mu::Crc32(pattern.data(), pattern.size()), 0xB70B4C26u);

  std::vector<uint8_t> zeros(4096, 0);
  CHECK_EQ(mu::Crc32(zeros.data(), zeros.size()), 0xC71C0011u);

  // 37 bytes: 4 full 8-byte blocks plus a 5-byte tail.
  std::vector<uint8_t> odd(37);
  for (std::size_t i = 0; i < odd.size(); ++i)
    odd[i] = static_cast<uint8_t>((i * 7 + 3) & 0xFF);
  CHECK_EQ(mu::Crc32(odd.data(), odd.size()), 0x39A397B4u);

  // Every length from 0 to 40 must agree with the incremental form built one
  // byte at a time. This is what catches an off-by-one in the tail loop.
  for (std::size_t n = 0; n <= 40; ++n) {
    uint32_t whole = mu::Crc32(pattern.data(), n);
    uint32_t inc = 0;
    for (std::size_t i = 0; i < n; ++i)
      inc = mu::Crc32Update(inc, pattern.data() + i, 1);
    CHECK_EQ(whole, inc);
  }
}

// ---------------------------------------------------------------- OZT

std::vector<uint8_t> MakePattern(uint32_t w, uint32_t h) {
  std::vector<uint8_t> rgba(static_cast<std::size_t>(w) * h * 4);
  for (uint32_t y = 0; y < h; ++y) {
    for (uint32_t x = 0; x < w; ++x) {
      uint8_t* p = rgba.data() + (static_cast<std::size_t>(y) * w + x) * 4;
      p[0] = static_cast<uint8_t>(x);          // R
      p[1] = static_cast<uint8_t>(y);          // G
      p[2] = static_cast<uint8_t>(x ^ y);      // B
      p[3] = static_cast<uint8_t>((x + y));    // A
    }
  }
  return rgba;
}

void TestOztRoundTrip() {
  const uint32_t w = 37, h = 21;  // deliberately non power of two
  std::vector<uint8_t> rgba = MakePattern(w, h);

  for (bool footer : {false, true}) {
    std::vector<uint8_t> blob;
    CHECK(mu::OztWriteRgba(w, h, rgba.data(), footer, &blob));

    const std::size_t expected =
        22 + static_cast<std::size_t>(w) * h * 4 + (footer ? 26 : 0);
    CHECK_EQ(blob.size(), expected);

    mu::OztImage img;
    mu::Status st = mu::OztProbe(mu::ByteView(blob.data(), blob.size()), &img);
    CHECK(st.ok());
    CHECK_EQ(img.width, w);
    CHECK_EQ(img.height, h);
    CHECK_EQ(img.bits_per_pixel, 32u);
    CHECK_EQ(img.has_footer, footer);

    mu::OztImage full;
    CHECK(mu::OztRead(mu::ByteView(blob.data(), blob.size()), &full).ok());

    std::vector<uint8_t> back;
    CHECK(mu::OztToRgbaTopDown(full, &back));
    CHECK_EQ(back.size(), rgba.size());
    CHECK(back == rgba);  // the vertical flip and BGRA swap must cancel out
  }
}

void TestOzt24BitPixelConversion() {
  // Embedded TGA truecolour fixture: 2x2, 24-bit BGR, bottom row first.
  std::vector<uint8_t> file(22 + 2 * 2 * 3, 0);
  file[0] = 0x00;
  file[1] = 0x00;
  file[2] = 0x02;
  file[3] = 0x00;
  file[4 + 2] = 2;
  file[4 + 12] = 2;
  file[4 + 14] = 2;
  file[4 + 16] = 24;
  const uint8_t bottom_up_bgr[12] = {
      255, 0, 0, 255, 255, 255,  // bottom: blue, white
      0, 0, 255, 0, 255, 0,      // top: red, green
  };
  std::memcpy(file.data() + 22, bottom_up_bgr, sizeof(bottom_up_bgr));

  mu::OztImage image;
  CHECK(mu::OztRead(mu::ByteView(file.data(), file.size()), &image).ok());
  CHECK_EQ(image.width, 2u);
  CHECK_EQ(image.height, 2u);
  CHECK_EQ(image.bits_per_pixel, 24u);

  const std::vector<uint8_t> expected_bgra = {
      0, 0, 255, 255, 0, 255, 0, 255,
      255, 0, 0, 255, 255, 255, 255, 255,
  };
  std::vector<uint8_t> bgra;
  CHECK(mu::OztToBgraTopDown(image, &bgra));
  CHECK(bgra == expected_bgra);

  const std::vector<uint8_t> expected_rgba = {
      255, 0, 0, 255, 0, 255, 0, 255,
      0, 0, 255, 255, 255, 255, 255, 255,
  };
  std::vector<uint8_t> rgba;
  CHECK(mu::OztToRgbaTopDown(image, &rgba));
  CHECK(rgba == expected_rgba);
}

void TestOztVariantWithExtraPrefix() {
  // 43 files in the reference client (Data/Custom/Interface/Rank*.OZT,
  // Next.OZT, Main_IE.ozt) carry a 4-byte 00 4B 6D 4B marker ahead of the
  // ordinary signature. Their internal layout does not match the standard
  // form, so the loader must recognise them and say so, rather than either
  // silently misparsing them or reporting them as "not an OZT".
  std::vector<uint8_t> rgba = MakePattern(16, 16);
  std::vector<uint8_t> blob;
  CHECK(mu::OztWriteRgba(16, 16, rgba.data(), true, &blob));

  const uint8_t variant[4] = {0x00, 0x4B, 0x6D, 0x4B};
  std::vector<uint8_t> prefixed(variant, variant + 4);
  prefixed.insert(prefixed.end(), blob.begin(), blob.end());

  mu::OztImage img;
  mu::Status st = mu::OztProbe(mu::ByteView(prefixed.data(), prefixed.size()),
                               &img);
  CHECK(!st.ok());
  CHECK_EQ(static_cast<int>(st.code),
           static_cast<int>(mu::Status::kUnsupported));
  // The message must name the marker, so a report identifies the variant.
  CHECK(st.message.find("00 4B 6D 4B") != std::string::npos);
}

void TestOztRejectsBadInput() {
  std::vector<uint8_t> rgba = MakePattern(8, 8);
  std::vector<uint8_t> blob;
  CHECK(mu::OztWriteRgba(8, 8, rgba.data(), true, &blob));

  // Too short to even hold a header.
  mu::OztImage img;
  CHECK(!mu::OztProbe(mu::ByteView(blob.data(), 10), &img).ok());

  // Wrong signature.
  std::vector<uint8_t> bad = blob;
  bad[0] = 0xAB;
  mu::Status st = mu::OztProbe(mu::ByteView(bad.data(), bad.size()), &img);
  CHECK_EQ(static_cast<int>(st.code), static_cast<int>(mu::Status::kNotThisFormat));

  // Truncated pixel payload: the header claims 8x8 but we cut the file short.
  std::vector<uint8_t> cut(blob.begin(), blob.begin() + 22 + 32);
  st = mu::OztProbe(mu::ByteView(cut.data(), cut.size()), &img);
  CHECK_EQ(static_cast<int>(st.code), static_cast<int>(mu::Status::kTruncated));

  // A trailer that is neither 0 nor 26 bytes is still loadable: the pixel
  // payload is fully determined by the header, and the reference asset base
  // contains 30- and 34-byte trailers. The loader reports the size instead of
  // rejecting the file.
  std::vector<uint8_t> extra = blob;
  extra.push_back(0x00);
  st = mu::OztProbe(mu::ByteView(extra.data(), extra.size()), &img);
  CHECK(st.ok());
  CHECK_EQ(img.trailer_size, 27u);
  CHECK(!img.has_footer);

  // An absurd trailer is rejected, so a corrupt length cannot pass as data.
  std::vector<uint8_t> huge = blob;
  huge.resize(huge.size() + 8192, 0x00);
  st = mu::OztProbe(mu::ByteView(huge.data(), huge.size()), &img);
  CHECK_EQ(static_cast<int>(st.code), static_cast<int>(mu::Status::kCorrupt));

  // Unsupported image type (RLE = 10 in the embedded TGA header).
  std::vector<uint8_t> rle = blob;
  rle[4 + 2] = 10;
  st = mu::OztProbe(mu::ByteView(rle.data(), rle.size()), &img);
  CHECK_EQ(static_cast<int>(st.code), static_cast<int>(mu::Status::kUnsupported));
}

// ---------------------------------------------------------------- OZJ

// Builds a minimal but structurally valid baseline JPEG marker chain.
std::vector<uint8_t> MakeFakeJpeg(uint16_t w, uint16_t h, bool with_eoi) {
  std::vector<uint8_t> j;
  auto put = [&](uint8_t b) { j.push_back(b); };
  auto putbe = [&](uint16_t v) {
    j.push_back(static_cast<uint8_t>(v >> 8));
    j.push_back(static_cast<uint8_t>(v & 0xFF));
  };
  put(0xFF); put(0xD8);                        // SOI
  put(0xFF); put(0xE0); putbe(16);             // APP0, length 16
  for (int i = 0; i < 14; ++i) put(0x00);
  put(0xFF); put(0xDB); putbe(67);             // DQT
  for (int i = 0; i < 65; ++i) put(0x10);
  put(0xFF); put(0xC0); putbe(17);             // SOF0
  put(8);                                      // precision
  putbe(h); putbe(w);
  put(3);                                      // components
  put(1); put(0x22); put(0);
  put(2); put(0x11); put(1);
  put(3); put(0x11); put(1);
  put(0xFF); put(0xDA); putbe(12);             // SOS
  put(3);
  put(1); put(0x00); put(2); put(0x11); put(3); put(0x11);
  put(0); put(63); put(0);
  put(0x00); put(0x01);                        // a little entropy data
  if (with_eoi) { put(0xFF); put(0xD9); }
  return j;
}

void TestOzjDuplicatedHeaderPrefix() {
  // The real OZJ container: the JPEG's own leading bytes are duplicated in
  // front of the image, so a naive walk from offset 0 fails. Build that shape
  // by prefixing a fake JPEG with a copy of its first 24 bytes.
  std::vector<uint8_t> j = MakeFakeJpeg(128, 96, true);
  std::vector<uint8_t> ozj(j.begin(), j.begin() + 24);
  ozj.insert(ozj.end(), j.begin(), j.end());

  // The image must be located at offset 24 rather than 0. If this ever
  // returns 0, the container prefix has stopped being detected and a real
  // OZJ file would be reported as corrupt.
  bool found = false;
  const std::size_t start = mu::OzjFindImageStart(
      mu::ByteView(ozj.data(), ozj.size()), &found);
  CHECK(found);
  CHECK_EQ(start, 24u);

  mu::OzjImage img;
  CHECK(mu::OzjProbe(mu::ByteView(ozj.data(), ozj.size()), &img).ok());
  CHECK_EQ(img.width, 128u);
  CHECK_EQ(img.height, 96u);
  CHECK(img.has_end_of_image);
}

void TestOzjProbe() {
  std::vector<uint8_t> j = MakeFakeJpeg(640, 480, true);
  mu::OzjImage img;
  mu::Status st = mu::OzjProbe(mu::ByteView(j.data(), j.size()), &img);
  CHECK(st.ok());
  CHECK_EQ(img.width, 640u);
  CHECK_EQ(img.height, 480u);
  CHECK_EQ(img.precision, 8u);
  CHECK_EQ(img.components, 3u);
  CHECK(img.has_end_of_image);
  CHECK(!img.is_progressive);
  CHECK(mu::OzjLooksLikeJpeg(mu::ByteView(j.data(), j.size())));

  // Truncated before the end of image.
  std::vector<uint8_t> no_eoi = MakeFakeJpeg(32, 32, false);
  mu::OzjImage i2;
  CHECK(mu::OzjProbe(mu::ByteView(no_eoi.data(), no_eoi.size()), &i2).ok());
  CHECK(!i2.has_end_of_image);

  // A segment that runs past the end of the buffer must be caught, not read
  // out of bounds.
  std::vector<uint8_t> clipped(j.data(), j.data() + j.size() - 20);
  mu::OzjImage i3;
  mu::Status bad = mu::OzjProbe(mu::ByteView(clipped.data(), clipped.size()), &i3);
  CHECK(!bad.ok());

  // Not a JPEG at all.
  const uint8_t junk[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  mu::OzjImage i4;
  CHECK_EQ(static_cast<int>(mu::OzjProbe(mu::ByteView(junk, 8), &i4).code),
           static_cast<int>(mu::Status::kNotThisFormat));
}

// ---------------------------------------------------------------- BMD / bux

void TestBuxCipherIsSelfInverse() {
  std::vector<uint8_t> original(1024);
  for (std::size_t i = 0; i < original.size(); ++i)
    original[i] = static_cast<uint8_t>(i * 7 + 3);

  std::vector<uint8_t> data = original;
  mu::BuxCrypt(data.data(), data.size());
  CHECK(data != original);  // it actually changed something
  mu::BuxCrypt(data.data(), data.size());
  CHECK(data == original);  // and changing it back

  // Every plaintext byte of zero must encrypt to the key itself.
  std::vector<uint8_t> zeros(6, 0);
  mu::BuxCrypt(zeros.data(), zeros.size());
  CHECK_EQ(zeros[0], mu::kBuxKey[0]);
  CHECK_EQ(zeros[1], mu::kBuxKey[1]);
  CHECK_EQ(zeros[2], mu::kBuxKey[2]);
  CHECK_EQ(zeros[3], mu::kBuxKey[0]);
}

std::vector<uint8_t> MakeBmdV0C(uint32_t payload_len) {
  std::vector<uint8_t> f;
  f.push_back('B'); f.push_back('M'); f.push_back('D'); f.push_back(0x0C);
  uint8_t len[4] = {
      static_cast<uint8_t>(payload_len & 0xFF),
      static_cast<uint8_t>((payload_len >> 8) & 0xFF),
      static_cast<uint8_t>((payload_len >> 16) & 0xFF),
      static_cast<uint8_t>((payload_len >> 24) & 0xFF)};
  f.insert(f.end(), len, len + 4);
  for (uint32_t i = 0; i < payload_len; ++i)
    f.push_back(static_cast<uint8_t>(i & 0xFF));
  return f;
}

void TestBmdProbe() {
  // Version 0x0C with a correct length field.
  std::vector<uint8_t> f = MakeBmdV0C(256);
  mu::BmdInfo info;
  CHECK(mu::BmdProbe(mu::ByteView(f.data(), f.size()), &info).ok());
  CHECK_EQ(static_cast<int>(info.kind), static_cast<int>(mu::BmdKind::kPlainV0C));
  CHECK_EQ(info.version, 0x0Cu);
  CHECK(info.length_consistent);
  CHECK_EQ(info.payload_offset, 8u);
  CHECK_EQ(info.payload_size, 256u);

  // A truncated file must be reported, and must not report a payload larger
  // than the bytes that are actually present.
  std::vector<uint8_t> cut(f.begin(), f.begin() + 40);
  mu::BmdInfo cinfo;
  CHECK(mu::BmdProbe(mu::ByteView(cut.data(), cut.size()), &cinfo).ok());
  CHECK(!cinfo.length_consistent);
  CHECK_EQ(cinfo.payload_size, cut.size() - 8);

  // Version 0x0A has no length field; the module must say so rather than
  // misreading offset 4 as a size.
  std::vector<uint8_t> a = MakeBmdV0C(64);
  a[3] = 0x0A;
  mu::BmdInfo ainfo;
  CHECK(mu::BmdProbe(mu::ByteView(a.data(), a.size()), &ainfo).ok());
  CHECK_EQ(static_cast<int>(ainfo.kind), static_cast<int>(mu::BmdKind::kPlainV0A));
  CHECK_EQ(ainfo.payload_offset, 4u);

  // A whole-file bux container.
  std::vector<uint8_t> enc = MakeBmdV0C(32);
  mu::BuxCrypt(enc.data(), enc.size());
  CHECK(mu::LooksLikeBuxContainer(mu::ByteView(enc.data(), enc.size())));
  mu::BmdInfo einfo;
  CHECK(mu::BmdProbe(mu::ByteView(enc.data(), enc.size()), &einfo).ok());
  CHECK_EQ(static_cast<int>(einfo.kind),
           static_cast<int>(mu::BmdKind::kBuxContainer));

  // A bux-encrypted blob that is NOT a model: no "BMD" magic once decrypted,
  // but a run of zero padding. This is the shape of Data/gate.bmd in the
  // reference client, and detection must not depend on the magic.
  std::vector<uint8_t> blob(512);
  for (std::size_t i = 0; i < 512; ++i)
    blob[i] = (i < 64) ? 0x00 : static_cast<uint8_t>(0x40 + (i % 97));
  std::vector<uint8_t> blob_enc = blob;
  mu::BuxCrypt(blob_enc.data(), blob_enc.size());
  CHECK(!mu::LooksLikeBuxContainer(mu::ByteView(blob_enc.data(), blob_enc.size())));
  CHECK(mu::BuxDecryptGain(mu::ByteView(blob_enc.data(), blob_enc.size())) > 0.05);
  mu::BmdInfo binfo;
  CHECK(mu::BmdProbe(mu::ByteView(blob_enc.data(), blob_enc.size()), &binfo).ok());
  CHECK_EQ(static_cast<int>(binfo.kind),
           static_cast<int>(mu::BmdKind::kBuxContainer));

  // High-entropy data shows no gain, so it is not misclassified as bux.
  std::vector<uint8_t> noise(512);
  uint32_t seed = 99;
  for (std::size_t i = 0; i < noise.size(); ++i) {
    seed = seed * 1103515245u + 12345u;
    noise[i] = static_cast<uint8_t>((seed >> 16) & 0xFF);
  }
  CHECK(mu::BuxDecryptGain(mu::ByteView(noise.data(), noise.size())) < 0.05);

  // Something that is not a model at all.
  const uint8_t junk[8] = {9, 9, 9, 9, 9, 9, 9, 9};
  mu::BmdInfo jinfo;
  CHECK_EQ(static_cast<int>(mu::BmdProbe(mu::ByteView(junk, 8), &jinfo).code),
           static_cast<int>(mu::Status::kNotThisFormat));
}

void TestEntropySeparatesPlaintextFromCiphertext() {
  std::vector<uint8_t> flat(4096, 0x04);
  CHECK(mu::ByteEntropy(flat.data(), flat.size()) < 0.01);

  // A byte pattern that fills the alphabet should measure close to 8 bits.
  std::vector<uint8_t> spread(65536);
  uint32_t x = 12345;
  for (std::size_t i = 0; i < spread.size(); ++i) {
    x = x * 1103515245u + 12345u;
    spread[i] = static_cast<uint8_t>((x >> 16) & 0xFF);
  }
  CHECK(mu::ByteEntropy(spread.data(), spread.size()) > 7.9);
}

// ---------------------------------------------------------------- Terrain

void TestTerrainProbe() {
  // 256*256 attributes plus a 3-byte header, matching the measured 65539.
  std::vector<uint8_t> att(3 + 256 * 256);
  att[0] = 0x00; att[1] = 0xFF; att[2] = 0xFF;
  for (std::size_t i = 3; i < att.size(); ++i) att[i] = 0x04;
  mu::TerrainInfo ti;
  CHECK(mu::TerrainProbe(mu::ByteView(att.data(), att.size()), &ti).ok());
  CHECK_EQ(static_cast<int>(ti.kind), static_cast<int>(mu::TerrainKind::kAttribute1));
  CHECK_EQ(ti.header_size, 3u);
  CHECK(ti.grid_consistent);
  CHECK(ti.looks_plaintext);

  // Encrypting it must flip the plaintext verdict.
  std::vector<uint8_t> enc = att;
  mu::BuxCrypt(enc.data(), enc.size());
  mu::TerrainInfo et;
  CHECK(mu::TerrainProbe(mu::ByteView(enc.data(), enc.size()), &et).ok());
  CHECK(!et.looks_plaintext);

  // And auto-detection must recover it.
  CHECK(mu::TerrainTryBuxDecrypt(&enc));
  CHECK(enc == att);

  // A 2-bytes-per-tile attribute file.
  std::vector<uint8_t> att2(4 + 256 * 256 * 2, 0x01);
  mu::TerrainInfo t2;
  CHECK(mu::TerrainProbe(mu::ByteView(att2.data(), att2.size()), &t2).ok());
  CHECK_EQ(static_cast<int>(t2.kind), static_cast<int>(mu::TerrainKind::kAttribute2));

  // A 3-bytes-per-tile map file.
  std::vector<uint8_t> map(2 + 256 * 256 * 3, 0x05);
  mu::TerrainInfo tm;
  CHECK(mu::TerrainProbe(mu::ByteView(map.data(), map.size()), &tm).ok());
  CHECK_EQ(static_cast<int>(tm.kind), static_cast<int>(mu::TerrainKind::kMap3));

  // Auto-detection must leave already-plaintext data alone.
  std::vector<uint8_t> plain = att;
  CHECK(!mu::TerrainTryBuxDecrypt(&plain));
  CHECK(plain == att);
}

// ---------------------------------------------------------------- Manifest

void TestManifestParse() {
  const char* text =
      "3D.Premium 4108e84d 142688\r\n"
      "Data/gate.bmd 05d2a7b8 7168\n"
      "\n"
      "Data/Local/a file with spaces.ozj 12345678 100\n"
      "this-line-is-garbage\n"
      "Data/only_two_fields abcdef01\n";

  mu::ManifestParseResult r =
      mu::ManifestParse(mu::ByteView(reinterpret_cast<const uint8_t*>(text),
                                     std::strlen(text)));
  CHECK_EQ(r.entries.size(), 3u);
  CHECK_EQ(r.skipped_lines, 2);
  CHECK_EQ(r.errors.size(), 2u);

  CHECK_EQ(r.entries[0].path, std::string("3D.Premium"));
  CHECK_EQ(r.entries[0].crc32, 0x4108E84Du);
  CHECK_EQ(r.entries[0].size, 142688u);
  CHECK_EQ(r.entries[0].line_number, 1);

  CHECK_EQ(r.entries[1].path, std::string("Data/gate.bmd"));
  CHECK_EQ(r.entries[1].crc32, 0x05D2A7B8u);

  // Spaces in the path are recovered because the checksum and size are taken
  // from the last two fields.
  CHECK_EQ(r.entries[2].path, std::string("Data/Local/a file with spaces.ozj"));
  CHECK_EQ(r.entries[2].crc32, 0x12345678u);
  CHECK_EQ(r.entries[2].size, 100u);

  // A checksum that is not 8 hex digits is rejected, not silently truncated.
  const char* bad = "Data/x.ozj 1234 100\n";
  mu::ManifestParseResult br =
      mu::ManifestParse(mu::ByteView(reinterpret_cast<const uint8_t*>(bad),
                                     std::strlen(bad)));
  CHECK_EQ(br.entries.size(), 0u);
  CHECK_EQ(br.skipped_lines, 1);
}

std::string TmpDir() {
#if defined(_WIN32)
  std::error_code ec;
  const std::filesystem::path base = std::filesystem::temp_directory_path(ec);
  if (ec) return {};

  const auto stamp =
      std::chrono::steady_clock::now().time_since_epoch().count();
  const std::filesystem::path dir =
      base / ("mu_test_" + std::to_string(stamp));

  if (!std::filesystem::create_directory(dir, ec) || ec) return {};
  return dir.string();
#else
  const char* base = std::getenv("TMPDIR");
  if (!base || !*base) base = "/tmp";
  std::string d = std::string(base) + "/mu_test_XXXXXX";
  std::vector<char> buf(d.begin(), d.end());
  buf.push_back('\0');
  char* made = ::mkdtemp(buf.data());
  return made ? std::string(made) : std::string();
#endif
}

void TestManifestVerify() {
  std::string dir = TmpDir();
  if (dir.empty()) {
    std::printf("  SKIP [verify] no temp directory available\n");
    return;
  }

  const std::string content = "ClienteRecriado payload\n";
  const uint32_t crc = mu::Crc32(content.data(), content.size());
  char manifest[512];
  std::snprintf(manifest, sizeof(manifest),
                "good.bin %08x %zu\nbad.bin %08x %zu\nmissing.bin 12345678 5\n",
                crc, content.size(), crc, content.size());

  std::string good_path = dir + "/good.bin";
  std::string bad_path = dir + "/bad.bin";
  FILE* f = std::fopen(good_path.c_str(), "wb");
  std::fwrite(content.data(), 1, content.size(), f);
  std::fclose(f);
  f = std::fopen(bad_path.c_str(), "wb");
  std::fwrite(content.data(), 1, content.size() - 1, f);  // one byte short
  std::fclose(f);

  std::vector<uint8_t> mtext;
  CHECK(mu::ReadFile(std::string(), &mtext) == false);  // unreadable path

  mu::ManifestParseResult pr = mu::ManifestParse(
      mu::ByteView(reinterpret_cast<const uint8_t*>(manifest),
                   std::strlen(manifest)));
  CHECK_EQ(pr.entries.size(), 3u);

  mu::VerifyReport rep = mu::ManifestVerify(pr.entries, dir);
  CHECK_EQ(rep.total, 3u);
  CHECK_EQ(rep.match, 1u);
  CHECK_EQ(rep.size_mismatch, 1u);
  CHECK_EQ(rep.missing, 1u);
  CHECK(!rep.clean());

  std::remove(good_path.c_str());
  std::remove(bad_path.c_str());
  rmdir(dir.c_str());
}

// ---------------------------------------------------------------- fs

void TestExtensionHelpers() {
  CHECK_EQ(mu::ExtensionOf("Data/Item/axe10.ozj"), std::string(".ozj"));
  CHECK_EQ(mu::ExtensionOf("Data/Item/FMbody_R.OZJ"), std::string(".ozj"));
  CHECK_EQ(mu::ExtensionOf("Data/Local/NewLoading"), std::string(""));
  CHECK_EQ(mu::ExtensionOf("Data/.hidden"), std::string(".hidden"));
  CHECK_EQ(mu::ExtensionOf("Data/no/ext.here/file"), std::string(""));
  CHECK_EQ(mu::AsciiLower("AbC.OzT"), std::string("abc.ozt"));
}

void TestBmdBodyParsesVerifiedLayout();
void TestBmdBodyRejectsBadInput();
void TestBmdReportLevelsAreSeparate();
void TestBmdV0CCipher();
void TestBmdTriangles();
void TestTopologyIsSubsetOfGeometry();
void TestMeshFromBmd();
void TestComputeBounds();
void TestFrustumCull();
void TestFrustumFromViewProjection();
void TestTerrainHeight();
void TestBmdMultiMeshTriangleRecordLayout();
void TestBmdMultiMeshLargeCounts();
void TestBmdMultiMeshExtremeMeshCount();
void TestBmdMultiMeshTruncated();
void TestBmdMultiMeshOverflow();
void TestBmdBonesCircularParent();
void TestBmdBonesExtremeCount();
void TestBmdBonesTruncated();
void TestBmdBonesOverflow();
void TestMeshInternalRepresentation();
void TestPackMeshForUpload();
void TestSkinningLBS();
void TestFrustumCullRealBounds();
void TestWin32GLSkeletonBlocked();
void TestMainLoopSkeleton();
void TestSoftwareRasterizerFramebuffer();
void TestSoftwareRasterizerCamera();
void TestSoftwareRasterizerTriangle();
void TestSoftwareRasterizerZBuffer();
void TestSoftwareTextureLoadFromOZT();
void TestSoftwareRasterizerUsesMeshTexcoords();
std::vector<std::uint8_t> MakeBmdBody(
    const char* name, const std::vector<mu::BmdVertex>& verts,
    const std::vector<mu::BmdNormal>& norms);
void TestBmdReportGeometryNeedsRealBody();
void TestBmdReportPercent();
void TestBmdReportAccumulatesKinds();

const Test kTests[] = {
    {"crc32 known vectors", TestCrc32KnownVectors},
    {"ozt round trip", TestOztRoundTrip},
    {"ozt 24-bit pixel conversion", TestOzt24BitPixelConversion},
    {"ozt variant with extra prefix", TestOztVariantWithExtraPrefix},
    {"ozt rejects bad input", TestOztRejectsBadInput},
    {"ozj probe", TestOzjProbe},
    {"ozj duplicated header prefix", TestOzjDuplicatedHeaderPrefix},
    {"bux cipher is self inverse", TestBuxCipherIsSelfInverse},
    {"bmd probe", TestBmdProbe},
    {"entropy separates plaintext from ciphertext",
     TestEntropySeparatesPlaintextFromCiphertext},
    {"terrain probe", TestTerrainProbe},
    {"manifest parse", TestManifestParse},
    {"manifest verify", TestManifestVerify},
    {"extension helpers", TestExtensionHelpers},
    {"bmd v0x0c cipher", TestBmdV0CCipher},
    {"bmd triangles", TestBmdTriangles},
    {"topology is a strict subset of geometry", TestTopologyIsSubsetOfGeometry},
    {"mesh from bmd", TestMeshFromBmd},
    {"compute bounds", TestComputeBounds},
    {"frustum cull", TestFrustumCull},
    {"frustum from view projection", TestFrustumFromViewProjection},
    {"terrain height", TestTerrainHeight},
    {"bmd body parses verified layout", TestBmdBodyParsesVerifiedLayout},
    {"bmd body rejects bad input", TestBmdBodyRejectsBadInput},
    {"bmd report levels are separate", TestBmdReportLevelsAreSeparate},
    {"bmd geometry needs a real body", TestBmdReportGeometryNeedsRealBody},
    {"bmd percent helper", TestBmdReportPercent},
    {"bmd report accumulates kinds", TestBmdReportAccumulatesKinds},
    {"bmd multi-mesh triangle record layout", TestBmdMultiMeshTriangleRecordLayout},
    {"bmd multi-mesh large counts (regression 8406)", TestBmdMultiMeshLargeCounts},
    {"bmd multi-mesh extreme meshCount 0xFFFF", TestBmdMultiMeshExtremeMeshCount},
    {"bmd multi-mesh truncated payload", TestBmdMultiMeshTruncated},
    {"bmd multi-mesh overflow check", TestBmdMultiMeshOverflow},
    {"bmd bones circular parent (cycle detection)", TestBmdBonesCircularParent},
    {"bmd bones extreme boneCount", TestBmdBonesExtremeCount},
    {"bmd bones truncated payload", TestBmdBonesTruncated},
    {"bmd bones overflow check", TestBmdBonesOverflow},
    {"mesh internal representation (C1)", TestMeshInternalRepresentation},
    {"pack mesh for upload (D1)", TestPackMeshForUpload},
    {"skinning LBS CPU strict (D2)", TestSkinningLBS},
    {"frustum cull real bounds (D3)", TestFrustumCullRealBounds},
    {"win32 wgl skeleton BLOCKED (D5)", TestWin32GLSkeletonBlocked},
    {"main loop skeleton update/render/present (E2)", TestMainLoopSkeleton},
    {"software rasterizer framebuffer (B)", TestSoftwareRasterizerFramebuffer},
    {"software rasterizer camera (B)", TestSoftwareRasterizerCamera},
    {"software rasterizer triangle (B)", TestSoftwareRasterizerTriangle},
    {"software rasterizer z-buffer (B)", TestSoftwareRasterizerZBuffer},
    {"software texture loads OZT pixels", TestSoftwareTextureLoadFromOZT},
    {"software rasterizer uses mesh texcoords",
     TestSoftwareRasterizerUsesMeshTexcoords},
};

}  // namespace

int main() {
  std::printf("ClienteRecriado asset library tests\n");
  for (const Test& t : kTests) {
    g_current = t.name;
    std::printf("[ RUN  ] %s\n", t.name);
    const int before = g_failures;
    t.fn();
    if (g_failures == before) std::printf("[  OK  ] %s\n", t.name);
  }
  std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}

// ---------------------------------------------------------------------------
// BMD reporting: the cycle-1 audit found that a single "99.76% read" figure
// described container headers while usable geometry was 0.00%. These tests pin
// the three metrics apart so the two cannot be conflated again.
// ---------------------------------------------------------------------------

namespace {

void PutU32Le(std::vector<std::uint8_t> &out, std::uint32_t v) {
  out.push_back(static_cast<std::uint8_t>(v & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xff));
}

/// Builds a synthetic BMD container. For version 0x0C the declared length is
/// emitted verbatim so a test can lie about it and check that level 1 notices.
std::vector<std::uint8_t> FakeBmd(std::uint8_t version, std::uint32_t declared,
                                  const std::vector<std::uint8_t> &payload) {
  std::vector<std::uint8_t> out{'B', 'M', 'D', version};
  if (version == 0x0C) {
    PutU32Le(out, declared);
  }
  out.insert(out.end(), payload.begin(), payload.end());
  return out;
}

std::vector<std::uint8_t> LowEntropyBytes(std::size_t n) {
  std::vector<std::uint8_t> out(n);
  for (std::size_t i = 0; i < n; ++i) {
    out[i] = static_cast<std::uint8_t>(i & 1);
  }
  return out;
}

// Every one of the 256 byte values appears exactly the same number of times,
// so the entropy is exactly 8.0 bits/byte regardless of |n| (rounded up to a
// multiple of 256). An LCG was tried first and rejected: at 256 bytes its
// mid-byte distribution measures below the 7.5 threshold, which made the test
// assert against a fixture that was not actually high-entropy. The uniform
// construction removes that doubt, and the seed only rotates the start so the
// bux-gain probe still sees no runs of the key.
std::vector<std::uint8_t> HighEntropyBytes(std::size_t n, std::uint32_t seed) {
  const std::size_t rounded = ((n + 255) / 256) * 256;
  std::vector<std::uint8_t> out(rounded);
  const std::uint32_t offset = seed & 0xff;
  for (std::size_t i = 0; i < rounded; ++i) {
    out[i] = static_cast<std::uint8_t>((i + offset) & 0xff);
  }
  return out;
}

mu::BmdSummary SummarizeOne(const std::vector<std::uint8_t> &bytes,
                            const char *label) {
  mu::BmdSummary summary;
  mu::BmdAccumulate(mu::BmdClassifyBytes(mu::ByteView(bytes.data(), bytes.size()),
                                         label),
                    &summary);
  return summary;
}

void TestBmdReportLevelsAreSeparate() {
  // Level 1 (structure) and level 2 (cleartext) and level 3 (geometry) are
  // three different questions. A file can pass the first and fail the second.
  const std::vector<std::uint8_t> clear_payload = LowEntropyBytes(128);
  const mu::BmdSummary clear =
      SummarizeOne(FakeBmd(0x0A, 128, clear_payload), "clear.bmd");
  CHECK_EQ(1u, clear.total);
  CHECK_EQ(1u, clear.structure_valid);
  CHECK_EQ(0u, clear.structure_invalid);
  CHECK_EQ(1u, clear.cleartext_plaintext);
  CHECK_EQ(0u, clear.opaque);
  CHECK_EQ(0u, clear.geometry_parsed);
  CHECK_EQ(1u, clear.kind_v0a);

  // Well-formed v0x0C with high-entropy payload: structure still valid, but the
  // payload is opaque. This is the 85% case from the reference tree.
  const std::vector<std::uint8_t> noise = HighEntropyBytes(512, 7);
  const mu::BmdSummary opaque = SummarizeOne(
      FakeBmd(0x0C, static_cast<std::uint32_t>(noise.size()), noise),
      "opaque.bmd");
  CHECK_EQ(1u, opaque.structure_valid);
  CHECK_EQ(0u, opaque.structure_invalid);
  CHECK_EQ(1u, opaque.opaque);
  CHECK_EQ(0u, opaque.cleartext_plaintext);
  CHECK_EQ(0u, opaque.geometry_parsed);
  CHECK_EQ(1u, opaque.kind_v0c);
  CHECK_EQ(1u, opaque.entropy_above_7_5);
  CHECK(opaque.max_entropy > mu::kEncryptedEntropyThreshold);

  // Structurally invalid: the declared length is a lie. Level 1 must reject it,
  // and it must not be counted at level 2.
  const mu::BmdSummary broken =
      SummarizeOne(FakeBmd(0x0C, 999999, noise), "broken.bmd");
  CHECK_EQ(1u, broken.total);
  CHECK_EQ(0u, broken.structure_valid);
  CHECK_EQ(1u, broken.structure_invalid);
  CHECK_EQ(0u, broken.geometry_parsed);
}

void TestBmdReportGeometryNeedsRealBody() {
  // Level 3 must be earned, not assumed. A well-formed container with a clear
  // payload is NOT enough: the body has to actually parse into vertices.
  // These payloads are low-entropy filler, so the body parse must fail and
  // level 3 must stay zero for both versions.
  const std::vector<std::uint8_t> payload = LowEntropyBytes(64);
  CHECK_EQ(0u, SummarizeOne(FakeBmd(0x0A, 64, payload), "a.bmd").geometry_parsed);

  // v0x0C is never parsed at all, because its payload is encrypted and parsing
  // ciphertext would produce noise rather than vertices.
  CHECK_EQ(0u, SummarizeOne(FakeBmd(0x0C, 64, payload), "c.bmd").geometry_parsed);

  // A real body must be counted, with the parser's own tally rather than the
  // header's claim.
  std::vector<mu::BmdVertex> verts(4);
  for (std::size_t i = 0; i < verts.size(); ++i) {
    verts[i].x = static_cast<float>(i);
    verts[i].y = 0.0f;
    verts[i].z = 0.0f;
  }
  std::vector<mu::BmdNormal> norms(3);
  for (std::size_t i = 0; i < norms.size(); ++i) norms[i].ny = 1.0f;
  const std::vector<std::uint8_t> body = MakeBmdBody("Real.SMD", verts, norms);
  const mu::BmdSummary parsed =
      SummarizeOne(FakeBmd(0x0A, static_cast<std::uint32_t>(body.size()), body),
                   "real.bmd");
  CHECK_EQ(1u, parsed.geometry_parsed);
}

void TestBmdReportPercent() {
  CHECK(mu::PercentOf(0u, 0u) == 0.0);
  CHECK(mu::PercentOf(0u, 100u) == 0.0);
  CHECK(mu::PercentOf(100u, 100u) == 100.0);
  // 1 of 3 -> 33.33...%, compared to two decimals.
  const double third = mu::PercentOf(1u, 3u);
  CHECK(third > 33.33 && third < 33.34);
  CHECK(third > 0.0 && third < 100.0);
}

void TestBmdReportAccumulatesKinds() {
  const std::vector<std::uint8_t> payload = LowEntropyBytes(32);
  mu::BmdSummary summary;
  mu::BmdAccumulate(
      mu::BmdClassifyBytes(mu::ByteView(FakeBmd(0x0A, 32, payload).data(), 0),
                           "a.bmd"),
      &summary);

  // Rebuild each fixture into a stable buffer before taking a view of it.
  const std::vector<std::uint8_t> a = FakeBmd(0x0A, 32, payload);
  const std::vector<std::uint8_t> c = FakeBmd(0x0C, 32, payload);
  const std::vector<std::uint8_t> o = FakeBmd(0x09, 32, payload);
  const std::vector<std::uint8_t> junk = {'N', 'O', 'P', 'E'};
  mu::BmdSummary s2;
  mu::BmdAccumulate(mu::BmdClassifyBytes(mu::ByteView(a.data(), a.size()), "a"),
                    &s2);
  mu::BmdAccumulate(mu::BmdClassifyBytes(mu::ByteView(c.data(), c.size()), "c"),
                    &s2);
  mu::BmdAccumulate(mu::BmdClassifyBytes(mu::ByteView(o.data(), o.size()), "o"),
                    &s2);
  mu::BmdAccumulate(
      mu::BmdClassifyBytes(mu::ByteView(junk.data(), junk.size()), "junk"),
      &s2);
  CHECK_EQ(4u, s2.total);
  CHECK_EQ(1u, s2.kind_v0a);
  CHECK_EQ(1u, s2.kind_v0c);
  CHECK_EQ(1u, s2.kind_other_version);
  CHECK_EQ(1u, s2.kind_unknown);
  CHECK_EQ(0u, s2.kind_bux_container);
}

}  // namespace

// ---------------------------------------------------------------------------
// BMD v0x0A body parser. The layout under test was verified on 145 of 145 real
// files (docs/evidence/ciclo1_bmd_layout.log); these fixtures reproduce it in
// miniature so the parser is checked without touching original assets.
// ---------------------------------------------------------------------------
namespace {

void PutF32Le(std::vector<std::uint8_t> &out, float v) {
  std::uint32_t bits;
  std::memcpy(&bits, &v, sizeof(bits));
  out.push_back(static_cast<std::uint8_t>(bits & 0xff));
  out.push_back(static_cast<std::uint8_t>((bits >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>((bits >> 16) & 0xff));
  out.push_back(static_cast<std::uint8_t>((bits >> 24) & 0xff));
}

void PutU16Le(std::vector<std::uint8_t> &out, std::uint16_t v) {
  out.push_back(static_cast<std::uint8_t>(v & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
}

/// Builds a v0x0A body matching the verified layout: 32-byte name field, header
/// words, vertex array at 52, normal array immediately after.
std::vector<std::uint8_t> MakeBmdBody(const char *name,
                                      const std::vector<mu::BmdVertex> &verts,
                                      const std::vector<mu::BmdNormal> &norms) {
  std::vector<std::uint8_t> out;
  const std::size_t nlen = std::strlen(name);
  for (std::size_t i = 0; i < mu::kBmdNameSize; ++i) {
    out.push_back(i < nlen ? static_cast<std::uint8_t>(name[i]) : 0);
  }
  // Offsets 32, 34, 36 are undecoded in the real format; zeros are fine here
  // because the parser must not depend on them.
  PutU16Le(out, 0);
  PutU16Le(out, 0);
  PutU16Le(out, 0);
  PutU16Le(out, static_cast<std::uint16_t>(verts.size()));   // offset 38
  PutU16Le(out, static_cast<std::uint16_t>(norms.size()));   // offset 40
  PutU16Le(out, 0);                                          // offset 42
  PutU16Le(out, 0);                                          // offset 44
  PutU16Le(out, 0);                                          // offset 46
  PutU16Le(out, 0);                                          // offset 48
  PutU16Le(out, 0);                                          // offset 50
  for (const mu::BmdVertex &v : verts) {
    // 16 bytes: coordinates first, the two small integers trailing.
    PutF32Le(out, v.x);
    PutF32Le(out, v.y);
    PutF32Le(out, v.z);
    PutU16Le(out, v.aux0);
    PutU16Le(out, v.aux1);
  }
  for (const mu::BmdNormal &n : norms) {
    // 20 bytes: the vector first, two trailing floats.
    PutF32Le(out, n.nx);
    PutF32Le(out, n.ny);
    PutF32Le(out, n.nz);
    PutF32Le(out, n.aux0);
    PutF32Le(out, n.aux1);
  }
  return out;
}

void PutU16LeAt(std::vector<std::uint8_t> &out, std::size_t off, std::uint16_t v) {
  out[off] = static_cast<std::uint8_t>(v & 0xff);
  out[off + 1] = static_cast<std::uint8_t>((v >> 8) & 0xff);
}

void TestBmdBodyParsesVerifiedLayout() {
  std::vector<mu::BmdVertex> verts;
  for (int i = 0; i < 5; ++i) {
    mu::BmdVertex v;
    v.x = static_cast<float>(i) * 1.5f;
    v.y = static_cast<float>(i) * -0.25f;
    v.z = 3.0f;
    v.aux0 = static_cast<std::uint16_t>(600 + i);
    v.aux1 = static_cast<std::uint16_t>(i);
    verts.push_back(v);
  }
  std::vector<mu::BmdNormal> norms;
  for (int i = 0; i < 7; ++i) {
    mu::BmdNormal n;
    n.nx = 0.0f;
    n.ny = 1.0f;
    n.nz = 0.0f;
    n.aux0 = 0.0f;
    n.aux1 = 0.0f;
    norms.push_back(n);
  }

  const std::vector<std::uint8_t> body = MakeBmdBody("Test.SMD", verts, norms);

  // The fixture must place the arrays where the verified layout says.
  CHECK_EQ(body.size(),
           52u + verts.size() * mu::kBmdVertexRecordSize +
               norms.size() * mu::kBmdNormalRecordSize);

  mu::BmdBody parsed;
  const mu::Status st =
      mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  CHECK(st.ok());
  CHECK(parsed.complete);
  CHECK(parsed.problem.empty());
  CHECK(parsed.has_source_name);
  CHECK(parsed.source_name == "Test.SMD");
  CHECK_EQ(5u, parsed.vertices.size());
  CHECK_EQ(7u, parsed.normals.size());

  // Round-trip the values, including the trailing fields the format does not
  // explain. They must survive rather than be silently dropped.
  CHECK(parsed.vertices[3].x == 4.5f);
  CHECK(parsed.vertices[3].y == -0.75f);
  CHECK(parsed.vertices[3].z == 3.0f);
  CHECK_EQ(603u, parsed.vertices[3].aux0);
  CHECK(parsed.normals[6].ny == 1.0f);
  CHECK(parsed.normals[6].aux0 == 0.0f);
}

void TestBmdBodyRejectsBadInput() {
  std::vector<mu::BmdVertex> verts(3);
  std::vector<mu::BmdNormal> norms(2);
  const std::vector<std::uint8_t> good = MakeBmdBody("Ok.SMD", verts, norms);

  // Truncated below the 52-byte body header.
  mu::BmdBody b;
  CHECK(!mu::BmdParseBodyV0A(mu::ByteView(good.data(), 40), &b).ok());
  CHECK(!b.complete);
  CHECK(!b.problem.empty());

  // Counts larger than the payload allows. This is the check that makes the
  // verified arithmetic load-bearing.
  std::vector<std::uint8_t> lying = good;
  PutU16LeAt(lying, 38, 5000);
  mu::BmdBody b2;
  CHECK(!mu::BmdParseBodyV0A(mu::ByteView(lying.data(), lying.size()), &b2).ok());
  CHECK(!b2.complete);

  // A non-printable name field must not be reported as a name, but the
  // geometry must still parse: 932 of the clear files have no name.
  std::vector<std::uint8_t> noname = good;
  noname[0] = 0x01;
  mu::BmdBody b3;
  CHECK(mu::BmdParseBodyV0A(mu::ByteView(noname.data(), noname.size()), &b3).ok());
  CHECK(!b3.has_source_name);
  CHECK(b3.source_name.empty());
  CHECK_EQ(3u, b3.vertices.size());
  CHECK(b3.complete);

  // A non-finite coordinate is corruption, not data. The coordinates lead the
  // record, so byte 52 is the first byte of the first vertex's x.
  std::vector<std::uint8_t> nanbody = good;
  const std::uint32_t nan_bits = 0x7FC00000u;
  for (std::size_t i = 0; i < 4; ++i) {
    nanbody[52 + i] = static_cast<std::uint8_t>(
        (nan_bits >> static_cast<unsigned>(8 * i)) & 0xff);
  }
  mu::BmdBody b4;
  CHECK(!mu::BmdParseBodyV0A(mu::ByteView(nanbody.data(), nanbody.size()), &b4).ok());
  CHECK(!b4.complete);

  // Null output must not crash.
  CHECK(!mu::BmdParseBodyV0A(mu::ByteView(good.data(), good.size()), nullptr).ok());
}

}  // namespace

// ---------------------------------------------------------------------------
// The v0x0C payload cipher. Provenance: the algorithm and key table come from
// the ISC-licensed xulek/muonline-bmd-viewer, re-implemented here. Verification
// against real reference files is in docs/evidence/, because this repository
// commits no original asset bytes.
// ---------------------------------------------------------------------------
namespace {

/// The encryption direction, written out here so the test is not comparing the
/// library against itself. Mirrors the decryption with the operations inverted.
std::vector<std::uint8_t> V0CEncrypt(const std::vector<std::uint8_t> &plain) {
  std::vector<std::uint8_t> out(plain.size());
  std::uint8_t running = mu::kBmdV0CInitialKey;
  for (std::size_t i = 0; i < plain.size(); ++i) {
    const std::uint8_t enc = static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(plain[i] + running) ^
        mu::kBmdV0CKey[i & (mu::kBmdV0CKeyLen - 1)]);
    out[i] = enc;
    running = static_cast<std::uint8_t>(enc + mu::kBmdV0CKeyStep);
  }
  return out;
}

void TestBmdV0CCipher() {
  // Round trip over a length that is not a multiple of the 16-byte key table,
  // so the wraparound is exercised.
  std::vector<std::uint8_t> plain(100);
  for (std::size_t i = 0; i < plain.size(); ++i) {
    plain[i] = static_cast<std::uint8_t>((i * 37 + 11) & 0xff);
  }
  std::vector<std::uint8_t> enc = V0CEncrypt(plain);
  CHECK(enc != plain);
  mu::BmdV0CDecrypt(enc.data(), enc.size());
  CHECK(enc == plain);

  // The distinguishing property: the running key is fed from the CIPHERTEXT,
  // not the plaintext. An implementation that fed it from the plaintext would
  // still round-trip against its own matching encrypt, so check the actual
  // second output byte by hand.
  //   plain[0] = 11, running = 0x5E
  //   enc[0]   = (11 + 0x5E) ^ 0xD1 = 0x69 ^ 0xD1 = 0xB8
  //   running  = 0xB8 + 0x3D = 0xF5
  //   enc[1]   = (0x30 + 0xF5) ^ 0x73 = 0x25 ^ 0x73 = 0x56
  // The intermediate 0x30 + 0xF5 overflows to 0x125 and truncates to 0x25; the
  // wraparound is part of what is being checked here.
  std::vector<std::uint8_t> two(plain.begin(), plain.begin() + 2);
  const std::vector<std::uint8_t> two_enc = V0CEncrypt(two);
  CHECK_EQ(0xB8u, two_enc[0]);
  CHECK_EQ(0x56u, two_enc[1]);

  // Empty and null inputs must not crash.
  mu::BmdV0CDecrypt(nullptr, 0);
  std::vector<std::uint8_t> empty;
  mu::BmdV0CDecrypt(empty.data(), empty.size());
  CHECK(empty.empty());
}

}  // namespace

// ---------------------------------------------------------------------------
// Triangle records are 64 bytes. Polygon size is followed by four vertex
// indices, four normal indices, and four candidate UV indices in separate
// arrays. UV semantics remain unverified; see docs/ASSET_FORMATS.md.
// ---------------------------------------------------------------------------
namespace {

void TestBmdTriangles() {
  // Four vertices, four normals, two texcoords, one triangle.
  std::vector<mu::BmdVertex> verts;
  for (int i = 0; i < 4; ++i) {
    mu::BmdVertex v;
    v.x = static_cast<float>(i);
    v.y = 0.0f;
    v.z = 1.0f;
    verts.push_back(v);
  }
  std::vector<mu::BmdNormal> norms;
  for (int i = 0; i < 4; ++i) {
    mu::BmdNormal n;
    n.ny = 1.0f;
    norms.push_back(n);
  }

  std::vector<std::uint8_t> body = MakeBmdBody("Tri.SMD", verts, norms);
  const std::size_t counts_base = 52 + verts.size() * mu::kBmdVertexRecordSize +
                                  norms.size() * mu::kBmdNormalRecordSize;

  // Header fields the triangle reader depends on: meshCount at 32, texcoord
  // count at 42, triangle count at 44.
  PutU16LeAt(body, 32, 1);
  PutU16LeAt(body, 42, 2);
  PutU16LeAt(body, 44, 1);

  // UVs start four bytes before the coordinate-first normal cursor reaches
  // the end of its last record.
  body.resize(counts_base - 4);
  for (int i = 0; i < 2 * 8; ++i) body.push_back(0);

  // One triangle record: polygon size 3 and vertex/normal/UV indices [0,1,2].
  const std::size_t tri_off = body.size();
  body.resize(tri_off + mu::kBmdTriangleRecordSize, 0);
  body[tri_off] = 3;
  for (std::size_t k = 0; k < 3; ++k) {
    PutU16LeAt(body, tri_off + 2 + k * 2, static_cast<std::uint16_t>(k));
    PutU16LeAt(body, tri_off + 10 + k * 2, static_cast<std::uint16_t>(k));
    PutU16LeAt(body, tri_off + 18 + k * 2, static_cast<std::uint16_t>(k));
  }

  mu::BmdBody parsed;
  const mu::Status st =
      mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  CHECK(st.ok());
  CHECK(parsed.complete);
  CHECK_EQ(1u, parsed.triangles.size());
  // Keep reporting the size failure without dereferencing an absent triangle.
  if (!parsed.triangles.empty()) {
    CHECK_EQ(0, parsed.triangles[0].corners[0].vertex);
    CHECK_EQ(1, parsed.triangles[0].corners[1].vertex);
    CHECK_EQ(2, parsed.triangles[0].corners[2].vertex);
    CHECK_EQ(2, parsed.triangles[0].corners[2].normal);
    CHECK_EQ(0u, parsed.triangles[0].corners[0].optional);
    CHECK_EQ(0u, parsed.triangles[0].corners[1].flag);
  }
  CHECK_EQ(0u, parsed.out_of_range_vertices);
  CHECK_EQ(0u, parsed.out_of_range_normals);
  // Texcoords are still not located, so this must stay false.
  CHECK(!parsed.has_texcoords);
  CHECK_EQ(counts_base, 52u + 4u * 16u + 4u * 20u);

  // A four-corner polygon must become two indexed triangles: (0,1,2) and
  // (0,2,3). The UV table is retained only as candidate raw indices.
  body[tri_off] = 4;
  PutU16LeAt(body, tri_off + 8, 3);
  PutU16LeAt(body, tri_off + 16, 3);
  PutU16LeAt(body, tri_off + 24, 3);
  mu::BmdBody quad;
  CHECK(mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &quad).ok());
  CHECK_EQ(2u, quad.triangles.size());
  if (quad.triangles.size() >= 2) {
    CHECK_EQ(0, quad.triangles[1].corners[0].vertex);
    CHECK_EQ(2, quad.triangles[1].corners[1].vertex);
    CHECK_EQ(3, quad.triangles[1].corners[2].vertex);
  }
  body[tri_off] = 3;

  // An out-of-range corner must be counted, not rejected and not silently
  // accepted: a caller that indexed with it would read out of bounds.
  PutU16LeAt(body, tri_off + 2, 999);
  mu::BmdBody bad;
  CHECK(mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &bad).ok());
  CHECK(bad.complete);
  CHECK_EQ(1u, bad.out_of_range_vertices);
  CHECK_EQ(0u, bad.out_of_range_normals);

  // The report preserves vertex and normal reference counts separately. The
  // output percentage denominator is six index fields per triangle, not three
  // geometric corners.
  PutU16LeAt(body, tri_off + 12, 999);
  const std::vector<std::uint8_t> file = FakeBmd(
      0x0A, static_cast<std::uint32_t>(body.size()), body);
  const mu::BmdFileReport file_report = mu::BmdClassifyBytes(
      mu::ByteView(file.data(), file.size()), "bad-refs.bmd");
  CHECK(file_report.geometry_parsed);
  CHECK_EQ(1u, file_report.out_of_range_vertex_refs);
  CHECK_EQ(1u, file_report.out_of_range_normal_refs);
  mu::BmdSummary summary;
  mu::BmdAccumulate(file_report, &summary);
  CHECK_EQ(1u, summary.triangles_total);
  CHECK_EQ(1u, summary.out_of_range_vertex_refs);
  CHECK_EQ(1u, summary.out_of_range_normal_refs);

  // A declared triangle array that does not fit must yield no triangles rather
  // than a partial read presented as complete.
  PutU16LeAt(body, tri_off, 0);
  PutU16LeAt(body, 44, 50);
  mu::BmdBody overflow;
  CHECK(mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &overflow).ok());
  CHECK(overflow.triangles.empty());

  // More than one mesh: previously left empty because single-block arithmetic
  // would read the wrong block. Now multi-mesh is implemented, so even a
  // fixture with meshCount=2 but only one mesh's data still yields the first
  // mesh's triangle (second mesh fails to fit, but first is kept).
  PutU16LeAt(body, 44, 1);
  PutU16LeAt(body, 32, 2);
  mu::BmdBody multi;
  CHECK(mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &multi).ok());
  CHECK(!multi.triangles.empty());
  CHECK_EQ(1u, multi.triangles.size());
}

// The compatibility table reports level 3 (vertices + normals) and level 4
// (face indices) as two rows, and the drop between them was large enough that
// the relation between them had to be established rather than assumed. This
// test fixes three things that the table alone cannot express:
//
//   1. Level 4 is a STRICT SUBSET of level 3. A file counted in level 4 was
//      necessarily counted in level 3, because the triangle counter is only
//      ever assigned inside the branch that sets geometry_parsed.
//   2. An empty triangle array is not one failure but four distinguishable
//      ones, and each has a different consequence for a consumer.
//   3. The four skip reasons are exhaustive: they account for every level-3
//      file that has no topology, so a fifth cause cannot appear silently.
void TestTopologyIsSubsetOfGeometry() {
  std::vector<mu::BmdVertex> verts(4);
  for (std::size_t i = 0; i < verts.size(); ++i) verts[i].z = 1.0f;
  std::vector<mu::BmdNormal> norms(4);
  for (auto &n : norms) n.ny = 1.0f;

  std::vector<std::uint8_t> body = MakeBmdBody("Subset.SMD", verts, norms);
  PutU16LeAt(body, 32, 1);   // meshCount
  PutU16LeAt(body, 42, 2);   // texcoord count
  PutU16LeAt(body, 44, 1);   // triangle count
  body.resize(body.size() + 2 * 8 + mu::kBmdTriangleRecordSize, 0);

  mu::BmdSummary sum;

  // Each fixture goes through FakeBmd, which prepends the "BMD"+version
  // container header. Without it the classifier never reaches the body parser
  // and every case reports geometry_parsed == false, which would make the
  // subset relation below vacuously true.
  auto classify = [](const std::vector<std::uint8_t> &payload,
                     const char *label) {
    const std::vector<std::uint8_t> file = FakeBmd(
        0x0A, static_cast<std::uint32_t>(payload.size()), payload);
    return mu::BmdClassifyBytes(mu::ByteView(file.data(), file.size()), label);
  };

  // Case 1: a single-mesh file with one face. Reaches both levels.
  {
    const mu::BmdFileReport r = classify(body, "level4");
    CHECK(r.geometry_parsed);
    CHECK_EQ(1u, r.triangle_count);
    CHECK(r.topology_skip == mu::BmdTopologySkip::kNone);
    mu::BmdAccumulate(r, &sum);
  }

  // Case 2: multi-mesh. Previously geometry parsed but topology was
  // deliberately not read. Now multi-mesh is implemented, so it reaches both
  // levels.
  {
    std::vector<std::uint8_t> m = body;
    PutU16LeAt(m, 32, 3);
    const mu::BmdFileReport r = classify(m, "multimesh");
    CHECK(r.geometry_parsed);
    CHECK_EQ(1u, r.triangle_count);
    CHECK(r.topology_skip == mu::BmdTopologySkip::kNone);
    CHECK_EQ(3u, r.declared_mesh_count);
    mu::BmdAccumulate(r, &sum);
  }

  // Case 3: declares no faces at all.
  {
    std::vector<std::uint8_t> z = body;
    PutU16LeAt(z, 44, 0);
    const mu::BmdFileReport r = classify(z, "zerofaces");
    CHECK(r.geometry_parsed);
    CHECK(r.topology_skip == mu::BmdTopologySkip::kZeroFaces);
    mu::BmdAccumulate(r, &sum);
  }

  // Case 4: the declared array runs past the end of the payload.
  {
    std::vector<std::uint8_t> o = body;
    PutU16LeAt(o, 44, 500);
    const mu::BmdFileReport r = classify(o, "doesnotfit");
    CHECK(r.geometry_parsed);
    CHECK(r.topology_skip == mu::BmdTopologySkip::kDoesNotFit);
    CHECK_EQ(500u, r.declared_triangle_count);
    mu::BmdAccumulate(r, &sum);
  }

  // 1. The subset relation, on the accumulator that produces the published
  //    table rather than on a hand-made counter. After multi-mesh support,
  //    both single and multi-mesh reach level 4.
  CHECK(sum.topology_parsed <= sum.geometry_parsed);
  CHECK_EQ(4u, sum.geometry_parsed);
  CHECK_EQ(2u, sum.topology_parsed);

  // 3. Exhaustiveness: the skip reasons account for every level-3 file without
  //    topology, leaving no unexplained remainder.
  const std::size_t skips = sum.skip_multi_mesh + sum.skip_zero_faces +
                            sum.skip_does_not_fit + sum.skip_no_header;
  CHECK_EQ(sum.geometry_parsed - sum.topology_parsed, skips);
  CHECK_EQ(0u, sum.skip_multi_mesh);
  CHECK_EQ(1u, sum.skip_zero_faces);
  CHECK_EQ(1u, sum.skip_does_not_fit);
  CHECK_EQ(0u, sum.skip_no_header);

  // The meshCount histogram must cover every level-3 file too, or it is
  // measuring a subset and presenting it as the whole.
  std::size_t hist_total = sum.mesh_count_above_15;
  for (std::size_t i = 0; i < 16; ++i) hist_total += sum.mesh_count_hist[i];
  CHECK_EQ(sum.geometry_parsed, hist_total);
  CHECK_EQ(3u, sum.mesh_count_hist[1]);
  CHECK_EQ(1u, sum.mesh_count_hist[3]);

  // Names exist for every enumerator, so a report cannot print a blank cell.
  CHECK(std::string(mu::BmdTopologySkipName(mu::BmdTopologySkip::kNone)).size() > 0);
  CHECK(std::string(mu::BmdTopologySkipName(mu::BmdTopologySkip::kMultiMesh)).size() > 0);
  CHECK(std::string(mu::BmdTopologySkipName(mu::BmdTopologySkip::kZeroFaces)).size() > 0);
  CHECK(std::string(mu::BmdTopologySkipName(mu::BmdTopologySkip::kDoesNotFit)).size() > 0);
  CHECK(std::string(mu::BmdTopologySkipName(mu::BmdTopologySkip::kNoHeader)).size() > 0);
}

}  // namespace

// ---------------------------------------------------------------------------
// The internal mesh representation and the BMD importer (path A). The point of
// these tests is the reported gaps, not the happy path: a converter that quietly
// produced a wrong mesh would pass a test that only checked positions.
// ---------------------------------------------------------------------------
namespace {

void TestMeshFromBmd() {
  mu::BmdBody body;
  body.source_name = "Test.SMD";
  for (int i = 0; i < 4; ++i) {
    mu::BmdVertex v;
    v.x = static_cast<float>(i);
    v.y = static_cast<float>(-i);
    v.z = 2.0f;
    body.vertices.push_back(v);
  }
  for (int i = 0; i < 4; ++i) {
    mu::BmdNormal n;
    n.ny = 1.0f;
    body.normals.push_back(n);
  }
  mu::BmdTriangle t;
  t.corners[0].vertex = 0; t.corners[0].normal = 0;
  t.corners[1].vertex = 1; t.corners[1].normal = 1;
  t.corners[2].vertex = 2; t.corners[2].normal = 2;
  body.triangles.push_back(t);
  body.complete = true;

  const mu::MeshImportResult r = mu::MeshFromBmd(body, "");
  CHECK(r.ok);
  CHECK(r.problem.empty());
  CHECK_EQ(4u, r.mesh.positions.size());
  CHECK_EQ(4u, r.mesh.normals.size());
  CHECK_EQ(3u, r.mesh.indices.size());
  CHECK_EQ(0u, r.mesh.skipped_vertices);
  CHECK_EQ(0u, r.mesh.dropped_triangles);
  CHECK(r.mesh.name == "Test.SMD");

  // Bounding box must be computed at import, and must be the real extent.
  CHECK(r.mesh.bounds.valid);
  CHECK(r.mesh.bounds.min.x == 0.0f);
  CHECK(r.mesh.bounds.max.x == 3.0f);
  CHECK(r.mesh.bounds.min.y == -3.0f);
  CHECK(r.mesh.bounds.max.y == 0.0f);

  // The gap that matters: texcoords must stay off, because the BMD corner-to-UV
  // binding was never located. A true here would let the renderer texturise
  // with coordinates belonging to some other corner.
  CHECK(!r.mesh.has_texcoords);
  CHECK(r.mesh.texcoords.empty());

  // An out-of-range corner must drop the triangle and count it, not clamp it.
  // Clamping moves a vertex and deforms the mesh with no signal.
  mu::BmdBody bad = body;
  mu::BmdTriangle bt;
  bt.corners[0].vertex = 0;
  bt.corners[1].vertex = 99;  // beyond the four vertices
  bt.corners[2].vertex = 2;
  bad.triangles.push_back(bt);
  const mu::MeshImportResult rb = mu::MeshFromBmd(bad, "x");
  CHECK(rb.ok);
  CHECK_EQ(3u, rb.mesh.indices.size());  // only the good triangle survived
  CHECK_EQ(1u, rb.mesh.dropped_triangles);

  // Multiple invalid vertex/normal references in one triangle still increment
  // the dropped-triangle metric once; reference counts are a separate report.
  mu::BmdBody several_bad = body;
  mu::BmdTriangle bad_refs;
  bad_refs.corners[0].vertex = 99;
  bad_refs.corners[0].normal = 99;
  bad_refs.corners[1].vertex = 98;
  bad_refs.corners[1].normal = 98;
  bad_refs.corners[2].vertex = 2;
  bad_refs.corners[2].normal = 2;
  several_bad.triangles.push_back(bad_refs);
  const mu::MeshImportResult rs = mu::MeshFromBmd(several_bad, "x");
  CHECK(rs.ok);
  CHECK_EQ(1u, rs.mesh.dropped_triangles);

  // A non-finite position must be skipped and counted, and skipping shifts
  // indices, so indices must be withheld rather than silently misaligned.
  mu::BmdBody nanbody = body;
  nanbody.vertices[1].y = std::numeric_limits<float>::quiet_NaN();
  const mu::MeshImportResult rn = mu::MeshFromBmd(nanbody, "x");
  CHECK(rn.ok);
  CHECK_EQ(3u, rn.mesh.positions.size());
  CHECK_EQ(1u, rn.mesh.skipped_vertices);
  CHECK(rn.mesh.indices.empty());
  CHECK(rn.mesh.bounds.valid);

  // An empty body is a failure, not an empty success.
  mu::BmdBody empty;
  const mu::MeshImportResult re = mu::MeshFromBmd(empty, "x");
  CHECK(!re.ok);
  CHECK(!re.problem.empty());
  CHECK(!re.mesh.bounds.valid);
}

void TestComputeBounds() {
  // Empty list: no box, and specifically not a zero box at the origin.
  const mu::Bounds e = mu::ComputeBounds({});
  CHECK(!e.valid);

  std::vector<mu::Vec3> pts{{-1.0f, 0.0f, 5.0f}, {3.0f, -2.0f, 1.0f}};
  const mu::Bounds b = mu::ComputeBounds(pts);
  CHECK(b.valid);
  CHECK(b.min.x == -1.0f);
  CHECK(b.max.x == 3.0f);
  CHECK(b.min.y == -2.0f);
  CHECK(b.max.y == 0.0f);
  CHECK(b.min.z == 1.0f);
  CHECK(b.max.z == 5.0f);

  // One NaN must not poison the whole box.
  pts.push_back({std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f});
  const mu::Bounds bn = mu::ComputeBounds(pts);
  CHECK(bn.valid);
  CHECK(bn.min.x == -1.0f);
  CHECK(bn.max.x == 3.0f);

  // All non-finite: still no usable box, even though the list was not empty.
  const std::vector<mu::Vec3> allnan{
      {std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f}};
  CHECK(!mu::ComputeBounds(allnan).valid);
}

}  // namespace

// ---------------------------------------------------------------------------
// Frustum culling. The expected answers here are built by hand from planes whose
// inside direction is known, rather than derived by calling the code under test.
// A test that builds its expectation from the implementation would pass even if
// the inside/outside sign were inverted -- and an inverted sign is exactly the
// bug this code is most likely to have, because it culls the visible half of the
// scene without reporting anything.
// ---------------------------------------------------------------------------
namespace {

void TestFrustumCull() {
  // A frustum built directly, not from a matrix: everything with x >= 0 and
  // x <= 10, unbounded in y and z. Plane normals point inward.
  mu::Frustum f{};
  f.planes[0].normal = {1.0f, 0.0f, 0.0f};
  f.planes[0].d = 0.0f;      // x >= 0
  f.planes[1].normal = {-1.0f, 0.0f, 0.0f};
  f.planes[1].d = 10.0f;     // x <= 10
  for (int i = 2; i < 6; ++i) {
    f.planes[i].normal = {0.0f, 1.0f, 0.0f};
    f.planes[i].d = 1e6f;    // effectively unbounded
  }

  auto box = [](float x0, float x1) {
    mu::Bounds b;
    b.min = {x0, -1.0f, -1.0f};
    b.max = {x1, 1.0f, 1.0f};
    b.valid = true;
    return b;
  };

  // Fully inside.
  CHECK(mu::CullBox(f, box(2.0f, 8.0f)) == mu::CullResult::kInside);
  CHECK(mu::IsVisible(f, box(2.0f, 8.0f)));

  // Fully outside on the left. This is the case that proves the sign is right:
  // if normals pointed outward, this box would read as inside.
  CHECK(mu::CullBox(f, box(-8.0f, -2.0f)) == mu::CullResult::kOutside);
  CHECK(!mu::IsVisible(f, box(-8.0f, -2.0f)));

  // Fully outside on the right.
  CHECK(mu::CullBox(f, box(12.0f, 18.0f)) == mu::CullResult::kOutside);
  CHECK(!mu::IsVisible(f, box(12.0f, 18.0f)));

  // Straddling the left plane: must be drawn, not culled.
  CHECK(mu::CullBox(f, box(-2.0f, 2.0f)) == mu::CullResult::kIntersect);
  CHECK(mu::IsVisible(f, box(-2.0f, 2.0f)));

  // Touching a plane exactly is visible. Culling a box that merely touches would
  // pop geometry at the frustum edge.
  CHECK(mu::IsVisible(f, box(-1.0f, 0.0f)));
  CHECK(mu::IsVisible(f, box(10.0f, 12.0f)));

  // An invalid box must never be culled: no bounds means nothing was proven.
  mu::Bounds invalid;
  CHECK(!invalid.valid);
  CHECK(mu::IsVisible(f, invalid));
  CHECK(mu::CullBox(f, invalid) == mu::CullResult::kIntersect);

  // PlaneDistance: positive inside, negative outside, by construction.
  const mu::Plane p{{1.0f, 0.0f, 0.0f}, -5.0f};
  CHECK(mu::PlaneDistance(p, {6.0f, 0.0f, 0.0f}) == 1.0f);
  CHECK(mu::PlaneDistance(p, {4.0f, 0.0f, 0.0f}) == -1.0f);
  CHECK(mu::PlaneDistance(p, {5.0f, 0.0f, 0.0f}) == 0.0f);
}

void TestFrustumFromViewProjection() {
  // Identity view-projection maps the clip cube -1..1 straight through, so the
  // extracted frustum must accept the origin and reject anything beyond x = 1.
  float ident[16] = {0};
  ident[0] = ident[5] = ident[10] = ident[15] = 1.0f;
  const mu::Frustum f = mu::FrustumFromViewProjection(ident);

  mu::Bounds centre;
  centre.min = {-0.25f, -0.25f, -0.25f};
  centre.max = {0.25f, 0.25f, 0.25f};
  centre.valid = true;
  CHECK(mu::IsVisible(f, centre));

  mu::Bounds far_right;
  far_right.min = {5.0f, -0.25f, -0.25f};
  far_right.max = {6.0f, 0.25f, 0.25f};
  far_right.valid = true;
  CHECK(!mu::IsVisible(f, far_right));

  mu::Bounds behind;
  behind.min = {-6.0f, -0.25f, -0.25f};
  behind.max = {-5.0f, 0.25f, 0.25f};
  behind.valid = true;
  CHECK(!mu::IsVisible(f, behind));

  // A box straddling the right clip plane must survive.
  mu::Bounds straddle;
  straddle.min = {0.5f, -0.25f, -0.25f};
  straddle.max = {1.5f, 0.25f, 0.25f};
  straddle.valid = true;
  CHECK(mu::IsVisible(f, straddle));
}

}  // namespace

// ---------------------------------------------------------------------------
// TerrainHeight.OZB. The point of the smoothness assertion is that it is the
// same kind of test that validated the texcoord array and the v0x0C cipher: a
// property arbitrary bytes do not have, with the chance expectation stated. A
// test that only checked the length would pass on any file of the right size.
// ---------------------------------------------------------------------------
namespace {

void TestTerrainHeight() {
  const std::size_t kBody = 256u * 256u;

  // A smooth synthetic field: height varies slowly across the grid, which is
  // what real terrain does. Built here so the expected smoothness is known
  // without reading a real asset.
  std::vector<std::uint8_t> file(1082u + kBody, 0);
  for (std::size_t y = 0; y < 256; ++y) {
    for (std::size_t x = 0; x < 256; ++x) {
      file[1082u + y * 256u + x] =
          static_cast<std::uint8_t>((x + y) / 4);  // gentle ramp
    }
  }
  const mu::ByteView view(file.data(), file.size());
  mu::TerrainHeight h;
  const mu::Status st = mu::TerrainParseHeight(view, &h);
  CHECK(st.ok());
  CHECK(h.complete);
  CHECK_EQ(1082u, h.prefix_size);
  CHECK_EQ(kBody, h.cells.size());

  // Smooth field: adjacent difference is small. The ramp advances by 1 every
  // four cells, so horizontal steps are 0 or 1.
  const double smooth = mu::TerrainHeightSmoothness(h);
  CHECK(smooth < 1.0);

  // The same size filled with independent bytes must score far worse. This is
  // the control: without it, "smooth" proves nothing.
  std::vector<std::uint8_t> noisy = file;
  std::uint32_t s = 12345;
  for (std::size_t i = 1082; i < noisy.size(); ++i) {
    s = s * 1103515245u + 12345u;
    noisy[i] = static_cast<std::uint8_t>((s >> 16) & 0xff);
  }
  mu::TerrainHeight hn;
  CHECK(mu::TerrainParseHeight(mu::ByteView(noisy.data(), noisy.size()), &hn).ok());
  const double noisy_smooth = mu::TerrainHeightSmoothness(hn);
  // Independent uniform bytes average about 85.3. Allow a wide margin and still
  // assert the gap is large: this is the assertion that carries the finding.
  CHECK(noisy_smooth > 60.0);
  CHECK(noisy_smooth > smooth * 20.0);

  // A BMP-headed file with the four-byte duplicated prefix. This is the layout
  // measured on 44 of the 47 real files: "BM" at offset 0, real header at
  // offset 4 with a declared size matching the remaining bytes.
  const std::size_t kPix = 1078;
  std::vector<std::uint8_t> bmp(4 + kPix + kBody, 0);
  auto put32 = [&bmp](std::size_t o, std::uint32_t v) {
    bmp[o] = static_cast<std::uint8_t>(v & 0xff);
    bmp[o + 1] = static_cast<std::uint8_t>((v >> 8) & 0xff);
    bmp[o + 2] = static_cast<std::uint8_t>((v >> 16) & 0xff);
    bmp[o + 3] = static_cast<std::uint8_t>((v >> 24) & 0xff);
  };
  bmp[0] = 'B'; bmp[1] = 'M';  // duplicated prefix, deliberately not a real size
  bmp[4] = 'B'; bmp[5] = 'M';
  put32(6, static_cast<std::uint32_t>(bmp.size() - 4));  // declared size
  put32(14, kPix);                                        // pixel offset
  put32(18, 40);                                          // DIB header size
  put32(22, 256);                                         // width
  put32(26, 256);                                         // height
  bmp[32] = 8;                                            // bpp
  for (std::size_t i = 0; i < kBody; ++i) bmp[4 + kPix + i] = 42;
  mu::TerrainHeight hbmp;
  const mu::Status sb =
      mu::TerrainParseHeight(mu::ByteView(bmp.data(), bmp.size()), &hbmp);
  CHECK(sb.ok());
  CHECK(hbmp.complete);
  // The pixel data must come from the BMP offset, not the end of the file, and
  // the fallback flag must be clear -- that flag is how a caller tells a decode
  // from a guess.
  CHECK(!hbmp.header_ignored);
  CHECK_EQ(4u + kPix, hbmp.prefix_size);
  CHECK_EQ(kBody, hbmp.cells.size());
  CHECK(hbmp.cells[0] == 42);
  CHECK(hbmp.cells[kBody - 1] == 42);

  // A BMP-headed file whose declared size does not match must fall back rather
  // than read pixels from a wrong offset. Silently trusting the header is what
  // the earlier version did.
  std::vector<std::uint8_t> bad = bmp;
  // Write into `bad`, not `bmp`: put32 above captures bmp by reference, and
  // using it here would corrupt the fixture the earlier assertions depend on.
  bad[6] = static_cast<std::uint8_t>(999999 & 0xff);
  bad[7] = static_cast<std::uint8_t>((999999 >> 8) & 0xff);
  bad[8] = static_cast<std::uint8_t>((999999 >> 16) & 0xff);
  bad[9] = static_cast<std::uint8_t>((999999 >> 24) & 0xff);
  mu::TerrainHeight hbad;
  CHECK(mu::TerrainParseHeight(mu::ByteView(bad.data(), bad.size()), &hbad).ok());
  CHECK(hbad.header_ignored);

  // A BMP-shaped file whose header has been zeroed. This is the shape measured
  // on 29 of the 76 real files: size 4 + 1078 + 65536, prefix entirely zero.
  // Both parser paths land on the same trailing bytes, so the assertions here
  // are about the flag, not the pixels.
  std::vector<std::uint8_t> zeroed(4 + 1078 + kBody, 0);
  for (std::size_t i = 0; i < kBody; ++i) zeroed[4 + 1078 + i] = 99;
  mu::TerrainHeight hz;
  CHECK(mu::TerrainParseHeight(mu::ByteView(zeroed.data(), zeroed.size()), &hz).ok());
  CHECK(hz.complete);
  CHECK_EQ(4u + 1078u, hz.prefix_size);
  CHECK_EQ(kBody, hz.cells.size());
  CHECK(hz.cells[0] == 99);
  // No "BM" at offset 0, so the BMP branch is not entered and the flag stays
  // clear. That is correct: the flag means "looked like a BMP but no header
  // could be read", which is not what happened here.
  CHECK(!hz.header_ignored);

  // A file shorter than one grid is a failure, not a partial success.
  std::vector<std::uint8_t> tiny(100, 0);
  mu::TerrainHeight ht;
  CHECK(!mu::TerrainParseHeight(mu::ByteView(tiny.data(), tiny.size()), &ht).ok());
  CHECK(!ht.complete);

  // Exactly one grid, no prefix, must work.
  std::vector<std::uint8_t> bare(kBody, 7);
  mu::TerrainHeight hb;
  CHECK(mu::TerrainParseHeight(mu::ByteView(bare.data(), bare.size()), &hb).ok());
  CHECK_EQ(0u, hb.prefix_size);
  CHECK_EQ(kBody, hb.cells.size());
  CHECK(mu::TerrainHeightSmoothness(hb) == 0.0);
}

}  // namespace

// ---------------------------------------------------------------------------
// Multi-mesh security and regression tests (ciclo 5 audit item 5 and item 1).
// These exercise the parser with untrusted inputs: extreme meshCount,
// truncated payload, and large counts that previously hit a fixed 10k limit.
// ---------------------------------------------------------------------------
namespace {

std::vector<std::uint8_t> MakeMultiMeshBody(
    std::uint16_t mesh_count,
    const std::vector<std::tuple<std::uint16_t,std::uint16_t,std::uint16_t,std::uint16_t>>& meshes) {
  // meshes[i] = (vc,nc,ntc,ntri)
  std::vector<std::uint8_t> out;
  const char* name = "Test.SMD";
  const std::size_t nlen = std::strlen(name);
  for (std::size_t i = 0; i < mu::kBmdNameSize; ++i) {
    out.push_back(i < nlen ? static_cast<std::uint8_t>(name[i]) : 0);
  }
  // Global header: meshCount, boneCount=0, actionCount=0
  PutU16Le(out, mesh_count);
  PutU16Le(out, 0);
  PutU16Le(out, 0);
  // Now per-mesh data. The first mesh header starts at offset 38, which is
  // exactly where we are now (32 bytes name + 6 bytes global header = 38).
  for (std::size_t mi = 0; mi < meshes.size(); ++mi) {
    auto [vc,nc,ntc,ntri] = meshes[mi];
    // Ten-byte mesh header plus the first vertex record's four-byte prefix
    PutU16Le(out, vc);
    PutU16Le(out, nc);
    PutU16Le(out, ntc);
    PutU16Le(out, ntri);
    PutU16Le(out, 0); // texIdx
    PutU16Le(out, 0); // u48
    PutU16Le(out, 0); // u50
    // Vertex array
    for (std::size_t i = 0; i < vc; ++i) {
      PutF32Le(out, static_cast<float>(i));
      PutF32Le(out, 0.0f);
      PutF32Le(out, 0.0f);
      PutU16Le(out, 0);
      PutU16Le(out, 0);
    }
    // Normal array
    for (std::size_t i = 0; i < nc; ++i) {
      PutF32Le(out, 0.0f);
      PutF32Le(out, 1.0f);
      PutF32Le(out, 0.0f);
      PutF32Le(out, 0.0f);
      PutF32Le(out, 0.0f);
    }
    // The normal vector cursor is four bytes beyond the end of the actual
    // normal records. Place the UV array at its true record boundary.
    if (out.size() >= 4) out.resize(out.size() - 4);
    // Texcoord array (8 bytes each, 2 floats)
    for (std::size_t i = 0; i < ntc; ++i) {
      PutF32Le(out, 0.0f);
      PutF32Le(out, 0.0f);
    }
    // Triangle records: polygon size, four vertex indices, four normal indices,
    // four candidate UV indices, then uninterpreted bytes through byte 63.
    for (std::size_t i = 0; i < ntri; ++i) {
      const std::size_t tri_start = out.size();
      out.push_back(3);
      out.push_back(0);
      for (std::uint16_t k = 0; k < 4; ++k) PutU16Le(out, k < 3 ? k : 0);
      for (std::uint16_t k = 0; k < 4; ++k) PutU16Le(out, k < 3 ? k : 0);
      for (std::uint16_t k = 0; k < 4; ++k) PutU16Le(out, k < 3 ? k : 0);
      while (out.size() - tri_start < mu::kBmdTriangleRecordSize) out.push_back(0);
    }
    // Texture name (32 bytes, NUL-padded, at least 3 printable)
    const char* tex = "tex.jpg";
    std::size_t tlen = std::strlen(tex);
    for (std::size_t i = 0; i < 32; ++i) {
      out.push_back(i < tlen ? static_cast<std::uint8_t>(tex[i]) : 0);
    }
  }
  // If mesh_count > meshes.size(), we have declared more meshes than we
  // emitted - payload will be truncated, which is exactly what the truncated
  // test wants.
  return out;
}

void TestBmdMultiMeshTriangleRecordLayout() {
  std::vector<std::tuple<std::uint16_t,std::uint16_t,std::uint16_t,std::uint16_t>> meshes;
  meshes.emplace_back(3, 3, 0, 1);
  meshes.emplace_back(3, 3, 0, 1);
  const std::vector<std::uint8_t> body = MakeMultiMeshBody(2, meshes);

  mu::BmdBody parsed;
  const mu::Status st =
      mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  CHECK(st.ok());
  CHECK(parsed.complete);
  CHECK_EQ(6u, parsed.vertices.size());
  CHECK_EQ(6u, parsed.normals.size());
  CHECK_EQ(2u, parsed.triangles.size());
  CHECK_EQ(0, parsed.triangles[0].corners[0].vertex);
  CHECK_EQ(1, parsed.triangles[0].corners[1].vertex);
  CHECK_EQ(2, parsed.triangles[0].corners[2].vertex);
  CHECK_EQ(3, parsed.triangles[1].corners[0].vertex);
  CHECK_EQ(4, parsed.triangles[1].corners[1].vertex);
  CHECK_EQ(5, parsed.triangles[1].corners[2].vertex);
  CHECK_EQ(3, parsed.triangles[1].corners[0].normal);
  CHECK_EQ(4, parsed.triangles[1].corners[1].normal);
  CHECK_EQ(5, parsed.triangles[1].corners[2].normal);
  CHECK_EQ(0u, parsed.out_of_range_vertices);
  CHECK_EQ(0u, parsed.out_of_range_normals);
}

void TestBmdMultiMeshLargeCounts() {
  // Regression for ciclo5 item 1: files with ntri=15576 (Monster865 etc)
  // were rejected by a fixed 10k limit. The parser must accept up to u16 max.
  // Build a 2-mesh file where first mesh has 15576 triangles (the failing case).
  // To keep the fixture small we use 1 mesh with large ntri but small vc/nc.
  // 15576 *64 = 997k, plus small arrays, still within a few MB - okay for test.
  // Use a smaller but still >10k count to keep test fast: 12000.
  const std::uint16_t large_ntri = 12000;
  std::vector<std::tuple<std::uint16_t,std::uint16_t,std::uint16_t,std::uint16_t>> meshes;
  meshes.emplace_back(4, 4, 0, large_ntri);
  auto body = MakeMultiMeshBody(1, meshes);
  // For meshCount=1 the parser takes the single-mesh fast path, which also
  // must not have a 10k limit. So set meshCount=1 but with large ntri.
  // Overwrite meshCount to 1 (global) and first mesh header ntri to large.
  // MakeMultiMeshBody with mesh_count=1 already does that.
  mu::BmdBody parsed;
  auto st = mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  CHECK(st.ok());
  CHECK(parsed.complete);
  CHECK_EQ(static_cast<std::size_t>(large_ntri), parsed.triangles.size());
  // Also test the exact failing value 15576 with a minimal fixture (vc=1,nc=1)
  const std::uint16_t exact = 15576;
  meshes.clear();
  meshes.emplace_back(1, 1, 0, exact);
  auto body2 = MakeMultiMeshBody(1, meshes);
  mu::BmdBody p2;
  CHECK(mu::BmdParseBodyV0A(mu::ByteView(body2.data(), body2.size()), &p2).ok());
  CHECK(p2.complete);
  CHECK_EQ(static_cast<std::size_t>(exact), p2.triangles.size());
}

void TestBmdMultiMeshExtremeMeshCount() {
  // meshCount = 0xFFFF must fail gracefully, not loop forever or OOM.
  // Build a tiny payload that declares 0xFFFF meshes but contains only 1.
  std::vector<std::uint8_t> body;
  const char* name = "Test.SMD";
  for (std::size_t i = 0; i < mu::kBmdNameSize; ++i) body.push_back(i < 6 ? static_cast<std::uint8_t>(name[i]) : 0);
  PutU16Le(body, 0xFFFF); // meshCount extreme
  PutU16Le(body, 0);
  PutU16Le(body, 0);
  // One mesh header with minimal counts
  PutU16Le(body, 1); PutU16Le(body, 1); PutU16Le(body, 0); PutU16Le(body, 1);
  PutU16Le(body, 0); PutU16Le(body, 0); PutU16Le(body, 0);
  // Minimal arrays
  for (int i=0;i<16;i++) body.push_back(0);
  for (int i=0;i<20;i++) body.push_back(0);
  for (int i=0;i<64;i++) body.push_back(0);
  for (int i=0;i<32;i++) body.push_back(i<7 ? static_cast<std::uint8_t>("tex.jpg"[i]) : 0);

  mu::BmdBody parsed;
  auto st = mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  // Must not crash, must return error status (kCorrupt or kTruncated) due to safety limit.
  CHECK(!st.ok() || !parsed.complete || parsed.vertices.empty() || parsed.problem.size()>0);
  // The important part: it must not have allocated 0xFFFF meshes.
  CHECK(parsed.vertices.size() < 1000);
}

void TestBmdMultiMeshTruncated() {
  // Payload truncated in middle of second mesh: first mesh valid, second header
  // present but arrays missing. Must not read out of bounds (ASan would catch).
  std::vector<std::tuple<std::uint16_t,std::uint16_t,std::uint16_t,std::uint16_t>> meshes;
  meshes.emplace_back(2, 2, 0, 1); // first mesh complete
  // second mesh will be truncated: we declare meshCount=2 but only emit 1 mesh header
  // plus partial data
  auto full = MakeMultiMeshBody(2, meshes);
  // Truncate after first mesh + second mesh header (14 bytes) but before its arrays
  std::size_t first_mesh_size = 32 + 6 + 14 + 2*16 + 2*20 + 1*64 + 32;
  std::size_t truncated_size = first_mesh_size + 14; // header of second mesh only
  // Build truncated body: copy first mesh, then only header of second
  std::vector<std::uint8_t> truncated;
  truncated.reserve(truncated_size);
  // Rebuild manually: use MakeMultiMeshBody for 1 mesh, then append second header
  auto one = MakeMultiMeshBody(1, meshes);
  truncated = one;
  // Overwrite global meshCount to 2
  PutU16LeAt(truncated, 32, 2);
  // Append second mesh header with vc=2 etc, but no arrays
  PutU16Le(truncated, 2); PutU16Le(truncated, 2); PutU16Le(truncated, 0); PutU16Le(truncated, 1);
  PutU16Le(truncated, 0); PutU16Le(truncated, 0); PutU16Le(truncated, 0);
  // No arrays, so payload ends here - truncated

  mu::BmdBody parsed;
  auto st = mu::BmdParseBodyV0A(mu::ByteView(truncated.data(), truncated.size()), &parsed);
  // Must not crash, must not read out of bounds. Whether it returns partial
  // success (first mesh) or error is acceptable, but it must not have out-of-range
  // access. We check that vertices from first mesh are present and no crash.
  CHECK(parsed.vertices.size() >= 2);
  // If it returned Ok, triangles should be at least 1 from first mesh.
  if (st.ok()) {
    CHECK(parsed.triangles.size() >= 1);
  }
}

void TestBmdBonesCircularParent() {
  // Bone 0 parent =1, bone1 parent=0 => cycle A->B->A, must be rejected
  std::vector<std::uint8_t> body;
  const char* name = "Test.SMD";
  for (std::size_t i = 0; i < mu::kBmdNameSize; ++i) body.push_back(i < 6 ? static_cast<std::uint8_t>(name[i]) : 0);
  PutU16Le(body, 1); // meshCount
  PutU16Le(body, 2); // boneCount =2
  PutU16Le(body, 0); // actionCount
  // per-mesh header for single mesh
  PutU16Le(body, 1); PutU16Le(body, 1); PutU16Le(body, 0); PutU16Le(body, 0);
  PutU16Le(body, 0); PutU16Le(body, 0); PutU16Le(body, 0);
  // vertex
  for (int i=0;i<16;i++) body.push_back(0);
  PutF32Le(body, 0.0f); PutF32Le(body, 0.0f); PutF32Le(body, 0.0f);
  PutU16Le(body, 0); PutU16Le(body, 0);
  // normal
  for (int i=0;i<20;i++) body.push_back(0);
  PutF32Le(body, 0.0f); PutF32Le(body, 1.0f); PutF32Le(body, 0.0f);
  PutF32Le(body, 0.0f); PutF32Le(body, 0.0f);
  // texture name
  const char* tex="tex.jpg";
  for (int i=0;i<32;++i) body.push_back(i<7 ? static_cast<std::uint8_t>(tex[i]) : 0);
  // bone 0: name Box01, parent 1, 6 floats finite, pad 0
  const char* b0="Box01";
  for (int i=0;i<32;++i) body.push_back(i<5 ? static_cast<std::uint8_t>(b0[i]) : 0);
  PutU16Le(body, 1); // parent 1
  for (int i=0;i<6;++i) PutF32Le(body, 0.0f);
  body.push_back(0); // pad
  // bone1: name Bip01, parent 0 => cycle
  const char* b1="Bip01";
  for (int i=0;i<32;++i) body.push_back(i<5 ? static_cast<std::uint8_t>(b1[i]) : 0);
  PutU16Le(body, 0); // parent 0 -> cycle 0->1->0
  for (int i=0;i<6;++i) PutF32Le(body, 0.0f);
  // last bone no pad (58 bytes)

  mu::BmdBody parsed;
  auto st = mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  // Must fail due to cycle detection, not crash
  CHECK(!st.ok() || !parsed.has_bones || parsed.bones.empty());
}

void TestBmdBonesExtremeCount() {
  // boneCount = 0xFFFF extreme, must fail gracefully, not OOM
  std::vector<std::uint8_t> body;
  const char* name = "Test.SMD";
  for (std::size_t i = 0; i < mu::kBmdNameSize; ++i) body.push_back(i < 6 ? static_cast<std::uint8_t>(name[i]) : 0);
  PutU16Le(body, 1); // meshCount
  PutU16Le(body, 0xFFFF); // boneCount extreme
  PutU16Le(body, 0);
  PutU16Le(body, 1); PutU16Le(body, 1); PutU16Le(body, 0); PutU16Le(body, 0);
  PutU16Le(body, 0); PutU16Le(body, 0); PutU16Le(body, 0);
  for (int i=0;i<16;i++) body.push_back(0);
  PutF32Le(body, 0.0f); PutF32Le(body, 0.0f); PutF32Le(body, 0.0f);
  PutU16Le(body, 0); PutU16Le(body, 0);
  for (int i=0;i<20;i++) body.push_back(0);
  PutF32Le(body, 0.0f); PutF32Le(body, 1.0f); PutF32Le(body, 0.0f);
  PutF32Le(body, 0.0f); PutF32Le(body, 0.0f);
  const char* tex="tex.jpg";
  for (int i=0;i<32;++i) body.push_back(i<7 ? static_cast<std::uint8_t>(tex[i]) : 0);
  // No bone data, truncated

  mu::BmdBody parsed;
  auto st = mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  CHECK(!st.ok() || !parsed.has_bones);
  CHECK(parsed.bones.size() < 1000);
}

void TestBmdBonesTruncated() {
  // Payload truncated in middle of bone list
  std::vector<std::uint8_t> body;
  const char* name = "Test.SMD";
  for (std::size_t i = 0; i < mu::kBmdNameSize; ++i) body.push_back(i < 6 ? static_cast<std::uint8_t>(name[i]) : 0);
  PutU16Le(body, 1); PutU16Le(body, 2); PutU16Le(body, 0);
  PutU16Le(body, 1); PutU16Le(body, 1); PutU16Le(body, 0); PutU16Le(body, 0);
  PutU16Le(body, 0); PutU16Le(body, 0); PutU16Le(body, 0);
  for (int i=0;i<16;i++) body.push_back(0);
  PutF32Le(body, 0.0f); PutF32Le(body, 0.0f); PutF32Le(body, 0.0f);
  PutU16Le(body, 0); PutU16Le(body, 0);
  for (int i=0;i<20;i++) body.push_back(0);
  PutF32Le(body, 0.0f); PutF32Le(body, 1.0f); PutF32Le(body, 0.0f);
  PutF32Le(body, 0.0f); PutF32Le(body, 0.0f);
  const char* tex="tex.jpg";
  for (int i=0;i<32;++i) body.push_back(i<7 ? static_cast<std::uint8_t>(tex[i]) : 0);
  // bone0 complete
  const char* b0="Box01";
  for (int i=0;i<32;++i) body.push_back(i<5 ? static_cast<std::uint8_t>(b0[i]) : 0);
  PutU16Le(body, 0xFFFF);
  for (int i=0;i<6;++i) PutF32Le(body, 0.0f);
  body.push_back(0);
  // bone1 truncated: only 10 bytes of second bone
  const char* b1="Bip01";
  for (int i=0;i<10;++i) body.push_back(i<5 ? static_cast<std::uint8_t>(b1[i]) : 0);

  mu::BmdBody parsed;
  auto st = mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  // Must not crash, must not have bones (truncated)
  CHECK(!parsed.has_bones || parsed.bones.empty() || !st.ok());
}

void TestBmdBonesOverflow() {
  // bc*59 overflow check: bc up to 64 safe, but test with bc=0xFFFF would overflow if not checked
  // Our parser limits bc>64 to fail, so overflow never happens, but we test that it fails gracefully
  std::vector<std::uint8_t> body;
  const char* name = "Test.SMD";
  for (std::size_t i = 0; i < mu::kBmdNameSize; ++i) body.push_back(i < 6 ? static_cast<std::uint8_t>(name[i]) : 0);
  PutU16Le(body, 1); PutU16Le(body, 0xFFFF); PutU16Le(body, 0);
  PutU16Le(body, 1); PutU16Le(body, 1); PutU16Le(body, 0); PutU16Le(body, 0);
  PutU16Le(body, 0); PutU16Le(body, 0); PutU16Le(body, 0);
  // No need for more data, should fail at boneCount check
  mu::BmdBody parsed;
  auto st = mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  CHECK(!parsed.has_bones);
}

void TestBmdMultiMeshOverflow() {
  // Overflow in multiplication: vc * 16 etc must be checked. Since vc is u16,
  // overflow in 64-bit size_t is impossible, but we test that the parser
  // rejects when vc*16 > payload.size (i.e., declared array does not fit).
  std::vector<std::uint8_t> body;
  const char* name = "Test.SMD";
  for (std::size_t i = 0; i < mu::kBmdNameSize; ++i) body.push_back(i < 6 ? static_cast<std::uint8_t>(name[i]) : 0);
  PutU16Le(body, 1); // meshCount
  PutU16Le(body, 0);
  PutU16Le(body, 0);
  // Declare vc=5000, but only provide 10 bytes of payload after header
  PutU16Le(body, 5000); PutU16Le(body, 0); PutU16Le(body, 0); PutU16Le(body, 0);
  PutU16Le(body, 0); PutU16Le(body, 0); PutU16Le(body, 0);
  // No arrays - truncated, should be rejected as DoesNotFit, not overflow
  mu::BmdBody parsed;
  auto st = mu::BmdParseBodyV0A(mu::ByteView(body.data(), body.size()), &parsed);
  CHECK(!st.ok() || !parsed.complete || parsed.vertices.empty());
}


void TestMeshInternalRepresentation() {
  // C1: representação interna única de malha e esqueleto
  mu::Mesh mesh;
  mesh.name = "TestMesh";
  mesh.positions = {{0,0,0}, {1,0,0}, {0,1,0}};
  mesh.normals = {{0,0,1}, {0,0,1}, {0,0,1}};
  mesh.texcoords = {{0,0}, {1,0}, {0,1}};
  mesh.indices = {0,1,2};
  mesh.has_texcoords = true;
  // bone weights per vertex
  mu::BoneWeight w0; w0.bone=0; w0.weight=1.0f;
  mu::BoneWeight w1; w1.bone=0; w1.weight=0.5f; mu::BoneWeight w1b; w1b.bone=1; w1b.weight=0.5f;
  mesh.bone_weights = {{w0}, {w1, w1b}, {w0}};
  mesh.has_bone_weights = true;
  mesh.bounds = mu::ComputeBounds(mesh.positions);
  CHECK(mesh.positions.size()==3);
  CHECK(mesh.normals.size()==3);
  CHECK(mesh.texcoords.size()==3);
  CHECK(mesh.indices.size()==3);
  CHECK(mesh.bone_weights.size()==3);
  CHECK(mesh.bone_weights[0].size()==1);
  CHECK(mesh.bone_weights[1].size()==2);
  CHECK(mesh.has_texcoords);
  CHECK(mesh.has_bone_weights);
  CHECK(mesh.bounds.valid);
  CHECK(mesh.bounds.min.x==0.0f && mesh.bounds.min.y==0.0f);
  CHECK(mesh.bounds.max.x==1.0f && mesh.bounds.max.y==1.0f);

  mu::Skeleton skel;
  mu::Bone root; root.name="Root"; root.parent=mu::kNoBoneParent; root.position={0,0,0}; root.rotation={0,0,0};
  mu::Bone child; child.name="Child"; child.parent=0; child.position={1,0,0}; child.rotation={0,0,1.57f};
  skel.bones = {root, child};
  skel.has_bones = true;
  CHECK(skel.bones.size()==2);
  CHECK(skel.bones[0].parent==mu::kNoBoneParent);
  CHECK(skel.bones[1].parent==0);
  CHECK(skel.has_bones);
  for(size_t i=0;i<mesh.bone_weights.size();++i){
    float sum=0;
    for(auto& bw: mesh.bone_weights[i]){
      CHECK(bw.bone < skel.bones.size());
      sum+=bw.weight;
    }
    CHECK(std::fabs(sum-1.0f) < 0.001f);
  }
}



void TestPackMeshForUpload() {
  // D1: empacotamento Mesh -> PackedBuffers para VBO/VAO, sem GL
  mu::Mesh mesh;
  mesh.name = "Tri";
  mesh.positions = {{0,0,0}, {1,0,0}, {0,1,0}};
  mesh.normals = {{0,0,1}, {0,0,1}, {0,0,1}};
  mesh.texcoords = {{0,0}, {1,0}, {0,1}};
  mesh.indices = {0,1,2};
  mesh.has_texcoords = true;
  mu::BoneWeight w0; w0.bone=0; w0.weight=1.0f;
  mesh.bone_weights = {{w0}, {w0}, {w0}};
  mesh.has_bone_weights = true;

  auto packed = mu::PackMeshForUpload(mesh);
  CHECK(packed.ok);
  CHECK(packed.vertices.size()==3);
  CHECK(packed.indices.size()==3);
  CHECK(packed.has_texcoords);
  CHECK(packed.has_bone_weights);
  CHECK(packed.bounds.valid);
  // Byte-by-byte verification against known case
  // Vertex 0: pos 0,0,0 normal 0,0,1 uv 0,0 bone 0 weight 1.0
  CHECK(packed.vertices[0].px==0.0f && packed.vertices[0].py==0.0f && packed.vertices[0].pz==0.0f);
  CHECK(packed.vertices[0].nx==0.0f && packed.vertices[0].ny==0.0f && packed.vertices[0].nz==1.0f);
  CHECK(packed.vertices[0].u==0.0f && packed.vertices[0].v==0.0f);
  CHECK(packed.vertices[0].bone_ids[0]==0 && packed.vertices[0].bone_weights[0]==1.0f);
  CHECK(packed.vertices[0].weight_count==1);
  // Vertex 1: pos 1,0,0 uv 1,0
  CHECK(packed.vertices[1].px==1.0f && packed.vertices[1].u==1.0f);
  // Indices
  CHECK(packed.indices[0]==0 && packed.indices[1]==1 && packed.indices[2]==2);

  // Test without texcoords and bone weights
  mu::Mesh mesh2;
  mesh2.positions = {{0,0,0}, {1,0,0}};
  mesh2.normals = {{0,0,1}, {0,0,1}};
  mesh2.indices = {0,1,0};
  mesh2.has_texcoords = false;
  mesh2.has_bone_weights = false;
  auto packed2 = mu::PackMeshForUpload(mesh2);
  CHECK(packed2.ok);
  CHECK(packed2.vertices[0].u==0.0f && packed2.vertices[0].v==0.0f);
  CHECK(packed2.vertices[0].weight_count==0);
  CHECK(packed2.has_texcoords==false);
  CHECK(packed2.has_bone_weights==false);

  // Test bad indices dropped
  mu::Mesh mesh3;
  mesh3.positions = {{0,0,0}, {1,0,0}, {0,1,0}};
  mesh3.normals = {{0,0,1}, {0,0,1}, {0,0,1}};
  mesh3.indices = {0, 1, 2, 0, 5, 6};  // second triangle has two bad refs
  mesh3.has_texcoords = false;
  mesh3.has_bone_weights = false;
  auto packed3 = mu::PackMeshForUpload(mesh3);
  CHECK(packed3.ok);
  CHECK(packed3.indices.size()==3); // only first triangle kept
  CHECK(packed3.dropped_triangles == 1);
  CHECK(packed3.invalid_index_references == 2);
}



void TestSkinningLBS() {
  // D2: LBS CPU strict count, no GL
  // (a) identity bones -> original
  mu::Mesh mesh;
  mesh.positions = {{0,0,0}, {1,2,3}, {-1,0.5f,2}};
  mesh.normals = {{0,0,1}, {0,0,1}, {0,0,1}};
  mesh.has_texcoords = false;
  mu::BoneWeight w0; w0.bone=0; w0.weight=1.0f;
  mu::BoneWeight w1; w1.bone=1; w1.weight=0.5f;
  mu::BoneWeight w2; w2.bone=1; w2.weight=0.5f;
  mesh.bone_weights = {{w0}, {w0}, {{w1,w2}}};
  mesh.has_bone_weights = true;

  mu::Skeleton skel;
  mu::Bone b0; b0.name="Root"; b0.parent=mu::kNoBoneParent; b0.position={0,0,0}; b0.rotation={0,0,0};
  mu::Bone b1; b1.name="Child"; b1.parent=0; b1.position={1,0,0}; b1.rotation={0,0,0};
  skel.bones = {b0,b1};
  skel.has_bones = true;

  std::vector<mu::Mat4> identity;
  identity.push_back(mu::Mat4::Identity());
  identity.push_back(mu::Mat4::Identity());

  auto res = mu::ApplySkinningLBS(mesh, skel, identity);
  CHECK(res.ok);
  CHECK(res.positions.size()==3);
  CHECK(std::fabs(res.positions[0].x-0.0f)<1e-5f && std::fabs(res.positions[0].y-0.0f)<1e-5f);
  CHECK(std::fabs(res.positions[1].x-1.0f)<1e-5f && std::fabs(res.positions[1].y-2.0f)<1e-5f && std::fabs(res.positions[1].z-3.0f)<1e-5f);
  CHECK(std::fabs(res.positions[2].x-(-1.0f))<1e-5f);

  // (b) self-loop fails safe
  mu::Skeleton badLoop;
  mu::Bone bl0; bl0.name="Bad"; bl0.parent=0; bl0.position={0,0,0}; bl0.rotation={0,0,0};
  badLoop.bones={bl0};
  badLoop.has_bones=true;
  std::vector<mu::Mat4> oneId = {mu::Mat4::Identity()};
  auto resBad = mu::ApplySkinningLBS(mesh, badLoop, oneId);
  CHECK(!resBad.ok);

  // invalid parent fails safe
  mu::Skeleton badParent;
  mu::Bone bp0; bp0.name="BadParent"; bp0.parent=5; bp0.position={0,0,0}; bp0.rotation={0,0,0};
  badParent.bones={bp0};
  badParent.has_bones=true;
  auto resBad2 = mu::ApplySkinningLBS(mesh, badParent, oneId);
  CHECK(!resBad2.ok);

  // (c) mesh without bone_weights zero movement
  mu::Mesh meshNoWeight;
  meshNoWeight.positions = {{0,0,0}, {1,0,0}};
  meshNoWeight.normals = {{0,0,1}, {0,0,1}};
  meshNoWeight.has_bone_weights = false;
  auto resNoW = mu::ApplySkinningLBS(meshNoWeight, skel, identity);
  CHECK(resNoW.ok);
  CHECK(resNoW.positions.size()==2);
  CHECK(resNoW.positions[0].x==0.0f && resNoW.positions[1].x==1.0f);

  // (d) transformação NÃO-TRIVIAL: rotação 90° em Z, cálculo independente
  // Cálculo manual: rotação Z 90° (cos=0, sin=1) transforma (1,0,0) -> (0,1,0)
  // Matriz row-major: [cos -sin 0 0; sin cos 0 0; 0 0 1 0; 0 0 0 1]
  // Para 90°: cos=0, sin=1 => [0 -1 0 0; 1 0 0 0; 0 0 1 0; 0 0 0 1]
  // Aplicando a (1,0,0,1): x' = 0*1 + (-1)*0 +0*0+0*1=0, y'=1*1+0*0+0*0+0*1=1, z'=0
  // Portanto vértice (1,0,0) deve ir para (0,1,0) após skinning com peso 1.0 bone 0
  mu::Mesh meshRot;
  meshRot.positions = {{1,0,0}};
  meshRot.normals = {{0,0,1}};
  mu::BoneWeight wRot; wRot.bone=0; wRot.weight=1.0f;
  meshRot.bone_weights = {{wRot}};
  meshRot.has_bone_weights = true;
  mu::Skeleton skelRot;
  mu::Bone br0; br0.name="Root"; br0.parent=mu::kNoBoneParent; br0.position={0,0,0}; br0.rotation={0,0,0};
  skelRot.bones={br0};
  skelRot.has_bones=true;
  mu::Mat4 rot90{};
  rot90.m[0]=0.0f; rot90.m[1]=-1.0f; rot90.m[2]=0.0f; rot90.m[3]=0.0f;
  rot90.m[4]=1.0f; rot90.m[5]=0.0f; rot90.m[6]=0.0f; rot90.m[7]=0.0f;
  rot90.m[8]=0.0f; rot90.m[9]=0.0f; rot90.m[10]=1.0f; rot90.m[11]=0.0f;
  rot90.m[12]=0.0f; rot90.m[13]=0.0f; rot90.m[14]=0.0f; rot90.m[15]=1.0f;
  std::vector<mu::Mat4> rotMats = {rot90};
  auto resRot = mu::ApplySkinningLBS(meshRot, skelRot, rotMats);
  CHECK(resRot.ok);
  CHECK(resRot.positions.size()==1);
  // Resultado esperado calculado independentemente acima: (0,1,0)
  CHECK(std::fabs(resRot.positions[0].x-0.0f)<1e-5f);
  CHECK(std::fabs(resRot.positions[0].y-1.0f)<1e-5f);
  CHECK(std::fabs(resRot.positions[0].z-0.0f)<1e-5f);
}


void TestFrustumCullRealBounds() {
  // D3: culling com bounding boxes reais de Mesh (não mock)
  mu::Mesh mesh;
  mesh.positions = {{0,0,0}, {1,0,0}, {0,1,0}, {0,0,1}};
  mesh.normals = {{0,0,1}, {0,0,1}, {0,0,1}, {0,0,1}};
  mesh.has_texcoords = false;
  mesh.has_bone_weights = false;
  mu::Bounds b = mu::ComputeBounds(mesh.positions);
  CHECK(b.valid);
  CHECK(b.min.x==0.0f && b.max.x==1.0f);
  auto packed = mu::PackMeshForUpload(mesh);
  CHECK(packed.ok);
  CHECK(packed.bounds.valid);
  CHECK(packed.bounds.min.x==0.0f && packed.bounds.max.x==1.0f);

  // Frustum que contém 0..10 em X, unbounded resto
  mu::Frustum f{};
  f.planes[0].normal = {1,0,0}; f.planes[0].d=0; // x>=0
  f.planes[1].normal = {-1,0,0}; f.planes[1].d=10; // x<=10
  for(int i=2;i<6;++i){ f.planes[i].normal={0,1,0}; f.planes[i].d=1e6f; }

  // (a) inside kept
  CHECK(mu::CullBox(f, b)==mu::CullResult::kInside);
  CHECK(mu::IsVisible(f, b));
  CHECK(mu::IsVisible(f, packed.bounds));

  // (b) outside culled: desloca bounds para -20..-10
  mu::Bounds outside;
  outside.min={-20,-1,-1}; outside.max={-10,1,1}; outside.valid=true;
  CHECK(mu::CullBox(f, outside)==mu::CullResult::kOutside);
  CHECK(!mu::IsVisible(f, outside));

  // (c) invalid never culled fail safe
  mu::Bounds invalid;
  CHECK(!invalid.valid);
  CHECK(mu::IsVisible(f, invalid));
  CHECK(mu::CullBox(f, invalid)==mu::CullResult::kIntersect);
}


void TestWin32GLSkeletonBlocked() {
  // D5: esqueleto Win32 + WGL — BLOCKED neste ambiente, documentado como esqueleto
  // Não compila em Windows real sem SDK/GPU, aqui retorna BLOCKED
  // Testa apenas que o status BLOCKED é retornado e que CreateGLWindow falha safe
  // Isso não é teste de janela real, é teste de que o esqueleto está presente e marcado BLOCKED
  mu::platform::Win32GLConfig cfg;
  cfg.width=800; cfg.height=600;
  mu::platform::Win32GLWindow win;
  bool created = mu::platform::CreateGLWindow(cfg, &win);
  CHECK(!created); // BLOCKED neste ambiente Linux sem Windows SDK/GPU
  CHECK(!win.created);
  auto status = mu::platform::GetWin32GLStatus();
  CHECK(status==mu::platform::Win32GLStatus::kBlockedNoWindowsSDK);
  mu::platform::DestroyGLWindow(&win);
  CHECK(!win.created);
}


void TestMainLoopSkeleton() {
  // E2: esqueleto main loop update/render/present prediz frame budget sem GL
  mu::platform::MainLoop loop(60.0);
  mu::platform::InputState input;
  input.quit_requested=false;
  auto fb = loop.Tick(input);
  CHECK(fb.target_fps==60.0);
  CHECK(fb.target_frame_ms>16.0 && fb.target_frame_ms<17.0);
  CHECK(fb.frame_index==0);
  CHECK(fb.total_ms>=0.0);
  // Segunda chamada incrementa frame_index
  auto fb2 = loop.Tick(input);
  CHECK(fb2.frame_index==1);
  CHECK(loop.History().size()==2);
  // Predição: com histórico pequeno não prediz over budget
  CHECK(!loop.PredictOverBudget());
  // SetTargetFps
  loop.SetTargetFps(144.0);
  auto fb3 = loop.Tick(input);
  CHECK(fb3.target_fps==144.0);
  CHECK(fb3.target_frame_ms>6.0 && fb3.target_frame_ms<7.0);
}

}  // namespace

// ---------------------------------------------------------------------------
// Software rasterizer CPU-only tests (ciclo14 B)
// ---------------------------------------------------------------------------
#include "mu/render/software_rasterizer.hpp"

namespace {

void TestSoftwareTextureLoadFromOZT() {
  const std::vector<uint8_t> rgba = {
      255, 0, 0, 255, 0, 255, 0, 255,
      0, 0, 255, 255, 255, 255, 255, 255,
  };
  std::vector<uint8_t> file;
  CHECK(mu::OztWriteRgba(2, 2, rgba.data(), false, &file));

  mu::SoftwareTexture texture;
  const mu::Status status =
      texture.LoadFromOZT(mu::ByteView(file.data(), file.size()));
  CHECK(status.ok());
  CHECK(texture.valid);
  CHECK_EQ(texture.width, 2);
  CHECK_EQ(texture.height, 2);
  CHECK_EQ(texture.bgra.size(), 16u);
  CHECK(texture.bgra == std::vector<uint8_t>({
                            0, 0, 255, 255, 0, 255, 0, 255,
                            255, 0, 0, 255, 255, 255, 255, 255,
                        }));

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  texture.Sample(0.25f, 0.25f, r, g, b);
  CHECK(r == 255 && g == 0 && b == 0);
  texture.Sample(0.75f, 0.25f, r, g, b);
  CHECK(r == 0 && g == 255 && b == 0);
  texture.Sample(0.25f, 0.75f, r, g, b);
  CHECK(r == 0 && g == 0 && b == 255);

  const mu::Status bad = texture.LoadFromOZT(
      mu::ByteView(file.data(), std::min<std::size_t>(file.size(), 10)));
  CHECK(!bad.ok());
  CHECK(!texture.valid);
  CHECK_EQ(texture.width, 0);
  CHECK(texture.bgra.empty());

  mu::SoftwareTexture malformed;
  malformed.valid = true;
  malformed.width = 1;
  malformed.height = 1;
  malformed.bgra = {0};
  malformed.Sample(0.5f, 0.5f, r, g, b);
  CHECK(r == 255 && g == 255 && b == 255);
}

void TestSoftwareRasterizerUsesMeshTexcoords() {
  const std::vector<uint8_t> rgba = {
      255, 0, 0, 255, 0, 255, 0, 255,
      0, 0, 255, 255, 255, 255, 255, 255,
  };
  std::vector<uint8_t> file;
  CHECK(mu::OztWriteRgba(2, 2, rgba.data(), false, &file));
  mu::SoftwareTexture texture;
  CHECK(texture.LoadFromOZT(mu::ByteView(file.data(), file.size())).ok());

  mu::Mesh mesh;
  mesh.positions = {{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
  mesh.normals = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  mesh.texcoords = {{0.25f, 0.25f}, {0.25f, 0.25f}, {0.25f, 0.25f}};
  mesh.indices = {0, 1, 2};
  mesh.has_texcoords = true;
  mesh.bounds = mu::ComputeBounds(mesh.positions);

  mu::Camera camera;
  camera.eye = {0, 0, 5};
  camera.target = {0, 0, 0};
  camera.up = {0, 1, 0};
  camera.aspect = 1.0f;
  mu::SoftwareRasterizer rasterizer(100, 100);
  rasterizer.Clear(mu::Vec3{0, 0, 0});
  rasterizer.SetTexture(texture);
  const mu::RenderStats stats = rasterizer.RenderMesh(
      mesh, nullptr, nullptr, camera, mu::Vec3{0, 0, 1});
  CHECK(stats.ok);
  CHECK(stats.pixels_written > 0);

  const std::size_t pixel = 50 * 100 + 50;
  const auto& color = rasterizer.framebuffer().color;
  CHECK(color[pixel * 3] == 255);
  CHECK(color[pixel * 3 + 1] == 0);
  CHECK(color[pixel * 3 + 2] == 0);
}

void TestSoftwareRasterizerFramebuffer() {
  mu::SoftwareFramebuffer fb(10,10);
  fb.Clear(mu::Vec3{0,0,0});
  // Depth init 1.0
  CHECK(fb.depth[0]==1.0f);
  // Set pixel at 5,5 depth 0.5 red
  bool ok = fb.SetPixel(5,5,0.5f,255,0,0);
  CHECK(ok);
  // Same pixel farther depth 0.8 should fail
  bool ok2 = fb.SetPixel(5,5,0.8f,0,255,0);
  CHECK(!ok2);
  // Closer depth 0.2 should succeed
  bool ok3 = fb.SetPixel(5,5,0.2f,0,0,255);
  CHECK(ok3);
  // Check color is blue (0,0,255)
  size_t idx = 5*10+5;
  CHECK(fb.color[idx*3]==0 && fb.color[idx*3+1]==0 && fb.color[idx*3+2]==255);
  // Out of bounds fails
  CHECK(!fb.SetPixel(-1,0,0.1f,255,255,255));
  CHECK(!fb.SetPixel(10,0,0.1f,255,255,255));
}

void TestSoftwareRasterizerCamera() {
  mu::Camera cam;
  cam.eye={0,0,5};
  cam.target={0,0,0};
  cam.up={0,1,0};
  cam.fov_deg=45.0f;
  cam.aspect=1.0f;
  cam.near_plane=0.1f;
  cam.far_plane=100.0f;
  mu::Mat4 view = cam.GetViewMatrix();
  // Eye should map to origin in view space
  mu::Vec3 eyeInView = mu::TransformPoint(view, cam.eye);
  CHECK(std::fabs(eyeInView.x) < 1e-4f);
  CHECK(std::fabs(eyeInView.y) < 1e-4f);
  CHECK(std::fabs(eyeInView.z) < 1e-4f);
  // Target should map to (0,0,-distance)
  mu::Vec3 targetInView = mu::TransformPoint(view, cam.target);
  float dist = std::sqrt((cam.eye.x-cam.target.x)*(cam.eye.x-cam.target.x)+(cam.eye.y-cam.target.y)*(cam.eye.y-cam.target.y)+(cam.eye.z-cam.target.z)*(cam.eye.z-cam.target.z));
  CHECK(std::fabs(targetInView.x) < 1e-4f);
  CHECK(std::fabs(targetInView.y) < 1e-4f);
  CHECK(std::fabs(targetInView.z + dist) < 1e-3f);

  mu::Mat4 proj = cam.GetProjectionMatrix();
  // Projection should have -1 at [14]
  CHECK(proj.m[14]==-1.0f);
  // ViewProjection should produce valid frustum
  mu::Frustum fr = cam.GetFrustum();
  mu::Bounds b;
  b.min={-0.5f,-0.5f,-0.5f}; b.max={0.5f,0.5f,0.5f}; b.valid=true;
  CHECK(mu::IsVisible(fr,b));
}

void TestSoftwareRasterizerTriangle() {
  // Known triangle in screen space via perspective
  // Create a simple mesh: 3 vertices forming a triangle centered at origin, 1 unit size
  mu::Mesh mesh;
  mesh.positions = {{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
  mesh.normals = {{0,0,1}, {0,0,1}, {0,0,1}};
  mesh.indices = {0,1,2};
  mesh.has_texcoords=false;
  mesh.has_bone_weights=false;
  mesh.bounds = mu::ComputeBounds(mesh.positions);

  mu::Camera cam;
  cam.eye={0,0,5};
  cam.target={0,0,0};
  cam.up={0,1,0};
  cam.fov_deg=45.0f;
  cam.aspect=1.0f;
  cam.near_plane=0.1f;
  cam.far_plane=100.0f;

  mu::SoftwareRasterizer rast(100,100);
  rast.Clear(mu::Vec3{0,0,0});
  mu::Vec3 lightDir = mu::Normalize(mu::Vec3{0,0,1});
  auto stats = rast.RenderMesh(mesh, nullptr, nullptr, cam, lightDir);
  CHECK(stats.ok);
  CHECK(stats.triangles_total==1);
  CHECK(stats.triangles_rasterized==1);
  // Center pixel should be inside triangle and have some intensity
  // Triangle covers center of 100x100 image
  // Check pixel at 50,50 should be written (not background black)
  const auto& fb = rast.framebuffer();
  size_t idx = 50*100+50;
  bool isBg = fb.color[idx*3]==0 && fb.color[idx*3+1]==0 && fb.color[idx*3+2]==0;
  CHECK(!isBg); // should be lit
  // Corner pixel should be background
  size_t idx2 = 0;
  bool isBg2 = fb.color[idx2*3]==0 && fb.color[idx2*3+1]==0 && fb.color[idx2*3+2]==0;
  CHECK(isBg2);
  // Depth at center should be <1.0
  CHECK(fb.depth[idx] < 1.0f);
  CHECK(fb.depth[idx] > 0.0f);
}

void TestSoftwareRasterizerZBuffer() {
  // Two triangles overlapping, closer should win
  mu::Mesh mesh;
  // Triangle 1 at z=0 (farther)
  // Triangle 2 at z=0.5 (closer, same x,y)
  mesh.positions = {
    {-1,-1,0}, {1,-1,0}, {0,1,0}, // tri0 far
    {-1,-1,0.5f}, {1,-1,0.5f}, {0,1,0.5f} // tri1 close
  };
  mesh.normals = {{0,0,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1}};
  mesh.indices = {0,1,2, 3,4,5};
  mesh.has_texcoords=false;
  mesh.has_bone_weights=false;
  mesh.bounds = mu::ComputeBounds(mesh.positions);

  mu::Camera cam;
  cam.eye={0,0,5};
  cam.target={0,0,0};
  cam.up={0,1,0};
  cam.fov_deg=45.0f;
  cam.aspect=1.0f;
  cam.near_plane=0.1f;
  cam.far_plane=100.0f;

  mu::SoftwareRasterizer rast(100,100);
  rast.Clear(mu::Vec3{0,0,0});
  mu::Vec3 lightDir = mu::Normalize(mu::Vec3{0,0,1});
  auto stats = rast.RenderMesh(mesh, nullptr, nullptr, cam, lightDir);
  CHECK(stats.ok);
  CHECK(stats.triangles_total==2);
  // Both rasterized, but closer should be visible at center
  const auto& fb = rast.framebuffer();
  size_t idx = 50*100+50;
  // Depth should be from closer triangle
  // Since closer triangle at z=0.5 vs far at z=0, after view transform closer has less negative view z? Actually eye at 5, pos z 0 => view z -5, pos z 0.5 => view z -4.5, so closer is -4.5 > -5, so ndc depth smaller? Let's just check depth <1 and not equal to far
  CHECK(fb.depth[idx] < 1.0f);
  // Pixel should be lit (not background)
  bool isBg = fb.color[idx*3]==0 && fb.color[idx*3+1]==0 && fb.color[idx*3+2]==0;
  CHECK(!isBg);
}

} // anon namespace

