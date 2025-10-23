// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/memory/system_memory_pressure_evaluator.h"

#include "build/build_config.h"

#if OS_WIN
#include "clay/memory/system_memory_pressure_evaluator_win.h"
#elif OS_MAC
#include "clay/memory/system_memory_pressure_evaluator_mac.h"
#endif
#include "clay/fml/logging.h"

namespace clay {

// static
std::unique_ptr<SystemMemoryPressureEvaluator>
SystemMemoryPressureEvaluator::CreateSystemSpecificEvaluator(
    fml::RefPtr<fml::TaskRunner> task_runner) {
#ifdef OS_WIN
  // Windows cannot simply use fml::MessageLoop::GetCurrent().GetTaskRunner() to
  // get current thread's task runner, so we need to pass it.
  auto evaluator =
      std::make_unique<SystemMemoryPressureEvaluatorWin>(task_runner);
  return evaluator;
#elif OS_MAC
  auto evaluator = std::make_unique<SystemMemoryPressureEvaluatorMac>();
  return evaluator;
#else
#error "SystemMemoryPressureEvaluator only supports Windows and MacOS for now";
#endif
}

void SystemMemoryPressureEvaluator::SendCurrentPressureLevel(
    bool notify_listeners) {
  if (notify_listeners) {
    MemoryPressureListener::NotifyMemoryPressure(current_pressure_level_);
  }
}

}  // namespace clay
