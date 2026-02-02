// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_MEMORY_MEMORY_PRESSURE_LEVEL_H_
#define BASE_INCLUDE_MEMORY_MEMORY_PRESSURE_LEVEL_H_

#include "base/include/base_export.h"

namespace lynx {
namespace base {

// A Java counterpart is in
// lynx/base/platform/android/src/main/java/com/lynx/base/memory/MemoryPressureLevel.java
enum BASE_EXPORT MemoryPressureLevel {
  // No problems, there is enough memory to use. This event is not sent via
  // callback.
  MEMORY_PRESSURE_LEVEL_NONE = 0,

  // Modules are advised to free buffers that are cheap to re-allocate and not
  // immediately needed.
  MEMORY_PRESSURE_LEVEL_MODERATE = 1,

  // At this level, modules are advised to free all possible memory.  The
  // alternative is to be killed by the system, which means all memory will
  // have to be re-created, plus the cost of a cold start.
  MEMORY_PRESSURE_LEVEL_CRITICAL = 2,

  // This must be the last value in the enum. The casing is different from the
  // other values to make this enum work well with the
  // UMA_HISTOGRAM_ENUMERATION macro.
  kMaxValue = MEMORY_PRESSURE_LEVEL_CRITICAL,
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_MEMORY_MEMORY_PRESSURE_LEVEL_H_
