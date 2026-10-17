// Ciclo: ciclo16 D
// Pergunta: Implementação CmdRender com flag --textured para textura placeholder checkerboard 256x256 32px UV planar placeholder NÃO UV real
// Evidência: docs/evidence/ciclo16_first_render_Beer01_textured.ppm
// Origem: /tmp/new_render.cpp movido para backup permanente ciclo16 para garantir que todo trabalho está salvo no repositório
// Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
// Data backup: 2026-10-12
// Branch: arena/519f8fc9-clienterecriado

int CmdRender(int argc, char** argv) {
  if (argc < 4) {
    std::fprintf(stderr, "usage: mutool render <bmd-file> <out.ppm> [width height] [--textured]\n");
    return 1;
  }
  std::string bmdPath = argv[2];
  std::string outPath = argv[3];
  int width = 800;
  int height = 600;
  bool textured = false;
  for(int i=4;i<argc;++i){
    std::string a=argv[i];
    if(a=="--textured" || a=="-t" || a=="textured"){
      textured=true;
    } else if(width==800){
      width = std::atoi(argv[i]);
    } else if(height==600){
      height = std::atoi(argv[i]);
    }
  }
  if(outPath.find("textured")!=std::string::npos) textured=true;
  if (width < 64) width = 64;
  if (height < 64) height = 64;
  if (width > 2048) width = 2048;
  if (height > 2048) height = 2048;

  std::vector<uint8_t> raw;
  if (!mu::ReadFile(bmdPath, &raw)) {
    std::fprintf(stderr, "error: cannot read '%s'\n", bmdPath.c_str());
    return 1;
  }
  mu::ByteView view(raw.data(), raw.size());
  mu::BmdInfo info;
  if (!mu::BmdProbe(view, &info) || !info.length_consistent) {
    std::fprintf(stderr, "error: not a valid BMD or length inconsistent: %s\n", bmdPath.c_str());
    return 1;
  }
  mu::ByteView payload = view.sub(info.payload_offset, info.payload_size);
  std::vector<uint8_t> plain;
  if (info.kind == mu::BmdKind::kPlainV0C) {
    plain.assign(payload.data, payload.data + payload.size);
    mu::BmdV0CDecrypt(plain.data(), plain.size());
    bool printable = plain.size() >= 32;
    for (size_t i=0; printable && i<32; ++i) {
      uint8_t c = plain[i];
      if (c!=0 && (c<32 || c>126)) printable=false;
    }
    if (!printable) {
      std::fprintf(stderr, "error: V0C decrypt failed plausibility\n");
      return 1;
    }
    payload = mu::ByteView(plain.data(), plain.size());
  } else if (info.kind == mu::BmdKind::kBuxContainer) {
    std::vector<uint8_t> tmp = raw;
    mu::BuxCrypt(tmp.data(), tmp.size());
    mu::ByteView view2(tmp.data(), tmp.size());
    mu::BmdInfo info2;
    if (!mu::BmdProbe(view2, &info2) || !info2.length_consistent) {
      std::fprintf(stderr, "error: BUX container decrypt failed\n");
      return 1;
    }
    raw.swap(tmp);
    view = mu::ByteView(raw.data(), raw.size());
    info = info2;
    payload = view.sub(info.payload_offset, info.payload_size);
    if (info.kind == mu::BmdKind::kPlainV0C) {
      plain.assign(payload.data, payload.data + payload.size);
      mu::BmdV0CDecrypt(plain.data(), plain.size());
      bool printable2 = plain.size() >= 32;
      for (size_t i=0; printable2 && i<32; ++i) {
        uint8_t c = plain[i];
        if (c!=0 && (c<32 || c>126)) printable2=false;
      }
      if (!printable2) {
        std::fprintf(stderr, "error: V0C decrypt after BUX failed\n");
        return 1;
      }
      payload = mu::ByteView(plain.data(), plain.size());
    }
  }

  mu::BmdBody body;
  mu::Status bs = mu::BmdParseBodyV0A(payload, &body);
  if (!bs || !body.complete) {
    std::fprintf(stderr, "error: body parse failed: %s\n", body.problem.c_str());
    return 1;
  }
  mu::MeshImportResult r = mu::MeshFromBmd(body, bmdPath);
  if (!r.ok) {
    std::fprintf(stderr, "error: mesh import failed: %s\n", r.problem.c_str());
    return 1;
  }
  mu::Mesh mesh = r.mesh;
  std::printf("Loaded %s: %zu vertices %zu triangles bounds %s\n",
              bmdPath.c_str(),
              mesh.positions.size(),
              mesh.indices.size()/3,
              mesh.bounds.valid ? "valid" : "invalid");
  if (mesh.bounds.valid) {
    std::printf("  bounds min (%.2f,%.2f,%.2f) max (%.2f,%.2f,%.2f)\n",
                static_cast<double>(mesh.bounds.min.x), static_cast<double>(mesh.bounds.min.y), static_cast<double>(mesh.bounds.min.z),
                static_cast<double>(mesh.bounds.max.x), static_cast<double>(mesh.bounds.max.y), static_cast<double>(mesh.bounds.max.z));
  }

  mu::Camera cam;
  mu::Vec3 center{0,0,0};
  float radius = 1.0f;
  if (mesh.bounds.valid) {
    center.x = (mesh.bounds.min.x + mesh.bounds.max.x)*0.5f;
    center.y = (mesh.bounds.min.y + mesh.bounds.max.y)*0.5f;
    center.z = (mesh.bounds.min.z + mesh.bounds.max.z)*0.5f;
    float dx = mesh.bounds.max.x - mesh.bounds.min.x;
    float dy = mesh.bounds.max.y - mesh.bounds.min.y;
    float dz = mesh.bounds.max.z - mesh.bounds.min.z;
    radius = std::sqrt(dx*dx + dy*dy + dz*dz)*0.5f;
    if (radius < 0.1f) radius = 0.1f;
  }
  cam.aspect = static_cast<float>(width)/static_cast<float>(height);
  cam.fov_deg = 45.0f;
  cam.near_plane = 0.1f;
  cam.far_plane = std::max(100.0f, radius*10.0f);
  cam.target = center;
  cam.eye = mu::Vec3{center.x, center.y + radius*0.3f, center.z + radius*2.2f};
  cam.up = mu::Vec3{0,1,0};

  mu::SoftwareRasterizer rast(width, height);
  rast.Clear(mu::Vec3{0.12f,0.12f,0.18f});
  if(textured){
    mu::SoftwareTexture tex;
    tex.GenerateCheckerboard(256,256,32);
    std::string sampleOzt = "/tmp/cliente_ref/Data/Item/ArmorMale01.ozt";
    std::vector<uint8_t> dummy;
    if(mu::ReadFile(sampleOzt, &dummy)){
      tex.LoadFromOZT(sampleOzt);
    }
    rast.SetTexture(tex);
    std::printf("PLACEHOLDER TEXTURE ENABLED — ciclo16 D — checkerboard 256x256 32px, UV planar baseado em posicao vertice, NAO e UV real decodificado (UV continua 0%% BLOQUEADO)\n");
  }
  mu::Vec3 lightDir = mu::Normalize(mu::Vec3{0.3f,0.5f,1.0f});

  mu::RenderStats stats = rast.RenderMesh(mesh, nullptr, nullptr, cam, lightDir);
  std::printf("Render stats: total %zu culled_frustum %zu rasterized %zu pixels %zu ok %d textured %d\n",
              stats.triangles_total, stats.triangles_culled_frustum,
              stats.triangles_rasterized, stats.pixels_written, stats.ok, textured?1:0);

  if (!rast.SavePPM(outPath)) {
    std::fprintf(stderr, "error: cannot write '%s'\n", outPath.c_str());
    return 1;
  }
  std::printf("Saved %s %dx%d\n", outPath.c_str(), width, height);
  return 0;
}
