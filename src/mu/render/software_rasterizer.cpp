// SPDX-License-Identifier: MIT
#include "mu/render/software_rasterizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

#include "mu/assets/ozt.hpp"
#include "mu/fs.hpp"

namespace mu {

// -------------------- Math --------------------
Vec3 Normalize(const Vec3& v) {
  float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
  if (len < 1e-6f) return Vec3{0,0,0};
  return Vec3{v.x/len, v.y/len, v.z/len};
}
float Dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
Vec3 Cross(const Vec3& a, const Vec3& b) {
  return Vec3{
    a.y*b.z - a.z*b.y,
    a.z*b.x - a.x*b.z,
    a.x*b.y - a.y*b.x
  };
}
Vec3 Sub(const Vec3& a, const Vec3& b) { return Vec3{a.x-b.x, a.y-b.y, a.z-b.z}; }
Vec3 Add(const Vec3& a, const Vec3& b) { return Vec3{a.x+b.x, a.y+b.y, a.z+b.z}; }

Mat4 Multiply(const Mat4& a, const Mat4& b) {
  Mat4 r{};
  for (int row=0; row<4; ++row) {
    for (int col=0; col<4; ++col) {
      float sum = 0.0f;
      for (int k=0; k<4; ++k) {
        sum += a.m[row*4 + k] * b.m[k*4 + col];
      }
      r.m[row*4 + col] = sum;
    }
  }
  return r;
}

Vec3 TransformDirection(const Mat4& mat, const Vec3& v) {
  float x = mat.m[0]*v.x + mat.m[1]*v.y + mat.m[2]*v.z;
  float y = mat.m[4]*v.x + mat.m[5]*v.y + mat.m[6]*v.z;
  float z = mat.m[8]*v.x + mat.m[9]*v.y + mat.m[10]*v.z;
  return Vec3{x,y,z};
}

// -------------------- Framebuffer --------------------
void SoftwareFramebuffer::Resize(int w, int h) {
  if (w <= 0 || h <= 0) {
    width = 0;
    height = 0;
    color.clear();
    depth.clear();
    return;
  }

  const std::size_t sw = static_cast<std::size_t>(w);
  const std::size_t sh = static_cast<std::size_t>(h);
  const std::size_t max_size = std::numeric_limits<std::size_t>::max();
  if (sw > max_size / sh || sw * sh > max_size / 3) {
    width = 0;
    height = 0;
    color.clear();
    depth.clear();
    return;
  }

  width = w;
  height = h;
  const std::size_t pixel_count = sw * sh;
  color.assign(pixel_count * 3, 0);
  depth.assign(pixel_count, 1.0f);
}
void SoftwareFramebuffer::Clear(const Vec3& clearColor) {
  uint8_t r = static_cast<uint8_t>(std::clamp(clearColor.x,0.0f,1.0f)*255.0f);
  uint8_t g = static_cast<uint8_t>(std::clamp(clearColor.y,0.0f,1.0f)*255.0f);
  uint8_t b = static_cast<uint8_t>(std::clamp(clearColor.z,0.0f,1.0f)*255.0f);
  for (size_t i=0;i<color.size();i+=3) { color[i]=r; color[i+1]=g; color[i+2]=b; }
  std::fill(depth.begin(), depth.end(), 1.0f);
}
void SoftwareFramebuffer::ClearDepth(float d) {
  std::fill(depth.begin(), depth.end(), d);
}
bool SoftwareFramebuffer::SetPixel(int x, int y, float z, uint8_t r, uint8_t g, uint8_t b) {
  if (x<0 || y<0 || x>=width || y>=height) return false;
  const std::size_t idx = static_cast<std::size_t>(y) *
                              static_cast<std::size_t>(width) +
                          static_cast<std::size_t>(x);
  if (z >= depth[idx]) return false;
  depth[idx] = z;
  const std::size_t cidx = idx * 3;
  color[cidx]=r; color[cidx+1]=g; color[cidx+2]=b;
  return true;
}
bool SoftwareFramebuffer::SavePPM(const std::string& path) const {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  std::fprintf(f, "P6\n%d %d\n255\n", width, height);
  std::fwrite(color.data(), 1, color.size(), f);
  std::fclose(f);
  return true;
}

// -------------------- Texture storage and sampling --------------------
void SoftwareTexture::GenerateCheckerboard(int w, int h, int checkerSize) {
  if (w <= 0 || h <= 0 || checkerSize <= 0) {
    width = 0;
    height = 0;
    bgra.clear();
    valid = false;
    return;
  }

  const std::size_t sw = static_cast<std::size_t>(w);
  const std::size_t sh = static_cast<std::size_t>(h);
  const std::size_t max_size = std::numeric_limits<std::size_t>::max();
  if (sw > max_size / sh || sw * sh > max_size / 4) {
    width = 0;
    height = 0;
    bgra.clear();
    valid = false;
    return;
  }

  width = w;
  height = h;
  bgra.resize(sw * sh * 4);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int cx = (x / checkerSize) % 2;
      const int cy = (y / checkerSize) % 2;
      const bool white = (cx ^ cy) == 0;
      const uint8_t c = white ? 255 : 30;
      const std::size_t pixel = static_cast<std::size_t>(y) * sw +
                                static_cast<std::size_t>(x);
      const std::size_t idx = pixel * 4;
      bgra[idx] = c;
      bgra[idx + 1] = c;
      bgra[idx + 2] = c;
      bgra[idx + 3] = 255;
    }
  }
  valid = true;
}

Status SoftwareTexture::LoadFromOZT(const std::string& oztPath) {
  width = 0;
  height = 0;
  bgra.clear();
  valid = false;

  std::vector<uint8_t> fileData;
  if (!ReadFile(oztPath, &fileData))
    return Status::Make(Status::kIoError, "could not read OZT file");
  return LoadFromOZT(ByteView(fileData.data(), fileData.size()));
}

Status SoftwareTexture::LoadFromOZT(ByteView file) {
  width = 0;
  height = 0;
  bgra.clear();
  valid = false;

  OztImage image;
  const Status readStatus = OztRead(file, &image);
  if (!readStatus) return readStatus;

  std::vector<uint8_t> decoded;
  if (!OztToBgraTopDown(image, &decoded))
    return Status::Make(Status::kCorrupt,
                        "OZT pixels could not be converted to BGRA8");

  width = static_cast<int>(image.width);
  height = static_cast<int>(image.height);
  bgra.swap(decoded);
  valid = true;
  return Status::Ok();
}

void SoftwareTexture::Sample(float u, float v, uint8_t& outR, uint8_t& outG, uint8_t& outB) const {
  if (!valid || bgra.empty() || width <= 0 || height <= 0) {
    outR = outG = outB = 255;
    return;
  }
  const std::size_t sw = static_cast<std::size_t>(width);
  const std::size_t sh = static_cast<std::size_t>(height);
  const std::size_t max_size = std::numeric_limits<std::size_t>::max();
  if (sw > max_size / sh || sw * sh > max_size / 4 ||
      bgra.size() < sw * sh * 4) {
    outR = outG = outB = 255;
    return;
  }
  // Clamp u,v 0..1 (repeat)
  // Use repeat wrapping: fract
  float fu = u - std::floor(u);
  float fv = v - std::floor(v);
  const float width_f = static_cast<float>(width);
  const float height_f = static_cast<float>(height);
  int x = static_cast<int>(fu * width_f) % width;
  int y = static_cast<int>(fv * height_f) % height;
  if (x < 0) x += width;
  if (y < 0) y += height;
  if (x >= width) x = width - 1;
  if (y >= height) y = height - 1;
  const std::size_t pixel = static_cast<std::size_t>(y) *
                                static_cast<std::size_t>(width) +
                            static_cast<std::size_t>(x);
  const std::size_t idx = pixel * 4;
  // BGRA -> RGB
  outB = bgra[idx];
  outG = bgra[idx+1];
  outR = bgra[idx+2];
}

// -------------------- Camera --------------------
Mat4 Camera::GetViewMatrix() const {
  Vec3 f = Normalize(Sub(target, eye));
  Vec3 s = Normalize(Cross(f, up));
  Vec3 u = Normalize(Cross(s, f));
  Mat4 view{};
  view.m[0]=s.x; view.m[1]=s.y; view.m[2]=s.z; view.m[3]= -Dot(s, eye);
  view.m[4]=u.x; view.m[5]=u.y; view.m[6]=u.z; view.m[7]= -Dot(u, eye);
  view.m[8]=-f.x; view.m[9]=-f.y; view.m[10]=-f.z; view.m[11]= Dot(f, eye);
  view.m[12]=0; view.m[13]=0; view.m[14]=0; view.m[15]=1;
  return view;
}
Mat4 Camera::GetProjectionMatrix() const {
  float fov_rad = fov_deg * 3.14159265358979323846f / 180.0f;
  float f = 1.0f / std::tan(fov_rad * 0.5f);
  float nf = 1.0f / (near_plane - far_plane);
  Mat4 p{};
  p.m[0]= f / aspect; p.m[1]=0; p.m[2]=0; p.m[3]=0;
  p.m[4]=0; p.m[5]= f; p.m[6]=0; p.m[7]=0;
  p.m[8]=0; p.m[9]=0; p.m[10]= (far_plane + near_plane) * nf; p.m[11]= (2.0f * far_plane * near_plane) * nf;
  p.m[12]=0; p.m[13]=0; p.m[14]= -1.0f; p.m[15]=0;
  return p;
}
Mat4 Camera::GetViewProjectionMatrix() const {
  Mat4 v = GetViewMatrix();
  Mat4 p = GetProjectionMatrix();
  return Multiply(p, v);
}
Frustum Camera::GetFrustum() const {
  Mat4 vp = GetViewProjectionMatrix();
  float colMajor[16];
  for (int row=0; row<4; ++row) for (int col=0; col<4; ++col) colMajor[col*4+row] = vp.m[row*4+col];
  return FrustumFromViewProjection(colMajor);
}

// -------------------- Rasterizer --------------------
SoftwareRasterizer::SoftwareRasterizer(int w, int h) : fb_(w,h) {}
void SoftwareRasterizer::Resize(int w, int h) { fb_.Resize(w,h); }
void SoftwareRasterizer::Clear(const Vec3& clearColor) { fb_.Clear(clearColor); }
bool SoftwareRasterizer::SavePPM(const std::string& path) const { return fb_.SavePPM(path); }

namespace {
struct VertexOut {
  float x, y;
  float z;
  float w;
  float intensity;
  // PLACEHOLDER UV — ciclo16 D — NÃO é UV real, é planar baseado em posição vértice
  float u, v;
  bool clipped = false;
};

inline float EdgeFunc(float ax, float ay, float bx, float by, float cx, float cy) {
  return (cx - ax)*(by - ay) - (cy - ay)*(bx - ax);
}
} // anon

RenderStats SoftwareRasterizer::RenderMesh(const Mesh& mesh,
                                           const Skeleton* skeleton,
                                           const std::vector<Mat4>* boneTransforms,
                                           const Camera& cam,
                                           const Vec3& lightDir) {
  Mesh skinnedMesh = mesh;
  if (mesh.has_bone_weights && skeleton && boneTransforms) {
    SkinnedResult sr = ApplySkinningLBS(mesh, *skeleton, *boneTransforms);
    if (sr.ok) {
      skinnedMesh.positions = sr.positions;
    }
  }
  std::vector<Mesh> meshes = {skinnedMesh};
  std::vector<Vec3> offsets = {{0,0,0}};
  return RenderMeshes(meshes, offsets, cam, lightDir);
}

RenderStats SoftwareRasterizer::RenderMeshes(const std::vector<Mesh>& meshes,
                                             const std::vector<Vec3>& worldOffsets,
                                             const Camera& cam,
                                             const Vec3& lightDir) {
  RenderStats stats;
  if (meshes.empty()) { stats.problem="no meshes"; return stats; }
  Mat4 view = cam.GetViewMatrix();
  Mat4 proj = cam.GetProjectionMatrix();
  Mat4 vp = Multiply(proj, view);
  float colMajor[16];
  for (int r=0;r<4;++r) for (int c=0;c<4;++c) colMajor[c*4+r]=vp.m[r*4+c];
  Frustum frustum = FrustumFromViewProjection(colMajor);
  Vec3 lightN = Normalize(lightDir);

  for (std::size_t mi = 0; mi < meshes.size(); ++mi) {
    const Mesh& mesh = meshes[mi];
    if (mesh.has_texcoords && mesh.texcoords.size() != mesh.positions.size()) {
      stats.problem = "texcoords size mismatch despite has_texcoords true";
      return stats;
    }
    Vec3 offset = {0, 0, 0};
    if (mi < worldOffsets.size()) offset = worldOffsets[mi];
    if (mesh.positions.empty() || mesh.indices.empty()) continue;
    if (mesh.bounds.valid) {
      Bounds wb = mesh.bounds;
      wb.min.x += offset.x; wb.min.y += offset.y; wb.min.z += offset.z;
      wb.max.x += offset.x; wb.max.y += offset.y; wb.max.z += offset.z;
      CullResult cr = CullBox(frustum, wb);
      if (cr == CullResult::kOutside) {
        stats.triangles_culled_frustum += mesh.indices.size()/3;
        continue;
      }
    }
    size_t triCount = mesh.indices.size()/3;
    stats.triangles_total += triCount;

    // Precompute bounds for placeholder UV planar mapping — ciclo16 D
    // PLACEHOLDER: usa posição vértice para gerar UV, NÃO é UV real
    Vec3 bmin{0,0,0}, bmax{1,1,1};
    bool hasBounds = mesh.bounds.valid;
    if(hasBounds){
      bmin = mesh.bounds.min;
      bmax = mesh.bounds.max;
    } else if(!mesh.positions.empty()){
      bmin = bmax = mesh.positions[0];
      for(auto &p: mesh.positions){
        if(p.x<bmin.x) bmin.x=p.x;
        if(p.y<bmin.y) bmin.y=p.y;
        if(p.z<bmin.z) bmin.z=p.z;
        if(p.x>bmax.x) bmax.x=p.x;
        if(p.y>bmax.y) bmax.y=p.y;
        if(p.z>bmax.z) bmax.z=p.z;
      }
    }
    float dx = bmax.x - bmin.x;
    float dy = bmax.y - bmin.y;
    float dz = bmax.z - bmin.z;
    if(std::fabs(dx)<1e-6f) dx=1.0f;
    if(std::fabs(dy)<1e-6f) dy=1.0f;
    if(std::fabs(dz)<1e-6f) dz=1.0f;

    std::vector<VertexOut> vout(mesh.positions.size());
    for (size_t vi=0; vi<mesh.positions.size(); ++vi) {
      Vec3 pos = mesh.positions[vi];
      Vec3 posForUV = pos; // save for placeholder UV before offset
      pos.x += offset.x; pos.y += offset.y; pos.z += offset.z;
      float x = vp.m[0]*pos.x + vp.m[1]*pos.y + vp.m[2]*pos.z + vp.m[3];
      float y = vp.m[4]*pos.x + vp.m[5]*pos.y + vp.m[6]*pos.z + vp.m[7];
      float z = vp.m[8]*pos.x + vp.m[9]*pos.y + vp.m[10]*pos.z + vp.m[11];
      float w = vp.m[12]*pos.x + vp.m[13]*pos.y + vp.m[14]*pos.z + vp.m[15];
      if (w <= 0.001f) { vout[vi].clipped = true; continue; }
      float ndc_x = x / w;
      float ndc_y = y / w;
      float ndc_z = z / w;
      float sx = (ndc_x * 0.5f + 0.5f) *
                 static_cast<float>(fb_.width);
      float sy = (1.0f - (ndc_y * 0.5f + 0.5f)) *
                 static_cast<float>(fb_.height);
      float depth = ndc_z * 0.5f + 0.5f;
      float intensity = 0.5f;
      if (vi < mesh.normals.size()) {
        Vec3 n = mesh.normals[vi];
        Vec3 n_view = TransformDirection(view, n);
        n_view = Normalize(n_view);
        float d = Dot(n_view, lightN);
        if (d < 0) d = 0;
        intensity = d * 0.8f + 0.2f;
      }
      float u = 0.0f;
      float v = 0.0f;
      if (mesh.has_texcoords) {
        u = mesh.texcoords[vi].u;
        v = mesh.texcoords[vi].v;
      } else {
        // Explicit diagnostic fallback only: planar coordinates derived from
        // position. This is not the BMD corner-to-UV mapping.
        u = (posForUV.x - bmin.x) / dx;
        v = (posForUV.y - bmin.y) / dy;
        if ((bmax.y - bmin.y) < (bmax.z - bmin.z) * 0.5f)
          v = (posForUV.z - bmin.z) / dz;
        u *= 4.0f;
        v *= 4.0f;
      }

      vout[vi].x = sx;
      vout[vi].y = sy;
      vout[vi].z = depth;
      vout[vi].w = w;
      vout[vi].intensity = intensity;
      vout[vi].u = u;
      vout[vi].v = v;
      vout[vi].clipped = false;
    }

    for (size_t ti=0; ti+2 < mesh.indices.size(); ti+=3) {
      uint32_t i0 = mesh.indices[ti];
      uint32_t i1 = mesh.indices[ti+1];
      uint32_t i2 = mesh.indices[ti+2];
      if (i0 >= vout.size() || i1 >= vout.size() || i2 >= vout.size()) continue;
      const VertexOut& v0 = vout[i0];
      const VertexOut& v1 = vout[i1];
      const VertexOut& v2 = vout[i2];
      if (v0.clipped || v1.clipped || v2.clipped) continue;
      float area = EdgeFunc(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
      if (std::fabs(area) < 1e-6f) continue;
      float minX = std::min({v0.x, v1.x, v2.x});
      float maxX = std::max({v0.x, v1.x, v2.x});
      float minY = std::min({v0.y, v1.y, v2.y});
      float maxY = std::max({v0.y, v1.y, v2.y});
      int x0 = std::max(0, static_cast<int>(std::floor(minX)));
      int x1 = std::min(fb_.width-1, static_cast<int>(std::ceil(maxX)));
      int y0 = std::max(0, static_cast<int>(std::floor(minY)));
      int y1 = std::min(fb_.height-1, static_cast<int>(std::ceil(maxY)));
      if (x0 > x1 || y0 > y1) continue;
      for (int y=y0; y<=y1; ++y) {
        for (int x=x0; x<=x1; ++x) {
          const float px = static_cast<float>(x) + 0.5f;
          const float py = static_cast<float>(y) + 0.5f;
          float w0 = EdgeFunc(v1.x, v1.y, v2.x, v2.y, px, py);
          float w1 = EdgeFunc(v2.x, v2.y, v0.x, v0.y, px, py);
          float w2 = EdgeFunc(v0.x, v0.y, v1.x, v1.y, px, py);
          if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
            w0 /= area; w1 /= area; w2 /= area;
            float invW = w0 / v0.w + w1 / v1.w + w2 / v2.w;
            if (std::fabs(invW) < 1e-6f) continue;
            float z = (w0 * v0.z / v0.w + w1 * v1.z / v1.w + w2 * v2.z / v2.w) / invW;
            float intensity = (w0 * v0.intensity / v0.w + w1 * v1.intensity / v1.w + w2 * v2.intensity / v2.w) / invW;
            // PLACEHOLDER UV perspective correct — ciclo16 D
            float u = (w0 * v0.u / v0.w + w1 * v1.u / v1.w + w2 * v2.u / v2.w) / invW;
            float v = (w0 * v0.v / v0.w + w1 * v1.v / v1.w + w2 * v2.v / v2.w) / invW;
            if (z < 0) z = 0;
            if (z > 1) z = 1;
            if (intensity < 0) intensity = 0;
            if (intensity > 1) intensity = 1;
            uint8_t r,g,b;
            if(hasTexture_ && texture_.valid){
              uint8_t tr,tg,tb;
              texture_.Sample(u, v, tr, tg, tb);
              // Modulate with intensity (lambert)
              r = static_cast<uint8_t>(tr * intensity);
              g = static_cast<uint8_t>(tg * intensity);
              b = static_cast<uint8_t>(tb * intensity);
            } else {
              uint8_t c = static_cast<uint8_t>(intensity * 255.0f);
              r=g=b=c;
            }
            if (fb_.SetPixel(x, y, z, r, g, b)) {
              stats.pixels_written++;
            }
          }
        }
      }
      stats.triangles_rasterized++;
    }
  }
  stats.ok = true;
  return stats;
}

} // namespace mu
