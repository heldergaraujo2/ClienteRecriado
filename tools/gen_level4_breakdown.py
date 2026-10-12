#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Gera breakdown dos arquivos que falham no nível 4.
# Usa mutool bmdreport para obter contagens, e também lista detalhada via list_bmd_level3.

import subprocess, os, sys, re

data_dir = sys.argv[1] if len(sys.argv)>1 else "/home/user/Cliente_ref/Data"
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
binary = os.path.join(root, "build", "mutool")
list_bin = "/tmp/list_bmd_fixed"
if not os.path.exists(list_bin):
    # build it
    subprocess.run(["g++","-std=c++17","-I","src","tools/list_bmd_level3.cpp",
                    "src/mu/assets/bmd.cpp","src/mu/assets/bmd_body.cpp",
                    "src/mu/assets/bmd_report.cpp","src/mu/assets/format.cpp","src/mu/fs.cpp",
                    "-o",list_bin,"-O2"], cwd=root, check=True)

# Run bmdreport
p = subprocess.run([binary, "bmdreport", data_dir], capture_output=True, text=True)
out = p.stdout
print(out)

# Also get detailed list
p2 = subprocess.run([list_bin, data_dir], capture_output=True, text=True)
lines = p2.stdout.strip().split("\n")
# Each line: path \t geom_parsed \t meshCount \t access \t problem

total=0
geom_parsed=0
topo_parsed=0
# We'll need to get topo from report, but we can approximate via list? Our list only has geom.
# For full breakdown we need to run a custom tool that also reports topo.
# Let's write a small C++ tool that outputs full classification.

# For now, parse report numbers
m_total = re.search(r"total \.bmd files\s+(\d+)", out)
m_struct_valid = re.search(r"valid\s+(\d+)", out)
m_opaque = re.search(r"OPAQUE.*?\s+(\d+)", out)
m_geom = re.search(r"parsed\s+(\d+)", out)
m_topo = re.search(r"with triangles\s+(\d+)", out)

total = int(m_total.group(1)) if m_total else 0
# We'll compute breakdown manually by iterating files using the same logic as bmd_report.cpp
# But we can also use the list_bmd tool that we have that reports geometry.
# For level4, we need triangle count.

# Let's create a more detailed list tool
cpp_code = r'''
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include "mu/fs.hpp"
#include "mu/assets/bmd_report.hpp"
int main(int argc, char** argv){
  if(argc<2) return 1;
  std::string root=argv[1];
  std::vector<mu::FileEntry> files;
  mu::WalkFiles(root,&files);
  std::sort(files.begin(), files.end(), [](auto& a, auto& b){return a.path<b.path;});
  for(auto& f: files){
    if(mu::ExtensionOf(f.path)!=".bmd") continue;
    auto r=mu::BmdClassifyFile(f.path);
    printf("%s\t%d\t%d\t%d\t%s\t%s\t%s\n",
      f.path.c_str(),
      r.structure_valid?1:0,
      r.access==mu::PayloadAccess::kOpaque?1:0,
      r.geometry_parsed?1:0,
      r.geometry_parsed && r.triangle_count>0 ? "1":"0",
      mu::BmdTopologySkipName(r.topology_skip),
      r.body_problem.c_str());
  }
  return 0;
}
'''
open('/tmp/detail.cpp','w').write(cpp_code)
subprocess.run(["g++","-std=c++17","-I","src","/tmp/detail.cpp",
                "src/mu/assets/bmd.cpp","src/mu/assets/bmd_body.cpp",
                "src/mu/assets/bmd_report.cpp","src/mu/assets/format.cpp","src/mu/fs.cpp",
                "-o","/tmp/detail","-O2"], cwd=root, check=True)
p3 = subprocess.run(["/tmp/detail", data_dir], capture_output=True, text=True)
detail_lines = p3.stdout.strip().split("\n")

# Count categories
from collections import Counter
cat = Counter()
geom_no_topo_reasons = Counter()
for line in detail_lines:
    parts=line.split("\t")
    if len(parts)<6: continue
    path=parts[0]
    struct_valid=int(parts[1])
    opaque=int(parts[2])
    geom=int(parts[3])
    topo=int(parts[4])
    skip=parts[5]
    problem=parts[6] if len(parts)>6 else ""
    if topo==1:
        cat["level4_ok"]+=1
    else:
        # failing level4
        if not struct_valid:
            cat["invalid_container"]+=1
        elif opaque:
            cat["opaque_payload"]+=1
        elif not geom:
            # geometry failed
            # body_problem contains reason
            if "shorter than" in problem or "exceed" in problem:
                cat["geom_does_not_fit"]+=1
            elif "meshCount exceeds" in problem:
                cat["geom_meshcount_limit"]+=1
            else:
                cat["geom_other"]+=1
        else:
            # geom ok but no topo
            cat[f"geom_ok_no_topo_{skip}"]+=1
            geom_no_topo_reasons[skip]+=1

print("\n=== BREAKDOWN LEVEL4 FAILING ===")
for k,v in cat.items():
    print(f"{k}: {v}")
print("\n=== GEOM OK BUT NO TOPO REASONS ===")
for k,v in geom_no_topo_reasons.items():
    print(f"{k}: {v}")

# Now produce table for docs
total_files = len(detail_lines)
level4_ok = cat["level4_ok"]
failing = total_files - level4_ok
print(f"\nTotal .bmd: {total_files}, level4 ok: {level4_ok}, failing: {failing}")

# For audit item 2, we need table with causes summing to failing (1690 originally, now 1685)
# Causes: multi-mesh still not supported, zero faces, array does not fit, payload short, other (opaque, invalid, geom fail)
# Let's produce final table

# Count opaque and invalid separately
invalid = cat["invalid_container"]
opaque = cat["opaque_payload"]
geom_fail = cat["geom_does_not_fit"]+cat["geom_other"]+cat.get("geom_meshcount_limit",0)
zero_faces = cat.get("geom_ok_no_topo_declares zero faces",0)
does_not_fit = cat.get("geom_ok_no_topo_declared array does not fit",0)
no_header = cat.get("geom_ok_no_topo_payload too short for counts",0)
multi_mesh = cat.get("geom_ok_no_topo_multi-mesh",0)

print("\n=== FINAL TABLE FOR DOCS ===")
print(f"OPAQUE (cipher not recovered): {opaque}")
print(f"Invalid container: {invalid}")
print(f"Geometry not parsed (counts exceed / truncated): {geom_fail}")
print(f"  - of which meshCount safety limit: {cat.get('geom_meshcount_limit',0)}")
print(f"Level3 ok but no topology:")
print(f"  multi-mesh still not supported: {multi_mesh}")
print(f"  zero faces: {zero_faces}")
print(f"  array does not fit: {does_not_fit}")
print(f"  payload too short: {no_header}")
print(f"Sum check: {opaque+invalid+geom_fail+zero_faces+does_not_fit+no_header+multi_mesh} vs failing {failing}")

# Write evidence log
with open("docs/evidence/ciclo6_level4_breakdown.log","w") as f:
    f.write("=== EVIDENCIA CICLO 6: breakdown nivel 4 ===\n")
    f.write(f"data: generated by tools/gen_level4_breakdown.py\n")
    f.write(f"comando: build/mutool bmdreport {data_dir}\n\n")
    f.write(out)
    f.write("\n=== DETAILED BREAKDOWN ===\n")
    for k,v in cat.items():
        f.write(f"{k}: {v}\n")
    f.write(f"\nTotal failing level4: {failing}\n")
    f.write(f"Sum check: {opaque+invalid+geom_fail+zero_faces+does_not_fit+no_header+multi_mesh}\n")
