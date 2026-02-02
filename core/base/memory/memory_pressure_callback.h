// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_MEMORY_MEMORY_PRESSURE_CALLBACK_H_
#define CORE_BASE_MEMORY_MEMORY_PRESSURE_CALLBACK_H_

#include <functional>

#include "base/include/memory/memory_pressure_level.h"

namespace lynx {
namespace base {

class MemoryPressureCallback {
 public:
  using Callback = std::function<void(MemoryPressureLevel)>;

  static void NotifyMemoryPressure(MemoryPressureLevel memory_pressure_level);

  explicit MemoryPressureCallback(Callback callback);

  ~MemoryPressureCallback();
  void OnLowMemory(MemoryPressureLevel memory_pressure_level);

 private:
  Callback callback_;
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_MEMORY_MEMORY_PRESSURE_CALLBACK_H_
