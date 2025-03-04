// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/event/event_listener_map.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifndef CORE_EVENT_EVENT_LISTENER_MAP_TEST_H_
#define CORE_EVENT_EVENT_LISTENER_MAP_TEST_H_

namespace lynx {
namespace event {
namespace test {

class EventListenerMapTest : public ::testing::Test {
 public:
  EventListenerMapTest() = default;
  ~EventListenerMapTest() override = default;

  void SetUp() override { map_ = std::make_unique<EventListenerMap>(); }

  void TearDown() override {}

  std::unique_ptr<EventListenerMap> map_;
};

}  // namespace test
}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_EVENT_LISTENER_MAP_TEST_H_
