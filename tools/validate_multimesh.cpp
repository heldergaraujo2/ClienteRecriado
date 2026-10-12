// SPDX-License-Identifier: MIT
// Validates multi-mesh layout with strict metrics
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include "mu/fs.hpp"
#include "mu/assets/bmd.hpp"
#include "mu/assets/bmd_body.hpp"
#include "mu/assets/bmd_report.hpp"
#include "mu/assets/format.hpp"

bool IsTextureNameStrict(const uint8_t* p, std::size_t remaining) {
  if (remaining < 32) return false;
  std::size_t nul = 0;
  while (nul < 32 && p[nul] != 0) ++nul;
  if (nul < 3 || nul >= 32) return false;
  for (std::size_t i = 0; i < nul; ++i) {
    unsigned char c = p[i];
    if (c < 32 || c > 126) return false;
  }
  for (std::size_t i = nul; i < 32; ++i) if (p[i] != 0) return false;
  return true;
}
bool IsTextureNameWeak(const uint8_t* p, std::size_t remaining) {
  if (remaining < 32) return false;
  std::size_t printable = 0;
  for (std::size_t i = 0; i < 32; ++i) {
    unsigned char c = p[i];
    if (c >= 32 && c < 127) ++printable;
  }
  return printable >= 8;
}

int main(int argc, char** argv){
  if(argc<2) return 1;
  std::string root=argv[1];
  std::vector<mu::FileEntry> files;
  mu::WalkFiles(root,&files);
  std::sort(files.begin(), files.end(), [](auto& a, auto& b){return a.path<b.path;});

  std::size_t total=0;
  std::size_t total_meshes=0;
  std::size_t parsed_ok=0;
  std::size_t strict_at_0=0, strict_at_m4=0, strict_at_p4=0, strict_at_m8=0, strict_at_p8=0;
  std::size_t weak_at_0=0, weak_at_m4=0;
  std::size_t found_any_strict=0, found_any_weak=0;
  std::size_t control_weak_hits=0, control_trials=0;

  for(auto& f: files){
    if(mu::ExtensionOf(f.path)!=".bmd") continue;
    auto r=mu::BmdClassifyFile(f.path);
    if(!r.geometry_parsed) continue;
    if(r.declared_mesh_count<=1) continue; // only multi-mesh
    ++total;
    total_meshes += r.declared_mesh_count;

    // Now need to re-parse payload to check texture names per mesh
    std::vector<uint8_t> raw;
    if(!mu::ReadFile(f.path,&raw)) continue;
    mu::ByteView view(raw.data(), raw.size());
    mu::BmdInfo info;
    if(!mu::BmdProbe(view,&info).ok() || !info.length_consistent) continue;
    mu::ByteView payload = view.sub(info.payload_offset, info.payload_size);
    std::vector<uint8_t> plain;
    if(info.kind==mu::BmdKind::kPlainV0C){
      plain.assign(payload.data, payload.data+payload.size);
      mu::BmdV0CDecrypt(plain.data(), plain.size());
      // check printable
      bool ok=true;
      if(plain.size()<32) ok=false;
      else for(int i=0;i<32;i++){ uint8_t c=plain[i]; if(c!=0 && (c<32||c>126)) ok=false; }
      if(!ok) continue;
      payload = mu::ByteView(plain.data(), plain.size());
    }
    if(payload.size < 52) continue;

    // Walk meshes
    std::size_t offset = 38;
    std::size_t meshes_parsed=0;
    bool all_found_strict=false;
    bool all_found_weak=false;
    // We'll count per file whether we found at least one strict/weak at expected deltas
    std::size_t file_strict_0=0, file_strict_m4=0, file_weak_0=0, file_weak_m4=0;
    std::size_t file_meshes=0;
    for(std::size_t mi=0; mi<r.declared_mesh_count && mi<64; ++mi){
      if(offset+14>payload.size) break;
      std::size_t vc = mu::ReadU16LE(payload.data+offset);
      std::size_t nc = mu::ReadU16LE(payload.data+offset+2);
      std::size_t ntc = mu::ReadU16LE(payload.data+offset+4);
      std::size_t ntri = mu::ReadU16LE(payload.data+offset+6);
      std::size_t v_start = offset+14;
      std::size_t n_start = v_start + vc*16;
      std::size_t tc_start = n_start + nc*20;
      std::size_t tri_start = tc_start + ntc*8;
      std::size_t tri_end = tri_start + ntri*64;
      if(tri_end>payload.size+8) break;

      // Check strict at deltas
      auto check_at = [&](int delta)->bool{
        std::size_t at;
        if(delta<0){ if(tri_end < (std::size_t)(-delta)) return false; at=tri_end+delta; }
        else at=tri_end+delta;
        if(at+32>payload.size) return false;
        return IsTextureNameStrict(payload.data+at, payload.size-at);
      };
      auto check_weak_at = [&](int delta)->bool{
        std::size_t at;
        if(delta<0){ if(tri_end < (std::size_t)(-delta)) return false; at=tri_end+delta; }
        else at=tri_end+delta;
        if(at+32>payload.size) return false;
        return IsTextureNameWeak(payload.data+at, payload.size-at);
      };
      if(check_at(0)) { ++strict_at_0; ++file_strict_0; }
      if(check_at(-4)) { ++strict_at_m4; ++file_strict_m4; }
      if(check_at(4)) ++strict_at_p4;
      if(check_at(-8)) ++strict_at_m8;
      if(check_at(8)) ++strict_at_p8;
      if(check_weak_at(0)) { ++weak_at_0; ++file_weak_0; }
      if(check_weak_at(-4)) { ++weak_at_m4; ++file_weak_m4; }

      // Control: search weak in vertex array (should be low)
      if(vc>0 && v_start+32<=payload.size){
        // Take middle of vertex array
        std::size_t mid = v_start + (vc*16)/2;
        if(mid+32<=payload.size){
          ++control_trials;
          if(IsTextureNameWeak(payload.data+mid, payload.size-mid)) ++control_weak_hits;
        }
      }

      // Next mesh
      // Find name
      std::size_t name_at=tri_end;
      bool found=false;
      for(int d: {0,-4,4,-8,8}){
        std::size_t at;
        if(d<0){ if(tri_end < (std::size_t)(-d)) continue; at=tri_end+d; }
        else at=tri_end+d;
        if(at+32>payload.size) continue;
        if(IsTextureNameStrict(payload.data+at, payload.size-at) ||
           IsTextureNameWeak(payload.data+at, payload.size-at)){
          name_at=at; found=true; break;
        }
      }
      if(!found) name_at=tri_end;
      offset = name_at+32;
      ++file_meshes;
      ++meshes_parsed;
    }
    if(file_meshes>0){
      ++parsed_ok;
      if(file_strict_0+file_strict_m4>0) ++found_any_strict;
      if(file_weak_0+file_weak_m4>0) ++found_any_weak;
    }
  }

  printf("Multi-mesh validation over %s\n", root.c_str());
  printf("  multi-mesh files with geometry parsed: %zu\n", total);
  printf("  total meshes declared (sum): %zu\n", total_meshes);
  printf("  files where at least 1 mesh parsed: %zu (%.2f%%)\n", parsed_ok, total?100.0*parsed_ok/total:0);
  printf("\n  Texture name strict matches per mesh:\n");
  printf("    at tri_end+0 : %zu (%.2f%% of meshes)\n", strict_at_0, total_meshes?100.0*strict_at_0/total_meshes:0);
  printf("    at tri_end-4 : %zu (%.2f%%)\n", strict_at_m4, total_meshes?100.0*strict_at_m4/total_meshes:0);
  printf("    at tri_end+4 : %zu (%.2f%%)\n", strict_at_p4, total_meshes?100.0*strict_at_p4/total_meshes:0);
  printf("    at tri_end-8 : %zu (%.2f%%)\n", strict_at_m8, total_meshes?100.0*strict_at_m8/total_meshes:0);
  printf("    at tri_end+8 : %zu (%.2f%%)\n", strict_at_p8, total_meshes?100.0*strict_at_p8/total_meshes:0);
  printf("\n  Weak matches (>=8 printable):\n");
  printf("    at 0 : %zu (%.2f%%)\n", weak_at_0, total_meshes?100.0*weak_at_0/total_meshes:0);
  printf("    at -4: %zu (%.2f%%)\n", weak_at_m4, total_meshes?100.0*weak_at_m4/total_meshes:0);
  printf("\n  Files with at least one strict name found: %zu (%.2f%% of files)\n", found_any_strict, total?100.0*found_any_strict/total:0);
  printf("  Files with at least one weak name found: %zu (%.2f%%)\n", found_any_weak, total?100.0*found_any_weak/total:0);
  printf("\n  Control (weak check inside vertex array, should be low):\n");
  printf("    trials: %zu, hits: %zu, rate: %.2f%%\n", control_trials, control_weak_hits, control_trials?100.0*control_weak_hits/control_trials:0);
  printf("\n  Layout per mesh: 14B header [vc,nc,ntc,ntri,texIdx,u48,u50] + vc*16 + nc*20 + ntc*8 + ntri*64 + 32B name, concatenated\n");
  return 0;
}
