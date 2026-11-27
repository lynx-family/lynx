// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_GRAPHICS_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_GRAPHICS_H_

#include <string_view>

namespace lynx {
namespace tasm {
namespace performance {

// Classifies graphics memory using the platform's component categories.
bool IsGraphicsMemoryCategory(std::string_view category);

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_GRAPHICS_H_
