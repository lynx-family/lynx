// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/memory/memory_pressure_listener.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
using MemoryPressureLevel = MemoryPressureListener::MemoryPressureLevel;

class MockMemoryPressureListener : public MemoryPressureListener {
 public:
  MockMemoryPressureListener(
      const MemoryPressureCallback& memory_pressure_callback,
      fml::RefPtr<fml::TaskRunner> task_runner)
      : MemoryPressureListener(memory_pressure_callback, task_runner) {}

  MockMemoryPressureListener(
      const MemoryPressureCallback& memory_pressure_callback,
      const SyncMemoryPressureCallback& sync_memory_pressure_callback,
      fml::RefPtr<fml::TaskRunner> task_runner)
      : MemoryPressureListener(memory_pressure_callback,
                               sync_memory_pressure_callback, task_runner) {}
  virtual ~MockMemoryPressureListener() = default;
  MOCK_METHOD(void, Notify, (MemoryPressureLevel memory_pressure_level),
              (override));
};

class MemoryPressureListenerTest : public testing::Test {
 public:
  MemoryPressureListenerTest() = default;

  void SetUp() override {
    listener_ = std::make_unique<MockMemoryPressureListener>(
        [this](MemoryPressureLevel level) { OnMemoryPressure(level); },
        nullptr);
  }

  void TearDown() override { listener_.reset(); }

  MOCK_METHOD1(OnMemoryPressure, void(MemoryPressureLevel));

 protected:
  std::unique_ptr<MockMemoryPressureListener> listener_;
};

TEST_F(MemoryPressureListenerTest, TestNotifyMemoryPressure) {
  {
    EXPECT_CALL(*listener_, Notify).Times(1);
    MemoryPressureListener::NotifyMemoryPressure(
        MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE);
  }

  {
    EXPECT_CALL(*listener_, Notify).Times(1);
    MemoryPressureListener::NotifyMemoryPressure(
        MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL);
  }

  {
    EXPECT_CALL(*listener_, Notify).Times(0);
    MemoryPressureListener::NotifyMemoryPressure(
        MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE);
  }
}

TEST_F(MemoryPressureListenerTest, TestAddRemoveListener) {
  MemoryPressureListener* listener_raw_ptr = listener_.get();
  EXPECT_TRUE(
      MemoryPressureListener::TestIfListenerExistsAsync(listener_raw_ptr));
  EXPECT_FALSE(
      MemoryPressureListener::TestIfListenerExistsSync(listener_raw_ptr));
  listener_.reset();
  EXPECT_FALSE(
      MemoryPressureListener::TestIfListenerExistsAsync(listener_raw_ptr));
  EXPECT_FALSE(
      MemoryPressureListener::TestIfListenerExistsSync(listener_raw_ptr));

  MemoryPressureListener::MemoryPressureCallback async_callback =
      [](MemoryPressureLevel level) {};
  MemoryPressureListener::MemoryPressureCallback sync_callback =
      [](MemoryPressureLevel level) {};

  auto listener2 = std::make_unique<MemoryPressureListener>(
      async_callback, sync_callback, nullptr);
  MemoryPressureListener* listener2_raw_ptr = listener2.get();
  EXPECT_TRUE(
      MemoryPressureListener::TestIfListenerExistsAsync(listener2_raw_ptr));
  EXPECT_TRUE(
      MemoryPressureListener::TestIfListenerExistsSync(listener2_raw_ptr));

  listener2.reset();
  EXPECT_FALSE(
      MemoryPressureListener::TestIfListenerExistsAsync(listener2_raw_ptr));
  EXPECT_FALSE(
      MemoryPressureListener::TestIfListenerExistsSync(listener2_raw_ptr));
}

}  // namespace clay
