// SPDX-License-Identifier: MIT
#include "mu/assets/manifest.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace mu {

const char* VerifyOutcomeName(VerifyOutcome o) {
  switch (o) {
    case VerifyOutcome::kMatch: return "MATCH";
    case VerifyOutcome::kSizeMismatch: return "SIZE_MISMATCH";
    case VerifyOutcome::kCrcMismatch: return "CRC_MISMATCH";
    case VerifyOutcome::kMissing: return "MISSING";
    case VerifyOutcome::kUnreadable: return "UNREADABLE";
  }
  return "UNKNOWN";
}

namespace {

bool HexToU32(const char* s, std::size_t len, uint32_t* out) {
  if (len != 8) return false;
  uint32_t v = 0;
  for (std::size_t i = 0; i < 8; ++i) {
    const char c = s[i];
    uint32_t d;
    if (c >= '0' && c <= '9') d = static_cast<uint32_t>(c - '0');
    else if (c >= 'a' && c <= 'f') d = static_cast<uint32_t>(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F') d = static_cast<uint32_t>(c - 'A' + 10);
    else return false;
    v = (v << 4) | d;
  }
  *out = v;
  return true;
}

bool AllDigits(const char* s, std::size_t len) {
  if (len == 0) return false;
  for (std::size_t i = 0; i < len; ++i)
    if (s[i] < '0' || s[i] > '9') return false;
  return true;
}

struct Field {
  std::size_t start = 0;
  std::size_t end = 0;
  std::size_t len() const { return end - start; }
};

}  // namespace

ManifestParseResult ManifestParse(ByteView text) {
  ManifestParseResult r;
  if (text.empty()) return r;

  std::size_t pos = 0;
  int line_no = 0;

  while (pos < text.size) {
    ++line_no;
    std::size_t eol = pos;
    while (eol < text.size && text.data[eol] != '\n') ++eol;

    const char* line = reinterpret_cast<const char*>(text.data + pos);
    std::size_t len = eol - pos;
    if (len > 0 && line[len - 1] == '\r') --len;  // tolerate CRLF

    std::size_t b = 0;
    while (b < len && (line[b] == ' ' || line[b] == '\t')) ++b;
    std::size_t e = len;
    while (e > b && (line[e - 1] == ' ' || line[e - 1] == '\t')) --e;

    if (e > b) {
      // Tokenise. The reference client ships asset paths containing spaces, so
      // the path cannot simply be "field 0". Instead the last two fields are
      // always the checksum and the size, and everything before them is the
      // path, rejoined with single spaces.
      std::vector<Field> fields;
      std::size_t i = b;
      while (i < e) {
        while (i < e && (line[i] == ' ' || line[i] == '\t')) ++i;
        if (i >= e) break;
        Field f;
        f.start = i;
        while (i < e && line[i] != ' ' && line[i] != '\t') ++i;
        f.end = i;
        fields.push_back(f);
      }

      bool ok = fields.size() >= 3;
      uint32_t crc = 0;
      uint64_t size = 0;

      if (ok) {
        const Field& crc_field = fields[fields.size() - 2];
        const Field& size_field = fields[fields.size() - 1];
        if (!HexToU32(line + crc_field.start, crc_field.len(), &crc)) ok = false;
        if (ok && !AllDigits(line + size_field.start, size_field.len()))
          ok = false;
        if (ok) {
          // strtoull would silently saturate on a 30-digit string, so reject
          // anything that cannot fit a 64-bit byte count first.
          if (size_field.len() > 20) {
            ok = false;
          } else {
            char buf[24];
            std::memcpy(buf, line + size_field.start, size_field.len());
            buf[size_field.len()] = '\0';
            size = std::strtoull(buf, nullptr, 10);
          }
        }
      }

      if (ok) {
        ManifestEntry ent;
        const std::size_t path_end = fields[fields.size() - 3].end;
        ent.path.assign(line + b, path_end - b);
        ent.crc32 = crc;
        ent.size = size;
        ent.line_number = line_no;
        r.entries.push_back(std::move(ent));
      } else {
        ++r.skipped_lines;
        r.errors.push_back("line " + std::to_string(line_no) +
                           ": expected '<path> <crc32hex> <size>'");
      }
    }

    pos = (eol < text.size) ? eol + 1 : text.size;
  }
  return r;
}

VerifyReport ManifestVerify(const std::vector<ManifestEntry>& entries,
                            const std::string& root) {
  VerifyReport rep;
  rep.total = entries.size();

  auto add_problem = [&](const char* kind, const ManifestEntry& e) {
    if (rep.problems.size() < VerifyReport::kMaxProblems)
      rep.problems.push_back(std::string(kind) + "  " + e.path);
  };

  for (const ManifestEntry& e : entries) {
    std::string full = root;
    if (!full.empty() && full.back() != '/' && full.back() != '\\') full += '/';
    full += e.path;

    std::vector<uint8_t> buf;
    if (!ReadFile(full, &buf)) {
      // Distinguish "not there" from "there but unreadable" so a permissions
      // problem is not silently reported as a missing patch file.
      FILE* probe = std::fopen(full.c_str(), "rb");
      if (probe) {
        std::fclose(probe);
        ++rep.unreadable;
        add_problem("UNREADABLE", e);
      } else {
        ++rep.missing;
        add_problem("MISSING", e);
      }
      continue;
    }
    if (buf.size() != e.size) {
      ++rep.size_mismatch;
      add_problem("SIZE_MISMATCH", e);
      continue;
    }
    if (Crc32(buf.data(), buf.size()) != e.crc32) {
      ++rep.crc_mismatch;
      add_problem("CRC_MISMATCH", e);
      continue;
    }
    ++rep.match;
  }
  return rep;
}

}  // namespace mu
