// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_UI_MEMORY_USAGE_H_
#define CORE_PUBLIC_UI_MEMORY_USAGE_H_

#include <cstdint>
#include <string>
#include <unordered_map>

namespace lynx {
namespace tasm {

struct UIMemoryUsage {
  int64_t size_bytes = 0;
  int32_t instance_count = 0;
};

using UIMemoryUsageByTag = std::unordered_map<std::string, UIMemoryUsage>;

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_UI_MEMORY_USAGE_H_
