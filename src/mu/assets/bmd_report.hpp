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
// Three levels, from weakest to strongest:
//
//   1. STRUCTURE  - the magic and version are recognised and, for v0x0C, the
//                   declared payload length agrees with the file length.
//   2. CLEARTEXT  - the payload bytes can be obtained in the clear, either
//                   because they are already plaintext or because the bux
//                   cipher decrypts them.
//   3. GEOMETRY   - vertices, faces and bones have actually been parsed out of
//                   the payload.
//
// Level 3 is currently zero for every file, because no BMD body parser exists.
// Reporting it as zero rather than omitting the column is the point of this
// module: the omission is what made the earlier percentage misleading.
#ifndef MU_ASSETS_BMD_REPORT_HPP
#define MU_ASSETS_BMD_REPORT_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "mu/assets/bmd.hpp"
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

  /// Level 3. Set only when a body parser actually extracted geometry, and
  /// only for the versions whose layout is verified (v0x0A). Counts are the
  /// parser's own tally, not the header's, so a mismatch cannot hide.
  bool geometry_parsed = false;
  std::size_t vertex_count = 0;
  std::size_t normal_count = 0;
  std::size_t triangle_count = 0;
  std::size_t bad_corner_refs = 0;
  /// Non-empty when a body parse was attempted and failed. Kept separate from
  /// |problem| so a container-level fault and a body-level fault are not
  /// confused with each other.
  std::string body_problem;
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
  std::size_t bad_corner_refs = 0;

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
