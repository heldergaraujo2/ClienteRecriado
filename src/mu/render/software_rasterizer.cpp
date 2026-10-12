// SPDX-License-Identifier: MIT
#include "mu/render/software_rasterizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

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
  // w=0, ignore translation column (col 3)
  float x = mat.m[0]*v.x + mat.m[1]*v.y + mat.m[2]*v.z;
  float y = mat.m[4]*v.x + mat.m[5]*v.y + mat.m[6]*v.z;
  float z = mat.m[8]*v.x + mat.m[9]*v.y + mat.m[10]*v.z;
  return Vec3{x,y,z};
}

// -------------------- Framebuffer --------------------
void SoftwareFramebuffer::Resize(int w, int h) {
  width = w;
  height = h;
  color.assign(static_cast<size_t>(w)*h*3, 0);
  depth.assign(static_cast<size_t>(w)*h, 1.0f);
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
  size_t idx = static_cast<size_t>(y)*width + x;
  if (z >= depth[idx]) return false; // farther or equal, keep closer
  // depth test passes
  depth[idx] = z;
  size_t cidx = idx*3;
  color[cidx]=r; color[cidx+1]=g; color[cidx+2]=b;
  return true;
}
bool SoftwareFramebuffer::SavePPM(const std::string& path) const {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  // P6 binary
  std::fprintf(f, "P6\n%d %d\n255\n", width, height);
  std::fwrite(color.data(), 1, color.size(), f);
  std::fclose(f);
  return true;
}

// -------------------- Camera --------------------
Mat4 Camera::GetViewMatrix() const {
  Vec3 f = Normalize(Sub(target, eye)); // forward
  Vec3 s = Normalize(Cross(f, up)); // right
  Vec3 u = Cross(s, f); // up corrected, already normalized if s,f orthonormal, but normalize
  u = Normalize(u);
  // Recompute s to be orthonormal: s = cross(f,u)?? Actually we have s = cross(f,up) then u = cross(s,f) gives orthonormal.
  // For safety recompute s = cross(f,u)?? Let's keep as above: f forward, s right, u up.
  // Actually typical: s = normalize(cross(f, up)), u = cross(s, f) ??? Wait cross(s,f) =? If s = f x up, then u = s x f? Let's test: f=(0,0,-1), up=(0,1,0), s = f x up = (1,0,0), u = s x f? cross((1,0,0),(0,0,-1)) = (0* -1 -0*0=0, 0*0 -1*-1=1, 1*0-0*0=0) => (0,1,0) good. So u = cross(s, f) ??? No cross(s,f) = (1,0,0) x (0,0,-1) = (0,1,0) same as cross? Let's compute cross(f, s) would be different. So we use u = cross(s, f)?? Actually we used cross(s,f) earlier and got correct. Let's implement u = cross(s,f)?? Wait we did u = cross(s,f) earlier? We did Cross(s,f)?? In code above we did Cross(s,f) ? No we did Cross(s,f) ??? We have u = Cross(s, f)?? In this file we compute u = Cross(s, f)?? Actually we computed u = Cross(s,f) ??? Let's recompute: we want u = cross(s, f) ? Let's see: s = (1,0,0), f=(0,0,-1), cross(s,f)= (0* -1 -0*0=0, 0*0 -1*-1=1, 1*0-0*0=0) => (0,1,0) good. cross(f,s)= (0*0 - (-1)*0=0, (-1)*1 -0*0=-1, 0*0-0*1=0) => (0,-1,0) opposite. So correct is u = cross(s,f)??? Wait we got cross(s,f) = (0,1,0) correct. So u = cross(s, f) is not? Actually cross(s,f) gave (0,1,0) correct. So u = cross(s, f) ??? No cross(s,f) is what we computed? Let's see: we computed cross(s,f) = s x f = (1,0,0) x (0,0,-1) = (0,1,0) correct. So u = s x f, not f x s. But earlier we wrote Cross(s,f) which is s x f, so correct. However typical formula is u = cross(s, f) ??? Wait typical: f = target-eye, r = f x up, u = r x f? Let's see r = f x up, then u = r x f? r x f =? If f=(0,0,-1), up=(0,1,0), r= f x up = (1,0,0), r x f = (1,0,0) x (0,0,-1) = (0,1,0) correct. So u = r x f = s x f, same. So we want u = cross(s, f) = s x f. Good.

  // So compute:
  // f = normalize(target-eye)
  // s = normalize(cross(f, up))
  // u = cross(s, f) (should be normalized)
  // Then view matrix:
  // [ s.x s.y s.z -dot(s,eye) ]
  // [ u.x u.y u.z -dot(u,eye) ]
  // [ -f.x -f.y -f.z dot(f,eye) ]
  // [ 0 0 0 1 ]
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
  return Multiply(p, v); // P*V
}
Frustum Camera::GetFrustum() const {
  Mat4 vp = GetViewProjectionMatrix();
  // vp is row-major, but FrustumFromViewProjection expects column-major OpenGL layout?
  // Our FrustumFromViewProjection extracts rows assuming column-major storage.
  // Our Mat4 is row-major, so we need to transpose or convert to column-major array.
  // Let's build column-major array from row-major Mat4: col-major element (row,col) at m[col*4+row]
  // Our Mat4 stores row*4+col. So to get col-major array, we need to transpose.
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
  float x, y; // screen
  float z; // depth 0..1
  float w; // clip w for perspective correct (1/w)
  float intensity; // shading
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
  std::vector<Mesh> single = {mesh};
  std::vector<Vec3> offsets = {{0,0,0}};
  // If skeleton provided, we will handle skinning inside RenderMeshes? For single mesh we need to apply skinning.
  // To avoid duplicating logic, we create a path that applies skinning here.
  // For simplicity, if mesh has bone weights and skeleton+transforms provided, apply skinning to positions.
  // We'll produce a temporary mesh with skinned positions.
  Mesh skinnedMesh = mesh;
  if (mesh.has_bone_weights && skeleton && boneTransforms) {
    SkinnedResult sr = ApplySkinningLBS(mesh, *skeleton, *boneTransforms);
    if (sr.ok) {
      skinnedMesh.positions = sr.positions;
      // Keep normals as is (could also skin normals, but not implemented)
    }
  }
  std::vector<Mesh> meshes = {skinnedMesh};
  return RenderMeshes(meshes, offsets, cam, lightDir);
}

RenderStats SoftwareRasterizer::RenderMeshes(const std::vector<Mesh>& meshes,
                                             const std::vector<Vec3>& worldOffsets,
                                             const Camera& cam,
                                             const Vec3& lightDir) {
  RenderStats stats;
  if (meshes.empty()) { stats.problem="no meshes"; return stats; }
  // Camera matrices
  Mat4 view = cam.GetViewMatrix();
  Mat4 proj = cam.GetProjectionMatrix();
  Mat4 vp = Multiply(proj, view);

  // For frustum culling, we need frustum from VP in column-major
  float colMajor[16];
  for (int r=0;r<4;++r) for (int c=0;c<4;++c) colMajor[c*4+r]=vp.m[r*4+c];
  Frustum frustum = FrustumFromViewProjection(colMajor);

  Vec3 lightN = Normalize(lightDir);

  // For each mesh
  for (size_t mi=0; mi<meshes.size(); ++mi) {
    const Mesh& mesh = meshes[mi];
    Vec3 offset = {0,0,0};
    if (mi < worldOffsets.size()) offset = worldOffsets[mi];

    if (mesh.positions.empty() || mesh.indices.empty()) continue;

    // Frustum culling using mesh bounds + offset
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

    // Transform all vertices to screen space
    std::vector<VertexOut> vout(mesh.positions.size());
    for (size_t vi=0; vi<mesh.positions.size(); ++vi) {
      Vec3 pos = mesh.positions[vi];
      pos.x += offset.x; pos.y += offset.y; pos.z += offset.z;

      // World -> clip
      Vec3 clip = TransformPoint(vp, pos);
      // TransformPoint does perspective divide if w !=1, but we need w before divide for depth and perspective correct
      // Let's compute clip manually to get w
      float x = vp.m[0]*pos.x + vp.m[1]*pos.y + vp.m[2]*pos.z + vp.m[3]*1.0f;
      float y = vp.m[4]*pos.x + vp.m[5]*pos.y + vp.m[6]*pos.z + vp.m[7]*1.0f;
      float z = vp.m[8]*pos.x + vp.m[9]*pos.y + vp.m[10]*pos.z + vp.m[11]*1.0f;
      float w = vp.m[12]*pos.x + vp.m[13]*pos.y + vp.m[14]*pos.z + vp.m[15]*1.0f;

      if (w <= 0.001f) { vout[vi].clipped = true; continue; }

      float ndc_x = x / w;
      float ndc_y = y / w;
      float ndc_z = z / w;

      // Map NDC -1..1 to screen
      float sx = (ndc_x * 0.5f + 0.5f) * fb_.width;
      float sy = (1.0f - (ndc_y * 0.5f + 0.5f)) * fb_.height; // flip Y

      float depth = ndc_z * 0.5f + 0.5f; // 0..1

      // Shading: transform normal to view space (rotation only) then dot with light
      float intensity = 0.5f;
      if (vi < mesh.normals.size()) {
        Vec3 n = mesh.normals[vi];
        // Transform normal by view matrix rotation only (upper 3x3)
        Vec3 n_view = TransformDirection(view, n);
        n_view = Normalize(n_view);
        float d = Dot(n_view, lightN);
        if (d < 0) d = 0;
        intensity = d * 0.8f + 0.2f; // ambient 0.2
      }

      vout[vi].x = sx;
      vout[vi].y = sy;
      vout[vi].z = depth;
      vout[vi].w = w;
      vout[vi].intensity = intensity;
      vout[vi].clipped = false;
    }

    // Rasterize triangles
    for (size_t ti=0; ti+2 < mesh.indices.size(); ti+=3) {
      uint32_t i0 = mesh.indices[ti];
      uint32_t i1 = mesh.indices[ti+1];
      uint32_t i2 = mesh.indices[ti+2];
      if (i0 >= vout.size() || i1 >= vout.size() || i2 >= vout.size()) continue;
      const VertexOut& v0 = vout[i0];
      const VertexOut& v1 = vout[i1];
      const VertexOut& v2 = vout[i2];
      if (v0.clipped || v1.clipped || v2.clipped) continue;

      // Backface culling in screen space? Compute signed area
      float area = EdgeFunc(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
      if (area <= 0.0f) { // CCW front? Let's cull if area <0 for consistency, but keep both for now? We'll cull backfaces with area <0
        // Actually need to decide winding. BMD winding unknown, so disable backface culling for first proof
        // stats.triangles_culled_backface++;
        // continue;
      }
      if (std::fabs(area) < 1e-6f) continue; // degenerate

      // Bounding box
      float minX = std::min({v0.x, v1.x, v2.x});
      float maxX = std::max({v0.x, v1.x, v2.x});
      float minY = std::min({v0.y, v1.y, v2.y});
      float maxY = std::max({v0.y, v1.y, v2.y});
      int x0 = std::max(0, static_cast<int>(std::floor(minX)));
      int x1 = std::min(fb_.width-1, static_cast<int>(std::ceil(maxX)));
      int y0 = std::max(0, static_cast<int>(std::floor(minY)));
      int y1 = std::min(fb_.height-1, static_cast<int>(std::ceil(maxY)));
      if (x0 > x1 || y0 > y1) continue;

      // Precompute edge function denominators
      // Use barycentric with edge functions
      for (int y=y0; y<=y1; ++y) {
        for (int x=x0; x<=x1; ++x) {
          float px = x + 0.5f;
          float py = y + 0.5f;
          float w0 = EdgeFunc(v1.x, v1.y, v2.x, v2.y, px, py);
          float w1 = EdgeFunc(v2.x, v2.y, v0.x, v0.y, px, py);
          float w2 = EdgeFunc(v0.x, v0.y, v1.x, v1.y, px, py);
          // If all same sign as area
          if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
            // Barycentric
            w0 /= area; w1 /= area; w2 /= area;
            // Perspective correct interpolation using 1/w
            float invW = w0 / v0.w + w1 / v1.w + w2 / v2.w;
            if (std::fabs(invW) < 1e-6f) continue;
            float z = (w0 * v0.z / v0.w + w1 * v1.z / v1.w + w2 * v2.z / v2.w) / invW;
            float intensity = (w0 * v0.intensity / v0.w + w1 * v1.intensity / v1.w + w2 * v2.intensity / v2.w) / invW;
            // Clamp
            if (z < 0) z = 0; if (z > 1) z = 1;
            if (intensity < 0) intensity = 0; if (intensity > 1) intensity = 1;
            uint8_t c = static_cast<uint8_t>(intensity * 255.0f);
            // Simple color: grayscale with slight tint based on normal? For now grayscale
            if (fb_.SetPixel(x, y, z, c, c, c)) {
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
