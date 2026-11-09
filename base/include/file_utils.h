// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FILE_UTILS_H_
#define BASE_INCLUDE_FILE_UTILS_H_

#include <cstdio>
#include <string>

#include "base/include/base_export.h"

namespace lynx {
namespace base {

// fopen/remove equivalents accepting UTF-8 paths, including on Windows.
BASE_EXPORT std::FILE* OpenFile(const std::string& path, const char* mode);
BASE_EXPORT int RemoveFile(const std::string& path);

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_FILE_UTILS_H_
