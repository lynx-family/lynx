// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_SYSTEM_MEMORY_PRESSURE_EVALUATOR_WIN_H_
#define CLAY_MEMORY_SYSTEM_MEMORY_PRESSURE_EVALUATOR_WIN_H_

#include <windows.h>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_delta.h"
#include "clay/memory/system_memory_pressure_evaluator.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {
class SystemMemoryPressureEvaluatorWin : public SystemMemoryPressureEvaluator {
 public:
  BASE_DISALLOW_COPY_AND_ASSIGN(SystemMemoryPressureEvaluatorWin);

  explicit SystemMemoryPressureEvaluatorWin(
      fml::RefPtr<fml::TaskRunner> task_runner);
  SystemMemoryPressureEvaluatorWin() : weak_factory_(this) {}
  ~SystemMemoryPressureEvaluatorWin();

  bool CheckMemoryPressure();
  // Declared as virtual for testing.
  virtual void PeriodicallyCheck();

  void SetEnablePolling(bool value) { enable_polling_ = value; }
  bool EnablePolling() const { return enable_polling_; }

  void UnregisterWaitInsideCallback();

 private:
  FRIEND_TEST(SystemMemoryPressureEvaluatorWinTest, TestPolling);
  void StartObserving() override;

  // Declared as virtual for testing.
  virtual bool TryOsSignalObserving();
  virtual void InferThresholds();

  bool GetSystemMemoryStatus(MEMORYSTATUSEX* mem_info);

  MemoryPressureListener::MemoryPressureLevel CalculateCurrentPressureLevel();

  // The time which should pass between 2 successive moderate memory pressure
  // signals.
  static const fml::TimeDelta kModeratePressureCooldown;

  // Constants governing the memory pressure level detection.

  // The amount of total system memory beyond which a system is considered
  // to be a large-memory system.
  static const int kLargeMemoryThresholdMb;
  // Default minimum free memory thresholds for small-memory systems, in MB.
  static const int kSmallMemoryDefaultModerateThresholdMb;
  static const int kSmallMemoryDefaultCriticalThresholdMb;
  // Default minimum free memory thresholds for large-memory systems, in MB.
  static const int kLargeMemoryDefaultModerateThresholdMb;
  static const int kLargeMemoryDefaultCriticalThresholdMb;

  // The memory sampling period, currently 5s.
  static const fml::TimeDelta kMemorySamplingPeriod;

  int moderate_pressure_repeat_count_;

  // Threshold amounts of available memory that trigger pressure levels.
  int moderate_threshold_mb_;
  int critical_threshold_mb_;
  fml::RefPtr<fml::TaskRunner> task_runner_;

  bool enable_polling_ = false;
  bool enable_os_signal_ = false;

  HANDLE low_memory_handle_ = nullptr;
  HANDLE wait_handle_ = nullptr;

  fml::WeakPtrFactory<SystemMemoryPressureEvaluatorWin> weak_factory_;
};
}  // namespace clay
#endif  // CLAY_MEMORY_SYSTEM_MEMORY_PRESSURE_EVALUATOR_WIN_H_
