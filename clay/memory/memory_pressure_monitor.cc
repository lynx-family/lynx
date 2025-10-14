// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/memory/memory_pressure_monitor.h"

#include "base/include/no_destructor.h"

namespace clay {

MemoryPressureMonitor& MemoryPressureMonitor::GetInstance() {
  static fml::NoDestructor<MemoryPressureMonitor> instance{};
  return *instance;
}

bool MemoryPressureMonitor::StartMonitoring() {
  // Creation of evaluator also starts monitoring.
  if (!system_memory_pressure_evaluator_) {
    system_memory_pressure_evaluator_ =
        SystemMemoryPressureEvaluator::CreateSystemSpecificEvaluator(
            task_runner_);
  }
  return system_memory_pressure_evaluator_ != nullptr;
}

MemoryPressureMonitor::~MemoryPressureMonitor() {
  system_memory_pressure_evaluator_.reset();
}

}  // namespace clay
