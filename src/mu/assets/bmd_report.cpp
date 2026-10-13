// SPDX-License-Identifier: MIT
#include "mu/assets/bmd_report.hpp"

#include <vector>

#include "mu/assets/bmd_body.hpp"

namespace mu {

const char* PayloadAccessName(PayloadAccess a) {
  switch (a) {
    case PayloadAccess::kOpaque: return "OPAQUE";
    case PayloadAccess::kBuxDecryptable: return "BUX_DECRYPTABLE";
    case PayloadAccess::kV0CDecryptable: return "V0C_DECRYPTABLE";
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
/// True when the 32-byte name field holds printable ASCII or NUL. Used as the
/// plausibility check on a v0x0C decryption: a wrong key yields high-entropy
/// bytes here, not a path or a filename.
bool PayloadNameIsPrintable(ByteView payload) {
  if (payload.size < kBmdNameSize) return false;
  for (std::size_t i = 0; i < kBmdNameSize; ++i) {
    const uint8_t c = payload.data[i];
    if (c != 0 && (c < 32 || c > 126)) return false;
  }
  return true;
}

PayloadAccess ClassifyAccess(ByteView payload, double entropy, double* gain,
                             bool v0c_decrypted) {
  // Checked first: the v0x0C cipher was recovered, so a v0x0C container whose
  // payload decrypted is in the clear regardless of what its entropy said while
  // it was still ciphertext. Entropy is the wrong test for a payload this
  // project can now decrypt.
  if (v0c_decrypted) return PayloadAccess::kV0CDecryptable;
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
  const ByteView cipher_payload = data.sub(info.payload_offset, info.payload_size);

  // For v0x0C the payload is decrypted here rather than left as ciphertext.
  // The cipher is a rolling XOR fed from the previous ciphertext byte, so it is
  // not self-inverse and the work cannot be deferred to the caller. The copy is
  // unavoidable for that reason; it is bounded by one model file at a time.
  //
  // Decryption is only trusted when it produces a plausible result: the 32-byte
  // name field must be printable ASCII. Without that check a wrong key would
  // silently reclassify garbage as decrypted.
  std::vector<uint8_t> plain_storage;
  ByteView payload = cipher_payload;
  bool v0c_decrypted = false;
  if (r.structure_valid && r.kind == BmdKind::kPlainV0C) {
    plain_storage.assign(cipher_payload.data, cipher_payload.data + cipher_payload.size);
    BmdV0CDecrypt(plain_storage.data(), plain_storage.size());
    if (PayloadNameIsPrintable(ByteView(plain_storage.data(), plain_storage.size()))) {
      v0c_decrypted = true;
      payload = ByteView(plain_storage.data(), plain_storage.size());
    } else {
      plain_storage.clear();
    }
  }

  double gain = 0.0;
  r.access = ClassifyAccess(payload, info.payload_entropy, &gain, v0c_decrypted);
  r.bux_gain = gain;

  // Level 3: geometry. Attempted where the payload is in the clear and the
  // layout is verified.
  //
  // The counts recorded are the parser's own tally of what it extracted, not
  // the header's declared counts, so a file whose header lies cannot be
  // counted as parsed.
  const bool body_readable =
      r.structure_valid && r.access != PayloadAccess::kOpaque &&
      (r.kind == BmdKind::kPlainV0A || v0c_decrypted);
  if (body_readable) {
    BmdBody body;
    const Status bs = BmdParseBodyV0A(payload, &body);
    if (bs && body.complete && !body.vertices.empty()) {
      r.geometry_parsed = true;
      r.vertex_count = body.vertices.size();
      r.normal_count = body.normals.size();
      r.triangle_count = body.triangles.size();
      r.out_of_range_vertex_refs = body.out_of_range_vertices;
      r.out_of_range_normal_refs = body.out_of_range_normals;
      r.topology_skip = body.topology_skip;
      r.declared_mesh_count = body.declared_mesh_count;
      r.declared_triangle_count = body.declared_triangle_count;
      r.declared_bone_count = body.declared_bone_count;
      r.declared_action_count = body.declared_action_count;
      if (body.has_bones) {
        r.has_bones = true;
        r.bone_count = body.bones.size();
      }
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
    case PayloadAccess::kV0CDecryptable: ++s->cleartext_v0c; break;
    case PayloadAccess::kOpaque: ++s->opaque; break;
  }

  if (r.geometry_parsed) {
    ++s->geometry_parsed;
    switch (r.topology_skip) {
      case BmdTopologySkip::kNone: break;
      case BmdTopologySkip::kMultiMesh: ++s->skip_multi_mesh; break;
      case BmdTopologySkip::kZeroFaces: ++s->skip_zero_faces; break;
      case BmdTopologySkip::kDoesNotFit: ++s->skip_does_not_fit; break;
      case BmdTopologySkip::kNoHeader: ++s->skip_no_header; break;
    }
    if (r.declared_mesh_count < 16) s->mesh_count_hist[r.declared_mesh_count]++;
    else ++s->mesh_count_above_15;
  }
  // Level 4 is a STRICT SUBSET of level 3, by construction: |triangle_count|
  // is only ever assigned inside the |geometry_parsed| branch above and stays
  // 0 otherwise, so a file cannot be counted here without also being counted
  // there. TestTopologyIsSubsetOfGeometry asserts it over the whole tree so the
  // relation is verified, not just asserted in a comment.
  if (r.geometry_parsed && r.triangle_count > 0) {
    ++s->topology_parsed;
    s->triangles_total += r.triangle_count;
    s->out_of_range_vertex_refs += r.out_of_range_vertex_refs;
    s->out_of_range_normal_refs += r.out_of_range_normal_refs;
  }
  // Level 5 - bones is a subset of level 3 as well.
  if (r.geometry_parsed && r.has_bones) {
    ++s->bones_parsed;
    s->total_bones += r.bone_count;
  }

  if (r.payload_entropy > s->max_entropy) s->max_entropy = r.payload_entropy;
  if (r.payload_entropy > kEncryptedEntropyThreshold) ++s->entropy_above_7_5;
}

double PercentOf(std::size_t part, std::size_t whole) {
  if (whole == 0) return 0.0;
  return 100.0 * static_cast<double>(part) / static_cast<double>(whole);
}

}  // namespace mu
