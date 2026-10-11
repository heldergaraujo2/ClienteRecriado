// SPDX-License-Identifier: MIT
#include "mu/assets/bmd_report.hpp"

#include <cstring>

#include "mu/assets/bmd_body.hpp"

namespace mu {

const char* PayloadAccessName(PayloadAccess a) {
  switch (a) {
    case PayloadAccess::kOpaque: return "OPAQUE";
    case PayloadAccess::kBuxDecryptable: return "BUX_DECRYPTABLE";
    case PayloadAccess::kPlaintext: return "PLAINTEXT";
  }
  return "OPAQUE";
}

namespace {

/// Decides whether the payload bytes are obtainable in the clear, and how.
///
/// Order matters. The bux gain is checked before the entropy verdict, because
/// a bux-encrypted payload has high entropy by construction and would
/// otherwise be classified as opaque even though we can decrypt it.
PayloadAccess ClassifyAccess(ByteView payload, double entropy, double* gain) {
  const double g = BuxDecryptGain(payload);
  *gain = g;
  if (g > 0.05) return PayloadAccess::kBuxDecryptable;
  if (entropy < kEncryptedEntropyThreshold) return PayloadAccess::kPlaintext;
  return PayloadAccess::kOpaque;
}

BmdFileReport Classify(ByteView data, const std::string& label) {
  BmdFileReport r;
  r.path = label;
  r.size = data.size;

  BmdInfo info;
  Status s = BmdProbe(data, &info);
  if (!s) {
    r.kind = BmdKind::kUnknown;
    r.structure_valid = false;
    r.problem = s.message;
    return r;
  }

  r.kind = info.kind;
  r.version = info.version;
  r.payload_entropy = info.payload_entropy;

  // Level 1: structure. For v0x0C the declared length must agree with the file
  // length; for the other kinds there is no length field to disagree with, so
  // recognising the container is the whole of level 1.
  r.structure_valid = info.length_consistent;
  if (!r.structure_valid)
    r.problem = "declared payload length does not match file length";

  // Levels 2 and 3.
  const ByteView payload = data.sub(info.payload_offset, info.payload_size);
  double gain = 0.0;
  r.access = ClassifyAccess(payload, info.payload_entropy, &gain);
  r.bux_gain = gain;

  // Level 3: geometry. Attempted only where the layout is verified and the
  // payload is in the clear. v0x0C is skipped because its payload is
  // encrypted; parsing it would produce noise, not vertices.
  //
  // The counts recorded are the parser's own tally of what it extracted, not
  // the header's declared counts, so a file whose header lies cannot be
  // counted as parsed.
  const bool body_readable =
      r.structure_valid && r.access != PayloadAccess::kOpaque &&
      r.kind == BmdKind::kPlainV0A;
  if (body_readable) {
    BmdBody body;
    const Status bs = BmdParseBodyV0A(payload, &body);
    if (bs && body.complete && !body.vertices.empty()) {
      r.geometry_parsed = true;
      r.vertex_count = body.vertices.size();
      r.normal_count = body.normals.size();
    } else {
      r.body_problem = body.problem.empty() ? bs.message : body.problem;
    }
  }
  return r;
}

}  // namespace

BmdFileReport BmdClassifyBytes(ByteView data, const std::string& label) {
  return Classify(data, label);
}

BmdFileReport BmdClassifyFile(const std::string& path) {
  BmdFileReport r;
  r.path = path;

  std::vector<uint8_t> buf;
  if (!ReadFile(path, &buf)) {
    r.kind = BmdKind::kUnknown;
    r.structure_valid = false;
    r.problem = "could not read file";
    return r;
  }
  r.size = buf.size();
  return Classify(ByteView(buf.data(), buf.size()), path);
}

void BmdAccumulate(const BmdFileReport& r, BmdSummary* s) {
  if (!s) return;
  ++s->total;

  switch (r.kind) {
    case BmdKind::kPlainV0C: ++s->kind_v0c; break;
    case BmdKind::kPlainV0A: ++s->kind_v0a; break;
    case BmdKind::kPlainOtherVer: ++s->kind_other_version; break;
    case BmdKind::kBuxContainer: ++s->kind_bux_container; break;
    case BmdKind::kUnknown: ++s->kind_unknown; break;
  }

  if (r.structure_valid) ++s->structure_valid;
  else ++s->structure_invalid;

  switch (r.access) {
    case PayloadAccess::kPlaintext: ++s->cleartext_plaintext; break;
    case PayloadAccess::kBuxDecryptable: ++s->cleartext_bux; break;
    case PayloadAccess::kOpaque: ++s->opaque; break;
  }

  if (r.geometry_parsed) ++s->geometry_parsed;

  if (r.payload_entropy > s->max_entropy) s->max_entropy = r.payload_entropy;
  if (r.payload_entropy > kEncryptedEntropyThreshold) ++s->entropy_above_7_5;
}

double PercentOf(std::size_t part, std::size_t whole) {
  if (whole == 0) return 0.0;
  return 100.0 * static_cast<double>(part) / static_cast<double>(whole);
}

}  // namespace mu
