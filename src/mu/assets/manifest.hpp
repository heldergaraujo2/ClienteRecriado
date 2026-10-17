// SPDX-License-Identifier: MIT
// Patch manifest handling, compatible with the reference client's
// ListaPatch.txt: one record per line, "relative/path CRC32HEX SIZE",
// space separated, CRC as 8 lowercase hex digits and size in decimal bytes.
//
// Example line from the reference client:
//     Data/gate.bmd 05d2a7b8 7168
//
// This module exists so the reconstruction can verify an installed Data tree
// against the same manifest the original client ships, and so patch payloads
// can be integrity-checked before they are applied.
#ifndef MU_ASSETS_MANIFEST_HPP
#define MU_ASSETS_MANIFEST_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "mu/assets/format.hpp"

namespace mu {

struct ManifestEntry {
  std::string path;
  uint32_t crc32 = 0;
  uint64_t size = 0;
  int line_number = 0;
};

struct ManifestParseResult {
  std::vector<ManifestEntry> entries;
  int skipped_lines = 0;
  std::vector<std::string> errors;  ///< Human readable, with line numbers.
};

/// Parses a manifest. Tolerates blank lines and CRLF. Does not throw on a bad
/// line; it records the line number and continues, because a single corrupt
/// line should not hide the other 187 entries.
ManifestParseResult ManifestParse(ByteView text);

enum class VerifyOutcome {
  kMatch,
  kSizeMismatch,
  kCrcMismatch,
  kMissing,
  kUnreadable,
};

const char* VerifyOutcomeName(VerifyOutcome o);

struct VerifyReport {
  std::size_t total = 0;
  std::size_t match = 0;
  std::size_t size_mismatch = 0;
  std::size_t crc_mismatch = 0;
  std::size_t missing = 0;
  std::size_t unreadable = 0;
  /// Capped list of offending entries, so a report cannot grow without bound.
  std::vector<std::string> problems;
  static constexpr std::size_t kMaxProblems = 64;

  bool clean() const {
    return size_mismatch == 0 && crc_mismatch == 0 && missing == 0 &&
           unreadable == 0;
  }
};

/// Verifies every manifest entry against files rooted at |root|.
VerifyReport ManifestVerify(const std::vector<ManifestEntry>& entries,
                            const std::string& root);

}  // namespace mu

#endif  // MU_ASSETS_MANIFEST_HPP
