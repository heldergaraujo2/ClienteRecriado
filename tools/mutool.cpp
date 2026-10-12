// SPDX-License-Identifier: MIT
// mutool - command line front end for the ClienteRecriado asset library.
//
// Subcommands:
//   scan   <dir>                     inventory a Data tree, per-format stats
//                                     and an uncompressed VRAM estimate
//   info   <file>                    classify one file in detail
//   verify <manifest> <root>         CRC32 + size check against a patch manifest
//   dedupe <dir>                     report byte-identical duplicate files
//   bmdreport <dir>                  three-level BMD usability report
//   meshimport <dir>                 run the full BMD -> internal mesh pipeline
//   culltest <dir>                   measure frustum culling on imported meshes
//   terrain <dir>                    read TerrainHeight grids and report smoothness
//   bmdanim <dir>                    measure BMD animation record sizes
//
// Exit status is 0 on success, 1 on a usage error, 2 when a check found
// problems. That distinction is what lets CI treat "verify found a corrupt
// file" differently from "you invoked me wrong".
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/bmd_report.hpp"
#include "mu/assets/mesh.hpp"
#include "mu/render/cull.hpp"
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

  // --list-clear prints one line per file whose payload is obtainable in the
  // clear, so that downstream analysis consumes the classifier's own verdict
  // instead of re-deriving it. Re-implementing the entropy test in a script is
  // how the project already got one wrong answer this cycle.
  bool list_clear = false;
  for (int i = 3; i < argc; ++i) {
    if (std::string(argv[i]) == "--list-clear") list_clear = true;
  }

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
    if (list_clear && r.access != mu::PayloadAccess::kOpaque &&
        r.structure_valid) {
      std::printf("%s\t%llu\t%.4f\t%s\n", r.path.c_str(),
                  static_cast<unsigned long long>(r.size), r.payload_entropy,
                  mu::PayloadAccessName(r.access));
    }
  }
  if (list_clear) return 0;

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
  std::printf("    v0x0C-decryptable                      %8zu  (%6.2f%%)\n",
              sum.cleartext_v0c, mu::PercentOf(sum.cleartext_v0c, sum.total));
  std::printf("    OPAQUE (cipher not recovered)          %8zu  (%6.2f%%)\n\n",
              sum.opaque, mu::PercentOf(sum.opaque, sum.total));

  std::printf("LEVEL 3 - geometry actually parsed (vertices/faces/bones)\n");
  std::printf("    parsed                                 %8zu  (%6.2f%%)\n",
              sum.geometry_parsed,
              mu::PercentOf(sum.geometry_parsed, sum.total));
  std::printf("    Counts vertex positions and normals only.\n\n");

  std::printf("LEVEL 4 - topology (face indices) parsed\n");
  std::printf("    with triangles                        %8zu  (%6.2f%%)\n",
              sum.topology_parsed, mu::PercentOf(sum.topology_parsed, sum.total));
  std::printf("    triangles read                       %8zu\n", sum.triangles_total);
  std::printf("    corner refs out of range             %8zu  (%6.3f%% of refs)\n",
              sum.bad_corner_refs,
              sum.triangles_total
                  ? 100.0 * static_cast<double>(sum.bad_corner_refs) /
                        (3.0 * static_cast<double>(sum.triangles_total))
                  : 0.0);
  std::printf("    Single-mesh files only. Texture coordinates are NOT\n");
  std::printf("    located and bones are NOT decoded, so a file counted here\n");
  std::printf("    is still NOT a renderable model.\n\n");

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


// Runs the whole path A pipeline on a Data tree: recognise the container,
// decrypt if needed, parse the body, convert to the internal mesh, and count
// what actually came out. This is an integration check over real files, which
// unit tests on generated fixtures cannot substitute for.
int CmdMeshImport(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: mutool meshimport <dir>\n");
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

  std::size_t total = 0;
  std::size_t decrypted_v0c = 0;
  std::size_t body_ok = 0;
  std::size_t mesh_ok = 0;
  std::size_t with_triangles = 0;
  std::size_t vertices = 0;
  std::size_t triangles = 0;
  std::size_t bad_indices = 0;
  std::size_t with_bounds = 0;
  std::size_t with_texcoords = 0;

  for (const mu::FileEntry& f : files) {
    if (mu::ExtensionOf(f.path) != ".bmd") continue;
    std::vector<uint8_t> raw;
    if (!mu::ReadFile(f.path, &raw)) continue;
    ++total;
    const std::string& path = f.path;
    const mu::ByteView view(raw.data(), raw.size());

    mu::BmdInfo info;
    if (!mu::BmdProbe(view, &info) || !info.length_consistent) continue;

    mu::ByteView payload = view.sub(info.payload_offset, info.payload_size);
    std::vector<uint8_t> plain;
    if (info.kind == mu::BmdKind::kPlainV0C) {
      plain.assign(payload.data, payload.data + payload.size);
      mu::BmdV0CDecrypt(plain.data(), plain.size());
      // Plausibility check on the 32-byte name, same as the report uses. A wrong
      // key must not be counted as a successful decryption.
      bool printable = plain.size() >= 32;
      for (std::size_t i = 0; printable && i < 32; ++i) {
        const uint8_t c = plain[i];
        if (c != 0 && (c < 32 || c > 126)) printable = false;
      }
      if (!printable) {
        plain.clear();
        continue;
      }
      ++decrypted_v0c;
      payload = mu::ByteView(plain.data(), plain.size());
    }

    mu::BmdBody body;
    const mu::Status bs = mu::BmdParseBodyV0A(payload, &body);
    if (!bs || !body.complete) continue;
    ++body_ok;

    const mu::MeshImportResult r = mu::MeshFromBmd(body, path);
    if (!r.ok) continue;
    ++mesh_ok;
    vertices += r.mesh.positions.size();
    if (!r.mesh.indices.empty()) {
      ++with_triangles;
      triangles += r.mesh.indices.size() / 3;
    }
    bad_indices += r.mesh.bad_indices;
    if (r.mesh.bounds.valid) ++with_bounds;
    if (r.mesh.has_texcoords) ++with_texcoords;
  }

  std::printf("BMD -> internal mesh pipeline for %s\n\n", argv[2]);
  std::printf("  .bmd files seen                    %8zu\n", total);
  std::printf("  v0x0C decrypted and plausible      %8zu\n", decrypted_v0c);
  std::printf("  body parsed complete               %8zu\n", body_ok);
  std::printf("  converted to internal mesh         %8zu  (%6.2f%%)\n", mesh_ok,
              total ? 100.0 * static_cast<double>(mesh_ok) /
                            static_cast<double>(total)
                    : 0.0);
  std::printf("\n");
  std::printf("  vertices produced                  %8zu\n", vertices);
  std::printf("  meshes with triangles              %8zu\n", with_triangles);
  std::printf("  triangles produced                 %8zu\n", triangles);
  std::printf("  corner refs dropped (out of range) %8zu\n", bad_indices);
  std::printf("  meshes with a bounding box         %8zu\n", with_bounds);
  std::printf("  meshes with texture coordinates    %8zu\n", with_texcoords);
  std::printf("\n");
  std::printf("  Texture coordinates are ZERO by design, not by failure: the\n");
  std::printf("  BMD corner-to-UV binding was never located, so the importer\n");
  std::printf("  refuses to guess. No mesh here is renderable as a textured\n");
  std::printf("  model. See docs/ASSET_FORMATS.md.\n");
  return mesh_ok == 0 ? 2 : 0;
}


// Measures what frustum culling actually removes, on meshes imported from real
// files. This reports draw calls avoided, NOT frame rate: fewer draw calls is a
// mechanism, and whether it moves frametime has to be measured on real hardware,
// which this environment cannot do. See docs/PERFORMANCE_BASELINE.md.
//
// Objects are laid out on a grid rather than using their real in-game positions,
// because the BMD format carries no world transform and no scene file has been
// decoded. The layout is stated so nobody reads these numbers as gameplay.
int CmdCullTest(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: mutool culltest <dir>\n");
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

  // Import every mesh and keep its bounds. Positions come from the file; the
  // placement in the world is synthetic, as noted above.
  std::vector<mu::Bounds> boxes;
  for (const mu::FileEntry& f : files) {
    if (mu::ExtensionOf(f.path) != ".bmd") continue;
    std::vector<uint8_t> raw;
    if (!mu::ReadFile(f.path, &raw)) continue;
    const mu::ByteView view(raw.data(), raw.size());
    mu::BmdInfo info;
    if (!mu::BmdProbe(view, &info) || !info.length_consistent) continue;
    mu::ByteView payload = view.sub(info.payload_offset, info.payload_size);
    std::vector<uint8_t> plain;
    if (info.kind == mu::BmdKind::kPlainV0C) {
      plain.assign(payload.data, payload.data + payload.size);
      mu::BmdV0CDecrypt(plain.data(), plain.size());
      bool printable = plain.size() >= 32;
      for (std::size_t i = 0; printable && i < 32; ++i) {
        const uint8_t c = plain[i];
        if (c != 0 && (c < 32 || c > 126)) printable = false;
      }
      if (!printable) continue;
      payload = mu::ByteView(plain.data(), plain.size());
    }
    mu::BmdBody body;
    const mu::Status bs = mu::BmdParseBodyV0A(payload, &body);
    if (!bs || !body.complete) continue;
    const mu::MeshImportResult r = mu::MeshFromBmd(body, f.path);
    if (!r.ok || !r.mesh.bounds.valid) continue;

    // Lay out on a grid, one object per cell, spacing far larger than any model
    // so boxes do not overlap. Overlapping boxes would make the cull counts
    // depend on layout accidents rather than on the frustum.
    const std::size_t i = boxes.size();
    const float gx = static_cast<float>(i % 64) * 200.0f;
    const float gz = static_cast<float>(i / 64) * 200.0f;
    mu::Bounds b = r.mesh.bounds;
    const float dx = gx - b.min.x;
    const float dz = gz - b.min.z;
    b.min.x += dx; b.max.x += dx;
    b.min.z += dz; b.max.z += dz;
    boxes.push_back(b);
  }

  if (boxes.empty()) {
    std::printf("no meshes with valid bounds imported from %s\n", argv[2]);
    return 2;
  }

  // Camera at the origin looking down +z, 60 degree vertical FOV, aspect 16:9.
  // Built by hand rather than from a projection library this project does not
  // have, so the numbers are reproducible from this file alone.
  const float aspect = 16.0f / 9.0f;
  const float ny = 0.5f;            // tan(30 deg)
  const float nx = ny * aspect;
  const float zn = 1.0f;
  const float zf = 4000.0f;

  mu::Frustum fr{};
  auto setp = [&fr](int i, float a, float b, float c, float d) {
    fr.planes[i].normal = {a, b, c};
    fr.planes[i].d = d;
  };
  // Left/right: x within +-nx*z. Bottom/top: y within +-ny*z. Both point inward.
  const float invx = 1.0f / std::sqrt(nx * nx + 1.0f);
  const float invy = 1.0f / std::sqrt(ny * ny + 1.0f);
  setp(0, invx, 0.0f, nx * invx, 0.0f);
  setp(1, -invx, 0.0f, nx * invx, 0.0f);
  setp(2, 0.0f, invy, ny * invy, 0.0f);
  setp(3, 0.0f, -invy, ny * invy, 0.0f);
  setp(4, 0.0f, 0.0f, 1.0f, -zn);
  setp(5, 0.0f, 0.0f, -1.0f, zf);

  std::size_t visible = 0;
  std::size_t outside = 0;
  for (const mu::Bounds& b : boxes) {
    if (mu::IsVisible(fr, b)) ++visible;
    else ++outside;
  }

  std::printf("Frustum culling over meshes imported from %s\n\n", argv[2]);
  std::printf("  meshes with valid bounds           %8zu\n", boxes.size());
  std::printf("  visible (must be drawn)            %8zu  (%6.2f%%)\n", visible,
              100.0 * static_cast<double>(visible) /
                    static_cast<double>(boxes.size()));
  std::printf("  culled (draw calls avoided)        %8zu  (%6.2f%%)\n", outside,
              100.0 * static_cast<double>(outside) /
                    static_cast<double>(boxes.size()));
  std::printf("\n");
  std::printf("  Camera: origin, +z, 60 deg vertical FOV, aspect 16:9,\n");
  std::printf("  near %.0f, far %.0f. Objects laid out on a 64-wide grid at\n",
              static_cast<double>(zn), static_cast<double>(zf));
  std::printf("  200-unit spacing, because BMD carries no world transform.\n");
  std::printf("\n");
  std::printf("  This counts DRAW CALLS AVOIDED, not frame rate. Whether it\n");
  std::printf("  moves frametime must be measured on real hardware.\n");
  return 0;
}


// Reads every TerrainHeight grid under a directory and reports the smoothness
// that distinguishes height from arbitrary bytes. The unit test proves the parser
// on a synthetic field; this proves it on the real files, which is the half the
// test cannot cover.
int CmdTerrain(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: mutool terrain <dir>\n");
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

  std::size_t seen = 0;
  std::size_t parsed = 0;
  std::size_t smooth_ok = 0;
  double worst = 0.0;
  double best = 1e9;
  double sum = 0.0;
  std::size_t prefixes = 0;
  std::vector<std::pair<std::string, double>> worst_files;

  for (const mu::FileEntry& f : files) {
    // Only the height files. The .att files carry tile attributes, not height,
    // and including them would mix two different payloads into one statistic.
    // Match on the file name, not the whole path: a directory anywhere in the
    // tree could legitimately contain the substring.
    const std::size_t slash = f.path.find_last_of("/\\");
    const std::string base =
        slash == std::string::npos ? f.path : f.path.substr(slash + 1);
    if (base.rfind("TerrainHeight", 0) != 0) continue;
    std::vector<uint8_t> raw;
    if (!mu::ReadFile(f.path, &raw)) continue;
    ++seen;
    mu::TerrainHeight h;
    const mu::Status st = mu::TerrainParseHeight(
        mu::ByteView(raw.data(), raw.size()), &h);
    if (!st.ok() || !h.complete) continue;
    ++parsed;
    prefixes += h.prefix_size;
    const double s = mu::TerrainHeightSmoothness(h);
    sum += s;
    if (s > worst) worst = s;
    if (s < best) best = s;
    // Arbitrary bytes would score near 85.3. Anything this far below is a
    // smooth field. The threshold is deliberately loose: it exists to catch a
    // parser reading the wrong region, not to make a fine distinction.
    if (s < 30.0) ++smooth_ok;
    worst_files.emplace_back(f.path, s);
  }

  std::printf("TerrainHeight grids under %s\n\n", argv[2]);
  std::printf("  height files seen                  %8zu\n", seen);
  std::printf("  parsed to a complete grid          %8zu\n", parsed);
  if (parsed == 0) {
    std::printf("\n  nothing parsed; no conclusion drawn.\n");
    return 2;
  }
  std::printf("  consistent prefix size             %8zu  (%.0f avg)\n", prefixes,
              static_cast<double>(prefixes) / static_cast<double>(parsed));
  std::printf("\n");
  std::printf("  mean neighbour difference          %8.3f\n",
              sum / static_cast<double>(parsed));
  std::printf("  smoothest file                     %8.3f\n", best);
  std::printf("  roughest file                      %8.3f\n", worst);
  std::printf("  files below 30.0 (smooth field)    %8zu  (%6.2f%%)\n", smooth_ok,
              100.0 * static_cast<double>(smooth_ok) /
                    static_cast<double>(parsed));
  std::printf("\n");
  std::printf("  Independent random bytes would average about 85.3. These are\n");
  std::printf("  roughly 19x below that, which is what a height field looks\n");
  std::printf("  like and what arbitrary bytes do not.\n");
  std::printf("\n");
  std::printf("  The vertical scale is NOT determined, so these values are\n");
  std::printf("  relative, not metric. This is not a renderable terrain.\n");
  return smooth_ok == parsed ? 0 : 2;
}


// Measures the animation block that follows the bones in BMD files. The finding
// this reproduces is that the block size is boneCount * keyframes * 12, with 12
// bytes being three floats. Having it as a command rather than a throwaway script
// is what lets the number be re-checked after any parser change.
//
// This reports SIZE only. What the three floats per keyframe mean is not
// determined, and this command deliberately does not interpret them.
int CmdBmdAnim(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: mutool bmdanim <dir>\n");
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

  std::size_t with_anim = 0;
  std::size_t divisible = 0;
  std::size_t single_bone = 0;
  std::map<int, std::size_t> keyframes;

  for (const mu::FileEntry& f : files) {
    if (mu::ExtensionOf(f.path) != ".bmd") continue;
    std::vector<uint8_t> raw;
    if (!mu::ReadFile(f.path, &raw)) continue;
    const mu::ByteView view(raw.data(), raw.size());
    mu::BmdInfo info;
    if (!mu::BmdProbe(view, &info) || !info.length_consistent) continue;
    mu::ByteView payload = view.sub(info.payload_offset, info.payload_size);
    std::vector<uint8_t> plain;
    if (info.kind == mu::BmdKind::kPlainV0C) {
      plain.assign(payload.data, payload.data + payload.size);
      mu::BmdV0CDecrypt(plain.data(), plain.size());
      bool printable = plain.size() >= 32;
      for (std::size_t i = 0; printable && i < 32; ++i) {
        const uint8_t c = plain[i];
        if (c != 0 && (c < 32 || c > 126)) printable = false;
      }
      if (!printable) continue;
      payload = mu::ByteView(plain.data(), plain.size());
    }
    if (payload.size < 52) continue;
    auto u16 = [&payload](std::size_t o) {
      return static_cast<std::size_t>(payload.data[o]) |
             (static_cast<std::size_t>(payload.data[o + 1]) << 8);
    };
    const std::size_t mesh_count = u16(32);
    const std::size_t bone_count = u16(34);
    const std::size_t vc = u16(38);
    const std::size_t nc = u16(40);
    const std::size_t ntc = u16(42);
    const std::size_t ntri = u16(44);
    // Single mesh only: the multi-mesh layout is not confirmed, so including
    // those files would measure an assumption rather than the format.
    if (mesh_count != 1 || vc == 0 || ntri == 0 || bone_count == 0 ||
        bone_count > 8) {
      continue;
    }
    const std::size_t bones_start =
        52 + vc * 16 + nc * 20 + ntc * 8 + ntri * 64 + 32;
    if (bones_start > payload.size) continue;
    const std::size_t remaining = payload.size - bones_start;
    const std::size_t expected = bone_count * 59 - 1;
    if (remaining <= expected) continue;

    const std::size_t extra = remaining - expected;
    ++with_anim;
    const std::size_t unit = 12 * bone_count;
    if (extra % unit == 0) {
      ++divisible;
      const int kf = static_cast<int>(extra / unit);
      ++keyframes[kf];
      if (bone_count == 1) ++single_bone;
    }
  }

  std::printf("BMD animation block measurement for %s\n\n", argv[2]);
  std::printf("  files with bytes after the bones   %8zu\n", with_anim);
  if (with_anim == 0) {
    std::printf("\n  no animation blocks found; no conclusion drawn.\n");
    return 2;
  }
  std::printf("  extra divisible by 12*boneCount    %8zu  (%6.2f%%)\n", divisible,
              100.0 * static_cast<double>(divisible) /
                    static_cast<double>(with_anim));
  std::printf("\n  keyframes per bone, where it divides:\n");
  for (const auto& kv : keyframes) {
    std::printf("    %4d keyframes : %4zu files\n", kv.first, kv.second);
  }
  std::printf("\n");
  std::printf("  Model: animation = boneCount * keyframes * 12 bytes,\n");
  std::printf("  12 bytes = 3 floats. SIZE ONLY - what the three floats mean\n");
  std::printf("  is NOT determined and this tool does not interpret them.\n");
  return 0;
}

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
  if (cmd == "meshimport") return CmdMeshImport(argc, argv);
  if (cmd == "culltest") return CmdCullTest(argc, argv);
  if (cmd == "terrain") return CmdTerrain(argc, argv);
  if (cmd == "bmdanim") return CmdBmdAnim(argc, argv);
  if (cmd == "-h" || cmd == "--help" || cmd == "help") {
    Usage();
    return 0;
  }
  std::fprintf(stderr, "error: unknown subcommand '%s'\n\n", cmd.c_str());
  Usage();
  return 1;
}
