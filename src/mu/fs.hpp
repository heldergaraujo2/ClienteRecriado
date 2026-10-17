// SPDX-License-Identifier: MIT
// Minimal portable directory walker. Only what the tooling needs: recursive
// enumeration of regular files with their sizes, on POSIX and Win32.
#ifndef MU_FS_HPP
#define MU_FS_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace mu {

struct FileEntry {
  std::string path;
  uint64_t size = 0;
};

/// Recursively collects every regular file under |root|. |out| paths are the
/// full path as built from |root|, using '/' as the separator on every
/// platform so that manifest paths compare directly.
/// Returns false if |root| cannot be opened.
bool WalkFiles(const std::string& root, std::vector<FileEntry>* out);

/// Reads a file's size without reading its contents. Returns false on failure.
bool FileSize(const std::string& path, uint64_t* out);

/// Lowercases an ASCII string; used for case-insensitive extension matching,
/// which matters because the reference client mixes .OZJ and .ozj.
std::string AsciiLower(std::string s);

/// Extension including the leading dot, lowercased. Empty when there is none.
std::string ExtensionOf(const std::string& path);

}  // namespace mu

#endif  // MU_FS_HPP
