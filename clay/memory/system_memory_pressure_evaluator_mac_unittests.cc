// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// #include "system_memory_pressure_evaluator.h"
#include "clay/memory/system_memory_pressure_evaluator_mac.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

using MemoryPressureLevel = SystemMemoryPressureEvaluator::MemoryPressureLevel;

TEST(SystemMemoryPressureEvaluatorMacTest, TestPressureValues) {
  SystemMemoryPressureEvaluatorMac evaluator;
  MemoryPressureLevel current_level = evaluator.GetCurrentPressureLevel();
  EXPECT_TRUE(
      current_level == MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL ||
      current_level == MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE ||
      current_level == MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE);
}

TEST(SystemMemoryPressureEvaluatorMacTest, TestMapPressureLevel) {
  EXPECT_EQ(MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE,
            SystemMemoryPressureEvaluatorMac::MapPressureLevelForTest(
                DISPATCH_MEMORYPRESSURE_NORMAL));
  EXPECT_EQ(MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE,
            SystemMemoryPressureEvaluatorMac::MapPressureLevelForTest(
                DISPATCH_MEMORYPRESSURE_WARN));
  EXPECT_EQ(MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL,
            SystemMemoryPressureEvaluatorMac::MapPressureLevelForTest(
                DISPATCH_MEMORYPRESSURE_CRITICAL));
  EXPECT_EQ(MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE,
            SystemMemoryPressureEvaluatorMac::MapPressureLevelForTest(0));
  EXPECT_EQ(MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE,
            SystemMemoryPressureEvaluatorMac::MapPressureLevelForTest(3));
  EXPECT_EQ(MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE,
            SystemMemoryPressureEvaluatorMac::MapPressureLevelForTest(5));
  EXPECT_EQ(MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE,
            SystemMemoryPressureEvaluatorMac::MapPressureLevelForTest(-1));
}

}  // namespace clay
