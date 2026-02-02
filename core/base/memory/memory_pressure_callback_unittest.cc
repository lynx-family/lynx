// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/memory/memory_pressure_callback.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(MemoryPressureCallbackTest, NotifyMemoryPressure) {
  int count = 0;
  MemoryPressureCallback listener(
      [&count](MemoryPressureLevel level) { ++count; });

  MemoryPressureCallback::NotifyMemoryPressure(
      MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE);
  EXPECT_EQ(1, count);

  MemoryPressureCallback::NotifyMemoryPressure(
      MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE);
  EXPECT_EQ(1, count);

  MemoryPressureCallback::NotifyMemoryPressure(
      MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL);
  EXPECT_EQ(2, count);
}

TEST(MemoryPressureCallbackTest, UnregisterOnDestruction) {
  int count = 0;
  {
    MemoryPressureCallback listener(
        [&count](MemoryPressureLevel level) { ++count; });
    MemoryPressureCallback::NotifyMemoryPressure(
        MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE);
  }

  EXPECT_EQ(1, count);

  MemoryPressureCallback::NotifyMemoryPressure(
      MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL);
  EXPECT_EQ(1, count);
}

TEST(MemoryPressureCallbackTest, MultipleListeners) {
  int count1 = 0;
  int count2 = 0;
  MemoryPressureCallback listener1(
      [&count1](MemoryPressureLevel level) { ++count1; });
  MemoryPressureCallback listener2(
      [&count2](MemoryPressureLevel level) { ++count2; });

  MemoryPressureCallback::NotifyMemoryPressure(
      MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE);
  EXPECT_EQ(1, count1);
  EXPECT_EQ(1, count2);
}

}  // namespace base
}  // namespace lynx
