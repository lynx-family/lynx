// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_UTILS_PATHS_WIN_H_
#define CORE_BASE_UTILS_PATHS_WIN_H_
#include <string>
#include <utility>

namespace lynx {
namespace base {
std::pair<bool, std::string> GetExecutableDirectoryPath();
bool DirectoryExists(const std::string& path);
std::string JoinPaths(std::initializer_list<std::string> components);
bool CreateDir(const std::string& path);
bool GetFileSize(const std::string& file_path, int64_t& file_size);
}  // namespace base
}  // namespace lynx
#endif  // CORE_BASE_UTILS_PATHS_WIN_H_
