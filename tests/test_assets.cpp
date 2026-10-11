// SPDX-License-Identifier: MIT
// Test suite for the ClienteRecriado asset library.
//
// Uses a small built-in harness rather than a downloaded framework, so the
// suite builds with nothing but a compiler. Every fixture is generated here at
// run time from scratch: this repository contains no original game assets.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <unistd.h>  // mkdtemp, rmdir
#endif

#include "mu/assets/bmd.hpp"
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
  const char* base = std::getenv("TMPDIR");
  if (!base || !*base) base = "/tmp";
  std::string d = std::string(base) + "/mu_test_XXXXXX";
  std::vector<char> buf(d.begin(), d.end());
  buf.push_back('\0');
#if defined(_WIN32)
  return std::string(base) + "/mu_test";
#else
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
  ::rmdir(dir.c_str());
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

void TestBmdReportLevelsAreSeparate();
void TestBmdReportGeometryIsAlwaysZero();
void TestBmdReportPercent();
void TestBmdReportAccumulatesKinds();

const Test kTests[] = {
    {"crc32 known vectors", TestCrc32KnownVectors},
    {"ozt round trip", TestOztRoundTrip},
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
    {"bmd report levels are separate", TestBmdReportLevelsAreSeparate},
    {"bmd geometry parsed is always zero", TestBmdReportGeometryIsAlwaysZero},
    {"bmd percent helper", TestBmdReportPercent},
    {"bmd report accumulates kinds", TestBmdReportAccumulatesKinds},
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

void TestBmdReportGeometryIsAlwaysZero() {
  // The honest zero. No BMD body parser exists, so level 3 is zero for every
  // input including a perfectly clear payload. If a parser is ever written
  // this test must be updated deliberately rather than silently start passing.
  const std::vector<std::uint8_t> payload = LowEntropyBytes(64);
  CHECK_EQ(0u, SummarizeOne(FakeBmd(0x0A, 64, payload), "a.bmd").geometry_parsed);
  CHECK_EQ(0u, SummarizeOne(FakeBmd(0x0C, 64, payload), "c.bmd").geometry_parsed);
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
