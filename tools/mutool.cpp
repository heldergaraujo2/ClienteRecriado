// SPDX-License-Identifier: MIT
// mutool - command line front end for the ClienteRecriado asset library.
//
// Subcommands:
//   scan   <dir>                     inventory a Data tree, per-format stats
//                                     and an uncompressed VRAM estimate
//   info   <file>                    classify one file in detail
//   verify <manifest> <root>         CRC32 + size check against a patch manifest
//   dedupe <dir>                     report byte-identical duplicate files
//
// Exit status is 0 on success, 1 on a usage error, 2 when a check found
// problems. That distinction is what lets CI treat "verify found a corrupt
// file" differently from "you invoked me wrong".
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_report.hpp"
#include "mu/assets/format.hpp"
#include "mu/assets/manifest.hpp"
#include "mu/assets/ozj.hpp"
#include "mu/assets/ozt.hpp"
#include "mu/assets/terrain.hpp"
#include "mu/assets/tga.hpp"
#include "mu/fs.hpp"

namespace {

void Usage() {
  std::printf(
      "mutool - MU Online asset inspection\n"
      "\n"
      "usage:\n"
      "  mutool scan   <data-dir>\n"
      "  mutool info   <file>\n"
      "  mutool verify <manifest> <root>\n"
      "  mutool dedupe <dir>\n");
}

struct ExtStats {
  std::size_t count = 0;
  uint64_t bytes = 0;
  std::size_t parsed_ok = 0;
  std::size_t parse_fail = 0;
  uint64_t decoded_pixels = 0;  // uncompressed size in GPU memory
  std::map<std::string, std::size_t> notes;
};

int CmdScan(int argc, char** argv) {
  if (argc < 3) {
    Usage();
    return 1;
  }
  const std::string root = argv[2];

  std::vector<mu::FileEntry> files;
  if (!mu::WalkFiles(root, &files)) {
    std::fprintf(stderr, "error: cannot open directory '%s'\n", root.c_str());
    return 1;
  }
  std::sort(files.begin(), files.end(),
            [](const mu::FileEntry& a, const mu::FileEntry& b) {
              return a.path < b.path;
            });

  std::map<std::string, ExtStats> stats;
  uint64_t total_bytes = 0;
  uint64_t total_vram = 0;
  std::size_t junk_files = 0;
  uint64_t junk_bytes = 0;

  for (const mu::FileEntry& f : files) {
    const std::string ext = mu::ExtensionOf(f.path);
    ExtStats& s = stats[ext.empty() ? std::string("(none)") : ext];
    ++s.count;
    s.bytes += f.size;
    total_bytes += f.size;

    const std::string base = f.path.substr(f.path.find_last_of("/\\") + 1);
    if (mu::AsciiLower(base) == "thumbs.db") {
      ++junk_files;
      junk_bytes += f.size;
      continue;
    }

    // Only open the files we can actually interpret; a 2.4 GB tree must not be
    // read end to end just to produce an inventory.
    if (ext == ".ozt" || ext == ".ozj" || ext == ".bmd" || ext == ".att" ||
        ext == ".map" || ext == ".obj" || ext == ".tga") {
      std::vector<uint8_t> buf;
      if (!mu::ReadFile(f.path, &buf)) {
        ++s.parse_fail;
        continue;
      }
      mu::ByteView view(buf.data(), buf.size());

      if (ext == ".ozt") {
        mu::OztImage img;
        mu::Status st = mu::OztProbe(view, &img);
        if (st) {
          ++s.parsed_ok;
          s.decoded_pixels += img.pixel_bytes();
          total_vram += img.pixel_bytes();
        } else {
          ++s.parse_fail;
          ++s.notes[st.message];
        }
      } else if (ext == ".ozj") {
        mu::OzjImage img;
        mu::Status st = mu::OzjProbe(view, &img);
        if (st) {
          ++s.parsed_ok;
          // A JPEG texture is expanded to RGBA before upload.
          const uint64_t px =
              static_cast<uint64_t>(img.width) * img.height * 4;
          s.decoded_pixels += px;
          total_vram += px;
          if (img.width > 256 || img.height > 256)
            ++s.notes["larger than 256x256"];
        } else {
          ++s.parse_fail;
          ++s.notes[st.message];
        }
      } else if (ext == ".bmd") {
        mu::BmdInfo info;
        mu::Status st = mu::BmdProbe(view, &info);
        if (st) {
          ++s.parsed_ok;
          ++s.notes[mu::BmdKindName(info.kind)];
          if (info.payload_entropy > 7.5) ++s.notes["payload entropy > 7.5"];
        } else {
          ++s.parse_fail;
          ++s.notes[st.message];
        }
      } else if (ext == ".tga") {
        mu::TgaImage tga;
        mu::Status st = mu::TgaProbe(view, &tga);
        if (st) {
          ++s.parsed_ok;
          s.decoded_pixels += tga.pixel_bytes;
          total_vram += tga.pixel_bytes;
        } else {
          ++s.parse_fail;
          ++s.notes[st.message];
        }
      } else {  // .att / .map / .obj
        mu::TerrainInfo ti;
        mu::Status st = mu::TerrainProbe(view, &ti);
        if (st) {
          ++s.parsed_ok;
          ++s.notes[std::string(mu::TerrainKindName(ti.kind))];
          ++s.notes[ti.looks_plaintext ? "plaintext" : "encrypted"];
        } else {
          ++s.parse_fail;
        }
      }
    }
  }

  std::printf("scan of %s\n", root.c_str());
  std::printf("  files: %zu   total: %.2f MB\n", files.size(),
              static_cast<double>(total_bytes) / 1048576.0);
  if (junk_files)
    std::printf("  junk (Thumbs.db): %zu files, %.2f MB\n", junk_files,
                static_cast<double>(junk_bytes) / 1048576.0);
  std::printf("  estimated uncompressed texture memory: %.2f MB\n\n",
              static_cast<double>(total_vram) / 1048576.0);

  std::printf("%-10s %8s %12s %8s %8s %14s\n", "ext", "count", "bytes",
              "ok", "fail", "decoded px");
  for (const auto& kv : stats) {
    const ExtStats& s = kv.second;
    std::printf("%-10s %8zu %12llu %8zu %8zu %14llu\n", kv.first.c_str(),
                s.count, static_cast<unsigned long long>(s.bytes), s.parsed_ok,
                s.parse_fail,
                static_cast<unsigned long long>(s.decoded_pixels));
    for (const auto& n : s.notes)
      std::printf("%-10s          note: %s (%zu)\n", "", n.first.c_str(),
                  n.second);
  }
  return 0;
}

int CmdInfo(int argc, char** argv) {
  if (argc < 3) {
    Usage();
    return 1;
  }
  const std::string path = argv[2];
  std::vector<uint8_t> buf;
  if (!mu::ReadFile(path, &buf)) {
    std::fprintf(stderr, "error: cannot read '%s'\n", path.c_str());
    return 1;
  }
  mu::ByteView view(buf.data(), buf.size());
  std::printf("file: %s (%llu bytes)\n", path.c_str(),
              static_cast<unsigned long long>(buf.size()));
  std::printf("  crc32: %08x\n", mu::Crc32(buf.data(), buf.size()));
  std::printf("  entropy: %.4f bits/byte\n",
              mu::ByteEntropy(buf.data(), buf.size()));

  mu::OztImage ozt;
  if (mu::OztProbe(view, &ozt)) {
    std::printf("  format: OZT (32-bit BGRA texture)\n");
    std::printf("    %ux%u  %u bpp  footer=%s  trailer=%zu bytes\n", ozt.width,
                ozt.height, ozt.bits_per_pixel, ozt.has_footer ? "yes" : "no",
                ozt.trailer_size);
    std::printf("    pixel payload: %zu bytes\n", ozt.pixel_bytes());
    return 0;
  }
  mu::OzjImage ozj;
  if (mu::OzjProbe(view, &ozj)) {
    bool found = false;
    const std::size_t start = mu::OzjFindImageStart(view, &found);
    if (start != 0)
      std::printf("  container: OZJ with a %zu-byte duplicated header prefix\n",
                  start);
    std::printf("  format: OZJ (JPEG texture)\n");
    std::printf("    %ux%u  %u bits/sample  %u components  %s\n", ozj.width,
                ozj.height, ozj.precision, ozj.components,
                ozj.is_progressive ? "progressive" : "baseline");
    std::printf("    end-of-image marker: %s\n",
                ozj.has_end_of_image ? "present" : "MISSING");
    return 0;
  }
  mu::BmdInfo bmd;
  if (mu::BmdProbe(view, &bmd)) {
    std::printf("  format: BMD (%s)\n", mu::BmdKindName(bmd.kind));
    // The version byte is only meaningful for the plain BMD variants; for a
    // bux container the first bytes are ciphertext and would print as noise.
    if (bmd.kind != mu::BmdKind::kBuxContainer)
      std::printf("    version: 0x%02X\n", bmd.version);
    std::printf("    declared payload: %u  actual: %zu at offset %zu\n",
                bmd.declared_payload_size, bmd.payload_size,
                bmd.payload_offset);
    std::printf("    length field consistent: %s\n",
                bmd.length_consistent ? "yes" : "NO");
    std::printf("    payload entropy: %.4f\n", bmd.payload_entropy);
    return 0;
  }
  mu::TgaImage tga;
  if (mu::TgaProbe(view, &tga)) {
    std::printf("  format: TGA\n");
    std::printf("    %ux%u  %u bpp  image type %u (%s)\n", tga.width,
                tga.height, tga.bits_per_pixel, tga.image_type,
                tga.image_type == 2 ? "uncompressed" : "RLE");
    std::printf("    origin: %s\n",
                tga.top_to_bottom ? "top-left" : "bottom-left");
    return 0;
  }
  mu::TerrainInfo ti;
  const std::string ext = mu::ExtensionOf(path);
  if ((ext == ".att" || ext == ".map" || ext == ".obj") &&
      mu::TerrainProbe(view, &ti)) {
    std::printf("  format: terrain %s\n", mu::TerrainKindName(ti.kind));
    std::printf("    header %zu bytes, payload %zu bytes\n", ti.header_size,
                ti.payload_size);
    std::printf("    grid consistent: %s\n", ti.grid_consistent ? "yes" : "no");
    std::printf("    payload: %s (small-byte density %.3f)\n",
                ti.looks_plaintext ? "plaintext" : "encrypted",
                ti.small_byte_density);
    return 0;
  }

  // Naming the extension and the reason each reader declined is what makes
  // this subcommand useful for diagnosing a file, instead of a dead end.
  std::printf("  format: unrecognised (extension '%s')\n",
              ext.empty() ? "(none)" : ext.c_str());
  mu::OztImage ozt2;
  std::printf("    ozt:     %s\n", mu::OztProbe(view, &ozt2).message.c_str());
  mu::OzjImage ozj2;
  std::printf("    ozj:     %s\n", mu::OzjProbe(view, &ozj2).message.c_str());
  mu::BmdInfo bmd2;
  std::printf("    bmd:     %s\n", mu::BmdProbe(view, &bmd2).message.c_str());
  mu::TgaImage tga2;
  std::printf("    tga:     %s\n", mu::TgaProbe(view, &tga2).message.c_str());
  return 0;
}

int CmdVerify(int argc, char** argv) {
  if (argc < 4) {
    Usage();
    return 1;
  }
  std::vector<uint8_t> text;
  if (!mu::ReadFile(argv[2], &text)) {
    std::fprintf(stderr, "error: cannot read manifest '%s'\n", argv[2]);
    return 1;
  }
  mu::ManifestParseResult parsed =
      mu::ManifestParse(mu::ByteView(text.data(), text.size()));
  std::printf("manifest %s: %zu entries, %d skipped\n", argv[2],
              parsed.entries.size(), parsed.skipped_lines);
  for (const std::string& e : parsed.errors) std::printf("  parse: %s\n", e.c_str());

  mu::VerifyReport rep = mu::ManifestVerify(parsed.entries, argv[3]);
  std::printf("\nverified against %s\n", argv[3]);
  std::printf("  total          %zu\n", rep.total);
  std::printf("  match          %zu\n", rep.match);
  std::printf("  size mismatch  %zu\n", rep.size_mismatch);
  std::printf("  crc mismatch   %zu\n", rep.crc_mismatch);
  std::printf("  missing        %zu\n", rep.missing);
  std::printf("  unreadable     %zu\n", rep.unreadable);
  for (const std::string& p : rep.problems) std::printf("    %s\n", p.c_str());
  if (rep.problems.size() < rep.total - rep.match &&
      rep.total - rep.match > mu::VerifyReport::kMaxProblems)
    std::printf("    ... (%zu more)\n",
                rep.total - rep.match - rep.problems.size());
  return rep.clean() ? 0 : 2;
}

int CmdDedupe(int argc, char** argv) {
  if (argc < 3) {
    Usage();
    return 1;
  }
  std::vector<mu::FileEntry> files;
  if (!mu::WalkFiles(argv[2], &files)) {
    std::fprintf(stderr, "error: cannot open directory '%s'\n", argv[2]);
    return 1;
  }
  // Group by size first: hashing is only worth doing for files that could
  // possibly collide, which on a 29k-file tree is a large saving.
  std::map<uint64_t, std::vector<const mu::FileEntry*>> by_size;
  for (const mu::FileEntry& f : files) by_size[f.size].push_back(&f);

  std::size_t dup_groups = 0;
  std::size_t dup_files = 0;
  uint64_t reclaimable = 0;

  for (const auto& kv : by_size) {
    if (kv.second.size() < 2) continue;
    std::map<uint32_t, std::vector<const mu::FileEntry*>> by_crc;
    for (const mu::FileEntry* f : kv.second) {
      std::vector<uint8_t> buf;
      if (!mu::ReadFile(f->path, &buf)) continue;
      by_crc[mu::Crc32(buf.data(), buf.size())].push_back(f);
    }
    for (const auto& g : by_crc) {
      if (g.second.size() < 2) continue;
      ++dup_groups;
      dup_files += g.second.size() - 1;
      reclaimable += kv.first * (g.second.size() - 1);
      std::printf("duplicate set (%llu bytes each, %zu copies):\n",
                  static_cast<unsigned long long>(kv.first), g.second.size());
      for (const mu::FileEntry* f : g.second)
        std::printf("    %s\n", f->path.c_str());
    }
  }

  std::printf("\n%d duplicate groups, %zu redundant copies, %.2f MB reclaimable\n",
              static_cast<int>(dup_groups), dup_files,
              static_cast<double>(reclaimable) / 1048576.0);
  return 0;
}

int CmdBmdReport(int argc, char** argv) {
  if (argc < 3) {
    Usage();
    return 1;
  }
  std::vector<mu::FileEntry> files;
  if (!mu::WalkFiles(argv[2], &files)) {
    std::fprintf(stderr, "error: cannot open directory '%s'\n", argv[2]);
    return 1;
  }
  std::sort(files.begin(), files.end(),
            [](const mu::FileEntry& a, const mu::FileEntry& b) {
              return a.path < b.path;
            });

  mu::BmdSummary sum;
  std::vector<mu::BmdFileReport> opaque_samples;
  std::vector<mu::BmdFileReport> plaintext_samples;
  std::vector<mu::BmdFileReport> invalid_samples;

  for (const mu::FileEntry& f : files) {
    if (mu::ExtensionOf(f.path) != ".bmd") continue;
    mu::BmdFileReport r = mu::BmdClassifyFile(f.path);
    mu::BmdAccumulate(r, &sum);
    if (r.access == mu::PayloadAccess::kOpaque && opaque_samples.size() < 5)
      opaque_samples.push_back(r);
    if (r.access == mu::PayloadAccess::kPlaintext && plaintext_samples.size() < 10)
      plaintext_samples.push_back(r);
    if (!r.structure_valid && invalid_samples.size() < 10)
      invalid_samples.push_back(r);
  }

  std::printf("BMD usability report for %s\n\n", argv[2]);
  std::printf("Three separate questions, three separate answers. A container\n"
              "header that classifies cleanly says nothing about whether a\n"
              "vertex can be extracted from it.\n\n");

  std::printf("  total .bmd files                         %8zu\n\n", sum.total);

  std::printf("LEVEL 1 - container structure valid\n");
  std::printf("  magic + version recognised, and for v0x0C the declared\n");
  std::printf("  payload length agrees with the file length.\n");
  std::printf("    valid                                  %8zu  (%6.2f%%)\n",
              sum.structure_valid, mu::PercentOf(sum.structure_valid, sum.total));
  std::printf("    invalid                                %8zu  (%6.2f%%)\n\n",
              sum.structure_invalid,
              mu::PercentOf(sum.structure_invalid, sum.total));

  std::printf("LEVEL 2 - payload obtainable in the clear\n");
  std::printf("    plaintext                              %8zu  (%6.2f%%)\n",
              sum.cleartext_plaintext,
              mu::PercentOf(sum.cleartext_plaintext, sum.total));
  std::printf("    bux-decryptable                        %8zu  (%6.2f%%)\n",
              sum.cleartext_bux, mu::PercentOf(sum.cleartext_bux, sum.total));
  std::printf("    OPAQUE (cipher not recovered)          %8zu  (%6.2f%%)\n\n",
              sum.opaque, mu::PercentOf(sum.opaque, sum.total));

  std::printf("LEVEL 3 - geometry actually parsed (vertices/faces/bones)\n");
  std::printf("    parsed                                 %8zu  (%6.2f%%)\n",
              sum.geometry_parsed,
              mu::PercentOf(sum.geometry_parsed, sum.total));
  std::printf("    No BMD body parser exists in this project. This line is 0\n");
  std::printf("    by construction, not by omission.\n\n");

  std::printf("container kinds\n");
  std::printf("    BMD v0x0C                              %8zu  (%6.2f%%)\n",
              sum.kind_v0c, mu::PercentOf(sum.kind_v0c, sum.total));
  std::printf("    BMD v0x0A (legacy)                     %8zu  (%6.2f%%)\n",
              sum.kind_v0a, mu::PercentOf(sum.kind_v0a, sum.total));
  std::printf("    BMD other version                      %8zu  (%6.2f%%)\n",
              sum.kind_other_version,
              mu::PercentOf(sum.kind_other_version, sum.total));
  std::printf("    bux-encrypted container                %8zu  (%6.2f%%)\n",
              sum.kind_bux_container,
              mu::PercentOf(sum.kind_bux_container, sum.total));
  std::printf("    unrecognised                           %8zu  (%6.2f%%)\n\n",
              sum.kind_unknown, mu::PercentOf(sum.kind_unknown, sum.total));

  std::printf("payload entropy: max %.4f bits/byte, %zu above %.1f\n\n",
              sum.max_entropy, sum.entropy_above_7_5,
              mu::kEncryptedEntropyThreshold);

  if (!plaintext_samples.empty()) {
    std::printf("sample files with a payload in the clear:\n");
    for (const mu::BmdFileReport& r : plaintext_samples)
      std::printf("    %-52s entropy %.3f  kind %s\n", r.path.c_str(),
                  r.payload_entropy, mu::BmdKindName(r.kind));
    std::printf("\n");
  }
  if (!opaque_samples.empty()) {
    std::printf("sample OPAQUE files (cipher not recovered):\n");
    for (const mu::BmdFileReport& r : opaque_samples)
      std::printf("    %-52s entropy %.3f  kind %s\n", r.path.c_str(),
                  r.payload_entropy, mu::BmdKindName(r.kind));
    std::printf("\n");
  }
  if (!invalid_samples.empty()) {
    std::printf("sample structurally invalid files:\n");
    for (const mu::BmdFileReport& r : invalid_samples)
      std::printf("    %-52s %s\n", r.path.c_str(), r.problem.c_str());
  }

  // Exit non-zero when any payload is opaque, so this doubles as a gate that
  // fails until the cipher is recovered.
  return sum.opaque == 0 ? 0 : 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    Usage();
    return 1;
  }
  const std::string cmd = argv[1];
  if (cmd == "scan") return CmdScan(argc, argv);
  if (cmd == "info") return CmdInfo(argc, argv);
  if (cmd == "verify") return CmdVerify(argc, argv);
  if (cmd == "dedupe") return CmdDedupe(argc, argv);
  if (cmd == "bmdreport") return CmdBmdReport(argc, argv);
  if (cmd == "-h" || cmd == "--help" || cmd == "help") {
    Usage();
    return 0;
  }
  std::fprintf(stderr, "error: unknown subcommand '%s'\n\n", cmd.c_str());
  Usage();
  return 1;
}
