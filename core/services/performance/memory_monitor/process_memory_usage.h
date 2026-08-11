// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_PROCESS_MEMORY_USAGE_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_PROCESS_MEMORY_USAGE_H_

#include <cstdint>

namespace lynx {
namespace tasm {
namespace performance {

// Process PSS estimate in bytes, or -1 on failure. Apple/Windows use their
// existing platform equivalents. Shared by high-water triggers and ratios.
// Call on a background thread: reading PSS can walk process mappings.
int64_t ReadProcessPssBytes();

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_PROCESS_MEMORY_USAGE_H_
