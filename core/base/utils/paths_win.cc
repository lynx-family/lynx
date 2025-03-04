// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/utils/paths_win.h"

#include <windows.h>

#include <chrono>
#include <sstream>

#include "base/include/string/string_conversion_win.h"

namespace lynx {
namespace base {

namespace {
struct FileInfo {
  // The size of the file in bytes.  Undefined when is_directory is true.
  int64_t size = 0;
  // True if the file corresponds to a directory.
  bool is_directory = false;
  // True if the file corresponds to a symbolic link.  For Windows currently
  // not supported and thus always false.
  bool is_symbolic_link = false;
  // The last modified time of a file.
  std::chrono::system_clock::time_point last_modified;
  // The last accessed time of a file.
  std::chrono::system_clock::time_point last_accessed;
  // The creation time of a file.
  std::chrono::system_clock::time_point creation_time;
};
}  // namespace

size_t RootLength(const std::string& path) {
  if (path.empty()) return 0;
  if (path[0] == '/') return 1;
  if (path[0] == '\\') {
    if (path.size() < 2 || path[1] != '\\') return 1;
    // The path is a network share. Search for up to two '\'s, as they are
    // the server and share - and part of the root part.
    size_t index = path.find('\\', 2);
    if (index > 0) {
      index = path.find('\\', index + 1);
      if (index > 0) return index;
    }
    return path.size();
  }
  // If the path is of the form 'C:/' or 'C:\', with C being any letter, it's
  // a root part.
  if (path.length() >= 2 && path[1] == ':' &&
      (path[2] == '/' || path[2] == '\\') &&
      ((path[0] >= 'A' && path[0] <= 'Z') ||
       (path[0] >= 'a' && path[0] <= 'z'))) {
    return 3;
  }
  return 0;
}

size_t LastSeparator(const std::string& path) {
  return path.find_last_of("/\\");
}

std::string GetDirectoryName(const std::string& path) {
  size_t root_length = RootLength(path);
  size_t separator = LastSeparator(path);
  if (separator < root_length) separator = root_length;
  if (separator == std::string::npos) return {};
  return path.substr(0, separator);
}

std::pair<bool, std::string> GetExecutableDirectoryPath() {
  HMODULE module = GetModuleHandle(nullptr);
  if (module == nullptr) {
    return {false, ""};
  }
  wchar_t path[MAX_PATH];
  DWORD read_size = GetModuleFileName(module, path, MAX_PATH);
  if (read_size == 0 || read_size == MAX_PATH) {
    return {false, ""};
  }
  return {true, GetDirectoryName(Utf8FromUtf16(std::wstring{path, read_size}))};
}

bool DirectoryExists(const std::string& path) {
  DWORD fileattr = GetFileAttributes(Utf16FromUtf8(std::string(path)).c_str());
  if (fileattr != INVALID_FILE_ATTRIBUTES)
    return (fileattr & FILE_ATTRIBUTE_DIRECTORY) != 0;
  return false;
}

std::string JoinPaths(std::initializer_list<std::string> components) {
  std::stringstream stream;
  size_t i = 0;
  const size_t size = components.size();
  constexpr static char SEPARATOR = '\\';
  for (const auto& component : components) {
    i++;
    stream << component;
    if (component.back() == SEPARATOR) {
      continue;
    }
    if (i != size) {
      stream << SEPARATOR;
    }
  }
  return stream.str();
}

bool CreateDir(const std::string& path) {
  // If the path exists, we've succeeded if it's a directory, failed otherwise.
  const DWORD fileattr = ::GetFileAttributes(Utf16FromUtf8(path).c_str());
  if (fileattr != INVALID_FILE_ATTRIBUTES) {
    if ((fileattr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      return true;
    }
    return false;
  }

  // Attempt to create the parent recursively.  This will immediately return
  // true if it already exists, otherwise will create all required parent
  // directories starting with the highest-level missing parent.
  auto parent_path = GetDirectoryName(std::string(path));
  if (parent_path == path) {
    return false;
  }
  if (!CreateDir(parent_path)) {
    return false;
  }

  if (::CreateDirectory(Utf16FromUtf8(path).c_str(), nullptr)) {
    return true;
  }
  const DWORD error_code = ::GetLastError();
  if (error_code == ERROR_ALREADY_EXISTS && DirectoryExists(path)) {
    // This error code ERROR_ALREADY_EXISTS doesn't indicate whether we were
    // racing with someone creating the same directory, or a file with the same
    // path.  If DirectoryExists() returns true, we lost the race to create the
    // same directory.
    return true;
  }
  return false;
}

std::chrono::system_clock::time_point FileTime2TimePoint(const FILETIME& ft) {
  SYSTEMTIME st = {0};
  if (!FileTimeToSystemTime(&ft, &st)) {
    return std::chrono::system_clock::time_point(
        (std::chrono::system_clock::time_point::min)());
  }

  // number of seconds
  ULARGE_INTEGER ull;
  ull.LowPart = ft.dwLowDateTime;
  ull.HighPart = ft.dwHighDateTime;

  time_t secs = ull.QuadPart / 10000000ULL - 11644473600ULL;
  std::chrono::milliseconds ms((ull.QuadPart / 10000ULL) % 1000);
  auto tp = std::chrono::system_clock::from_time_t(secs);
  tp += ms;
  return tp;
}

bool GetFileInfo(const std::string& file_path, FileInfo& results) {
  WIN32_FILE_ATTRIBUTE_DATA attr;
  if (!GetFileAttributesEx(Utf16FromUtf8(std::string(file_path)).c_str(),
                           GetFileExInfoStandard, &attr)) {
    return false;
  }

  ULARGE_INTEGER size;
  size.HighPart = attr.nFileSizeHigh;
  size.LowPart = attr.nFileSizeLow;
  results.size = size.QuadPart;

  results.is_directory =
      (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  results.last_modified = FileTime2TimePoint(attr.ftLastWriteTime);
  results.last_accessed = FileTime2TimePoint(attr.ftLastAccessTime);
  results.creation_time = FileTime2TimePoint(attr.ftCreationTime);
  return true;
}

bool GetFileSize(const std::string& file_path, int64_t& file_size) {
  FileInfo info{};
  if (!GetFileInfo(file_path, info)) {
    return false;
  }
  file_size = info.size;
  return true;
}

}  // namespace base
}  // namespace lynx
