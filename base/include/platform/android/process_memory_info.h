// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_PLATFORM_ANDROID_PROCESS_MEMORY_INFO_H_
#define BASE_INCLUDE_PLATFORM_ANDROID_PROCESS_MEMORY_INFO_H_

#include <string>
#include <vector>

#include "base/include/base_export.h"

namespace lynx {
namespace base {
namespace android {

// Returns alternating Android memory statistic names and kilobyte values.
BASE_EXPORT std::vector<std::string> GetProcessMemoryInfo();

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_PLATFORM_ANDROID_PROCESS_MEMORY_INFO_H_
