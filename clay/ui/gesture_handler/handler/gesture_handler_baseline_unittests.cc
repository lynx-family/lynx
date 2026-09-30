// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "clay/gfx/pixel_helper.h"
#include "clay/ui/gesture_handler/arena/gesture_arena_manager.h"
#include "clay/ui/gesture_handler/common/gesture_extra_bundle.h"
#include "clay/ui/gesture_handler/detector/gesture_detector_manager.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "clay/ui/window/viewport_metrics.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {
namespace {

using ::testing::_;
using EventType = PointerEvent::EventType;

const std::vector<std::string> kCallbacks = {
    GestureConstants::ON_BEGIN, GestureConstants::ON_START,
    GestureConstants::ON_UPDATE, GestureConstants::ON_END};

Value Config(const char* key, Value value) {
  Value::Map map;
  map.emplace(key, std::move(value));
  return Value(std::move(map));
}

class GestureHandlerBaselineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    page_->SetEventDelegate(&delegate_);
    member_.SetScrollContainerDirection(GestureConstants::DIRECTION_VERTICAL);
    EXPECT_CALL(delegate_, OnGestureHandlerEvent(_, _, _, _, _, _, _, _, _))
        .Times(::testing::AnyNumber())
        .WillRepeatedly([this](const std::string& name, int sign, uint32_t id,
                               float, float, float, float, int64_t,
                               Value& params) {
          names_.push_back(name);
          EXPECT_EQ(sign, 10);
          EXPECT_EQ(id, 1u);
          if (params.IsMap() && params.GetMap().count("deltaX")) {
            delta_x_ = params.GetMap().at("deltaX").GetFloat();
            delta_y_ = params.GetMap().at("deltaY").GetFloat();
          }
        });
  }

  std::shared_ptr<BaseGestureHandler> Handler(
      GestureHandlerType type, Value config = {},
      std::vector<std::string> callbacks = kCallbacks) {
    GestureMap detectors;
    detectors.emplace(
        1, MakeDetector(1, type, std::move(callbacks), {}, std::move(config)));
    return BaseGestureHandler::ConvertToGestureHandler(
               member_.Sign(), page_.get(), member_.GetWeakPtr(), detectors)
        .at(1);
  }

  void Send(const std::shared_ptr<BaseGestureHandler>& handler, EventType type,
            FloatPoint point = {},
            const std::shared_ptr<GestureExtraBundle>& bundle = nullptr) {
    auto event = MakePointerEvent(type, point, ++timestamp_);
    handler->HandleMotionEvent(&event, 0, 0, false, bundle);
  }

  fml::RefPtr<TestTaskRunner> runner_ = TestTaskRunner::Create();
  ::testing::NiceMock<MockEventDelegate> delegate_;
  std::unique_ptr<PageView> page_ = MakeTestPageView(0, runner_);
  TestGestureArenaMember member_{10};
  std::vector<std::string> names_;
  float delta_x_ = 0;
  float delta_y_ = 0;
  uint64_t timestamp_ = 0;
};

TEST_F(GestureHandlerBaselineTest, PanAndNativeReleasePreserveCallbackOrder) {
  for (auto type : {GestureHandlerType::Pan, GestureHandlerType::Native}) {
    SCOPED_TRACE(static_cast<int>(type));
    names_.clear();
    auto handler = Handler(type);
    Send(handler, EventType::kDownEvent);
    Send(handler, EventType::kMoveEvent, {20, 0});
    Send(handler, EventType::kMoveEvent, {30, 0});
    Send(handler, EventType::kUpEvent, {30, 0});
    EXPECT_EQ(names_,
              (std::vector<std::string>{"onBegin", "onStart", "onUpdate",
                                        "onUpdate", "onEnd"}));
    EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
  }
}

TEST_F(GestureHandlerBaselineTest, PanThresholdIsStrictAxisBasedAndFractional) {
  ViewportMetrics metrics;
  metrics.device_pixel_ratio = 2;
  page_->SetViewportMetrics(metrics);
  const float threshold = kPixelTypeClay == kPixelTypePhysical ? 20.5f : 10.25f;
  for (float direction : {-1.f, 1.f}) {
    auto handler =
        Handler(GestureHandlerType::Pan, Config("minDistance", Value(10.25f)));
    Send(handler, EventType::kDownEvent);
    Send(handler, EventType::kMoveEvent,
         {direction * threshold, direction * threshold});
    EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
    Send(handler, EventType::kMoveEvent, {direction * (threshold + 0.25f), 0});
    EXPECT_TRUE(handler->IsActive());
  }
}

TEST_F(GestureHandlerBaselineTest,
       PanWithoutBeginSubscriptionSuppressesStartAndEnd) {
  auto handler = Handler(GestureHandlerType::Pan, {}, {"onStart", "onEnd"});
  Send(handler, EventType::kDownEvent);
  Send(handler, EventType::kMoveEvent, {1, 0});
  EXPECT_TRUE(handler->IsActive());
  Send(handler, EventType::kUpEvent, {1, 0});
  EXPECT_TRUE(names_.empty());
}

TEST_F(GestureHandlerBaselineTest, HandlerCancelLeavesPanAndTapPending) {
  for (auto type : {GestureHandlerType::Pan, GestureHandlerType::Tap}) {
    names_.clear();
    auto handler = Handler(type);
    Send(handler, EventType::kDownEvent);
    Send(handler, EventType::kCancel);
    EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
    EXPECT_EQ(names_, (std::vector<std::string>{"onBegin"}));
    handler->End();
  }
}

TEST_F(GestureHandlerBaselineTest,
       PanEndDeduplicatesAndResetStartsAnotherSequence) {
  auto handler = Handler(GestureHandlerType::Pan);
  Send(handler, EventType::kDownEvent);
  handler->End();
  handler->Fail();
  EXPECT_TRUE(handler->IsEnd());
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onEnd"}));
  handler->Reset();
  Send(handler, EventType::kDownEvent);
  Send(handler, EventType::kMoveEvent, {1, 0});
  Send(handler, EventType::kUpEvent, {1, 0});
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onEnd", "onBegin",
                                              "onStart", "onUpdate", "onEnd"}));
}

TEST_F(GestureHandlerBaselineTest, TapDistanceBoundaryAndBeyond) {
  for (float distance : {-10.25f, -10.f, 10.f, 10.25f}) {
    SCOPED_TRACE(distance);
    names_.clear();
    auto handler = Handler(GestureHandlerType::Tap);
    Send(handler, EventType::kDownEvent);
    Send(handler, EventType::kMoveEvent, {distance, distance});
    Send(handler, EventType::kUpEvent);
    EXPECT_EQ(names_,
              std::abs(distance) <= 10
                  ? (std::vector<std::string>{"onBegin", "onStart", "onEnd"})
                  : (std::vector<std::string>{"onBegin", "onEnd"}));
  }
}

TEST_F(GestureHandlerBaselineTest,
       DefaultDistanceAndExplicitDistanceFollowClayPixelUnits) {
  ViewportMetrics metrics;
  metrics.device_pixel_ratio = 2;
  page_->SetViewportMetrics(metrics);
  for (auto type : {GestureHandlerType::Tap, GestureHandlerType::LongPress}) {
    auto implicit = Handler(type);
    auto explicit_value = Handler(type, Config("maxDistance", Value(10)));
    Send(implicit, EventType::kDownEvent);
    Send(explicit_value, EventType::kDownEvent);
    Send(implicit, EventType::kMoveEvent, {15, 0});
    Send(explicit_value, EventType::kMoveEvent, {15, 0});
    EXPECT_EQ(implicit->GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
    EXPECT_EQ(explicit_value->GetGestureStatus(),
              kPixelTypeClay == kPixelTypePhysical
                  ? GestureConstants::LYNX_STATE_BEGIN
                  : GestureConstants::LYNX_STATE_FAIL);
    implicit->End();
    explicit_value->End();
  }
}

TEST_F(GestureHandlerBaselineTest, TapTimesOutExactlyAtDeadlineAndCannotClick) {
  auto handler =
      Handler(GestureHandlerType::Tap, Config("maxDuration", Value(100)));
  Send(handler, EventType::kDownEvent);
  runner_->AdvanceBy(fml::TimeDelta::FromMilliseconds(99));
  EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
  runner_->AdvanceBy(fml::TimeDelta::FromMilliseconds(1));
  EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
  Send(handler, EventType::kUpEvent);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onEnd"}));
}

TEST_F(GestureHandlerBaselineTest,
       LongPressFailsBeyondDistanceAfterActivation) {
  auto handler =
      Handler(GestureHandlerType::LongPress, Config("minDuration", Value(100)));
  Send(handler, EventType::kDownEvent);
  runner_->AdvanceBy(fml::TimeDelta::FromMilliseconds(99));
  EXPECT_FALSE(handler->IsActive());
  runner_->AdvanceBy(fml::TimeDelta::FromMilliseconds(1));
  ASSERT_TRUE(handler->IsActive());
  Send(handler, EventType::kMoveEvent, {10.25f, 0});
  EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
  Send(handler, EventType::kUpEvent);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onStart", "onEnd"}));
}

TEST_F(GestureHandlerBaselineTest, DiscreteResetAndEndCancelPendingTimers) {
  for (auto type : {GestureHandlerType::Tap, GestureHandlerType::LongPress}) {
    for (bool end : {false, true}) {
      names_.clear();
      auto handler = Handler(type);
      Send(handler, EventType::kDownEvent);
      if (end) {
        handler->End();
      } else {
        handler->Reset();
      }
      auto expected = names_;
      runner_->AdvanceBy(fml::TimeDelta::FromMilliseconds(1000));
      EXPECT_EQ(names_, expected);
      EXPECT_EQ(handler->GetGestureStatus(),
                end ? GestureConstants::LYNX_STATE_END
                    : GestureConstants::LYNX_STATE_INIT);
    }
  }
}

TEST_F(GestureHandlerBaselineTest, DestroyedDiscreteHandlersDoNotRunTimers) {
  for (auto type : {GestureHandlerType::Tap, GestureHandlerType::LongPress}) {
    names_.clear();
    auto handler = Handler(type);
    Send(handler, EventType::kDownEvent);
    handler.reset();
    runner_->AdvanceBy(fml::TimeDelta::FromMilliseconds(1000));
    EXPECT_EQ(names_, (std::vector<std::string>{"onBegin"}));
  }
}

TEST_F(GestureHandlerBaselineTest,
       FlingAfterReleaseUpdatesWithoutStartCallback) {
  auto handler = Handler(GestureHandlerType::Fling);
  Send(handler, EventType::kDownEvent);
  EXPECT_EQ(handler->GetGestureStatus(),
            GestureConstants::LYNX_STATE_UNDETERMINED);
  Send(handler, EventType::kUpEvent);
  handler->HandleMotionEvent(nullptr, 4.25f, -7.5f, false, nullptr);
  EXPECT_EQ(handler->GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onUpdate"}));
  EXPECT_FLOAT_EQ(delta_x_, 4.25f);
  EXPECT_FLOAT_EQ(delta_y_, -7.5f);
  handler->HandleMotionEvent(nullptr, std::numeric_limits<float>::min(),
                             std::numeric_limits<float>::min(), false, nullptr);
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onUpdate", "onEnd"}));
}

TEST_F(GestureHandlerBaselineTest,
       FlingWithoutReleaseConsumesFirstFrameAsActivation) {
  auto handler = Handler(GestureHandlerType::Fling);
  handler->HandleMotionEvent(nullptr, 4, -7, false, nullptr);
  EXPECT_TRUE(handler->IsActive());
  EXPECT_EQ(names_, (std::vector<std::string>{"onBegin", "onStart"}));
  handler->HandleMotionEvent(nullptr, 3, -6, false, nullptr);
  EXPECT_EQ(names_,
            (std::vector<std::string>{"onBegin", "onStart", "onUpdate"}));
}

TEST_F(GestureHandlerBaselineTest,
       FailedSimultaneousDefaultDoesNotConsumeSharedDelta) {
  auto handler = Handler(GestureHandlerType::Default);
  auto bundle = std::make_shared<GestureExtraBundle>();
  bundle->SetIsNeedConsumedSimultaneousGesture(true);
  bundle->SetSimultaneousDeltaX(4.25f);
  bundle->SetSimultaneousDeltaY(-7.5f);
  handler->HandleMotionEvent(nullptr, 0, 0, true, bundle);
  EXPECT_EQ(member_.TakeScrollCalls(),
            (std::vector<std::pair<float, float>>{{4.25f, -7.5f}}));
  handler->Fail();
  handler->HandleMotionEvent(nullptr, 0, 0, true, bundle);
  EXPECT_TRUE(member_.TakeScrollCalls().empty());
}

TEST_F(GestureHandlerBaselineTest,
       DefaultLocksDirectionAndPublishesFullConsumedDelta) {
  auto handler = Handler(GestureHandlerType::Default);
  auto bundle = std::make_shared<GestureExtraBundle>();
  Send(handler, EventType::kDownEvent, {}, bundle);
  Send(handler, EventType::kMoveEvent, {4, 4}, bundle);
  EXPECT_EQ(member_.TakeScrollCalls(),
            (std::vector<std::pair<float, float>>{{0, -4}}));
  EXPECT_EQ(bundle->GestureDirection(), GestureConstants::DIRECTION_VERTICAL);
  EXPECT_TRUE(bundle->IsNeedConsumedSimultaneousGesture());
  EXPECT_FLOAT_EQ(bundle->SimultaneousDeltaX(), 0);
  EXPECT_FLOAT_EQ(bundle->SimultaneousDeltaY(), -4);
  bundle->ResetSimultaneousDelta();
  EXPECT_EQ(bundle->GestureDirection(), GestureConstants::DIRECTION_VERTICAL);
  EXPECT_FALSE(bundle->IsNeedConsumedSimultaneousGesture());
  Send(handler, EventType::kMoveEvent, {100, 5}, bundle);
  EXPECT_EQ(member_.TakeScrollCalls(),
            (std::vector<std::pair<float, float>>{{0, -1}}));
}

TEST_F(GestureHandlerBaselineTest, DefaultScrollsWithoutUpdateSubscription) {
  auto handler = Handler(GestureHandlerType::Default, {}, {});
  Send(handler, EventType::kDownEvent);
  Send(handler, EventType::kMoveEvent, {0, 4});
  EXPECT_EQ(member_.TakeScrollCalls(),
            (std::vector<std::pair<float, float>>{{0, -4}}));
  EXPECT_TRUE(names_.empty());
}

TEST_F(GestureHandlerBaselineTest,
       FactoryKeepsDuplicateTypesByIdAndIgnoresUnsupportedTypes) {
  GestureMap detectors = {{8, MakeDetector(8, GestureHandlerType::Pan)},
                          {3, MakeDetector(3, GestureHandlerType::Pan)},
                          {9, MakeDetector(9, GestureHandlerType::Pinch)},
                          {10, MakeDetector(10, GestureHandlerType::Rotation)}};
  auto handlers = BaseGestureHandler::ConvertToGestureHandler(
      member_.Sign(), page_.get(), member_.GetWeakPtr(), detectors);
  ASSERT_EQ(handlers.size(), 2u);
  EXPECT_EQ(handlers.at(3)->GetGestureDetector()->gesture_id(), 3u);
  EXPECT_EQ(handlers.at(8)->GetGestureDetector()->gesture_id(), 8u);
}

TEST_F(GestureHandlerBaselineTest,
       WaitForOrdersLaterCandidatesAndTruncatesAncestors) {
  auto arena = std::make_shared<GestureArenaManager>(true, page_.get());
  GestureDetectorManager manager(arena);
  TestGestureArenaMember first(1), second(2), third(3), ancestor(4);
  first.SetGestureDetectorMap({{1, MakeDetector(1, GestureHandlerType::Pan, {},
                                                {{"waitFor", {3, 2}}})}});
  second.SetGestureDetectorMap({{2, MakeDetector(2, GestureHandlerType::Pan)}});
  third.SetGestureDetectorMap({{3, MakeDetector(3, GestureHandlerType::Pan)}});
  for (auto member : {&first, &second, &third}) {
    manager.RegisterGestureDetector(
        member->Sign(), member->GetGestureDetectorMap().at(member->Sign()));
  }
  auto chain = manager.ConvertResponseChainToCompeteChain(
      {first.GetWeakPtr(), second.GetWeakPtr(), third.GetWeakPtr(),
       ancestor.GetWeakPtr()});
  ASSERT_EQ(chain.size(), 3u);
  EXPECT_EQ(chain[0].get(), &third);
  EXPECT_EQ(chain[1].get(), &second);
  EXPECT_EQ(chain[2].get(), &first);
}

TEST_F(GestureHandlerBaselineTest,
       ContinueWithCanReachMemberOutsideResponseChainAndUnregisterRemovesIt) {
  auto arena = std::make_shared<GestureArenaManager>(true, page_.get());
  GestureDetectorManager manager(arena);
  TestGestureArenaMember first(1), next(2);
  first.SetGestureDetectorMap({{1, MakeDetector(1, GestureHandlerType::Pan, {},
                                                {{"continueWith", {2}}})}});
  next.SetGestureDetectorMap({{2, MakeDetector(2, GestureHandlerType::Pan)}});
  arena->AddMember(first.GetWeakPtr());
  arena->AddMember(next.GetWeakPtr());
  manager.RegisterGestureDetector(1, first.GetGestureDetectorMap().at(1));
  manager.RegisterGestureDetector(2, next.GetGestureDetectorMap().at(2));
  auto chain = manager.ConvertResponseChainToCompeteChain({first.GetWeakPtr()});
  ASSERT_EQ(chain.size(), 2u);
  EXPECT_EQ(chain[0].get(), &first);
  EXPECT_EQ(chain[1].get(), &next);
  manager.UnregisterGestureDetector(2, next.GetGestureDetectorMap().at(2));
  EXPECT_EQ(
      manager.ConvertResponseChainToCompeteChain({first.GetWeakPtr()}).size(),
      1u);
}

}  // namespace
}  // namespace testing
}  // namespace clay
