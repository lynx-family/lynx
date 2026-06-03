// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/notification_center.h"

#include "base/include/memory/memory_pressure_level.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(NotificationCallbackTest, NotifyMemoryPressure) {
  int count = 0;
  NotificationCallback listener(
      MEMORY_PRESSURE_NOTIFICATION,
      [&count](const std::string& tag, intptr_t data) { ++count; });

  NotificationCallback::Notify(
      MEMORY_PRESSURE_NOTIFICATION,
      static_cast<intptr_t>(
          MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE));
  EXPECT_EQ(1, count);

  NotificationCallback::Notify(
      MEMORY_PRESSURE_NOTIFICATION,
      static_cast<intptr_t>(MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE));
  EXPECT_EQ(2, count);

  NotificationCallback::Notify(
      MEMORY_PRESSURE_NOTIFICATION,
      static_cast<intptr_t>(
          MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL));
  EXPECT_EQ(3, count);
}

TEST(NotificationCallbackTest, UnregisterOnDestruction) {
  int count = 0;
  {
    NotificationCallback listener(
        MEMORY_PRESSURE_NOTIFICATION,
        [&count](const std::string& tag, intptr_t data) { ++count; });
    NotificationCallback::Notify(
        MEMORY_PRESSURE_NOTIFICATION,
        static_cast<intptr_t>(
            MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE));
  }

  EXPECT_EQ(1, count);

  NotificationCallback::Notify(
      MEMORY_PRESSURE_NOTIFICATION,
      static_cast<intptr_t>(
          MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL));
  EXPECT_EQ(1, count);
}

TEST(NotificationCallbackTest, MultipleListeners) {
  int count1 = 0;
  int count2 = 0;
  NotificationCallback listener1(
      MEMORY_PRESSURE_NOTIFICATION,
      [&count1](const std::string& tag, intptr_t data) { ++count1; });
  NotificationCallback listener2(
      MEMORY_PRESSURE_NOTIFICATION,
      [&count2](const std::string& tag, intptr_t data) { ++count2; });

  NotificationCallback::Notify(
      MEMORY_PRESSURE_NOTIFICATION,
      static_cast<intptr_t>(
          MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE));
  EXPECT_EQ(1, count1);
  EXPECT_EQ(1, count2);
}

static constexpr const char* CPU_PRESSURE_NOTIFICATION = "cpu_pressure";

TEST(NotificationCallbackTest, MultipleTags) {
  intptr_t memory_arg1 = 0;
  intptr_t memory_arg2 = 0;
  intptr_t cpu_arg1 = 0;
  intptr_t cpu_arg2 = 0;
  NotificationCallback listener1(
      {{MEMORY_PRESSURE_NOTIFICATION,
        [&memory_arg1](const std::string& tag, intptr_t data) {
          EXPECT_EQ(tag, MEMORY_PRESSURE_NOTIFICATION);
          memory_arg1 = data;
        }},
       {CPU_PRESSURE_NOTIFICATION,
        [&cpu_arg1](const std::string& tag, intptr_t data) {
          EXPECT_EQ(tag, CPU_PRESSURE_NOTIFICATION);
          cpu_arg1 = data;
        }}});
  NotificationCallback listener2(
      {{MEMORY_PRESSURE_NOTIFICATION,
        [&memory_arg2](const std::string& tag, intptr_t data) {
          EXPECT_EQ(tag, MEMORY_PRESSURE_NOTIFICATION);
          memory_arg2 = data;
        }},
       {CPU_PRESSURE_NOTIFICATION,
        [&cpu_arg2](const std::string& tag, intptr_t data) {
          EXPECT_EQ(tag, CPU_PRESSURE_NOTIFICATION);
          cpu_arg2 = data;
        }}});

  NotificationCallback::Notify(
      MEMORY_PRESSURE_NOTIFICATION,
      static_cast<intptr_t>(
          MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE));
  EXPECT_EQ(static_cast<intptr_t>(
                MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE),
            memory_arg1);
  EXPECT_EQ(static_cast<intptr_t>(
                MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE),
            memory_arg2);
  EXPECT_EQ(0, cpu_arg1);
  EXPECT_EQ(0, cpu_arg2);

  NotificationCallback::Notify(CPU_PRESSURE_NOTIFICATION, 100);
  EXPECT_EQ(100, cpu_arg1);
  EXPECT_EQ(100, cpu_arg2);
}

}  // namespace base
}  // namespace lynx
