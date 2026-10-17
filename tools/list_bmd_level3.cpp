// SPDX-License-Identifier: MIT
// List files by level 3 success
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include "mu/fs.hpp"
#include "mu/assets/bmd_report.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <data-dir> [--failed-only|--parsed-only]\n", argv[0]);
    return 1;
  }
  const std::string root = argv[1];
  bool failed_only = false;
  bool parsed_only = false;
  for (int i=2;i<argc;++i){
    std::string a=argv[i];
    if (a=="--failed-only") failed_only=true;
    if (a=="--parsed-only") parsed_only=true;
  }
  std::vector<mu::FileEntry> files;
  if (!mu::WalkFiles(root, &files)) {
    std::fprintf(stderr, "cannot open %s\n", root.c_str());
    return 1;
  }
  std::sort(files.begin(), files.end(), [](auto& a, auto& b){return a.path<b.path;});
  for (auto& f: files) {
    if (mu::ExtensionOf(f.path)!=".bmd") continue;
    auto r = mu::BmdClassifyFile(f.path);
    if (failed_only && r.geometry_parsed) continue;
    if (parsed_only && !r.geometry_parsed) continue;
    std::printf("%s\t%d\t%d\t%s\t%s\n", f.path.c_str(),
      r.geometry_parsed?1:0,
      (int)r.declared_mesh_count,
      mu::PayloadAccessName(r.access),
      r.body_problem.c_str());
  }
  return 0;
}
