// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/public/gesture_handler.h"
#include "core/public/pub_value.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm::gesture {
namespace {

struct StateChange {
  MemberId member_id;
  uint32_t gesture_id;
  GestureState state;
};

class FakeDelegate : public GestureArenaDelegate {
 public:
  bool IsMemberValid(MemberId member_id) const override {
    return valid_members.find(member_id) != valid_members.end();
  }

  Point ConvertPageToMember(MemberId member_id,
                            const Point &page_point) const override {
    return {page_point.x - member_id, page_point.y - member_id};
  }

  ScrollState GetScrollState(MemberId member_id) const override {
    const auto it = scroll_states.find(member_id);
    return it == scroll_states.end() ? ScrollState{} : it->second;
  }

  GestureDirection GetScrollDirection(MemberId member_id) const override {
    const auto it = directions.find(member_id);
    return it == directions.end() ? GestureDirection::kUndetermined
                                  : it->second;
  }

  bool CanConsumeGesture(MemberId member_id, GestureDirection direction,
                         const Point &delta) const override {
    return can_consume;
  }

  bool ShouldConsumeGesture(MemberId member_id) const override {
    return should_consume;
  }

  ScrollResult ScrollBy(MemberId member_id, const Point &delta) override {
    scroll_calls.emplace_back(member_id, delta);
    if (scroll_hook) {
      scroll_hook();
    }
    return {delta, {}};
  }

  void OnGestureRecognized(MemberId member_id) override {
    recognized_members.push_back(member_id);
  }

  void OnGestureStateChanged(MemberId member_id, uint32_t gesture_id,
                             GestureState state) override {
    states.push_back({member_id, gesture_id, state});
  }

  void DispatchGestureEvent(const GestureEvent &event) override {
    events.push_back(event);
    if (event_hook) {
      event_hook(event);
    }
  }

  void ScheduleTimer(TimerToken token, double delay_ms) override {
    scheduled_timers.emplace_back(token, delay_ms);
  }

  void CancelTimer(TimerToken token) override {
    cancelled_timers.push_back(token);
  }

  bool StartFling(const Point &velocity) override {
    fling_velocities.push_back(velocity);
    if (fling_hook) {
      fling_hook();
    }
    return start_fling;
  }

  void StopFling() override { ++stop_fling_count; }

  std::set<MemberId> valid_members;
  std::map<MemberId, ScrollState> scroll_states;
  std::map<MemberId, GestureDirection> directions;
  std::vector<std::pair<MemberId, Point>> scroll_calls;
  std::vector<MemberId> recognized_members;
  std::vector<StateChange> states;
  std::vector<GestureEvent> events;
  std::vector<std::pair<TimerToken, double>> scheduled_timers;
  std::vector<TimerToken> cancelled_timers;
  std::vector<Point> fling_velocities;
  std::function<void(const GestureEvent &)> event_hook;
  std::function<void()> fling_hook;
  std::function<void()> scroll_hook;
  bool can_consume = true;
  bool should_consume = true;
  bool start_fling = false;
  int stop_fling_count = 0;
};

GestureDefinition Gesture(uint32_t id, GestureType type,
                          std::vector<GestureCallbackType> callbacks = {},
                          GestureRelations relations = {}) {
  GestureDefinition result;
  result.gesture_id = id;
  result.gesture_type = type;
  result.callbacks = std::move(callbacks);
  result.relations = std::move(relations);
  return result;
}

InputEvent Event(InputType type, uint64_t sequence, double x = 0,
                 double y = 0) {
  InputEvent result;
  result.type = type;
  result.sequence_id = sequence;
  result.pointer_id = 7;
  result.monotonic_time_ms = 100 + sequence;
  result.epoch_time_ms = 1000 + sequence;
  result.screen = {x + 20, y + 20};
  result.page = {x, y};
  result.client = {x + 10, y + 10};
  return result;
}

size_t CountEvents(const FakeDelegate &delegate, MemberId member_id,
                   GestureCallbackType callback) {
  return std::count_if(delegate.events.begin(), delegate.events.end(),
                       [member_id, callback](const auto &event) {
                         return event.member_id == member_id &&
                                event.callback == callback;
                       });
}

size_t CountStates(const FakeDelegate &delegate, MemberId member_id,
                   GestureState state) {
  return std::count_if(delegate.states.begin(), delegate.states.end(),
                       [member_id, state](const auto &change) {
                         return change.member_id == member_id &&
                                change.state == state;
                       });
}

TEST(GestureArenaTest, KeepsSmallestGestureIdForEachType) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);

  arena.ReplaceMemberGestures(
      1, {Gesture(9, GestureType::PAN), Gesture(3, GestureType::PAN),
          Gesture(8, GestureType::TAP)});

  EXPECT_TRUE(arena.ContainsGesture(1, 3));
  EXPECT_TRUE(arena.ContainsGesture(1, 8));
  EXPECT_FALSE(arena.ContainsGesture(1, 9));
}

TEST(GestureArenaTest, DispatchesTouchesBeforeRecognition) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(1, {Gesture(1, GestureType::PAN,
                                          {GestureCallbackType::kTouchesDown,
                                           GestureCallbackType::kBegin})});

  arena.HandleInput(Event(InputType::kDown, 1, 20, 30), {1});

  ASSERT_EQ(delegate.events.size(), 2u);
  EXPECT_EQ(delegate.events[0].callback, GestureCallbackType::kTouchesDown);
  EXPECT_EQ(delegate.events[1].callback, GestureCallbackType::kBegin);
  EXPECT_EQ(delegate.events[0].gesture_type, GestureType::PAN);
  EXPECT_EQ(delegate.events[0].timestamp_epoch_ms, 1001);
  EXPECT_EQ(delegate.events[0].local.x, 19);
  EXPECT_EQ(delegate.events[0].local.y, 29);
}

TEST(GestureArenaTest, CallbackSubscriptionDoesNotGatePanState) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::PAN, {GestureCallbackType::kStart})});

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.HandleInput(Event(InputType::kMove, 1, 1, 0));

  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 0u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 1u);
  EXPECT_EQ(CountStates(delegate, 1, GestureState::kBegin), 1u);
  EXPECT_EQ(CountStates(delegate, 1, GestureState::kActive), 1u);
}

TEST(GestureArenaTest, PanThresholdIsStrictAxisBasedAndFractional) {
  for (double direction : {-1.0, 1.0}) {
    FakeDelegate delegate;
    delegate.valid_members = {1};
    GestureArena arena(delegate);
    auto pan = Gesture(1, GestureType::PAN, {GestureCallbackType::kStart});
    pan.config.min_distance = 10.25;
    arena.ReplaceMemberGestures(1, {pan});
    arena.HandleInput(Event(InputType::kDown, 1), {1});
    arena.HandleInput(
        Event(InputType::kMove, 1, direction * 10.25, direction * 10.25));
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 0u);
    arena.HandleInput(Event(InputType::kMove, 1, direction * 10.5, 0));
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 1u);
  }
}

TEST(GestureArenaTest, TapDistanceBoundaryAndBeyond) {
  const std::pair<double, bool> cases[] = {
      {-10.25, true}, {-10.0, true}, {10.0, true},
      {10.25, true},  {10.0, false}, {10.25, false},
  };
  for (const auto &[distance, move_first] : cases) {
    SCOPED_TRACE(distance);
    SCOPED_TRACE(move_first);
    FakeDelegate delegate;
    delegate.valid_members = {1};
    GestureArena arena(delegate);
    arena.ReplaceMemberGestures(
        1, {Gesture(1, GestureType::TAP,
                    {GestureCallbackType::kBegin, GestureCallbackType::kStart,
                     GestureCallbackType::kEnd})});
    arena.HandleInput(Event(InputType::kDown, 1), {1});
    if (move_first) {
      arena.HandleInput(Event(InputType::kMove, 1, distance, distance));
    }
    const double up_distance = move_first ? 0.0 : distance;
    arena.HandleInput(Event(InputType::kUp, 1, up_distance, up_distance));
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 1u);
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart),
              std::abs(distance) <= 10 ? 1u : 0u);
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
  }
}

TEST(GestureArenaTest, TapTimeoutAtDeadlineCannotClick) {
  FakeDelegate delegate;
  delegate.valid_members = {1};
  GestureArena arena(delegate);
  auto tap = Gesture(1, GestureType::TAP,
                     {GestureCallbackType::kStart, GestureCallbackType::kEnd});
  tap.config.max_duration_ms = 100;
  arena.ReplaceMemberGestures(1, {tap});
  arena.HandleInput(Event(InputType::kDown, 1), {1});
  ASSERT_EQ(delegate.scheduled_timers.size(), 1u);
  EXPECT_EQ(delegate.scheduled_timers.front().second, 100);
  arena.HandleTimer(delegate.scheduled_timers.front().first, 201, 1201);
  arena.HandleInput(Event(InputType::kUp, 1));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 0u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
}

TEST(GestureArenaTest, EndDeduplicatesAndResetStartsAnotherSequence) {
  FakeDelegate delegate;
  delegate.valid_members = {1};
  GestureArena arena(delegate);
  auto pan = Gesture(1, GestureType::PAN,
                     {GestureCallbackType::kBegin, GestureCallbackType::kStart,
                      GestureCallbackType::kEnd});
  arena.ReplaceMemberGestures(1, {pan});
  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.SetGestureState(1, 1, GestureStateCommand::kEnd);
  arena.SetGestureState(1, 1, GestureStateCommand::kEnd);
  arena.SetGestureState(1, 1, GestureStateCommand::kFail);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
  arena.Reset();
  arena.ReplaceMemberGestures(1, {pan});
  arena.HandleInput(Event(InputType::kDown, 2), {1});
  arena.HandleInput(Event(InputType::kMove, 2, 1, 0));
  arena.HandleInput(Event(InputType::kUp, 2, 1, 0));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 2u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 2u);
}

TEST(GestureArenaTest, ResetEndAndDestructionCancelDiscreteTimers) {
  for (auto type : {GestureType::TAP, GestureType::LONG_PRESS}) {
    for (bool end : {false, true}) {
      FakeDelegate delegate;
      delegate.valid_members = {1};
      auto arena = std::make_unique<GestureArena>(delegate);
      arena->ReplaceMemberGestures(
          1, {Gesture(1, type,
                      {GestureCallbackType::kBegin, GestureCallbackType::kStart,
                       GestureCallbackType::kEnd})});
      arena->HandleInput(Event(InputType::kDown, 1), {1});
      ASSERT_EQ(delegate.scheduled_timers.size(), 1u);
      const auto token = delegate.scheduled_timers.front().first;
      if (end) {
        arena->SetGestureState(1, 1, GestureStateCommand::kEnd);
      } else {
        arena->Reset();
      }
      EXPECT_EQ(delegate.cancelled_timers, (std::vector<TimerToken>{token}));
      const auto event_count = delegate.events.size();
      arena->HandleTimer(token, 1101, 2101);
      EXPECT_EQ(delegate.events.size(), event_count);
      arena->ReplaceMemberGestures(1, {Gesture(1, type)});
      arena->HandleInput(Event(InputType::kDown, 2), {1});
      ASSERT_EQ(delegate.scheduled_timers.size(), 2u);
      const auto next_token = delegate.scheduled_timers.back().first;
      arena.reset();
      EXPECT_EQ(delegate.cancelled_timers,
                (std::vector<TimerToken>{token, next_token}));
    }
  }
}

TEST(GestureArenaTest, DefaultLocksDirectionWithoutUpdateSubscription) {
  FakeDelegate delegate;
  delegate.valid_members = {1};
  delegate.directions[1] = GestureDirection::kVertical;
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(1, {Gesture(1, GestureType::DEFAULT)});
  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.HandleInput(Event(InputType::kMove, 1, 4, 4));
  arena.HandleInput(Event(InputType::kMove, 1, 100, 5));
  ASSERT_EQ(delegate.scroll_calls.size(), 2u);
  EXPECT_EQ(delegate.scroll_calls[0].second.x, 0);
  EXPECT_EQ(delegate.scroll_calls[0].second.y, -4);
  EXPECT_EQ(delegate.scroll_calls[1].second.x, 0);
  EXPECT_EQ(delegate.scroll_calls[1].second.y, -1);
  EXPECT_TRUE(delegate.events.empty());
}

TEST(GestureArenaTest, CancelEndsBegunHandlerOnceAndNeverStartsFling) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  delegate.start_fling = true;
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(1, {Gesture(1, GestureType::PAN,
                                          {GestureCallbackType::kTouchesCancel,
                                           GestureCallbackType::kEnd})});

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.HandleInput(Event(InputType::kCancel, 1));
  arena.HandleInput(Event(InputType::kCancel, 1));

  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kTouchesCancel), 1u);
  EXPECT_EQ(CountStates(delegate, 1, GestureState::kCancel), 1u);
  EXPECT_TRUE(delegate.fling_velocities.empty());
}

TEST(GestureArenaTest, LongPressIgnoresDistanceAfterActivation) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  auto long_press =
      Gesture(1, GestureType::LONG_PRESS,
              {GestureCallbackType::kStart, GestureCallbackType::kEnd});
  long_press.config.max_distance = 10;
  long_press.config.min_duration_ms = 500;
  arena.ReplaceMemberGestures(1, {long_press});

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  ASSERT_EQ(delegate.scheduled_timers.size(), 1u);
  arena.HandleTimer(delegate.scheduled_timers[0].first, 700, 1700);
  arena.HandleInput(Event(InputType::kMove, 1, 100, 100));
  arena.HandleInput(Event(InputType::kUp, 1, 100, 100));

  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 1u);
  EXPECT_EQ(CountStates(delegate, 1, GestureState::kFail), 0u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
}

TEST(GestureArenaTest, ReplacementTerminatesOldHandlerUntilNextDown) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::PAN,
                  {GestureCallbackType::kBegin, GestureCallbackType::kEnd})});
  arena.HandleInput(Event(InputType::kDown, 1), {1});

  arena.ReplaceMemberGestures(1, {Gesture(2, GestureType::TAP,
                                          {GestureCallbackType::kBegin,
                                           GestureCallbackType::kTouchesDown,
                                           GestureCallbackType::kTouchesMove,
                                           GestureCallbackType::kTouchesUp})});
  arena.HandleInput(Event(InputType::kMove, 1, 5, 0));
  arena.HandleInput(Event(InputType::kUp, 1, 5, 0));

  EXPECT_FALSE(arena.ContainsGesture(1, 1));
  EXPECT_TRUE(arena.ContainsGesture(1, 2));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kTouchesMove), 0u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kTouchesUp), 0u);

  arena.HandleInput(Event(InputType::kDown, 2), {1});
  arena.HandleInput(Event(InputType::kMove, 2, 5, 0));
  arena.HandleInput(Event(InputType::kUp, 2, 5, 0));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 2u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kTouchesDown), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kTouchesMove), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kTouchesUp), 1u);
}

TEST(GestureArenaTest, ReplacementDuringTouchCallbackStopsRemainingDelivery) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  auto pan = Gesture(1, GestureType::PAN, {GestureCallbackType::kTouchesMove});
  auto tap = Gesture(2, GestureType::TAP, {GestureCallbackType::kTouchesMove});
  arena.ReplaceMemberGestures(1, {pan, tap});
  arena.HandleInput(Event(InputType::kDown, 1), {1});
  delegate.event_hook = [&](const GestureEvent &event) {
    if (event.callback == GestureCallbackType::kTouchesMove &&
        event.gesture_id == 1) {
      pan.config.min_distance = 5;
      arena.ReplaceMemberGestures(1, {pan, tap});
    }
  };

  arena.HandleInput(Event(InputType::kMove, 1, 5, 0));
  arena.HandleInput(Event(InputType::kMove, 1, 10, 0));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kTouchesMove), 1u);

  delegate.event_hook = nullptr;
  arena.HandleInput(Event(InputType::kDown, 2), {1});
  arena.HandleInput(Event(InputType::kMove, 2, 5, 0));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kTouchesMove), 3u);
}

TEST(GestureArenaTest, FailedSimultaneousDefaultStillConsumesRawDelta) {
  FakeDelegate delegate;
  delegate.valid_members = {1, 2};
  delegate.directions[1] = GestureDirection::kHorizontal;
  delegate.directions[2] = GestureDirection::kHorizontal;
  GestureArena arena(delegate);
  GestureRelations relations;
  relations.simultaneous = {2};
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::DEFAULT, {}, relations)});
  arena.ReplaceMemberGestures(2, {Gesture(2, GestureType::DEFAULT)});

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.SetGestureState(2, 2, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 1, -5, 0));

  ASSERT_EQ(delegate.scroll_calls.size(), 2u);
  EXPECT_EQ(delegate.scroll_calls[0].first, 1);
  EXPECT_EQ(delegate.scroll_calls[1].first, 2);
  EXPECT_EQ(delegate.scroll_calls[0].second.x, 5);
  EXPECT_EQ(delegate.scroll_calls[1].second.x, 5);
}

TEST(GestureArenaTest, EndStateWinsOverActiveStateOnSameMember) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  delegate.directions[1] = GestureDirection::kHorizontal;
  GestureArena arena(delegate);
  GestureRelations default_relations;
  default_relations.simultaneous = {2};
  GestureRelations tap_relations;
  tap_relations.simultaneous = {1};
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::DEFAULT,
                  {GestureCallbackType::kUpdate, GestureCallbackType::kEnd},
                  default_relations),
          Gesture(2, GestureType::TAP, {GestureCallbackType::kEnd},
                  tap_relations)});

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.HandleInput(Event(InputType::kMove, 1, -5, 0));
  arena.HandleInput(Event(InputType::kUp, 1, -5, 0));
  const auto updates = CountEvents(delegate, 1, GestureCallbackType::kUpdate);
  arena.HandleInput(Event(InputType::kMove, 1, -10, 0));

  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), updates);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 2u);
  EXPECT_TRUE(delegate.fling_velocities.empty());
}

TEST(GestureArenaTest, WaitForMovesLaterResponseMemberBeforeCurrent) {
  FakeDelegate delegate;
  delegate.valid_members = {1, 2};
  GestureArena arena(delegate);
  GestureRelations wait_for;
  wait_for.wait_for = {2};
  arena.ReplaceMemberGestures(
      1,
      {Gesture(1, GestureType::PAN, {GestureCallbackType::kBegin}, wait_for)});
  arena.ReplaceMemberGestures(
      2, {Gesture(2, GestureType::PAN, {GestureCallbackType::kBegin})});

  arena.HandleInput(Event(InputType::kDown, 1), {1, 2});

  ASSERT_FALSE(delegate.events.empty());
  EXPECT_EQ(delegate.events.front().member_id, 2);
}

TEST(GestureArenaTest,
     WaitForPreservesDeclarationOrderWithoutDuplicateMembers) {
  for (const auto &wait_for :
       {std::vector<uint32_t>{3, 2}, std::vector<uint32_t>{2, 3},
        std::vector<uint32_t>{3, 2, 3}}) {
    FakeDelegate delegate;
    delegate.valid_members = {1, 2, 3};
    GestureArena arena(delegate);
    GestureRelations relations;
    relations.wait_for = wait_for;
    arena.ReplaceMemberGestures(
        1, {Gesture(1, GestureType::PAN, {GestureCallbackType::kBegin},
                    relations)});
    for (MemberId member_id : {2, 3}) {
      arena.ReplaceMemberGestures(member_id,
                                  {Gesture(member_id, GestureType::PAN,
                                           {GestureCallbackType::kBegin})});
    }

    arena.HandleInput(Event(InputType::kDown, 1), {1, 2, 3});
    arena.SetGestureState(wait_for[0], wait_for[0], GestureStateCommand::kFail);
    arena.HandleInput(Event(InputType::kMove, 1, 10, 0));
    arena.SetGestureState(wait_for[1], wait_for[1], GestureStateCommand::kFail);
    arena.HandleInput(Event(InputType::kMove, 1, 20, 0));

    std::vector<MemberId> begin_order;
    for (const auto &event : delegate.events) {
      if (event.callback == GestureCallbackType::kBegin) {
        begin_order.push_back(event.member_id);
      }
    }
    EXPECT_EQ(begin_order,
              (std::vector<MemberId>{wait_for[0], wait_for[1], 1}));
  }
}

TEST(GestureArenaTest, ContinueWithCanPromoteMemberOutsideHitChain) {
  FakeDelegate delegate;
  delegate.valid_members = {1, 3};
  GestureArena arena(delegate);
  GestureRelations continue_with;
  continue_with.continue_with = {3};
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::PAN, {GestureCallbackType::kBegin},
                  continue_with)});
  arena.ReplaceMemberGestures(
      3, {Gesture(3, GestureType::PAN, {GestureCallbackType::kBegin})});

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.SetGestureState(1, 1, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 1, 10, 0));

  EXPECT_EQ(CountEvents(delegate, 3, GestureCallbackType::kBegin), 1u);

  arena.RemoveMember(3);
  delegate.events.clear();
  arena.HandleInput(Event(InputType::kDown, 2), {1});
  arena.SetGestureState(1, 1, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 2, 10, 0));
  EXPECT_EQ(CountEvents(delegate, 3, GestureCallbackType::kBegin), 0u);
}

TEST(GestureArenaTest, MatchedWaitForPreservesContinueWithOutsideHitChain) {
  FakeDelegate delegate;
  delegate.valid_members = {1, 2, 3, 4};
  GestureArena arena(delegate);
  GestureRelations relations;
  relations.wait_for = {2};
  relations.continue_with = {3};
  arena.ReplaceMemberGestures(
      1,
      {Gesture(1, GestureType::PAN, {GestureCallbackType::kBegin}, relations)});
  for (MemberId member_id : {2, 3, 4}) {
    arena.ReplaceMemberGestures(
        member_id,
        {Gesture(member_id, GestureType::PAN, {GestureCallbackType::kBegin})});
  }

  arena.HandleInput(Event(InputType::kDown, 1), {1, 2, 4});
  EXPECT_EQ(CountEvents(delegate, 2, GestureCallbackType::kBegin), 1u);
  arena.SetGestureState(2, 2, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 1, 10, 0));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 1u);
  arena.SetGestureState(1, 1, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 1, 20, 0));

  EXPECT_EQ(CountEvents(delegate, 3, GestureCallbackType::kBegin), 1u);
  EXPECT_EQ(CountEvents(delegate, 2, GestureCallbackType::kBegin), 1u);
  EXPECT_EQ(CountEvents(delegate, 4, GestureCallbackType::kBegin), 0u);
}

TEST(GestureArenaTest, UnmatchedWaitForRetainsRemainingResponseChain) {
  FakeDelegate delegate;
  delegate.valid_members = {1, 2, 3, 4};
  GestureArena arena(delegate);
  GestureRelations relations;
  relations.wait_for = {2};
  relations.continue_with = {3};
  arena.ReplaceMemberGestures(
      1,
      {Gesture(1, GestureType::PAN, {GestureCallbackType::kBegin}, relations)});
  for (MemberId member_id : {2, 3, 4}) {
    arena.ReplaceMemberGestures(
        member_id,
        {Gesture(member_id, GestureType::PAN, {GestureCallbackType::kBegin})});
  }

  arena.HandleInput(Event(InputType::kDown, 1), {1, 4});
  arena.SetGestureState(1, 1, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 1, 10, 0));

  EXPECT_EQ(CountEvents(delegate, 4, GestureCallbackType::kBegin), 1u);
  EXPECT_EQ(CountEvents(delegate, 2, GestureCallbackType::kBegin), 0u);
  EXPECT_EQ(CountEvents(delegate, 3, GestureCallbackType::kBegin), 0u);
}

TEST(GestureArenaTest, UnsupportedOnlyMemberDoesNotOccupyWinner) {
  FakeDelegate delegate;
  delegate.valid_members = {1, 2};
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(1, {Gesture(1, GestureType::ROTATION)});
  arena.ReplaceMemberGestures(
      2, {Gesture(2, GestureType::PAN, {GestureCallbackType::kBegin})});

  arena.HandleInput(Event(InputType::kDown, 1), {1, 2});

  EXPECT_FALSE(arena.ContainsMember(1));
  EXPECT_EQ(CountEvents(delegate, 2, GestureCallbackType::kBegin), 1u);
}

TEST(GestureArenaTest,
     FlingStartsOnlyAfterPlatformAcceptsAndFirstFrameArrives) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  delegate.start_fling = true;
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::FLING,
                  {GestureCallbackType::kBegin, GestureCallbackType::kStart,
                   GestureCallbackType::kUpdate, GestureCallbackType::kEnd})});

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  auto up = Event(InputType::kUp, 1);
  up.velocity = {20, 30};
  arena.HandleInput(up);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 0u);
  ASSERT_EQ(delegate.fling_velocities.size(), 1u);

  auto frame = Event(InputType::kFlingFrame, 1);
  frame.delta = {1.5, -2.5};
  arena.HandleInput(frame);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), 1u);

  frame.fling_finished = true;
  frame.delta = {};
  arena.HandleInput(frame);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);

  delegate.start_fling = false;
  arena.HandleInput(Event(InputType::kDown, 2), {1});
  arena.HandleInput(Event(InputType::kUp, 2));
  EXPECT_EQ(CountStates(delegate, 1, GestureState::kFail), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 2u);
}

TEST(GestureArenaTest, PanAndNativeCanHandOffToSimultaneousFling) {
  for (auto type : {GestureType::PAN, GestureType::NATIVE}) {
    FakeDelegate delegate;
    delegate.valid_members = {1};
    delegate.start_fling = true;
    GestureArena arena(delegate);
    GestureRelations pan_relations;
    pan_relations.simultaneous = {2};
    GestureRelations fling_relations;
    fling_relations.simultaneous = {1};
    arena.ReplaceMemberGestures(
        1, {Gesture(1, type,
                    {GestureCallbackType::kUpdate, GestureCallbackType::kEnd},
                    pan_relations),
            Gesture(2, GestureType::FLING,
                    {GestureCallbackType::kStart, GestureCallbackType::kUpdate},
                    fling_relations)});

    arena.HandleInput(Event(InputType::kDown, 1), {1});
    arena.HandleInput(Event(InputType::kMove, 1, 20, 30));
    ASSERT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), 1u);
    auto up = Event(InputType::kUp, 1, 20, 30);
    up.velocity = {500, 600};
    arena.HandleInput(up);
    ASSERT_EQ(delegate.fling_velocities.size(), 1u);
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 0u);

    auto frame = Event(InputType::kFlingFrame, 1, 20, 30);
    frame.delta = {5, 6};
    arena.HandleInput(frame);
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 1u);
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), 2u);

    arena.SetGestureState(1, 1, GestureStateCommand::kEnd);
    arena.HandleInput(frame);
    EXPECT_EQ(delegate.stop_fling_count, 1);
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), 2u);
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
  }
}

TEST(GestureArenaTest, TouchOnlyGesturesDoNotBlockFlingHandoff) {
  for (auto type : {GestureType::PAN, GestureType::NATIVE, GestureType::TAP,
                    GestureType::LONG_PRESS}) {
    FakeDelegate delegate;
    delegate.valid_members = {1, 2, 3};
    delegate.start_fling = true;
    GestureArena arena(delegate);
    arena.ReplaceMemberGestures(
        1, {Gesture(1, GestureType::FLING, {GestureCallbackType::kUpdate})});
    arena.ReplaceMemberGestures(
        2, {Gesture(2, type,
                    {GestureCallbackType::kBegin, GestureCallbackType::kEnd})});
    arena.ReplaceMemberGestures(
        3, {Gesture(3, GestureType::FLING, {GestureCallbackType::kUpdate})});

    arena.HandleInput(Event(InputType::kDown, 1), {1, 2, 3});
    auto up = Event(InputType::kUp, 1);
    up.velocity = {0, 600};
    arena.HandleInput(up);
    auto frame = Event(InputType::kFlingFrame, 1);
    frame.delta = {0, -12};
    arena.HandleInput(frame);
    ASSERT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), 1u);

    arena.SetGestureState(1, 1, GestureStateCommand::kFail);
    arena.HandleInput(frame);
    arena.HandleInput(frame);

    EXPECT_EQ(CountStates(delegate, 2, GestureState::kFail), 1u);
    EXPECT_EQ(CountEvents(delegate, 2, GestureCallbackType::kBegin), 0u);
    EXPECT_EQ(CountEvents(delegate, 2, GestureCallbackType::kEnd), 0u);
    EXPECT_EQ(CountEvents(delegate, 3, GestureCallbackType::kUpdate), 1u);
    EXPECT_EQ(delegate.stop_fling_count, 0);
  }
}

TEST(GestureArenaTest, SimultaneousPanDoesNotBlockFlingHandoff) {
  FakeDelegate delegate;
  delegate.valid_members = {1, 2, 3};
  delegate.start_fling = true;
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::FLING, {GestureCallbackType::kUpdate})});
  GestureRelations pan_relations;
  pan_relations.simultaneous = {4};
  GestureRelations fling_relations;
  fling_relations.simultaneous = {2};
  arena.ReplaceMemberGestures(
      2, {Gesture(2, GestureType::PAN, {}, pan_relations),
          Gesture(4, GestureType::FLING, {GestureCallbackType::kUpdate},
                  fling_relations)});
  arena.ReplaceMemberGestures(
      3, {Gesture(3, GestureType::FLING, {GestureCallbackType::kUpdate})});

  arena.HandleInput(Event(InputType::kDown, 1), {1, 2, 3});
  auto up = Event(InputType::kUp, 1);
  up.velocity = {0, 600};
  arena.HandleInput(up);
  auto frame = Event(InputType::kFlingFrame, 1);
  frame.delta = {0, -12};
  arena.HandleInput(frame);
  ASSERT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), 1u);

  arena.SetGestureState(1, 1, GestureStateCommand::kFail);
  arena.HandleInput(frame);
  ASSERT_EQ(CountEvents(delegate, 2, GestureCallbackType::kUpdate), 1u);
  arena.SetGestureState(2, 4, GestureStateCommand::kFail);
  arena.HandleInput(frame);

  EXPECT_EQ(CountStates(delegate, 2, GestureState::kFail), 2u);
  EXPECT_EQ(CountEvents(delegate, 3, GestureCallbackType::kUpdate), 1u);
  EXPECT_EQ(delegate.stop_fling_count, 0);
}

TEST(GestureArenaTest, FailedPanWithDormantFlingCanReenterCompetition) {
  FakeDelegate delegate;
  delegate.valid_members = {1, 2};
  GestureArena arena(delegate);
  GestureRelations pan_relations;
  pan_relations.simultaneous = {3};
  GestureRelations fling_relations;
  fling_relations.simultaneous = {1};
  GestureRelations native_relations;
  native_relations.wait_for = {1};
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::PAN,
                  {GestureCallbackType::kBegin, GestureCallbackType::kUpdate},
                  pan_relations),
          Gesture(3, GestureType::FLING, {}, fling_relations)});
  arena.ReplaceMemberGestures(
      2, {Gesture(2, GestureType::NATIVE,
                  {GestureCallbackType::kBegin, GestureCallbackType::kUpdate},
                  native_relations)});

  arena.HandleInput(Event(InputType::kDown, 1), {2, 1});
  arena.HandleInput(Event(InputType::kMove, 1, 0, -10));
  arena.SetGestureState(1, 1, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 1, 0, -20));
  arena.HandleInput(Event(InputType::kMove, 1, 0, -30));
  ASSERT_EQ(CountEvents(delegate, 2, GestureCallbackType::kUpdate), 1u);

  arena.SetGestureState(2, 2, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 1, 0, -20));
  arena.HandleInput(Event(InputType::kMove, 1, 0, 0));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 2u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), 2u);

  arena.SetGestureState(1, 1, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 1, 0, -10));
  arena.HandleInput(Event(InputType::kMove, 1, 0, -20));
  EXPECT_EQ(CountEvents(delegate, 2, GestureCallbackType::kBegin), 2u);
  EXPECT_EQ(CountEvents(delegate, 2, GestureCallbackType::kUpdate), 2u);

  arena.ReplaceMemberGestures(
      1, {Gesture(4, GestureType::PAN, {GestureCallbackType::kBegin})});
  arena.SetGestureState(2, 2, GestureStateCommand::kFail);
  arena.HandleInput(Event(InputType::kMove, 1, 0, 0));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 2u);
  arena.HandleInput(Event(InputType::kDown, 2), {1});
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 3u);
}

TEST(GestureArenaTest, FailedOnlyMemberDoesNotRestartOnMove) {
  for (bool duplicate : {false, true}) {
    FakeDelegate delegate;
    delegate.valid_members = {1};
    GestureArena arena(delegate);
    GestureRelations relations;
    if (duplicate) {
      relations.continue_with = {1};
    }
    arena.ReplaceMemberGestures(
        1, {Gesture(1, GestureType::PAN, {GestureCallbackType::kBegin},
                    relations)});
    arena.HandleInput(Event(InputType::kDown, 1), {1});
    arena.SetGestureState(1, 1, GestureStateCommand::kFail);
    arena.HandleInput(Event(InputType::kMove, 1, 10, 0));
    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 1u);
  }
}

TEST(GestureArenaTest, ReentrantRemovalInvalidatesCurrentCallbackSafely) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::LONG_PRESS,
                  {GestureCallbackType::kBegin, GestureCallbackType::kEnd})});
  delegate.event_hook = [&arena](const GestureEvent &event) {
    if (event.callback == GestureCallbackType::kBegin) {
      arena.RemoveMember(event.member_id);
    }
  };

  arena.HandleInput(Event(InputType::kDown, 1), {1});

  EXPECT_FALSE(arena.ContainsMember(1));
  EXPECT_TRUE(delegate.scheduled_timers.empty());
}

TEST(GestureArenaTest, ResetFromBeginIsDeferredUntilHandlerReturns) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::PAN,
                  {GestureCallbackType::kBegin, GestureCallbackType::kEnd})});
  delegate.event_hook = [&arena](const GestureEvent &event) {
    if (event.callback == GestureCallbackType::kBegin) {
      arena.Reset();
    }
  };

  arena.HandleInput(Event(InputType::kDown, 1), {1});

  EXPECT_FALSE(arena.ContainsMember(1));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
}

TEST(GestureArenaTest, ResetFromUpdateIsDeferredUntilHandlerReturns) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::PAN,
                  {GestureCallbackType::kUpdate, GestureCallbackType::kEnd})});
  delegate.event_hook = [&arena](const GestureEvent &event) {
    if (event.callback == GestureCallbackType::kUpdate) {
      arena.Reset();
    }
  };

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.HandleInput(Event(InputType::kMove, 1, 5, 0));

  EXPECT_FALSE(arena.ContainsMember(1));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
}

TEST(GestureArenaTest, EquivalentReplacementDoesNotInterruptCurrentMove) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  delegate.directions[1] = GestureDirection::kHorizontal;
  GestureArena arena(delegate);
  const auto definition =
      Gesture(1, GestureType::DEFAULT,
              {GestureCallbackType::kStart, GestureCallbackType::kUpdate});
  arena.ReplaceMemberGestures(1, {definition});
  delegate.event_hook = [&arena, &definition](const GestureEvent &event) {
    if (event.callback == GestureCallbackType::kStart) {
      arena.ReplaceMemberGestures(1, {definition});
    }
  };

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.HandleInput(Event(InputType::kMove, 1, -5, 0));

  EXPECT_TRUE(arena.ContainsMember(1));
  EXPECT_EQ(delegate.scroll_calls.size(), 1u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kUpdate), 1u);
}

TEST(GestureArenaTest, EndFromStartPreventsScrollSideEffect) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  delegate.directions[1] = GestureDirection::kHorizontal;
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::DEFAULT,
                  {GestureCallbackType::kStart, GestureCallbackType::kEnd})});
  delegate.event_hook = [&arena](const GestureEvent &event) {
    if (event.callback == GestureCallbackType::kStart) {
      arena.SetGestureState(event.member_id, event.gesture_id,
                            GestureStateCommand::kEnd);
    }
  };

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.HandleInput(Event(InputType::kMove, 1, -5, 0));

  EXPECT_TRUE(delegate.scroll_calls.empty());
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
}

TEST(GestureArenaTest, TapRejectsLateUpBeforeTimerDelivery) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  auto tap = Gesture(1, GestureType::TAP,
                     {GestureCallbackType::kStart, GestureCallbackType::kEnd});
  tap.config.max_duration_ms = 500;
  arena.ReplaceMemberGestures(1, {tap});

  auto down = Event(InputType::kDown, 1);
  down.monotonic_time_ms = 100;
  arena.HandleInput(down, {1});
  auto up = Event(InputType::kUp, 1);
  up.monotonic_time_ms = 601;
  arena.HandleInput(up);

  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 0u);
  EXPECT_EQ(CountStates(delegate, 1, GestureState::kFail), 1u);
}

TEST(GestureArenaTest, EndFromBeginDoesNotScheduleTimer) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::LONG_PRESS,
                  {GestureCallbackType::kBegin, GestureCallbackType::kEnd})});
  delegate.event_hook = [&arena](const GestureEvent &event) {
    if (event.callback == GestureCallbackType::kBegin) {
      arena.SetGestureState(event.member_id, event.gesture_id,
                            GestureStateCommand::kEnd);
    }
  };

  arena.HandleInput(Event(InputType::kDown, 1), {1});

  EXPECT_TRUE(delegate.scheduled_timers.empty());
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
}

TEST(GestureConfigTest, InvalidNumbersFallBackToDefaults) {
  auto definition = Gesture(1, GestureType::LONG_PRESS);
  definition.config.min_distance = -1;
  definition.config.max_distance = std::numeric_limits<double>::infinity();
  definition.config.min_duration_ms = std::numeric_limits<double>::quiet_NaN();
  definition.config.max_duration_ms = -1;
  definition.config.tap_slop = std::numeric_limits<double>::infinity();

  const auto normalized = NormalizeGestureDefinitions({definition});

  ASSERT_EQ(normalized.size(), 1u);
  EXPECT_EQ(normalized[0].config.min_distance, 0);
  EXPECT_EQ(normalized[0].config.max_distance, 10);
  EXPECT_EQ(normalized[0].config.min_duration_ms, 500);
  EXPECT_EQ(normalized[0].config.max_duration_ms, 500);
  EXPECT_EQ(normalized[0].config.tap_slop, 3);
}

TEST(GestureConfigTest, RejectsUnknownGestureTypes) {
  for (const auto type : {8u, std::numeric_limits<unsigned int>::max()}) {
    auto definition = Gesture(1, static_cast<GestureType>(type));
    EXPECT_TRUE(NormalizeGestureDefinitions({definition}).empty());
  }
}

TEST(GestureArenaTest, ReplacementCancelsTimerAndRejectsStaleFire) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::LONG_PRESS, {GestureCallbackType::kStart})});
  arena.HandleInput(Event(InputType::kDown, 1), {1});
  ASSERT_EQ(delegate.scheduled_timers.size(), 1u);
  const auto stale_token = delegate.scheduled_timers.front().first;

  arena.ReplaceMemberGestures(1, {Gesture(2, GestureType::TAP)});
  arena.HandleTimer(stale_token, 700, 1700);

  EXPECT_NE(std::find(delegate.cancelled_timers.begin(),
                      delegate.cancelled_timers.end(), stale_token),
            delegate.cancelled_timers.end());
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 0u);
}

TEST(GestureArenaTest, RemovingWinnerWhileStartingFlingStopsDriver) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  delegate.start_fling = true;
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(1, {Gesture(1, GestureType::FLING)});
  arena.HandleInput(Event(InputType::kDown, 1), {1});
  delegate.fling_hook = [&arena]() { arena.RemoveMember(1); };
  auto up = Event(InputType::kUp, 1);
  up.velocity = {100, 0};
  arena.HandleInput(up);

  EXPECT_EQ(delegate.fling_velocities.size(), 1u);
  EXPECT_EQ(delegate.stop_fling_count, 1);
  EXPECT_FALSE(arena.ContainsMember(1));
}

TEST(GestureArenaTest, NewDownCancelsPreviousSequence) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::PAN,
                  {GestureCallbackType::kBegin, GestureCallbackType::kEnd})});

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.HandleInput(Event(InputType::kDown, 2), {1});

  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kBegin), 2u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
  EXPECT_EQ(CountStates(delegate, 1, GestureState::kCancel), 1u);
}

TEST(GestureArenaTest, ActiveCommandRemainsUnsupported) {
  FakeDelegate delegate;
  delegate.valid_members.insert(1);
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(1, {Gesture(1, GestureType::PAN)});
  arena.HandleInput(Event(InputType::kDown, 1), {1});

  arena.SetGestureState(1, 1, GestureStateCommand::kActive);

  EXPECT_EQ(CountStates(delegate, 1, GestureState::kActive), 0u);
}

TEST(GestureTypesTest, CallbackNamesAreStable) {
  const char *callbacks[] = {"onTouchesDown",   "onTouchesMove", "onTouchesUp",
                             "onTouchesCancel", "onBegin",       "onStart",
                             "onUpdate",        "onEnd"};
  for (size_t index = 0; index < std::size(callbacks); ++index) {
    const auto callback = static_cast<GestureCallbackType>(index);
    EXPECT_STREQ(GestureCallbackName(callback), callbacks[index]);
    EXPECT_EQ(ParseGestureCallback(callbacks[index]), callback);
  }
  EXPECT_FALSE(ParseGestureCallback(""));
  EXPECT_FALSE(ParseGestureCallback("touchmove"));
  EXPECT_STREQ(GestureInputTypeName(InputType::kDown), "touchstart");
  EXPECT_STREQ(GestureInputTypeName(InputType::kMove), "touchmove");
  EXPECT_STREQ(GestureInputTypeName(InputType::kUp), "touchend");
  EXPECT_STREQ(GestureInputTypeName(InputType::kCancel), "touchcancel");
  EXPECT_STREQ(GestureInputTypeName(InputType::kFlingFrame), "unknown");
}

constexpr std::tuple<GestureType, const char *, double GestureConfig::*>
    kConfigFields[] = {
        {GestureType::PAN, "minDistance", &GestureConfig::min_distance},
        {GestureType::NATIVE, "minDistance", &GestureConfig::min_distance},
        {GestureType::TAP, "maxDistance", &GestureConfig::max_distance},
        {GestureType::TAP, "maxDuration", &GestureConfig::max_duration_ms},
        {GestureType::LONG_PRESS, "maxDistance", &GestureConfig::max_distance},
        {GestureType::LONG_PRESS, "minDuration",
         &GestureConfig::min_duration_ms},
        {GestureType::DEFAULT, "tapSlop", &GestureConfig::tap_slop},
};

// Parsing must not depend on a platform's value backend.
class MockConfigValue : public pub::Value {
 public:
  MockConfigValue()
      : pub::Value(pub::ValueBackendType::ValueBackendTypeCustom) {}

  MOCK_METHOD(int64_t, Type, (), (const, override));
  MOCK_METHOD(bool, IsUndefined, (), (const, override));
  MOCK_METHOD(bool, IsBool, (), (const, override));
  MOCK_METHOD(bool, IsInt32, (), (const, override));
  MOCK_METHOD(bool, IsInt64, (), (const, override));
  MOCK_METHOD(bool, IsUInt32, (), (const, override));
  MOCK_METHOD(bool, IsUInt64, (), (const, override));
  MOCK_METHOD(bool, IsDouble, (), (const, override));
  MOCK_METHOD(bool, IsNumber, (), (const, override));
  MOCK_METHOD(bool, IsNil, (), (const, override));
  MOCK_METHOD(bool, IsString, (), (const, override));
  MOCK_METHOD(bool, IsArray, (), (const, override));
  MOCK_METHOD(bool, IsMap, (), (const, override));
  MOCK_METHOD(bool, IsFunction, (), (const, override));
  MOCK_METHOD(bool, Bool, (), (const, override));
  MOCK_METHOD(double, Double, (), (const, override));
  MOCK_METHOD(int32_t, Int32, (), (const, override));
  MOCK_METHOD(uint32_t, UInt32, (), (const, override));
  MOCK_METHOD(int64_t, Int64, (), (const, override));
  MOCK_METHOD(uint64_t, UInt64, (), (const, override));
  MOCK_METHOD(double, Number, (), (const, override));
  MOCK_METHOD(const std::string &, str, (), (const, override));
  MOCK_METHOD(int, Length, (), (const, override));
  MOCK_METHOD(void, ForeachArray, (pub::ForeachArrayFunc), (const, override));
  MOCK_METHOD(void, ForeachMap, (pub::ForeachMapFunc), (const, override));
  MOCK_METHOD(std::unique_ptr<pub::Value>, GetValueAtIndex, (uint32_t),
              (const, override));
  MOCK_METHOD(bool, Erase, (uint32_t), (const, override));
  MOCK_METHOD(std::unique_ptr<pub::Value>, GetValueForKey,
              (const std::string &), (const, override));
  MOCK_METHOD(bool, Erase, (const std::string &), (const, override));
  MOCK_METHOD(bool, Contains, (const std::string &), (const, override));
};

class TestGestureDetector : public GestureDetector {
 public:
  TestGestureDetector(uint32_t id, GestureType type, const pub::Value &config)
      : id_(id), type_(type), config_(config) {}

  uint32_t gesture_id() const override { return id_; }
  GestureType gesture_type() const override { return type_; }
  const pub::Value &gesture_config() const override { return config_; }
  std::vector<std::string> gesture_callback_names() const override {
    return callbacks;
  }
  const std::unordered_map<std::string, std::vector<uint32_t>> &relation_map()
      const override {
    return relations;
  }

  std::vector<std::string> callbacks;
  std::unordered_map<std::string, std::vector<uint32_t>> relations;

 private:
  uint32_t id_;
  GestureType type_;
  const pub::Value &config_;
};

void SetNumberConfig(MockConfigValue &config, const char *key, double value) {
  ON_CALL(config, IsMap()).WillByDefault(::testing::Return(true));
  ON_CALL(config, Contains(::testing::StrEq(key)))
      .WillByDefault(::testing::Return(true));
  ON_CALL(config, GetValueForKey(::testing::StrEq(key)))
      .WillByDefault([value]() {
        auto number = std::make_unique<::testing::NiceMock<MockConfigValue>>();
        ON_CALL(*number, IsNumber()).WillByDefault(::testing::Return(true));
        ON_CALL(*number, Number()).WillByDefault(::testing::Return(value));
        return number;
      });
}

TEST(GestureConfigTest, DetectorSnapshotPreservesZeroAndFractionalConfig) {
  for (const auto &[type, key, field] : kConfigFields) {
    SCOPED_TRACE(key);
    for (double value : {0.0, 12.25}) {
      ::testing::NiceMock<MockConfigValue> config;
      SetNumberConfig(config, key, value);
      TestGestureDetector detector(1, type, config);

      const auto snapshot = SnapshotGestureDetector(detector);

      ASSERT_TRUE(snapshot);
      EXPECT_DOUBLE_EQ(snapshot->config.*field, value);
    }
  }
}

TEST(GestureConfigTest, DetectorSnapshotDefaultsMissingAndInvalidConfig) {
  enum class InvalidConfig { kNotMap, kMissing, kNonNumber };
  const GestureConfig defaults;
  for (const auto &[type, key, field] : kConfigFields) {
    SCOPED_TRACE(key);
    for (double value : {-1.0, std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
      ::testing::NiceMock<MockConfigValue> config;
      SetNumberConfig(config, key, value);
      TestGestureDetector detector(1, type, config);
      const auto snapshot = SnapshotGestureDetector(detector);
      ASSERT_TRUE(snapshot);
      EXPECT_DOUBLE_EQ(snapshot->config.*field, defaults.*field);
    }
    for (auto invalid_kind : {InvalidConfig::kNotMap, InvalidConfig::kMissing,
                              InvalidConfig::kNonNumber}) {
      ::testing::NiceMock<MockConfigValue> config;
      ON_CALL(config, IsMap())
          .WillByDefault(
              ::testing::Return(invalid_kind != InvalidConfig::kNotMap));
      ON_CALL(config, Contains(::testing::_))
          .WillByDefault(
              ::testing::Return(invalid_kind == InvalidConfig::kNonNumber));
      ON_CALL(config, GetValueForKey(::testing::_)).WillByDefault([]() {
        return std::make_unique<::testing::NiceMock<MockConfigValue>>();
      });
      TestGestureDetector detector(1, type, config);
      const auto snapshot = SnapshotGestureDetector(detector);
      ASSERT_TRUE(snapshot);
      EXPECT_DOUBLE_EQ(snapshot->config.*field, defaults.*field);
    }
  }
}

TEST(GestureConfigTest, DetectorSnapshotFiltersCallbacksAndUnsupportedEntries) {
  ::testing::NiceMock<MockConfigValue> config;
  TestGestureDetector pan(9, GestureType::PAN, config);
  pan.callbacks = {"onStart", "onStart", "unknown"};
  pan.relations = {
      {"simultaneous", {3, 2}}, {"waitFor", {4}}, {"continueWith", {5}}};
  TestGestureDetector tap(2, GestureType::TAP, config);
  TestGestureDetector unsupported(1, GestureType::PINCH, config);

  const auto definitions =
      NormalizeGestureDetectors({nullptr, &pan, &unsupported, &tap});

  ASSERT_EQ(definitions.size(), 2u);
  EXPECT_EQ(definitions[0].gesture_id, 2u);
  EXPECT_EQ(definitions[1].gesture_id, 9u);
  EXPECT_EQ(definitions[1].callbacks,
            (std::vector<GestureCallbackType>{GestureCallbackType::kStart}));
  EXPECT_EQ(definitions[1].relations.simultaneous,
            (std::vector<uint32_t>{3, 2}));
  EXPECT_EQ(definitions[1].relations.wait_for, (std::vector<uint32_t>{4}));
  EXPECT_EQ(definitions[1].relations.continue_with, (std::vector<uint32_t>{5}));
}

TEST(GestureArenaTest, IgnoresInputWithoutDownAndFromAnotherSequence) {
  FakeDelegate delegate;
  delegate.valid_members = {1};
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1,
      {Gesture(
          1, GestureType::PAN,
          {GestureCallbackType::kTouchesMove, GestureCallbackType::kTouchesUp,
           GestureCallbackType::kTouchesCancel, GestureCallbackType::kStart,
           GestureCallbackType::kUpdate, GestureCallbackType::kEnd})});
  const auto stale_types = {InputType::kMove, InputType::kUp,
                            InputType::kCancel, InputType::kFlingFrame};
  for (auto type : stale_types) {
    arena.HandleInput(Event(type, 1, 100, 100), {1});
  }
  EXPECT_TRUE(delegate.events.empty());
  EXPECT_TRUE(delegate.states.empty());

  arena.HandleInput(Event(InputType::kDown, 2), {1});
  const auto state_count = delegate.states.size();
  for (auto type : stale_types) {
    arena.HandleInput(Event(type, 1, 100, 100));
  }
  EXPECT_TRUE(delegate.events.empty());
  EXPECT_EQ(delegate.states.size(), state_count);
  EXPECT_TRUE(delegate.fling_velocities.empty());

  arena.HandleInput(Event(InputType::kMove, 2, 1, 0));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 1u);
  arena.HandleInput(Event(InputType::kUp, 2, 1, 0));
  const auto event_count = delegate.events.size();
  for (auto type : stale_types) {
    arena.HandleInput(Event(type, 2, 100, 100));
  }
  EXPECT_EQ(delegate.events.size(), event_count);
}

TEST(GestureArenaTest, NewDownRejectsOldTimerWithoutCancellingCurrentTimer) {
  FakeDelegate delegate;
  delegate.valid_members = {1};
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(1, GestureType::LONG_PRESS,
                  {GestureCallbackType::kStart, GestureCallbackType::kEnd})});
  arena.HandleInput(Event(InputType::kDown, 1), {1});
  ASSERT_EQ(delegate.scheduled_timers.size(), 1u);
  const auto old_token = delegate.scheduled_timers.front().first;

  arena.HandleInput(Event(InputType::kDown, 2), {1});
  ASSERT_EQ(delegate.scheduled_timers.size(), 2u);
  const auto current_token = delegate.scheduled_timers.back().first;
  EXPECT_NE(old_token, current_token);
  EXPECT_EQ(delegate.cancelled_timers, (std::vector<TimerToken>{old_token}));
  const auto event_count = delegate.events.size();
  arena.HandleTimer(old_token, 700, 1700);
  EXPECT_EQ(delegate.events.size(), event_count);
  arena.HandleTimer(current_token, 700, 1700);
  arena.HandleTimer(current_token, 701, 1701);

  ASSERT_FALSE(delegate.events.empty());
  EXPECT_EQ(delegate.events.back().sequence_id, 2u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 1u);
}

TEST(GestureArenaTest, LongPressReleaseBeforeDeadlineRejectsPendingTimer) {
  FakeDelegate delegate;
  delegate.valid_members = {1};
  GestureArena arena(delegate);
  auto definition =
      Gesture(1, GestureType::LONG_PRESS,
              {GestureCallbackType::kStart, GestureCallbackType::kEnd});
  definition.config.min_duration_ms = 500;
  arena.ReplaceMemberGestures(1, {definition});
  arena.HandleInput(Event(InputType::kDown, 1), {1});
  ASSERT_EQ(delegate.scheduled_timers.size(), 1u);
  const auto token = delegate.scheduled_timers.front().first;
  auto up = Event(InputType::kUp, 1);
  up.monotonic_time_ms = 600;

  arena.HandleInput(up);
  arena.HandleTimer(token, 601, 1601);

  EXPECT_EQ(delegate.cancelled_timers, (std::vector<TimerToken>{token}));
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart), 0u);
  EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kEnd), 1u);
  EXPECT_EQ(CountStates(delegate, 1, GestureState::kFail), 1u);
}

TEST(GestureArenaTest, LongPressChecksDistanceBeforeActivation) {
  const std::vector<std::pair<Point, bool>> cases = {
      {{-10.5, 0}, false},    {{0, -10.5}, false}, {{-10.25, -10.25}, true},
      {{10.25, 10.25}, true}, {{10.5, 0}, false},  {{0, 10.5}, false},
  };
  for (const auto &[point, within_distance] : cases) {
    SCOPED_TRACE(point.x);
    SCOPED_TRACE(point.y);
    FakeDelegate delegate;
    delegate.valid_members = {1};
    GestureArena arena(delegate);
    auto definition =
        Gesture(1, GestureType::LONG_PRESS, {GestureCallbackType::kStart});
    definition.config.max_distance = 10.25;
    arena.ReplaceMemberGestures(1, {definition});
    arena.HandleInput(Event(InputType::kDown, 1), {1});
    ASSERT_EQ(delegate.scheduled_timers.size(), 1u);
    const auto token = delegate.scheduled_timers.front().first;

    arena.HandleInput(Event(InputType::kMove, 1, point.x, point.y));
    arena.HandleTimer(token, 700, 1700);

    EXPECT_EQ(CountEvents(delegate, 1, GestureCallbackType::kStart),
              within_distance ? 1u : 0u);
    EXPECT_EQ(delegate.cancelled_timers.empty(), within_distance);
    EXPECT_EQ(CountStates(delegate, 1, GestureState::kFail),
              within_distance ? 0u : 1u);
  }
}

TEST(GestureArenaTest, DefaultRejectsScrollingWhenDelegateCannotConsume) {
  for (bool should_consume : {false, true}) {
    FakeDelegate delegate;
    delegate.valid_members = {1};
    delegate.directions[1] = GestureDirection::kHorizontal;
    delegate.should_consume = should_consume;
    delegate.can_consume = !should_consume;
    GestureArena arena(delegate);
    arena.ReplaceMemberGestures(
        1,
        {Gesture(1, GestureType::DEFAULT,
                 {GestureCallbackType::kStart, GestureCallbackType::kUpdate})});

    arena.HandleInput(Event(InputType::kDown, 1), {1});
    arena.HandleInput(Event(InputType::kMove, 1, -5, 0));

    EXPECT_TRUE(delegate.scroll_calls.empty());
    EXPECT_TRUE(delegate.recognized_members.empty());
    EXPECT_TRUE(delegate.events.empty());
    EXPECT_EQ(CountStates(delegate, 1, GestureState::kFail), 1u);
  }
}

TEST(GestureArenaTest, DefaultRejectsDirectionMismatch) {
  FakeDelegate delegate;
  delegate.valid_members = {1};
  delegate.directions[1] = GestureDirection::kVertical;
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(1, {Gesture(1, GestureType::DEFAULT)});

  arena.HandleInput(Event(InputType::kDown, 1), {1});
  arena.HandleInput(Event(InputType::kMove, 1, -5, 0));

  EXPECT_TRUE(delegate.scroll_calls.empty());
  EXPECT_TRUE(delegate.recognized_members.empty());
  EXPECT_EQ(CountStates(delegate, 1, GestureState::kFail), 1u);
}

TEST(GestureArenaTest, ScrollUpdatePreservesCompleteEventPayload) {
  FakeDelegate delegate;
  delegate.valid_members = {1};
  delegate.directions[1] = GestureDirection::kHorizontal;
  delegate.scroll_states[1] = {12.5, -3.25, true, false};
  GestureArena arena(delegate);
  arena.ReplaceMemberGestures(
      1, {Gesture(9, GestureType::DEFAULT, {GestureCallbackType::kUpdate})});
  arena.HandleInput(Event(InputType::kDown, 42, 20, 30), {1});
  auto move = Event(InputType::kMove, 42, 14.5, 31.25);
  move.pointer_id = 53;
  move.epoch_time_ms = 1700000000123;

  arena.HandleInput(move);

  ASSERT_EQ(delegate.events.size(), 1u);
  const auto &event = delegate.events.front();
  EXPECT_EQ(event.member_id, 1);
  EXPECT_EQ(event.gesture_id, 9u);
  EXPECT_EQ(event.gesture_type, GestureType::DEFAULT);
  EXPECT_EQ(event.callback, GestureCallbackType::kUpdate);
  EXPECT_EQ(event.source, InputType::kMove);
  EXPECT_EQ(event.sequence_id, 42u);
  EXPECT_EQ(event.pointer_id, 53);
  EXPECT_EQ(event.timestamp_epoch_ms, 1700000000123);
  EXPECT_DOUBLE_EQ(event.screen.x, 34.5);
  EXPECT_DOUBLE_EQ(event.screen.y, 51.25);
  EXPECT_DOUBLE_EQ(event.page.x, 14.5);
  EXPECT_DOUBLE_EQ(event.page.y, 31.25);
  EXPECT_DOUBLE_EQ(event.client.x, 24.5);
  EXPECT_DOUBLE_EQ(event.client.y, 41.25);
  EXPECT_DOUBLE_EQ(event.local.x, 13.5);
  EXPECT_DOUBLE_EQ(event.local.y, 30.25);
  EXPECT_DOUBLE_EQ(event.delta.x, 5.5);
  EXPECT_DOUBLE_EQ(event.delta.y, 0);
  EXPECT_DOUBLE_EQ(event.scroll.x, 12.5);
  EXPECT_DOUBLE_EQ(event.scroll.y, -3.25);
  EXPECT_TRUE(event.scroll.is_at_start);
  EXPECT_FALSE(event.scroll.is_at_end);
}

}  // namespace
}  // namespace lynx::tasm::gesture
