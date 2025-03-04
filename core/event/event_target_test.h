// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/event/event_target.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifndef CORE_EVENT_EVENT_TARGET_TEST_H_
#define CORE_EVENT_EVENT_TARGET_TEST_H_

namespace lynx {
namespace event {
namespace test {

class MockEventTarget : public EventTarget {
 public:
  MockEventTarget() = default;
  virtual ~MockEventTarget() override = default;

  virtual EventTarget* GetParentTarget() override;
};

class EventTargetTest : public ::testing::Test {
 public:
  EventTargetTest() = default;
  ~EventTargetTest() override = default;

  void SetUp() override { mock_target_ = std::make_unique<MockEventTarget>(); }

  void TearDown() override {}

  std::unique_ptr<MockEventTarget> mock_target_;
};

}  // namespace test
}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_EVENT_TARGET_TEST_H_
