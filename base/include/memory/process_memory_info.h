// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_MEMORY_PROCESS_MEMORY_INFO_H_
#define BASE_INCLUDE_MEMORY_PROCESS_MEMORY_INFO_H_

#include <stdint.h>

#include "base/include/base_defines.h"
#include "base/include/base_export.h"

#ifdef __cplusplus
namespace lynx {
namespace base {
#endif

// Returns the process PSS in bytes, or -1 on failure. Apple and Windows use
// their platform-equivalent process memory metrics.
BASE_EXTERN_C BASE_EXPORT int64_t GetProcessPssBytes(void);

#ifdef __cplusplus
}  // namespace base
}  // namespace lynx
#endif

#endif  // BASE_INCLUDE_MEMORY_PROCESS_MEMORY_INFO_H_
