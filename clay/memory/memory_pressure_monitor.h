// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_MEMORY_PRESSURE_MONITOR_H_
#define CLAY_MEMORY_MEMORY_PRESSURE_MONITOR_H_

#include <memory>

#include "base/include/fml/task_runner.h"
#include "clay/memory/memory_pressure_listener.h"
#include "clay/memory/system_memory_pressure_evaluator.h"

namespace clay {

// This monitor is not for OS Android. For Android, see
// io/flutter/embedding/engine/memory/MemoryPressureMonitor.java
class MemoryPressureMonitor {
 public:
  BASE_DISALLOW_COPY_AND_ASSIGN(MemoryPressureMonitor);
  using MemoryPressureLevel = MemoryPressureListener::MemoryPressureLevel;
  MemoryPressureMonitor() = default;
  ~MemoryPressureMonitor();
  static MemoryPressureMonitor& GetInstance();

  void SetTaskRunner(fml::RefPtr<fml::TaskRunner> task_runner) {
    task_runner_ = task_runner;
  }

  // Start memory pressure monitoring.
  bool StartMonitoring();

 private:
  std::unique_ptr<SystemMemoryPressureEvaluator>
      system_memory_pressure_evaluator_ = nullptr;
  fml::RefPtr<fml::TaskRunner> task_runner_;
};
}  // namespace clay

#endif  // CLAY_MEMORY_MEMORY_PRESSURE_MONITOR_H_
