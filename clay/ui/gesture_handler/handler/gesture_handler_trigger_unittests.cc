// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#include "clay/ui/gesture_handler/handler/gesture_handler_trigger.h"
#undef private

#include <cfloat>
#include <cmath>
#include <memory>
#include <vector>

#include "clay/ui/gesture_handler/arena/gesture_arena_manager.h"
#include "clay/ui/gesture_handler/common/gesture_extra_bundle.h"
#include "clay/ui/gesture_handler/detector/gesture_detector_manager.h"
#include "clay/ui/gesture_handler/gesture_handler_delegate.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/default_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "clay/ui/gesture_handler/handler/pan_gesture_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

using ::testing::Eq;

class GestureHandlerTriggerTest : public ::testing::Test {};

TEST_F(GestureHandlerTriggerTest,
       InitCurrentWinnerWhenDown_ResetsHandlersAndCreatesBundle) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember member(1);
  GestureMap detectors;
  detectors.emplace(1, MakeDetector(1, GestureHandlerType::Pan));
  member.SetGestureDetectorMap(detectors);
  auto handlers = BaseGestureHandler::ConvertToGestureHandler(
      member.Sign(), page_view.get(), member.GetWeakPtr(), detectors);
  member.SetGestureHandlers(handlers);

  handlers.begin()->second->Activate();
  ASSERT_TRUE(handlers.begin()->second->IsActive());

  trigger.InitCurrentWinnerWhenDown(member.GetWeakPtr());
  EXPECT_EQ(handlers.begin()->second->GetGestureStatus(),
            GestureConstants::LYNX_STATE_INIT);
  EXPECT_NE(trigger.current_extra_bundle_, nullptr);
}

TEST_F(GestureHandlerTriggerTest,
       DispatchBubbleTouchEvent_ForwardsTouchesToAllBubbleMembers) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember m1(1);
  TestGestureArenaMember m2(2);

  GestureMap d1;
  d1.emplace(1, MakeDetector(1, GestureHandlerType::Pan,
                             {GestureConstants::ON_TOUCHES_DOWN}));
  m1.SetGestureDetectorMap(d1);
  m1.SetGestureHandlers(BaseGestureHandler::ConvertToGestureHandler(
      m1.Sign(), page_view.get(), m1.GetWeakPtr(), d1));

  GestureMap d2;
  d2.emplace(2, MakeDetector(2, GestureHandlerType::Pan,
                             {GestureConstants::ON_TOUCHES_DOWN}));
  m2.SetGestureDetectorMap(d2);
  m2.SetGestureHandlers(BaseGestureHandler::ConvertToGestureHandler(
      m2.Sign(), page_view.get(), m2.GetWeakPtr(), d2));

  std::vector<fml::WeakPtr<GestureArenaMember>> bubble = {m1.GetWeakPtr(),
                                                          m2.GetWeakPtr()};
  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);

  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(
                  ::testing::StrEq(GestureConstants::ON_TOUCHES_DOWN), Eq(1),
                  Eq(1), ::testing::_, ::testing::_, ::testing::_, ::testing::_,
                  ::testing::_, ::testing::_))
      .Times(1);
  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(
                  ::testing::StrEq(GestureConstants::ON_TOUCHES_DOWN), Eq(2),
                  Eq(2), ::testing::_, ::testing::_, ::testing::_, ::testing::_,
                  ::testing::_, ::testing::_))
      .Times(1);

  trigger.DispatchBubbleTouchEvent(&down, bubble, m1.GetWeakPtr());
}

TEST_F(GestureHandlerTriggerTest,
       HandleGestureDetectorState_FailRoutesToMatchingHandler) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember member(1);
  GestureMap detectors;
  detectors.emplace(1, MakeDetector(1, GestureHandlerType::Pan));
  detectors.emplace(2, MakeDetector(2, GestureHandlerType::Default));
  member.SetGestureDetectorMap(detectors);
  auto handlers = BaseGestureHandler::ConvertToGestureHandler(
      member.Sign(), page_view.get(), member.GetWeakPtr(), detectors);
  member.SetGestureHandlers(handlers);

  trigger.HandleGestureDetectorState(
      member.GetWeakPtr(), 2,
      static_cast<int>(GestureHandlerDelegate::LynxGestureState::FAIL));
  EXPECT_EQ(handlers.at(2)->GetGestureStatus(),
            GestureConstants::LYNX_STATE_FAIL);
  EXPECT_EQ(handlers.at(1)->GetGestureStatus(),
            GestureConstants::LYNX_STATE_INIT);
}

TEST_F(GestureHandlerTriggerTest,
       HandleGestureDetectorState_EndRoutesToMatchingHandler) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember member(1);
  GestureMap detectors;
  detectors.emplace(1, MakeDetector(1, GestureHandlerType::Pan));
  detectors.emplace(2, MakeDetector(2, GestureHandlerType::Default));
  member.SetGestureDetectorMap(detectors);
  auto handlers = BaseGestureHandler::ConvertToGestureHandler(
      member.Sign(), page_view.get(), member.GetWeakPtr(), detectors);
  member.SetGestureHandlers(handlers);

  trigger.HandleGestureDetectorState(
      member.GetWeakPtr(), 1,
      static_cast<int>(GestureHandlerDelegate::LynxGestureState::END));
  EXPECT_EQ(handlers.at(1)->GetGestureStatus(),
            GestureConstants::LYNX_STATE_END);
  EXPECT_EQ(handlers.at(2)->GetGestureStatus(),
            GestureConstants::LYNX_STATE_INIT);
}

TEST_F(GestureHandlerTriggerTest,
       ResolveTouchEvent_Down_ResetsCandidatesAndDispatches) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember winner(1);
  GestureMap detectors;
  detectors.emplace(1, MakeDetector(1, GestureHandlerType::Pan,
                                    {GestureConstants::ON_BEGIN}));
  winner.SetGestureDetectorMap(detectors);
  auto handlers = BaseGestureHandler::ConvertToGestureHandler(
      winner.Sign(), page_view.get(), winner.GetWeakPtr(), detectors);
  winner.SetGestureHandlers(handlers);

  trigger.InitCurrentWinnerWhenDown(winner.GetWeakPtr());

  TestGestureArenaMember other(2);
  other.SetGestureDetectorMap({});
  other.SetGestureHandlers({});

  std::vector<fml::WeakPtr<GestureArenaMember>> compete = {winner.GetWeakPtr(),
                                                           other.GetWeakPtr()};
  std::vector<fml::WeakPtr<GestureArenaMember>> bubble = {winner.GetWeakPtr()};

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  trigger.ResolveTouchEvent(&down, compete, bubble);

  EXPECT_EQ(handlers.at(1)->GetGestureStatus(),
            GestureConstants::LYNX_STATE_BEGIN);
}

TEST_F(GestureHandlerTriggerTest,
       ResolveTouchEvent_Move_ReCompetesAndDispatchesStableWinner) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember winner(1);
  GestureMap detectors;
  detectors.emplace(1, MakeDetector(1, GestureHandlerType::Pan,
                                    {GestureConstants::ON_BEGIN}));
  winner.SetGestureDetectorMap(detectors);
  auto handlers = BaseGestureHandler::ConvertToGestureHandler(
      winner.Sign(), page_view.get(), winner.GetWeakPtr(), detectors);
  winner.SetGestureHandlers(handlers);

  trigger.InitCurrentWinnerWhenDown(winner.GetWeakPtr());

  std::vector<fml::WeakPtr<GestureArenaMember>> compete = {winner.GetWeakPtr()};
  std::vector<fml::WeakPtr<GestureArenaMember>> bubble = {winner.GetWeakPtr()};

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  trigger.ResolveTouchEvent(&down, compete, bubble);

  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 2);
  trigger.ResolveTouchEvent(&move, compete, bubble);

  EXPECT_NE(trigger.winner_, fml::WeakPtr<GestureArenaMember>());
}

TEST_F(GestureHandlerTriggerTest,
       ResolveTouchEvent_Up_StartsFlingWhenVelocityExceedsThreshold) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember winner(1);
  winner.SetCanConsume(true);
  GestureMap detectors;
  detectors.emplace(1, MakeDetector(1, GestureHandlerType::Default,
                                    {GestureConstants::ON_BEGIN,
                                     GestureConstants::ON_UPDATE}));
  winner.SetGestureDetectorMap(detectors);
  winner.SetGestureHandlers(BaseGestureHandler::ConvertToGestureHandler(
      winner.Sign(), page_view.get(), winner.GetWeakPtr(), detectors));

  arena_manager->AddMember(winner.GetWeakPtr());
  detector_manager->RegisterGestureDetector(winner.Sign(), detectors.at(1));

  trigger.InitCurrentWinnerWhenDown(winner.GetWeakPtr());

  std::vector<fml::WeakPtr<GestureArenaMember>> compete = {winner.GetWeakPtr()};
  std::vector<fml::WeakPtr<GestureArenaMember>> bubble = {winner.GetWeakPtr()};

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  trigger.ResolveTouchEvent(&down, compete, bubble);

  trigger.SetVelocity(1000, 0);
  PointerEvent up =
      MakePointerEvent(PointerEvent::EventType::kUpEvent, {0, 0}, 2);
  trigger.ResolveTouchEvent(&up, compete, bubble);

  auto& animation_handler = *page_view->GetAnimationHandler();
  for (int64_t t = 0; t < 200; t += 16) {
    animation_handler.DoAnimationFrame(t);
  }

  auto calls = winner.TakeScrollCalls();
  EXPECT_FALSE(calls.empty());
}

TEST_F(GestureHandlerTriggerTest,
       ResolveTouchEvent_Up_NoFling_DispatchesSentinelMinDelta) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember winner(1);
  winner.SetCanConsume(true);
  GestureMap detectors;
  detectors.emplace(
      1, MakeDetector(1, GestureHandlerType::Default,
                      {GestureConstants::ON_BEGIN, GestureConstants::ON_END}));
  winner.SetGestureDetectorMap(detectors);
  winner.SetGestureHandlers(BaseGestureHandler::ConvertToGestureHandler(
      winner.Sign(), page_view.get(), winner.GetWeakPtr(), detectors));

  trigger.InitCurrentWinnerWhenDown(winner.GetWeakPtr());
  winner.GetGestureHandlers().begin()->second->Activate();

  std::vector<fml::WeakPtr<GestureArenaMember>> compete = {winner.GetWeakPtr()};
  std::vector<fml::WeakPtr<GestureArenaMember>> bubble = {winner.GetWeakPtr()};

  PointerEvent up =
      MakePointerEvent(PointerEvent::EventType::kUpEvent, {0, 0}, 2);
  trigger.SetVelocity(0, 0);
  trigger.ResolveTouchEvent(&up, compete, bubble);

  EXPECT_EQ(winner.GetGestureHandlers().begin()->second->GetGestureStatus(),
            GestureConstants::LYNX_STATE_FAIL);
}

TEST_F(GestureHandlerTriggerTest,
       SimultaneousWinners_DispatchesWithSimultaneousFlag) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember m1(1);
  TestGestureArenaMember m2(2);
  m1.SetCanConsume(true);
  m2.SetCanConsume(true);

  GestureMap d1;
  d1.emplace(
      1, MakeDetector(1, GestureHandlerType::Default,
                      {GestureConstants::ON_UPDATE}, {{"simultaneous", {2}}}));
  m1.SetGestureDetectorMap(d1);
  m1.SetGestureHandlers(BaseGestureHandler::ConvertToGestureHandler(
      m1.Sign(), page_view.get(), m1.GetWeakPtr(), d1));

  GestureMap d2;
  d2.emplace(2, MakeDetector(2, GestureHandlerType::Default, {}));
  m2.SetGestureDetectorMap(d2);
  m2.SetGestureHandlers(BaseGestureHandler::ConvertToGestureHandler(
      m2.Sign(), page_view.get(), m2.GetWeakPtr(), d2));

  arena_manager->AddMember(m1.GetWeakPtr());
  arena_manager->AddMember(m2.GetWeakPtr());
  detector_manager->RegisterGestureDetector(m1.Sign(), d1.at(1));
  detector_manager->RegisterGestureDetector(m2.Sign(), d2.at(2));

  trigger.InitCurrentWinnerWhenDown(m1.GetWeakPtr());

  std::vector<fml::WeakPtr<GestureArenaMember>> compete = {m1.GetWeakPtr()};
  std::vector<fml::WeakPtr<GestureArenaMember>> bubble = {m1.GetWeakPtr(),
                                                          m2.GetWeakPtr()};

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 2);

  trigger.ResolveTouchEvent(&down, compete, bubble);
  trigger.ResolveTouchEvent(&move, compete, bubble);

  auto calls = m2.TakeScrollCalls();
  EXPECT_FALSE(calls.empty());
}

TEST_F(GestureHandlerTriggerTest,
       DuplicateMembersInCompeteChain_DoesNotCrashAndResetsCorrectly) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);

  auto arena_manager =
      std::make_shared<GestureArenaManager>(true, page_view.get());
  auto detector_manager =
      std::make_shared<GestureDetectorManager>(arena_manager);
  GestureHandlerTrigger trigger(page_view.get(), detector_manager);

  TestGestureArenaMember m1(1);
  m1.SetCanConsume(true);
  GestureMap d1;
  d1.emplace(1, MakeDetector(1, GestureHandlerType::Pan,
                             {GestureConstants::ON_BEGIN}));
  m1.SetGestureDetectorMap(d1);
  m1.SetGestureHandlers(BaseGestureHandler::ConvertToGestureHandler(
      m1.Sign(), page_view.get(), m1.GetWeakPtr(), d1));

  trigger.InitCurrentWinnerWhenDown(m1.GetWeakPtr());

  std::vector<fml::WeakPtr<GestureArenaMember>> compete = {m1.GetWeakPtr(),
                                                           m1.GetWeakPtr()};
  std::vector<fml::WeakPtr<GestureArenaMember>> bubble = {m1.GetWeakPtr()};

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 2);

  trigger.ResolveTouchEvent(&down, compete, bubble);
  trigger.ResolveTouchEvent(&move, compete, bubble);

  EXPECT_NE(trigger.winner_, fml::WeakPtr<GestureArenaMember>());
}
}  // namespace testing

}  // namespace clay
