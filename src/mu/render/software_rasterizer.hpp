 // SPDX-License-Identifier: MIT
// Software rasterizer CPU-only — first visual proof without GPU
// Implements framebuffer, camera, perspective, z-buffer, simple lambert shading
// Uses Mesh/Skeleton from C1 and skinning D2 and culling D3
// No OpenGL/DirectX/Windows dependency, pure C++17

#ifndef MU_RENDER_SOFTWARE_RASTERIZER_HPP
#define MU_RENDER_SOFTWARE_RASTERIZER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "mu/assets/mesh.hpp"
#include "mu/render/skinning.hpp"
#include "mu/render/cull.hpp"

namespace mu {

struct SoftwareFramebuffer {
  int width = 0;
  int height = 0;
  // RGB 8-bit per channel, 3*W*H
  std::vector<uint8_t> color;
  // depth 0..1, 1 = far, 0 = near, size W*H, init 1.0f
  std::vector<float> depth;

  SoftwareFramebuffer() = default;
  SoftwareFramebuffer(int w, int h) { Resize(w,h); }

  void Resize(int w, int h);
  void Clear(const Vec3& clearColor = Vec3{0.1f,0.1f,0.15f});
  void ClearDepth(float d = 1.0f);
  bool SavePPM(const std::string& path) const; // binary P6

  // Set pixel if depth test passes (depth < stored)
  // color RGB 0..255
  bool SetPixel(int x, int y, float z, uint8_t r, uint8_t g, uint8_t b);
};

struct Camera {
  Vec3 eye = {0,0,5};
  Vec3 target = {0,0,0};
  Vec3 up = {0,1,0};
  float fov_deg = 45.0f;
  float aspect = 1.3333f;
  float near_plane = 0.1f;
  float far_plane = 100.0f;

  Mat4 GetViewMatrix() const;
  Mat4 GetProjectionMatrix() const;
  Mat4 GetViewProjectionMatrix() const; // P*V

  Frustum GetFrustum() const; // from VP
};

struct RenderStats {
  size_t triangles_total = 0;
  size_t triangles_culled_frustum = 0;
  size_t triangles_culled_backface = 0;
  size_t triangles_rasterized = 0;
  size_t pixels_written = 0;
  bool ok = false;
  std::string problem;
};

// Main rasterizer
class SoftwareRasterizer {
public:
  explicit SoftwareRasterizer(int w = 800, int h = 600);
  void Resize(int w, int h);
  void Clear(const Vec3& clearColor = Vec3{0.1f,0.1f,0.15f});

  // Render one mesh with optional skeleton+transforms
  // lightDir should be normalized, e.g. (0.2,0.5,1) normalized
  RenderStats RenderMesh(const Mesh& mesh,
                         const Skeleton* skeleton,
                         const std::vector<Mat4>* boneTransforms,
                         const Camera& cam,
                         const Vec3& lightDir);

  // Render multiple meshes (each with own world offset for grid layout)
  // worldOffset applied to positions before view transform
  RenderStats RenderMeshes(const std::vector<Mesh>& meshes,
                           const std::vector<Vec3>& worldOffsets,
                           const Camera& cam,
                           const Vec3& lightDir);

  bool SavePPM(const std::string& path) const;
  const SoftwareFramebuffer& framebuffer() const { return fb_; }
  SoftwareFramebuffer& framebuffer() { return fb_; }

private:
  SoftwareFramebuffer fb_;
};

// Utility math exposed for tests
Mat4 Multiply(const Mat4& a, const Mat4& b); // a*b row-major
Vec3 TransformDirection(const Mat4& mat, const Vec3& v); // w=0
Vec3 Normalize(const Vec3& v);
float Dot(const Vec3& a, const Vec3& b);
Vec3 Cross(const Vec3& a, const Vec3& b);
Vec3 Sub(const Vec3& a, const Vec3& b);
Vec3 Add(const Vec3& a, const Vec3& b);

} // namespace mu

#endif // MU_RENDER_SOFTWARE_RASTERIZER_HPP
