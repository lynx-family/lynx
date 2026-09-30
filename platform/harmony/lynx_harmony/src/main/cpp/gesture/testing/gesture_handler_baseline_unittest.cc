// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "core/renderer/events/gesture.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/arena/gesture_arena_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/common/gesture_extra_bundle.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/detector/gesture_detector_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/base_gesture_handler.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/fling_scroller.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#define private public
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/gesture_handler_trigger.h"
#undef private

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

class BaselineMember final : public GestureArenaMember {
 public:
  explicit BaselineMember(int id) : id_(id) {}
  std::vector<float> GestureScrollBy(float x, float y) override {
    scrolls.emplace_back(x, y);
    scroll_x += x;
    scroll_y += y;
    return {scroll_x, scroll_y};
  }
  bool CanConsumeGesture(float, float) override { return can_consume; }
  int Sign() const override { return id_; }
  int GestureArenaMemberId() override { return id_; }
  float ScrollX() override { return scroll_x; }
  float ScrollY() override { return scroll_y; }
  int8_t GetScrollContainerDirection() override { return direction; }
  bool IsAtBorder(bool start) override { return start ? at_start : at_end; }
  const GestureMap& GetGestureDetectorMap() override { return detectors; }
  const GestureHandlerMap& GetGestureHandlers() override { return handlers; }

  bool can_consume = true;
  bool at_start = false;
  bool at_end = false;
  int8_t direction = GestureConstants::DIRECTION_VERTICAL;
  float scroll_x = 0;
  float scroll_y = 0;
  std::vector<std::pair<float, float>> scrolls;
  GestureMap detectors;
  GestureHandlerMap handlers;

 private:
  int id_;
};

const std::vector<std::string> kCallbacks = {"onBegin", "onStart", "onUpdate",
                                             "onEnd"};

lepus::Value Config(const char* name, double value) {
  auto map = lepus::Dictionary::Create();
  map->SetValue(name, lepus::Value(value));
  return lepus::Value(std::move(map));
}

class HarmonyGestureBaselineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    context_.emit = [this](const GestureEvent& event) {
      events_.push_back(event);
      names_.push_back(event.Name());
    };
  }

  std::shared_ptr<GestureDetectorImpl> Detector(
      uint32_t id, GestureType type, lepus::Value config = {},
      const std::vector<std::string>& callbacks = kCallbacks,
      const std::unordered_map<std::string, std::vector<uint32_t>>& relations =
          {}) {
    std::vector<GestureCallback> names;
    for (const auto& name : callbacks) {
      names.emplace_back(base::String(name), lepus::Value(), lepus::Value());
    }
    return std::make_shared<GestureDetectorImpl>(id, type, names, relations,
                                                 std::move(config));
  }

  std::shared_ptr<BaseGestureHandler> Handler(
      GestureType type, lepus::Value config = {},
      const std::vector<std::string>& callbacks = kCallbacks) {
    GestureMap detectors{{1, Detector(1, type, std::move(config), callbacks)}};
    return BaseGestureHandler::ConvertToGestureHandler(10, &context_, member_,
                                                       detectors)
        .at(1);
  }

  std::shared_ptr<BaselineMember> Member(
      int id, GestureType type,
      const std::unordered_map<std::string, std::vector<uint32_t>>& relations =
          {}) {
    auto member = std::make_shared<BaselineMember>(id);
    member->detectors.emplace(id,
                              Detector(id, type, {}, kCallbacks, relations));
    member->handlers = BaseGestureHandler::ConvertToGestureHandler(
        id, &context_, member, member->detectors);
    return member;
  }

  void Send(const std::shared_ptr<BaseGestureHandler>& handler, int action,
            float x = 0, float y = 0,
            const std::shared_ptr<GestureExtraBundle>& bundle = nullptr) {
    ArkUI_UIInputEvent input{action, x, y};
    const char* name = action == UI_TOUCH_EVENT_ACTION_DOWN ? TouchEvent::START
                       : action == UI_TOUCH_EVENT_ACTION_MOVE ? TouchEvent::MOVE
                       : action == UI_TOUCH_EVENT_ACTION_UP
                           ? TouchEvent::UP
                           : TouchEvent::CANCEL;
    auto touch = std::make_shared<TouchEvent>(10, name);
    float target[] = {x, y};
    float page[] = {30.5f, -40.25f};
    float client[] = {100.25f, -200.5f};
    touch->SetTargetPoint(target);
    touch->SetPagePoint(page);
    touch->SetClientPoint(client);
    handler->HandleMotionEvent(&input, touch, 0, 0, false, bundle);
  }

  void DrainTimers() { fml::MessageLoop::GetCurrent().RunExpiredTasksNow(); }

  LynxContext context_;
  std::shared_ptr<BaselineMember> member_ =
      std::make_shared<BaselineMember>(10);
  std::vector<GestureEvent> events_;
  std::vector<std::string> names_;
};

TEST_F(HarmonyGestureBaselineTest, PanAndNativeReleasePreserveCallbackOrder) {
  for (auto type : {GestureType::PAN, GestureType::NATIVE}) {
    names_.clear();
    auto handler = Handler(type);
    Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
    Send(handler, UI_TOUCH_EVENT_ACTION_MOVE, 20, 0);
    Send(handler, UI_TOUCH_EVENT_ACTION_MOVE, 30, 0);
    Send(handler, UI_TOUCH_EVENT_ACTION_UP, 30, 0);
    EXPECT_EQ(names_,
              (std::vector<std::string>{"onBegin", "onStart", "onUpdate",
                                        "onUpdate", "onEnd"}));
    EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
  }
}

TEST_F(HarmonyGestureBaselineTest,
       PanThresholdTruncatesConvertedPhysicalPixels) {
  context_.density = 2;
  for (float direction : {-1.f, 1.f}) {
    auto handler = Handler(GestureType::PAN, Config("minDistance", 10.25));
    Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
    Send(handler, UI_TOUCH_EVENT_ACTION_MOVE, direction * 20, direction * 20);
    EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
    Send(handler, UI_TOUCH_EVENT_ACTION_MOVE, direction * 20.25f, 0);
    EXPECT_TRUE(handler->IsActive());
  }
}

TEST_F(HarmonyGestureBaselineTest,
       PanWithoutBeginSubscriptionSuppressesStartAndEnd) {
  auto handler = Handler(GestureType::PAN, {}, {"onStart", "onEnd"});
  Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
  Send(handler, UI_TOUCH_EVENT_ACTION_MOVE, 1, 0);
  EXPECT_TRUE(handler->IsActive());
  Send(handler, UI_TOUCH_EVENT_ACTION_UP, 1, 0);
  EXPECT_TRUE(names_.empty());
}

TEST_F(HarmonyGestureBaselineTest, HandlerCancelLeavesRecognizersPending) {
  for (auto type :
       {GestureType::PAN, GestureType::TAP, GestureType::LONG_PRESS}) {
    names_.clear();
    auto handler = Handler(type);
    Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
    Send(handler, UI_TOUCH_EVENT_ACTION_CANCEL);
    EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
    EXPECT_EQ(names_, (std::vector<std::string>{"onBegin"}));
  }
}

TEST_F(HarmonyGestureBaselineTest,
       PanEndDeduplicatesAndResetStartsAnotherSequence) {
  auto handler = Handler(GestureType::PAN);
  Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
  handler->End();
  handler->Fail();
  EXPECT_TRUE(handler->IsEnd());
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onEnd"}));
  handler->Reset();
  Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
  Send(handler, UI_TOUCH_EVENT_ACTION_MOVE, 1, 0);
  Send(handler, UI_TOUCH_EVENT_ACTION_UP, 1, 0);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onEnd", "onBegin",
                                              "onStart", "onUpdate", "onEnd"}));
}

TEST_F(HarmonyGestureBaselineTest,
       TouchPayloadPreservesCoordinatesAndUsesUnixMilliseconds) {
  auto handler = Handler(GestureType::PAN);
  const auto now_ms = [] {
    return std::chrono::system_clock::now().time_since_epoch() /
           std::chrono::milliseconds(1);
  };
  auto before = now_ms();
  Send(handler, UI_TOUCH_EVENT_ACTION_DOWN, -1.25f, 2.75f);
  ASSERT_EQ(events_.size(), 1u);
  const auto& params = events_.back().ParamValue();
  EXPECT_EQ(events_.back().ID(), 10);
  EXPECT_EQ(events_.back().GestureId(), 1u);
  EXPECT_DOUBLE_EQ(params.GetProperty("x").Number(), -1.25);
  EXPECT_DOUBLE_EQ(params.GetProperty("y").Number(), 2.75);
  EXPECT_DOUBLE_EQ(params.GetProperty("pageX").Number(), 30.5);
  EXPECT_DOUBLE_EQ(params.GetProperty("pageY").Number(), -40.25);
  EXPECT_DOUBLE_EQ(params.GetProperty("clientX").Number(), 100.25);
  EXPECT_DOUBLE_EQ(params.GetProperty("clientY").Number(), -200.5);
  EXPECT_EQ(params.GetProperty("type").StdString(), TouchEvent::START);
  EXPECT_GE(params.GetProperty("timestamp").Number(), before);
  EXPECT_LE(params.GetProperty("timestamp").Number(), now_ms());
}

TEST_F(HarmonyGestureBaselineTest, TapDistanceBoundaryAndBeyond) {
  for (float distance : {-10.25f, -10.f, 10.f, 10.25f}) {
    SCOPED_TRACE(distance);
    names_.clear();
    auto handler = Handler(GestureType::TAP);
    Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
    Send(handler, UI_TOUCH_EVENT_ACTION_MOVE, distance, distance);
    Send(handler, UI_TOUCH_EVENT_ACTION_UP);
    EXPECT_EQ(names_,
              std::abs(distance) <= 10
                  ? (std::vector<std::string>{"onBegin", "onStart", "onEnd"})
                  : (std::vector<std::string>{"onBegin", "onEnd"}));
  }
}

TEST_F(HarmonyGestureBaselineTest,
       DefaultAndExplicitDistanceDifferAtDensityTwo) {
  context_.density = 2;
  for (auto type : {GestureType::TAP, GestureType::LONG_PRESS}) {
    auto implicit = Handler(type);
    auto explicit_value = Handler(type, Config("maxDistance", 10));
    Send(implicit, UI_TOUCH_EVENT_ACTION_DOWN);
    Send(explicit_value, UI_TOUCH_EVENT_ACTION_DOWN);
    Send(implicit, UI_TOUCH_EVENT_ACTION_MOVE, 15, 0);
    Send(explicit_value, UI_TOUCH_EVENT_ACTION_MOVE, 15, 0);
    EXPECT_EQ(implicit->GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
    EXPECT_EQ(explicit_value->GetGestureStatus(),
              GestureConstants::LYNX_STATE_BEGIN);
  }
}

TEST_F(HarmonyGestureBaselineTest, TapTimeoutPreventsSubsequentClick) {
  auto handler = Handler(GestureType::TAP, Config("maxDuration", 0));
  Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
  DrainTimers();
  Send(handler, UI_TOUCH_EVENT_ACTION_UP);
  EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onEnd"}));
}

TEST_F(HarmonyGestureBaselineTest,
       LongPressFailsBeyondDistanceAfterActivation) {
  auto handler = Handler(GestureType::LONG_PRESS, Config("minDuration", 0));
  Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
  DrainTimers();
  ASSERT_TRUE(handler->IsActive());
  Send(handler, UI_TOUCH_EVENT_ACTION_MOVE, 10.25f, 0);
  EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
  Send(handler, UI_TOUCH_EVENT_ACTION_UP);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onStart", "onEnd"}));
}

TEST_F(HarmonyGestureBaselineTest, LongPressReleaseCancelsPendingActivation) {
  auto handler = Handler(GestureType::LONG_PRESS, Config("minDuration", 0));
  Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
  Send(handler, UI_TOUCH_EVENT_ACTION_UP);
  DrainTimers();
  EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onEnd"}));
}

TEST_F(HarmonyGestureBaselineTest, ResetCurrentlyLeavesDiscreteTimersPending) {
  for (auto type : {GestureType::TAP, GestureType::LONG_PRESS}) {
    names_.clear();
    auto handler = Handler(
        type,
        Config(type == GestureType::TAP ? "maxDuration" : "minDuration", 0));
    Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
    handler->Reset();
    DrainTimers();
    EXPECT_EQ(handler->GetGestureStatus(),
              type == GestureType::TAP ? GestureConstants::LYNX_STATE_FAIL
                                       : GestureConstants::LYNX_STATE_ACTIVE);
    EXPECT_EQ(names_, type == GestureType::TAP
                          ? (std::vector<std::string>{"onBegin", "onEnd"})
                          : (std::vector<std::string>{"onBegin", "onStart"}));
  }
}

TEST_F(HarmonyGestureBaselineTest, DestroyedDiscreteHandlersDoNotRunTimers) {
  for (auto type : {GestureType::TAP, GestureType::LONG_PRESS}) {
    names_.clear();
    auto handler = Handler(
        type,
        Config(type == GestureType::TAP ? "maxDuration" : "minDuration", 0));
    Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
    handler.reset();
    DrainTimers();
    EXPECT_EQ(names_, (std::vector<std::string>{"onBegin"}));
  }
}

TEST_F(HarmonyGestureBaselineTest,
       FlingAfterReleaseUpdatesWithoutStartCallback) {
  auto handler = Handler(GestureType::FLING);
  Send(handler, UI_TOUCH_EVENT_ACTION_DOWN);
  EXPECT_EQ(handler->GetGestureStatus(),
            GestureConstants::LYNX_STATE_UNDETERMINED);
  Send(handler, UI_TOUCH_EVENT_ACTION_UP);
  handler->HandleMotionEvent(nullptr, nullptr, 4.25f, -7.5f, false, nullptr);
  EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onUpdate"}));
  const auto& params = events_.back().ParamValue();
  EXPECT_DOUBLE_EQ(params.GetProperty("deltaX").Number(), 4.25);
  EXPECT_DOUBLE_EQ(params.GetProperty("deltaY").Number(), -7.5);
  EXPECT_FALSE(params.Contains("timestamp"));
  EXPECT_FALSE(params.Contains("pageX"));
  handler->HandleMotionEvent(nullptr, nullptr,
                             std::numeric_limits<float>::min(),
                             std::numeric_limits<float>::min(), false, nullptr);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onUpdate", "onEnd"}));
}

TEST_F(HarmonyGestureBaselineTest,
       FlingWithoutReleaseConsumesFirstFrameAsActivation) {
  auto handler = Handler(GestureType::FLING);
  handler->HandleMotionEvent(nullptr, nullptr, 4, -7, false, nullptr);
  EXPECT_TRUE(handler->IsActive());
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onStart"}));
  handler->HandleMotionEvent(nullptr, nullptr, 3, -6, false, nullptr);
  EXPECT_EQ(names_,
            (std::vector<std::string>{"onBegin", "onStart", "onUpdate"}));
}

TEST_F(HarmonyGestureBaselineTest,
       FailedSimultaneousDefaultDoesNotConsumeSharedDelta) {
  auto handler = Handler(GestureType::DEFAULT);
  auto bundle = std::make_shared<GestureExtraBundle>();
  bundle->SetIsNeedConsumedSimultaneousGesture(true);
  bundle->SetSimultaneousDeltaX(4.25f);
  bundle->SetSimultaneousDeltaY(-7.5f);
  handler->HandleMotionEvent(nullptr, nullptr, 0, 0, true, bundle);
  ASSERT_EQ(member_->scrolls,
            (std::vector<std::pair<float, float>>{{4.25f, -7.5f}}));
  member_->scrolls.clear();
  handler->Fail();
  handler->HandleMotionEvent(nullptr, nullptr, 0, 0, true, bundle);
  EXPECT_TRUE(member_->scrolls.empty());
}

TEST_F(HarmonyGestureBaselineTest,
       DefaultLocksDirectionAndPublishesFullConsumedDelta) {
  auto handler = Handler(GestureType::DEFAULT);
  auto bundle = std::make_shared<GestureExtraBundle>();
  Send(handler, UI_TOUCH_EVENT_ACTION_DOWN, 0, 0, bundle);
  Send(handler, UI_TOUCH_EVENT_ACTION_MOVE, 4, 4, bundle);
  EXPECT_EQ(member_->scrolls, (std::vector<std::pair<float, float>>{{0, -4}}));
  EXPECT_EQ(bundle->GestureDirection(), GestureConstants::DIRECTION_VERTICAL);
  EXPECT_TRUE(bundle->IsNeedConsumedSimultaneousGesture());
  EXPECT_FLOAT_EQ(bundle->SimultaneousDeltaX(), 0);
  EXPECT_FLOAT_EQ(bundle->SimultaneousDeltaY(), -4);
  bundle->ResetSimultaneousDelta();
  EXPECT_EQ(bundle->GestureDirection(), GestureConstants::DIRECTION_VERTICAL);
  EXPECT_FALSE(bundle->IsNeedConsumedSimultaneousGesture());
}

TEST_F(HarmonyGestureBaselineTest,
       FactoryKeepsDuplicateTypesByIdAndSkipsUnsupportedTypes) {
  GestureMap detectors{{8, Detector(8, GestureType::PAN)},
                       {3, Detector(3, GestureType::PAN)},
                       {9, Detector(9, GestureType::PINCH)},
                       {10, Detector(10, GestureType::ROTATION)}};
  auto handlers = BaseGestureHandler::ConvertToGestureHandler(
      10, &context_, member_, detectors);
  ASSERT_EQ(handlers.size(), 2u);
  EXPECT_EQ(handlers.at(3)->GetGestureDetector()->gesture_id(), 3u);
  EXPECT_EQ(handlers.at(8)->GetGestureDetector()->gesture_id(), 8u);
}

TEST_F(HarmonyGestureBaselineTest, FailedParentCanReenterAfterChildFails) {
  auto arena = std::make_shared<GestureArenaManager>(true, &context_);
  auto manager = std::make_shared<GestureDetectorManager>(arena);
  GestureHandlerTrigger trigger(&context_, manager);
  auto parent = Member(1, GestureType::PAN);
  auto child = Member(2, GestureType::DEFAULT);
  std::vector<std::weak_ptr<GestureArenaMember>> chain{parent, child};
  trigger.InitCurrentWinnerWhenDown(parent);
  parent->handlers.at(1)->Fail();
  EXPECT_EQ(trigger.ReCompeteByGestures(chain, parent).lock(), child);
  child->handlers.at(2)->Fail();
  EXPECT_EQ(trigger.ReCompeteByGestures(chain, child).lock(), parent);
  EXPECT_EQ(parent->handlers.at(1)->GetGestureStatus(),
            GestureConstants::LYNX_STATE_INIT);
}

TEST_F(HarmonyGestureBaselineTest,
       ExplicitEndStopsCompetitionAndEmptyHandlersYield) {
  auto arena = std::make_shared<GestureArenaManager>(true, &context_);
  auto manager = std::make_shared<GestureDetectorManager>(arena);
  GestureHandlerTrigger trigger(&context_, manager);
  auto first = Member(1, GestureType::PAN);
  auto second = Member(2, GestureType::PAN);
  std::vector<std::weak_ptr<GestureArenaMember>> chain{first, second};
  trigger.InitCurrentWinnerWhenDown(first);
  first->handlers.at(1)->End();
  EXPECT_TRUE(trigger.ReCompeteByGestures(chain, first).expired());
  first->handlers.clear();
  EXPECT_EQ(trigger.ReCompeteByGestures(chain, first).lock(), second);
}

TEST_F(HarmonyGestureBaselineTest,
       WaitForOrdersLaterCandidatesAndTruncatesAncestors) {
  auto arena = std::make_shared<GestureArenaManager>(true, &context_);
  GestureDetectorManager manager(arena);
  auto first = Member(1, GestureType::PAN, {{"waitFor", {3, 2}}});
  auto second = Member(2, GestureType::PAN);
  auto third = Member(3, GestureType::PAN);
  auto ancestor = Member(4, GestureType::PAN);
  for (const auto& member : {first, second, third, ancestor}) {
    manager.RegisterGestureDetector(member->Sign(),
                                    member->detectors.at(member->Sign()));
  }
  auto chain = manager.ConvertResponseChainToCompeteChain(
      {first, second, third, ancestor});
  ASSERT_EQ(chain.size(), 3u);
  EXPECT_EQ(chain[0].lock(), third);
  EXPECT_EQ(chain[1].lock(), second);
  EXPECT_EQ(chain[2].lock(), first);
}

TEST_F(HarmonyGestureBaselineTest,
       SimultaneousRegistrationAndRemovalFollowGestureIds) {
  auto arena = std::make_shared<GestureArenaManager>(true, &context_);
  GestureDetectorManager manager(arena);
  auto first = Member(1, GestureType::PAN, {{"simultaneous", {2}}});
  auto second = Member(2, GestureType::DEFAULT);
  arena->AddMember(first);
  arena->AddMember(second);
  manager.RegisterGestureDetector(1, first->detectors.at(1));
  manager.RegisterGestureDetector(2, second->detectors.at(2));
  auto simultaneous = manager.HandleSimultaneousWinner(first);
  ASSERT_EQ(simultaneous.first.size(), 1u);
  EXPECT_EQ(simultaneous.first.begin()->lock(), second);
  EXPECT_TRUE(simultaneous.second.empty());
  manager.UnregisterGestureDetector(2, second->detectors.at(2));
  EXPECT_TRUE(manager.HandleSimultaneousWinner(first).first.empty());
}

TEST_F(HarmonyGestureBaselineTest, PanReleaseDoesNotDiscardSameNodeFling) {
  auto arena = std::make_shared<GestureArenaManager>(true, &context_);
  auto manager = std::make_shared<GestureDetectorManager>(arena);
  GestureHandlerTrigger trigger(&context_, manager);
  member_->detectors = {{1, Detector(1, GestureType::PAN)},
                        {2, Detector(2, GestureType::FLING)}};
  member_->handlers = BaseGestureHandler::ConvertToGestureHandler(
      10, &context_, member_, member_->detectors);
  trigger.InitCurrentWinnerWhenDown(member_);
  for (auto& entry : member_->handlers) {
    Send(entry.second, UI_TOUCH_EVENT_ACTION_DOWN);
    Send(entry.second, UI_TOUCH_EVENT_ACTION_MOVE, 20, 0);
    Send(entry.second, UI_TOUCH_EVENT_ACTION_UP, 20, 0);
  }
  EXPECT_EQ(trigger.GetCurrentMemberState(member_),
            GestureConstants::LYNX_STATE_BEGIN);
  names_.clear();
  member_->handlers.at(2)->HandleMotionEvent(nullptr, nullptr, 8, -9, false,
                                             nullptr);
  EXPECT_EQ(names_, (std::vector<std::string>{"onUpdate"}));
}

TEST_F(HarmonyGestureBaselineTest,
       ContinueWithCanReachMemberOutsideResponseChain) {
  auto arena = std::make_shared<GestureArenaManager>(true, &context_);
  GestureDetectorManager manager(arena);
  auto first = Member(1, GestureType::PAN, {{"continueWith", {2}}});
  auto next = Member(2, GestureType::PAN);
  arena->AddMember(first);
  arena->AddMember(next);
  manager.RegisterGestureDetector(1, first->detectors.at(1));
  manager.RegisterGestureDetector(2, next->detectors.at(2));
  auto chain = manager.ConvertResponseChainToCompeteChain({first});
  ASSERT_EQ(chain.size(), 2u);
  EXPECT_EQ(chain[0].lock(), first);
  EXPECT_EQ(chain[1].lock(), next);
  manager.UnregisterGestureDetector(2, next->detectors.at(2));
  EXPECT_EQ(manager.ConvertResponseChainToCompeteChain({first}).size(), 1u);
}

}  // namespace
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
