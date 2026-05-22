// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_SYSTEM_MEMORY_PRESSURE_EVALUATOR_H_
#define CLAY_MEMORY_SYSTEM_MEMORY_PRESSURE_EVALUATOR_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "base/include/fml/task_runner.h"
#include "clay/memory/memory_pressure_listener.h"

namespace clay {

class SystemMemoryPressureEvaluator {
 public:
  using MemoryPressureLevel = MemoryPressureListener::MemoryPressureLevel;
  BASE_DISALLOW_COPY_AND_ASSIGN(SystemMemoryPressureEvaluator);

  SystemMemoryPressureEvaluator() = default;
  virtual ~SystemMemoryPressureEvaluator() = default;

  // Create a system specific pressure evaluator;
  static std::unique_ptr<SystemMemoryPressureEvaluator>
  CreateSystemSpecificEvaluator(fml::RefPtr<fml::TaskRunner> task_runner);

  void SetCurrentPressureLevel(MemoryPressureLevel level) {
    current_pressure_level_ = level;
  }

  MemoryPressureLevel GetCurrentPressureLevel() const {
    return current_pressure_level_;
  }

  void SendCurrentPressureLevel(bool notify_listeners);

 private:
  virtual void StartObserving() {}
  virtual void StopObserving() {}

  MemoryPressureLevel current_pressure_level_ =
      MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE;
};

}  // namespace clay

#endif  // CLAY_MEMORY_SYSTEM_MEMORY_PRESSURE_EVALUATOR_H_
