// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <errno.h>
#include <sys/stat.h>

#include <cstdlib>

#include "base/src/application_directory/application_directory_impl.h"

namespace lynx {
namespace base {
namespace {

bool CreateDirectory(const char* path) {
  if (mkdir(path, 0700) == 0) return true;
  if (errno != EEXIST) return false;
  struct stat info {};
  return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

bool CreateDirectories(std::string& path) {
  for (size_t separator = path.find('/', 1); separator != std::string::npos;
       separator = path.find('/', separator + 1)) {
    if (path[separator - 1] == '/') continue;
    path[separator] = '\0';
    const bool created = CreateDirectory(path.c_str());
    path[separator] = '/';
    if (!created) return false;
  }
  return CreateDirectory(path.c_str());
}

}  // namespace

std::string GetDefaultApplicationDataDirectory() {
  const char* data_home = std::getenv("XDG_DATA_HOME");
  std::string directory;
  if (data_home && data_home[0] == '/') {
    directory = data_home;
  } else {
    const char* home = std::getenv("HOME");
    if (!home || home[0] != '/') return {};
    directory = home;
    if (directory.back() != '/') directory.push_back('/');
    directory += ".local/share";
  }
  if (directory.back() != '/') directory.push_back('/');
  directory += "lynx";
  return CreateDirectories(directory) ? directory : std::string{};
}

}  // namespace base
}  // namespace lynx
