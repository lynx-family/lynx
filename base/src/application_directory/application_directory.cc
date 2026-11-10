// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/application_directory.h"

#include <mutex>
#include <utility>

#if defined(_WIN32)
#include <windows.h>

#include "base/include/string/string_conversion_win.h"
#else
#include <sys/stat.h>

#include <cerrno>
#endif

#include "base/include/no_destructor.h"
#include "base/src/application_directory/application_directory_impl.h"

namespace lynx {
namespace base {
namespace {

struct ApplicationDirectory {
  std::mutex mutex;
  std::string path;
};

ApplicationDirectory& Directory() {
  static NoDestructor<ApplicationDirectory> directory;
  return *directory;
}

}  // namespace

std::string GetApplicationDataDirectory() {
  auto& directory = Directory();
  std::lock_guard<std::mutex> lock(directory.mutex);
  if (directory.path.empty()) {
    directory.path = GetDefaultApplicationDataDirectory();
  }
  return directory.path;
}

void SetApplicationDataDirectory(std::string path) {
  auto& directory = Directory();
  std::lock_guard<std::mutex> lock(directory.mutex);
  directory.path = std::move(path);
}

std::string GetApplicationDataSubDirectory(const std::string& name) {
  if (name.empty() || name == "." || name == ".." ||
      name.find_first_of("/\\") != std::string::npos ||
      name.find('\0') != std::string::npos) {
    return {};
  }
  auto directory = GetApplicationDataDirectory();
  if (directory.empty()) return {};
  if (directory.back() != '/'
#if defined(_WIN32)
      && directory.back() != '\\'
#endif
  ) {
    directory.push_back('/');
  }
  directory += name;
#if defined(_WIN32)
  const auto path = Utf16FromUtf8(directory);
  if (!CreateDirectoryW(path.c_str(), nullptr)) {
    if (GetLastError() != ERROR_ALREADY_EXISTS) return {};
    const auto attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES ||
        !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
      return {};
    }
  }
#else
  if (mkdir(directory.c_str(), 0700) != 0) {
    if (errno != EEXIST) return {};
    struct stat info {};
    if (stat(directory.c_str(), &info) != 0 || !S_ISDIR(info.st_mode)) {
      return {};
    }
  }
#endif
  return directory;
}

}  // namespace base
}  // namespace lynx
