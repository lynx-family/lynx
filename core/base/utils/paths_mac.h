// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_UTILS_PATHS_MAC_H_
#define CORE_BASE_UTILS_PATHS_MAC_H_
#include <string>
#include <utility>

namespace lynx {
namespace common {
std::pair<bool, std::string> GetResourceDirectoryPath();
}  // namespace common
}  // namespace lynx
#endif  // CORE_BASE_UTILS_PATHS_MAC_H_
