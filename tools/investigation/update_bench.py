# Ciclo: ciclo13 item10
# Pergunta: Atualiza bench_assets.cpp com bench OztToBgraTopDown 256x256 GL_BGRA
# Evidência: docs/evidence/ciclo13_bench_after.log
# Origem: /tmp/update_bench.py movido para backup permanente ciclo15 Parte A
# Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
# Data backup: 2026-10-12
# Branch: arena/519f8fc9-clienterecriado

path="bench/bench_assets.cpp"
text=open(path,encoding='utf-8').read()
old="    // Conversion to the layout glTexImage2D wants.\n    std::vector<uint8_t> out;\n    s = TimeIt(2000, [&] { mu::OztToRgbaTopDown(full, &out); });\n    PrintRow(\"OztToRgbaTopDown 256x256\", full.pixel_bytes(), s);"
new="    // Conversion to the layout glTexImage2D wants.\n    std::vector<uint8_t> out;\n    s = TimeIt(2000, [&] { mu::OztToRgbaTopDown(full, &out); });\n    PrintRow(\"OztToRgbaTopDown 256x256\", full.pixel_bytes(), s);\n    // CAND-001 / OPT-005: BGRA direct via GL_BGRA — skip swap, only flip via memcpy — 10,2x speedup 66,4us->6,5us\n    s = TimeIt(2000, [&] { mu::OztToBgraTopDown(full, &out); });\n    PrintRow(\"OztToBgraTopDown 256x256 (GL_BGRA)\", full.pixel_bytes(), s);"
text=text.replace(old,new)
open(path,'w',encoding='utf-8').write(text)
print("updated bench")
