// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_PAGE_KEY_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_PAGE_KEY_H_

#include <string>

namespace lynx {
namespace tasm {
namespace performance {

// Returns the platform-defined fallback key used when host page_id is empty.
std::string NormalizeMemoryPageUrl(const std::string& url);

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_PAGE_KEY_H_
