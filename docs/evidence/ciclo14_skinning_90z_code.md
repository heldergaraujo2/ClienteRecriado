# Trecho código teste skinning rotação 90° Z — cálculo independente valor esperado

Extraído de tests/test_assets.cpp TestSkinningLBS (d) — ciclo12 D2 e ciclo13 revisado.

```cpp
  // (d) transformação NÃO-TRIVIAL: rotação 90° em Z, cálculo independente
  // Cálculo manual: rotação Z 90° (cos=0, sin=1) transforma (1,0,0) -> (0,1,0)
  // Matriz row-major: [cos -sin 0 0; sin cos 0 0; 0 0 1 0; 0 0 0 1]
  // Para 90°: cos=0, sin=1 => [0 -1 0 0; 1 0 0 0; 0 0 1 0; 0 0 0 1]
  // Aplicando a (1,0,0,1): x' = 0*1 + (-1)*0 +0*0+0*1=0, y'=1*1+0*0+0*0+0*1=1, z'=0
  // Portanto vértice (1,0,0) deve ir para (0,1,0) após skinning com peso 1.0 bone 0
  mu::Mesh meshRot;
  meshRot.positions = {{1,0,0}};
  meshRot.normals = {{0,0,1}};
  mu::BoneWeight wRot; wRot.bone=0; wRot.weight=1.0f;
  meshRot.bone_weights = {{wRot}};
  meshRot.has_bone_weights = true;
  mu::Skeleton skelRot;
  mu::Bone br0; br0.name="Root"; br0.parent=mu::kNoBoneParent; br0.position={0,0,0}; br0.rotation={0,0,0};
  skelRot.bones={br0};
  skelRot.has_bones=true;
  mu::Mat4 rot90{};
  rot90.m[0]=0.0f; rot90.m[1]=-1.0f; rot90.m[2]=0.0f; rot90.m[3]=0.0f;
  rot90.m[4]=1.0f; rot90.m[5]=0.0f; rot90.m[6]=0.0f; rot90.m[7]=0.0f;
  rot90.m[8]=0.0f; rot90.m[9]=0.0f; rot90.m[10]=1.0f; rot90.m[11]=0.0f;
  rot90.m[12]=0.0f; rot90.m[13]=0.0f; rot90.m[14]=0.0f; rot90.m[15]=1.0f;
  std::vector<mu::Mat4> rotMats = {rot90};
  auto resRot = mu::ApplySkinningLBS(meshRot, skelRot, rotMats);
  CHECK(resRot.ok);
  CHECK(resRot.positions.size()==1);
  // Resultado esperado calculado independentemente acima: (0,1,0)
  CHECK(std::fabs(resRot.positions[0].x-0.0f)<1e-5f);
  CHECK(std::fabs(resRot.positions[0].y-1.0f)<1e-5f);
  CHECK(std::fabs(resRot.positions[0].z-0.0f)<1e-5f);
```

## Explicação cálculo independente

- Entrada: vértice (1,0,0) peso 1.0 bone 0
- Transform bone: rotação 90° em Z, cos=0 sin=1
- Matriz row-major rotação Z: [cos -sin 0 0; sin cos 0 0; 0 0 1 0; 0 0 0 1] => [0 -1 0 0; 1 0 0 0; 0 0 1 0; 0 0 0 1]
- Multiplicação: x' = 0*1 + (-1)*0 =0, y'=1*1+0*0=1, z'=0
- Esperado (0,1,0) fabs<1e-5
- Teste verifica ApplySkinningLBS produz (0,1,0) não apenas identity/self-loop/zero
- Evidência ciclo13: TestSkinningLBS 15 checks incluindo (a) identity (b) self-loop fails safe (c) zero movement (d) rotação 90° Z não-trivial

## Arquivo origem
tests/test_assets.cpp linha ~TestSkinningLBS, src/mu/render/skinning.hpp/.cpp Mat4 Identity TransformPoint ValidateSkeletonForSkinning has_bones parent range self-loop cycle visited[64] ApplySkinningLBS strict count

