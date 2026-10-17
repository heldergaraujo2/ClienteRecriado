// SPDX-License-Identifier: MIT
// BMD usability reporting.
//
// This module exists because a single "parsed OK" percentage was misleading.
// Saying "10,066 of 10,090 BMD files read" described only whether the
// container header could be classified. It said nothing about whether a single
// vertex could be extracted, which is the thing that actually matters for
// rendering. These are different questions and they have very different
// answers, so they get different metrics.
//
// The report keeps distinct stages and sub-metrics rather than collapsing them
// into one "parsed" percentage:
//
//   1. STRUCTURE  - magic and version are recognised and, for v0x0C, the
//                   declared payload length agrees with the file length.
//   2. CLEARTEXT  - payload bytes are plaintext or a supported cipher is
//                   verified/decrypted.
//   3. GEOMETRY   - a supported body layout yields vertices and normals.
//   4. TOPOLOGY   - triangle records are read; out-of-range vertex and normal
//                   references are counted separately from usable triangles.
//   5. BONES      - bone records pass the current parser's checks.
//
// These stages do not all form a cumulative hierarchy: topology is a subset of
// parsed geometry, but the bone metric is reported independently. UV semantics,
// texture association, animation meaning, and rendered appearance are not
// measured by this module.
#ifndef MU_ASSETS_BMD_REPORT_HPP
#define MU_ASSETS_BMD_REPORT_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/format.hpp"

namespace mu {

enum class PayloadAccess {
  kOpaque,        ///< Encrypted, cipher not recovered.
  kBuxDecryptable,///< Recoverable with the bux 3-byte XOR key.
  kV0CDecryptable,///< Recoverable with the v0x0C rolling-XOR cipher.
  kPlaintext,     ///< Already in the clear.
};

const char* PayloadAccessName(PayloadAccess a);

struct BmdFileReport {
  std::string path;
  uint64_t size = 0;
  BmdKind kind = BmdKind::kUnknown;
  uint8_t version = 0;
  bool structure_valid = false;
  PayloadAccess access = PayloadAccess::kOpaque;
  double payload_entropy = 0.0;
  double bux_gain = 0.0;
  std::string problem;  ///< Empty when nothing is wrong.

  /// Level 3. Set only when a body parser extracted geometry from a supported
  /// clear payload (currently v0x0A plaintext and verified v0x0C decryption).
  /// Counts are the parser's own tally, not the header's.
  bool geometry_parsed = false;
  std::size_t vertex_count = 0;
  std::size_t normal_count = 0;
  std::size_t triangle_count = 0;
  std::size_t out_of_range_vertex_refs = 0;
  std::size_t out_of_range_normal_refs = 0;
  /// Non-empty when a body parse was attempted and failed. Kept separate from
  /// |problem| so a container-level fault and a body-level fault are not
  /// confused with each other.
  std::string body_problem;

  /// Why the triangle array was not read, for files that reached level 3 with
  /// no faces. Meaningful only when |geometry_parsed| is true.
  BmdTopologySkip topology_skip = BmdTopologySkip::kNone;
  std::size_t declared_mesh_count = 0;
  std::size_t declared_triangle_count = 0;

  /// Level 5 - bones. Meaningful only when |geometry_parsed| is true.
  bool has_bones = false;
  std::size_t bone_count = 0;
  std::size_t declared_bone_count = 0;
  std::size_t declared_action_count = 0;
};

struct BmdSummary {
  std::size_t total = 0;

  // Level 1
  std::size_t structure_valid = 0;
  std::size_t structure_invalid = 0;

  // Level 2
  std::size_t cleartext_plaintext = 0;
  std::size_t cleartext_bux = 0;
  std::size_t cleartext_v0c = 0;
  std::size_t opaque = 0;

  // Level 3
  std::size_t geometry_parsed = 0;
  std::size_t topology_parsed = 0;
  std::size_t triangles_total = 0;
  std::size_t out_of_range_vertex_refs = 0;
  std::size_t out_of_range_normal_refs = 0;

  // Level 5 - bones
  std::size_t bones_parsed = 0;
  std::size_t total_bones = 0;

  /// Why the level-3 files that have no topology have none. These four counts
  /// sum to |geometry_parsed| - |topology_parsed|, and the sum is asserted in
  /// the test suite so a fifth cause cannot appear silently.
  std::size_t skip_multi_mesh = 0;
  std::size_t skip_zero_faces = 0;
  std::size_t skip_does_not_fit = 0;
  std::size_t skip_no_header = 0;
  /// meshCount as declared, across all level-3 files, so the multi-mesh share
  /// is measured rather than inferred from the skip counts.
  std::size_t mesh_count_hist[16] = {0};
  std::size_t mesh_count_above_15 = 0;

  // Breakdown by container kind.
  std::size_t kind_v0c = 0;
  std::size_t kind_v0a = 0;
  std::size_t kind_other_version = 0;
  std::size_t kind_bux_container = 0;
  std::size_t kind_unknown = 0;

  /// Highest payload entropy observed, for the write-up.
  double max_entropy = 0.0;
  /// Count of payloads above the encrypted threshold.
  std::size_t entropy_above_7_5 = 0;
};

/// Threshold above which a payload is treated as encrypted or compressed.
/// Measured separation: plaintext BMD bodies sit well below 6.5, encrypted
/// ones measure 7.9 or higher. 7.5 sits between them with margin.
constexpr double kEncryptedEntropyThreshold = 7.5;

/// Classifies one file. Never fails: an unrecognisable file is reported with
/// structure_valid false and a reason, rather than dropped.
BmdFileReport BmdClassifyFile(const std::string& path);

/// Classifies an in-memory buffer, for tests.
BmdFileReport BmdClassifyBytes(ByteView data, const std::string& label);

/// Folds a per-file report into a summary.
void BmdAccumulate(const BmdFileReport& r, BmdSummary* s);

/// Percent of |part| out of |whole|, safe against division by zero.
double PercentOf(std::size_t part, std::size_t whole);

}  // namespace mu

#endif  // MU_ASSETS_BMD_REPORT_HPP
