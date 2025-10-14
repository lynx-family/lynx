// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_SYSTEM_MEMORY_PRESSURE_EVALUATOR_MAC_H_
#define CLAY_MEMORY_SYSTEM_MEMORY_PRESSURE_EVALUATOR_MAC_H_

#include <dispatch/dispatch.h>

#include "base/include/fml/time/time_delta.h"
#include "clay/memory/memory_pressure_listener.h"
#include "clay/memory/system_memory_pressure_evaluator.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class SystemMemoryPressureEvaluatorMac : public SystemMemoryPressureEvaluator {
 public:
  static const float kCriticalPressureThreshold;
  static const float kModeratePressureThreshold;
  static const fml::TimeDelta kReCheckMemoryPeriod;
  using MemoryPressureLevel = MemoryPressureListener::MemoryPressureLevel;

  SystemMemoryPressureEvaluatorMac();
  ~SystemMemoryPressureEvaluatorMac() override;
  BASE_DISALLOW_COPY_AND_ASSIGN(SystemMemoryPressureEvaluatorMac);

 private:
  FRIEND_TEST(SystemMemoryPressureEvaluatorMacTest, TestMapPressureLevel);
  static MemoryPressureListener::MemoryPressureLevel MapPressureLevelForTest(
      dispatch_source_memorypressure_flags_t mac_level);

  void StartObserving() override;
  void StopObserving() override;
  void OnMemoryPressureChanged();

  void CheckMemoryPressure();
  bool CalculatePressureLevelByAvailableMemory(int* pressure_level);

  uint64_t GetTotalMemory();

  dispatch_source_t memory_event_source_;
  fml::RefPtr<fml::TaskRunner> task_runner_;
};

}  // namespace clay

#endif  // CLAY_MEMORY_SYSTEM_MEMORY_PRESSURE_EVALUATOR_MAC_H_
