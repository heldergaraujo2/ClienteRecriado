# Ciclo: ciclo13 item4
# Pergunta: Adiciona teste rotação 90° Z não-trivial para D2, corrige classificação otimista só identity/self-loop/zero
# Evidência: docs/evidence/ciclo13_c1_e3_detailed.log
# Origem: /tmp/add_skinning_nontrivial.py movido para backup permanente ciclo15 Parte A
# Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
# Data backup: 2026-10-12
# Branch: arena/519f8fc9-clienterecriado

path="tests/test_assets.cpp"
text=open(path,encoding='utf-8').read()
old="  CHECK(resNoW.positions.size()==2);\n  CHECK(resNoW.positions[0].x==0.0f && resNoW.positions[1].x==1.0f);\n}\n\n\nvoid TestFrustumCullRealBounds() {"
new="  CHECK(resNoW.positions.size()==2);\n  CHECK(resNoW.positions[0].x==0.0f && resNoW.positions[1].x==1.0f);\n\n  // (d) transformação NÃO-TRIVIAL: rotação 90° em Z, cálculo independente
  // Cálculo manual: rotação Z 90° (cos=0, sin=1) transforma (1,0,0) -> (0,1,0)
  // Matriz row-major: [cos -sin 0 0; sin cos 0 0; 0 0 1 0; 0 0 0 1]
  // Para 90°: cos=0, sin=1 => [0 -1 0 0; 1 0 0 0; 0 0 1 0; 0 0 0 1]
  // Aplicando a (1,0,0,1): x' = 0*1 + (-1)*0 +0*0+0*1=0, y'=1*1+0*0+0*0+0*1=1, z'=0
  // Portanto vértice (1,0,0) deve ir para (0,1,0) após skinning com peso 1.0 bone 0
  mu::Mesh meshRot;\n  meshRot.positions = {{1,0,0}};\n  meshRot.normals = {{0,0,1}};\n  mu::BoneWeight wRot; wRot.bone=0; wRot.weight=1.0f;\n  meshRot.bone_weights = {{wRot}};\n  meshRot.has_bone_weights = true;\n  mu::Skeleton skelRot;\n  mu::Bone br0; br0.name=\"Root\"; br0.parent=mu::kNoBoneParent; br0.position={0,0,0}; br0.rotation={0,0,0};\n  skelRot.bones={br0};\n  skelRot.has_bones=true;\n  mu::Mat4 rot90{};\n  rot90.m[0]=0.0f; rot90.m[1]=-1.0f; rot90.m[2]=0.0f; rot90.m[3]=0.0f;\n  rot90.m[4]=1.0f; rot90.m[5]=0.0f; rot90.m[6]=0.0f; rot90.m[7]=0.0f;\n  rot90.m[8]=0.0f; rot90.m[9]=0.0f; rot90.m[10]=1.0f; rot90.m[11]=0.0f;\n  rot90.m[12]=0.0f; rot90.m[13]=0.0f; rot90.m[14]=0.0f; rot90.m[15]=1.0f;\n  std::vector<mu::Mat4> rotMats = {rot90};\n  auto resRot = mu::ApplySkinningLBS(meshRot, skelRot, rotMats);\n  CHECK(resRot.ok);\n  CHECK(resRot.positions.size()==1);\n  // Resultado esperado calculado independentemente acima: (0,1,0)\n  CHECK(std::fabs(resRot.positions[0].x-0.0f)<1e-5f);\n  CHECK(std::fabs(resRot.positions[0].y-1.0f)<1e-5f);\n  CHECK(std::fabs(resRot.positions[0].z-0.0f)<1e-5f);\n}\n\n\nvoid TestFrustumCullRealBounds() {"
text=text.replace(old,new)
open(path,'w',encoding='utf-8').write(text)
print("added nontrivial")
