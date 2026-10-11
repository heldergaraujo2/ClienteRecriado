// SPDX-License-Identifier: MIT
// BMD v0x0A body parsing. See bmd_body.hpp for the layout and its evidence.
#include "mu/assets/bmd_body.hpp"

#include <cmath>
#include <cstring>

namespace mu {

namespace {

/// Reads the NUL-terminated name from the fixed 32-byte field.
///
/// The field is only meaningful when it holds printable ASCII. 932 of the 1,465
/// clear files do not begin with printable bytes at all, so an empty or
/// non-printable field is treated as "no name" rather than as an error: the
/// geometry that follows is still valid in those files.
bool ReadSourceName(ByteView payload, std::string* out) {
  if (payload.size < kBmdNameSize) return false;
  std::size_t end = 0;
  while (end < kBmdNameSize && payload.data[end] != 0) ++end;
  if (end == 0) return false;
  for (std::size_t i = 0; i < end; ++i) {
    const unsigned char c = payload.data[i];
    if (c < 32 || c > 126) return false;
  }
  out->assign(reinterpret_cast<const char*>(payload.data), end);
  return true;
}

bool IsFiniteFloat(float v) { return std::isfinite(v); }

}  // namespace

Status BmdParseBodyV0A(ByteView payload, BmdBody* out) {
  if (out == nullptr) {
    return Status::Make(Status::kCorrupt, "null output");
  }
  *out = BmdBody{};

  // The header counts sit at 38 and 40, so anything shorter cannot be parsed.
  if (payload.size < kBmdVertexArrayOffset) {
    out->problem = "payload shorter than the 52-byte body header";
    return Status::Make(Status::kTruncated, out->problem);
  }

  out->has_source_name = ReadSourceName(payload, &out->source_name);

  const std::size_t vertex_count = ReadU16LE(payload.data + kBmdVertexCountOffset);
  const std::size_t normal_count = ReadU16LE(payload.data + kBmdNormalCountOffset);

  // Both arrays must fit. This is the check that makes the verified arithmetic
  // load-bearing: 52 + vc*16 is exactly where the normals start, and the
  // normals occupy nc*20 bytes after that.
  const std::size_t vertex_bytes = vertex_count * kBmdVertexRecordSize;
  const std::size_t normal_bytes = normal_count * kBmdNormalRecordSize;
  const std::size_t normals_offset = kBmdVertexArrayOffset + vertex_bytes;
  const std::size_t required = normals_offset + normal_bytes;

  if (required < normals_offset || required > payload.size) {
    out->problem = "declared vertex/normal counts exceed the payload";
    return Status::Make(Status::kTruncated, out->problem);
  }

  out->vertices.reserve(vertex_count);
  for (std::size_t i = 0; i < vertex_count; ++i) {
    const uint8_t* p = payload.data + kBmdVertexArrayOffset +
                       i * kBmdVertexRecordSize;
    BmdVertex v;
    v.x = ReadF32LE(p);
    v.y = ReadF32LE(p + 4);
    v.z = ReadF32LE(p + 8);
    v.aux0 = ReadU16LE(p + 12);
    v.aux1 = ReadU16LE(p + 14);
    if (!IsFiniteFloat(v.x) || !IsFiniteFloat(v.y) || !IsFiniteFloat(v.z)) {
      out->problem = "non-finite vertex coordinate";
      return Status::Make(Status::kCorrupt, out->problem);
    }
    out->vertices.push_back(v);
  }

  out->normals.reserve(normal_count);
  for (std::size_t i = 0; i < normal_count; ++i) {
    const uint8_t* p = payload.data + normals_offset + i * kBmdNormalRecordSize;
    BmdNormal n;
    n.nx = ReadF32LE(p);
    n.ny = ReadF32LE(p + 4);
    n.nz = ReadF32LE(p + 8);
    n.aux0 = ReadF32LE(p + 12);
    n.aux1 = ReadF32LE(p + 16);
    if (!IsFiniteFloat(n.nx) || !IsFiniteFloat(n.ny) || !IsFiniteFloat(n.nz)) {
      out->problem = "non-finite normal component";
      return Status::Make(Status::kCorrupt, out->problem);
    }
    out->normals.push_back(n);
  }

  if (out->vertices.size() != vertex_count ||
      out->normals.size() != normal_count) {
    out->problem = "array size does not match the header count";
    return Status::Make(Status::kCorrupt, out->problem);
  }

  out->complete = true;
  return Status::Ok();
}

}  // namespace mu
