 // SPDX-License-Identifier: MIT
// Software rasterizer CPU-only — first visual proof without GPU
// Implements framebuffer, camera, perspective, z-buffer, simple lambert shading
// Uses Mesh/Skeleton from C1 and skinning D2 and culling D3
// No OpenGL/DirectX/Windows dependency, pure C++17
// Ciclo16 D: textura placeholder — prova de conceito pipeline textura, NÃO UV real

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

// PLACEHOLDER TEXTURE — ciclo16 D
// Prova de conceito pipeline textura, NÃO é UV real decodificado (UV continua 0% BLOQUEADO)
// Usa OztToBgra/OzjProbe já otimizados (OPT-004/OPT-005) para carregar textura real OZT/OZJ,
// mas mapeamento UV é placeholder EXPLICITAMENTE marcado: planar baseado em posição vértice
// ou checkerboard procedural, nunca apresentado como "UV real"
struct SoftwareTexture {
  int width = 0;
  int height = 0;
  // BGRA 8-bit per channel, 4*W*H, ready for GL_BGRA (OPT-005)
  std::vector<uint8_t> bgra;
  bool valid = false;

  // Gera checkerboard procedural 256x256 — PLACEHOLDER
  void GenerateCheckerboard(int w=256, int h=256, int checkerSize=32);

  // Carrega OZT real via OztToBgraTopDown — PLACEHOLDER UV ainda, mas textura real
  // Usa OztProbe + OztToBgraTopDown já otimizados
  bool LoadFromOZT(const std::string& oztPath);

  // Sample com UV placeholder (u,v 0..1), retorna RGB, clamp
  void Sample(float u, float v, uint8_t& outR, uint8_t& outG, uint8_t& outB) const;
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

  // Textura placeholder — ciclo16 D
  void SetTexture(const SoftwareTexture& tex) { texture_ = tex; hasTexture_ = tex.valid; }
  void ClearTexture() { hasTexture_ = false; texture_.valid=false; }
  bool HasTexture() const { return hasTexture_; }
  const SoftwareTexture& GetTexture() const { return texture_; }

  bool SavePPM(const std::string& path) const;
  const SoftwareFramebuffer& framebuffer() const { return fb_; }
  SoftwareFramebuffer& framebuffer() { return fb_; }

private:
  SoftwareFramebuffer fb_;
  SoftwareTexture texture_;
  bool hasTexture_ = false;
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
