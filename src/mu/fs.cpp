// SPDX-License-Identifier: MIT
#include "mu/fs.hpp"

#include <cctype>
#include <cstring>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace mu {

std::string AsciiLower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::string ExtensionOf(const std::string& path) {
  const std::size_t slash = path.find_last_of("/\\");
  const std::size_t dot = path.find_last_of('.');
  if (dot == std::string::npos) return std::string();
  if (slash != std::string::npos && dot < slash) return std::string();
  return AsciiLower(path.substr(dot));
}

bool FileSize(const std::string& path, uint64_t* out) {
#if defined(_WIN32)
  WIN32_FILE_ATTRIBUTE_DATA fad;
  if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fad))
    return false;
  if (!out) return true;
  ULARGE_INTEGER u;
  u.LowPart = fad.nFileSizeLow;
  u.HighPart = fad.nFileSizeHigh;
  *out = u.QuadPart;
  return true;
#else
  struct stat st;
  if (::stat(path.c_str(), &st) != 0) return false;
  if (out) *out = static_cast<uint64_t>(st.st_size);
  return true;
#endif
}

#if defined(_WIN32)

bool WalkFiles(const std::string& root, std::vector<FileEntry>* out) {
  if (!out) return false;
  std::string pattern = root;
  if (!pattern.empty() && pattern.back() != '/' && pattern.back() != '\\')
    pattern += '\\';
  pattern += "*";

  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE) return false;

  do {
    if (std::strcmp(fd.cFileName, ".") == 0 || std::strcmp(fd.cFileName, "..") == 0)
      continue;
    std::string full = root;
    if (!full.empty() && full.back() != '/' && full.back() != '\\') full += '/';
    full += fd.cFileName;

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      // A subdirectory that cannot be opened should not abort the whole scan.
      WalkFiles(full, out);
    } else {
      FileEntry e;
      e.path = full;
      ULARGE_INTEGER u;
      u.LowPart = fd.nFileSizeLow;
      u.HighPart = fd.nFileSizeHigh;
      e.size = u.QuadPart;
      out->push_back(std::move(e));
    }
  } while (FindNextFileA(h, &fd));

  FindClose(h);
  return true;
}

#else

namespace {
bool WalkPosix(const std::string& dir, std::vector<FileEntry>* out) {
  DIR* d = ::opendir(dir.c_str());
  if (!d) return false;
  struct dirent* ent;
  while ((ent = ::readdir(d)) != nullptr) {
    if (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0)
      continue;
    std::string full = dir;
    if (!full.empty() && full.back() != '/') full += '/';
    full += ent->d_name;

    struct stat st;
    if (::stat(full.c_str(), &st) != 0) continue;
    if (S_ISDIR(st.st_mode)) {
      WalkPosix(full, out);
    } else if (S_ISREG(st.st_mode)) {
      FileEntry e;
      e.path = full;
      e.size = static_cast<uint64_t>(st.st_size);
      out->push_back(std::move(e));
    }
  }
  ::closedir(d);
  return true;
}
}  // namespace

bool WalkFiles(const std::string& root, std::vector<FileEntry>* out) {
  if (!out) return false;
  return WalkPosix(root, out);
}

#endif

}  // namespace mu
