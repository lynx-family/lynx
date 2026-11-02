// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/touch_event.h"

#include <atomic>
#include <memory>
#include <thread>
#include <utility>

#include "core/event/event_dispatcher.h"
#include "core/event/event_listener_test.h"
#include "core/event/event_target_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace event {
namespace test {
namespace {

class TouchEventTarget : public MockEventTarget {
 public:
  explicit TouchEventTarget(TouchEventState* state = nullptr,
                            bool compatible = false)
      : state_(state ? state : &default_state_), compatible_(compatible) {}

  TouchEventState* GetTouchEventState() override { return state_; }
  bool GetEnableMultiTouchParamsCompatible() override { return compatible_; }

 private:
  TouchEventState default_state_;
  TouchEventState* state_;
  bool compatible_;
};

lepus::Value TouchParams(int identifier, float x) {
  auto point = lepus::CArray::Create();
  point->push_back(lepus::Value(identifier));
  for (int i = 0; i < 6; ++i) {
    point->push_back(lepus::Value(x));
  }
  auto points = lepus::CArray::Create();
  points->push_back(lepus::Value(std::move(point)));
  auto targets = lepus::Dictionary::Create();
  targets->SetValue("1", lepus::Value(std::move(points)));
  return lepus::Value(std::move(targets));
}

fml::RefPtr<TouchEvent> DispatchTouch(TouchEventTarget& target,
                                      const char* name, int identifier = 0,
                                      float x = 0.f) {
  auto event =
      fml::MakeRefCounted<TouchEvent>(name, TouchParams(identifier, x));
  EventDispatcher::DispatchEvent(target, event);
  return event;
}

void ExpectTouch(const lepus::Value& touches, int identifier, float x) {
  ASSERT_TRUE(touches.IsArray());
  ASSERT_EQ(touches.GetLength(), 1u);
  const auto touch = touches.GetProperty(0);
  EXPECT_EQ(touch.GetProperty("identifier").Number(), identifier);
  EXPECT_FLOAT_EQ(touch.GetProperty("x").Number(), x);
}

}  // namespace

TEST(TouchEventTest, MissingPageStateStopsTouchDispatch) {
  for (const char* name : {EVENT_TOUCH_START, EVENT_TOUCH_MOVE, EVENT_TOUCH_END,
                           EVENT_TOUCH_CANCEL, EVENT_TAP, EVENT_LONG_PRESS}) {
    SCOPED_TRACE(name);
    MockEventTarget target;
    auto listener = std::make_shared<MockEventListener>(
        EventListener::Type::kJSClosureEventListener, name);
    target.AddEventListener(name, listener);
    auto event = fml::MakeRefCounted<TouchEvent>(name, TouchParams(0, 10.f));

    auto result = EventDispatcher::DispatchEvent(target, event);
    EXPECT_TRUE(result.IsCanceled());
    EXPECT_FALSE(result.consumed);
    EXPECT_EQ(listener->GetCount(), 0);
  }
}

TEST(TouchEventTest, PagesDoNotShareActiveTouches) {
  TouchEventTarget first;
  TouchEventTarget second;
  DispatchTouch(first, EVENT_TOUCH_START, 0, 10.f);
  auto second_start = DispatchTouch(second, EVENT_TOUCH_START, 0, 20.f);
  ExpectTouch(second_start->detail().GetProperty("touches"), 0, 20.f);

  DispatchTouch(first, EVENT_TOUCH_CANCEL);
  auto second_move = DispatchTouch(second, EVENT_TOUCH_MOVE, 0, 30.f);
  ExpectTouch(second_move->detail().GetProperty("touches"), 0, 30.f);

  // Previously delivered event details must remain snapshots.
  ExpectTouch(second_start->detail().GetProperty("touches"), 0, 20.f);
}

TEST(TouchEventTest, TargetsOnOnePageShareActiveTouches) {
  TouchEventState state;
  TouchEventTarget first(&state);
  TouchEventTarget second(&state);
  DispatchTouch(first, EVENT_TOUCH_START, 0, 10.f);
  auto second_start = DispatchTouch(second, EVENT_TOUCH_START, 1, 20.f);
  EXPECT_EQ(second_start->detail().GetProperty("touches").GetLength(), 2u);

  auto first_end = DispatchTouch(first, EVENT_TOUCH_END, 0, 15.f);
  ExpectTouch(first_end->detail().GetProperty("touches"), 1, 20.f);
  ExpectTouch(first_end->detail().GetProperty("changedTouches"), 0, 15.f);
}

TEST(TouchEventTest, ActiveTouchesFollowPageAcrossThreads) {
  TouchEventState state;
  std::thread first([&state] {
    TouchEventTarget target(&state);
    DispatchTouch(target, EVENT_TOUCH_START, 0, 10.f);
  });
  first.join();

  std::thread second([&state] {
    TouchEventTarget target(&state);
    auto move = DispatchTouch(target, EVENT_TOUCH_MOVE, 0, 20.f);
    ExpectTouch(move->detail().GetProperty("touches"), 0, 20.f);
    auto end = DispatchTouch(target, EVENT_TOUCH_END, 0, 30.f);
    EXPECT_EQ(end->detail().GetProperty("touches").GetLength(), 0u);
  });
  second.join();
}

TEST(TouchEventTest, ConcurrentPagesKeepTouchSequencesIndependent) {
  std::atomic<int> ready{0};
  auto run_page = [&ready](float x) {
    TouchEventTarget target;
    ready.fetch_add(1, std::memory_order_release);
    while (ready.load(std::memory_order_acquire) != 2) {
      std::this_thread::yield();
    }
    for (int i = 0; i < 32; ++i) {
      auto start = DispatchTouch(target, EVENT_TOUCH_START, 0, x);
      ExpectTouch(start->detail().GetProperty("touches"), 0, x);
      auto move = DispatchTouch(target, EVENT_TOUCH_MOVE, 0, x + 1.f);
      ExpectTouch(move->detail().GetProperty("touches"), 0, x + 1.f);
      auto end = DispatchTouch(target, EVENT_TOUCH_END, 0, x + 2.f);
      EXPECT_EQ(end->detail().GetProperty("touches").GetLength(), 0u);
      DispatchTouch(target, EVENT_TOUCH_START, 0, x);
      auto cancel = DispatchTouch(target, EVENT_TOUCH_CANCEL);
      EXPECT_EQ(cancel->detail().GetProperty("touches").GetLength(), 0u);
      EXPECT_EQ(cancel->detail().GetProperty("changedTouches").GetLength(), 0u);
    }
  };
  std::thread first(run_page, 10.f);
  std::thread second(run_page, 20.f);
  first.join();
  second.join();
}

TEST(TouchEventTest, PreservesTouchEndCompatibilityAndCancelParameters) {
  for (bool compatible : {false, true}) {
    SCOPED_TRACE(compatible);
    TouchEventTarget target(nullptr, compatible);
    DispatchTouch(target, EVENT_TOUCH_START, 0, 10.f);
    auto end = DispatchTouch(target, EVENT_TOUCH_END, 0, 20.f);
    const auto touches = end->detail().GetProperty("touches");
    if (compatible) {
      ExpectTouch(touches, 0, 10.f);
    } else {
      EXPECT_EQ(touches.GetLength(), 0u);
    }
    ExpectTouch(end->detail().GetProperty("changedTouches"), 0, 20.f);

    DispatchTouch(target, EVENT_TOUCH_START, 0, 30.f);
    auto cancel = DispatchTouch(target, EVENT_TOUCH_CANCEL);
    EXPECT_EQ(cancel->detail().GetProperty("touches").GetLength(), 0u);
    EXPECT_EQ(cancel->detail().GetProperty("changedTouches").GetLength(), 0u);
    auto move = DispatchTouch(target, EVENT_TOUCH_MOVE, 0, 40.f);
    EXPECT_EQ(move->detail().GetProperty("touches").GetLength(), 0u);
  }
}

TEST(TouchEventTest, ConsumedLongPressOnlySuppressesTapOnItsPage) {
  TouchEventTarget first;
  TouchEventTarget second;
  auto long_press = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "longpress");
  auto first_tap = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "first tap");
  auto second_tap = std::make_shared<MockEventListener>(
      EventListener::Type::kJSClosureEventListener, "second tap");
  first.AddEventListener(EVENT_LONG_PRESS, long_press);
  first.AddEventListener(EVENT_TAP, first_tap);
  second.AddEventListener(EVENT_TAP, second_tap);

  DispatchTouch(first, EVENT_TOUCH_START);
  DispatchTouch(first, EVENT_LONG_PRESS);
  EXPECT_EQ(long_press->GetCount(), 1);
  DispatchTouch(first, EVENT_TAP);
  DispatchTouch(second, EVENT_TAP);
  EXPECT_EQ(first_tap->GetCount(), 0);
  EXPECT_EQ(second_tap->GetCount(), 1);

  // Another page's touchstart must not reset the consumed long press.
  DispatchTouch(second, EVENT_TOUCH_START);
  DispatchTouch(first, EVENT_TAP);
  EXPECT_EQ(first_tap->GetCount(), 0);
  DispatchTouch(first, EVENT_TOUCH_START);
  DispatchTouch(first, EVENT_TAP);
  EXPECT_EQ(first_tap->GetCount(), 1);
}

}  // namespace test
}  // namespace event
}  // namespace lynx
