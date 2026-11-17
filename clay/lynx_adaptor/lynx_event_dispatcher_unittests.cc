// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <initializer_list>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "clay/gfx/animation/transition_manager.h"
#include "clay/lynx_adaptor/base_def.h"
#include "clay/lynx_adaptor/lynx_event_dispatcher.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/public/value.h"
#include "clay/ui/component/view.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

struct TouchRecord {
  std::string name;
  Value params;
};

class RecordingEngineProxy : public lynx::shell::LynxEngineProxy {
 public:
  bool SendTouchEvent(const std::string& name, const lynx::pub::Value& params,
                      int64_t) override {
    touches.push_back({name, lynx::ValueConverter::CreateClayValue(params)});
    return true;
  }

  bool SendTouchEvent(const std::string& name, int32_t, float, float, float,
                      float, float, float, int64_t,
                      lynx::event::TouchEventTargetPoints) override {
    single_touch_names.push_back(name);
    return true;
  }

  void DispatchTaskToLynxEngine(lynx::base::closure) override {}
  MOCK_METHOD(void, SendCustomEvent,
              (const std::string&, int32_t, const lynx::pub::Value&,
               const std::string&),
              (override));
  MOCK_METHOD(void, SendCustomEventWithOptions,
              (const std::string&, int32_t, const lynx::pub::Value&,
               const std::string&,
               const lynx::shell::CustomEventDispatchOptions&),
              (override));
  MOCK_METHOD(void, SendGestureEvent,
              (int, int, std::string, const lynx::pub::Value&), (override));
  MOCK_METHOD(void, SendBubbleEvent,
              (const std::string&, int32_t, const lynx::pub::Value&),
              (override));
  MOCK_METHOD(void, OnPseudoStatusesChanged,
              (std::vector<lynx::event::PseudoStatusChange>), (override));
  void StartEventGenerate(const lynx::pub::Value&) override {}
  void StartEventCapture(int64_t) override {}
  void StartEventBubble(int64_t) override {}
  void StartEventFire(bool, int64_t) override {}
  void ScrollByListContainer(int32_t, float, float, float, float) override {}
  void ScrollToPosition(int32_t, int, float, int, bool) override {}
  void ScrollStopped(int32_t) override {}
  int32_t ObtainListChild(int32_t, uint32_t, int64_t, bool) override {
    return -1;
  }
  void RecycleListChild(int32_t, uint32_t) override {}
  void RenderListChild(int32_t, uint32_t, int64_t) override {}
  void UpdateListChild(int32_t, uint32_t, uint32_t, int64_t) override {}
  lynx::tasm::ListData GetListData(int) override { return {}; }
  std::list<int32_t> GetAncestorElements(int32_t) override { return {}; }
  void MarkLayoutDirty(int) override {}
  bool EnableRasterAnimation() override { return false; }
  float GetDensity() const override { return density; }
  void OnFirstMeaningfulPaint() override {}
  void TriggerLayout() override {}

  std::vector<TouchRecord> touches;
  std::vector<std::string> single_touch_names;
  float density = 1.f;
};

class LynxEventDispatcherTest : public ::testing::Test {
 protected:
  void SetUp() override {
    engine_ = std::make_shared<RecordingEngineProxy>();
    dispatcher_.SetEngineProxy(engine_);
    dispatcher_.SetEnableMultiTouch(true);
  }

  void Touch(const char* name, int pointer, int tag = 10) {
    dispatcher_.OnTouchEvent(name, tag, 10.f, 20.f, 30.f, 40.f, pointer);
  }

  int Identifier(size_t event, int tag) const {
    return engine_->touches.at(event)
        .params.GetMap()
        .at(std::to_string(tag))
        .GetArray()
        .at(0)
        .GetArray()
        .at(0)
        .GetInt();
  }

  LynxEventDispatcher dispatcher_;
  std::shared_ptr<RecordingEngineProxy> engine_;
};

TEST_F(LynxEventDispatcherTest, AnimationElapsedTimeIsSentInParams) {
  EXPECT_CALL(*engine_,
              SendCustomEvent("animationstart", 10, ::testing::_, "params"))
      .WillOnce(::testing::WithArg<2>([](const lynx::pub::Value& params) {
        const auto elapsed_time = params.GetValueForKey("elapsedTime");
        ASSERT_TRUE(elapsed_time && elapsed_time->IsNumber());
        EXPECT_DOUBLE_EQ(elapsed_time->Number(), 0.25);
      }));
  EXPECT_CALL(*engine_,
              SendCustomEvent("transitionend", 10, ::testing::_, "params"))
      .WillOnce(::testing::WithArg<2>([](const lynx::pub::Value& params) {
        const auto elapsed_time = params.GetValueForKey("elapsedTime");
        ASSERT_TRUE(elapsed_time && elapsed_time->IsNumber());
        EXPECT_DOUBLE_EQ(elapsed_time->Number(), 0.5);
      }));

  dispatcher_.OnAnimationEvent("animationstart", "opacity", 10, 0.25);
  dispatcher_.OnTransitionEvent("transitionend", "", 10,
                                ClayAnimationPropertyType::kOpacity, 0.5);
}

using TransitionCancellationTest = UITest;

TEST_F_UI(TransitionCancellationTest, ReplacedTransitionReportsElapsedTime) {
  constexpr auto property = ClayAnimationPropertyType::kOpacity;
  auto engine = std::make_shared<RecordingEngineProxy>();
  LynxEventDispatcher dispatcher;
  dispatcher.SetEngineProxy(engine);
  auto* previous_delegate = page_->GetEventDelegate();
  page_->SetEventDelegate(&dispatcher);
  page_->SetRasterAnimationEnabled(false);
  View view(10, page_.get());
  page_->AddChild(&view);
  fml::ScopedCleanupClosure cleanup([&] {
    page_->RemoveChild(&view);
    view.Destroy();
    page_->SetEventDelegate(previous_delegate);
  });
  view.SetBound(0, 0, 100, 100);
  view.AddEventCallback("transitioncancel");
  TransitionData transition;
  transition.duration = 100;
  transition.property = property;
  view.SetTransition({transition});
  view.OnNodeReady();
  view.SetOpacity(0.25f);

  page_->GetAnimationHandler()->DoAnimationFrame(1000);
  const auto animators = view.TransitionMgr()->GetRunningAnimators();
  ASSERT_EQ(animators.size(), 1u);
  animators.front()->PauseForTest(1050);
  EXPECT_CALL(*engine,
              SendCustomEvent("transitioncancel", 10, ::testing::_, "params"))
      .WillOnce(::testing::WithArg<2>([](const lynx::pub::Value& params) {
        const auto type = params.GetValueForKey("animation_type");
        ASSERT_TRUE(type && type->IsString());
        EXPECT_EQ(type->str(), "transition-opacity");
        const auto elapsed_time = params.GetValueForKey("elapsedTime");
        ASSERT_TRUE(elapsed_time && elapsed_time->IsNumber());
        EXPECT_DOUBLE_EQ(elapsed_time->Number(), 0.05);
      }));

  view.SetOpacity(0.75f);
}

class TouchCancellationEventDispatcher : public LynxEventDispatcher {
 public:
  // Raw-touch cancellation does not depend on synthesized desktop events.
  // Avoid querying the host's keyboard state from this test's UI thread.
  void OnMouseEvent(const std::string&, int, int, int, float, float, float,
                    float, float) override {}
#if defined(OS_WIN) || defined(OS_MAC) || defined(OS_LINUX)
  void OnPointerEvent(const std::string&, int, int, ClayPointerDeviceKind, bool,
                      int, int, float, float, float, float, float, float, float,
                      int64_t, int) override {}
#endif
};

class TouchCancellationTest : public UITest {
 protected:
  void UISetUp() override {
    engine_ = std::make_shared<RecordingEngineProxy>();
    dispatcher_.SetEngineProxy(engine_);
    dispatcher_.SetEnableMultiTouch(true);
    page_->SetEventDelegate(&dispatcher_);
    page_->SetEnableMultiTouch(true);
    page_->SetBound(0, 0, 400, 200);
    for (int id : {1, 2}) {
      auto* parent = new View(id, page_.get());
      page_->AddChild(parent);
      parent->SetBound((id - 1) * 200, 0, 200, 200);
    }
    find_view_by_id_callback_ = [this](int id) -> BaseView* {
      std::vector<BaseView*> pending{page_.get()};
      while (!pending.empty()) {
        auto* view = pending.back();
        pending.pop_back();
        if (view->id() == id) {
          return view;
        }
        pending.insert(pending.end(), view->GetChildren().begin(),
                       view->GetChildren().end());
      }
      return nullptr;
    };
  }

  void CheckCancellation(bool reverse, bool shared_parent, bool batch) {
    auto* parent = page_->GetChildren().front();
    auto* other_parent = shared_parent ? parent : page_->GetChildren().back();
    auto* first_target = new View(3, page_.get());
    parent->AddChild(first_target);
    first_target->SetBound(10, 10, 80, 80);
    auto* second_target = new View(4, page_.get());
    other_parent->AddChild(second_target);
    second_target->SetBound(shared_parent ? 110 : 10, 10, 80, 80);
    auto first =
        CreatePointer(17, PointerEvent::EventType::kDownEvent, {50, 50});
    auto second = CreatePointer(18, PointerEvent::EventType::kDownEvent,
                                {shared_parent ? 150.f : 250.f, 50});
    page_->DispatchPointerEvent({first, second});
    ASSERT_EQ(engine_->touches.size(), 2u);
    ASSERT_EQ(engine_->touches.at(0).params.GetMap().count("3"), 1u);
    ASSERT_EQ(engine_->touches.at(1).params.GetMap().count("4"), 1u);

    parent->RemoveChild(first_target);
    other_parent->RemoveChild(second_target);
    delete first_target;
    delete second_target;
    if (reverse) {
      std::swap(first, second);
    }
    first.type = PointerEvent::EventType::kCancel;
    second.type = PointerEvent::EventType::kCancel;
    if (batch) {
      page_->DispatchPointerEvent({first, second});
    } else {
      // The first cancel must already include both surviving recipients,
      // even when the other physical contact has not reported a cancel yet.
      page_->DispatchPointerEvent({first});
    }
    ASSERT_EQ(engine_->touches.size(), 3u);
    EXPECT_EQ(engine_->touches.back().name, "touchcancel");
    const auto& targets = engine_->touches.back().params.GetMap();
    EXPECT_EQ(targets.size(), shared_parent ? 1u : 2u);
    EXPECT_EQ(targets.count("3"), 0u);
    EXPECT_EQ(targets.count("4"), 0u);
    ASSERT_EQ(targets.count("1"), 1u);
    EXPECT_TRUE(targets.at("1").GetArray().empty());
    if (!shared_parent) {
      ASSERT_EQ(targets.count("2"), 1u);
      EXPECT_TRUE(targets.at("2").GetArray().empty());
    }
    if (!batch) {
      second.type = PointerEvent::EventType::kMoveEvent;
      page_->DispatchPointerEvent({second});
      EXPECT_EQ(engine_->touches.size(), 3u);
      second.type = PointerEvent::EventType::kCancel;
      page_->DispatchPointerEvent({second});
    }
    EXPECT_EQ(engine_->touches.size(), 3u);

    auto next =
        CreatePointer(19, PointerEvent::EventType::kDownEvent, {50, 50});
    page_->DispatchPointerEvent({next});
    next.type = PointerEvent::EventType::kUpEvent;
    page_->DispatchPointerEvent({next});
    ASSERT_EQ(engine_->touches.size(), 5u);
    EXPECT_EQ(engine_->touches.at(3).name, "touchstart");
    EXPECT_EQ(engine_->touches.at(4).name, "touchend");
    EXPECT_EQ(engine_->touches.at(3)
                  .params.GetMap()
                  .at("1")
                  .GetArray()
                  .front()
                  .GetArray()
                  .front()
                  .GetInt(),
              0);
  }

  TouchCancellationEventDispatcher dispatcher_;
  std::shared_ptr<RecordingEngineProxy> engine_;
};

TEST_F_UI(TouchCancellationTest, ResolvesAllRemovedTargetsBeforeFirstCancel) {
  CheckCancellation(false, false, false);
}

TEST_F_UI(TouchCancellationTest, ResolvesRemovedTargetsInReverseCancelOrder) {
  CheckCancellation(true, false, false);
}

TEST_F_UI(TouchCancellationTest, ResolvesRemovedTargetsInOneCancelBatch) {
  CheckCancellation(false, false, true);
}

TEST_F_UI(TouchCancellationTest, DeduplicatesSharedSurvivingAncestor) {
  CheckCancellation(false, true, false);
}

TEST_F(LynxEventDispatcherTest, RecyclesOnlyReleasedIdentifiers) {
  Touch("touchstart", 17);
  Touch("touchstart", 18, 20);
  Touch("touchend", 17);
  Touch("touchmove", 18, 20);
  Touch("touchstart", 19, 30);
  Touch("touchend", 18, 20);
  Touch("touchend", 19, 30);
  Touch("touchstart", 20);

  ASSERT_EQ(engine_->touches.size(), 8u);
  EXPECT_EQ(Identifier(0, 10), 0);
  EXPECT_EQ(Identifier(1, 20), 1);
  EXPECT_EQ(Identifier(2, 10), 0);
  EXPECT_EQ(Identifier(3, 20), 1);
  EXPECT_EQ(Identifier(4, 30), 0);
  EXPECT_EQ(Identifier(5, 20), 1);
  EXPECT_EQ(Identifier(6, 30), 0);
  EXPECT_EQ(Identifier(7, 10), 0);
  const auto& point = engine_->touches.front()
                          .params.GetMap()
                          .at("10")
                          .GetArray()
                          .front()
                          .GetArray();
  EXPECT_DOUBLE_EQ(point.at(3).GetDouble(), 30.);
  EXPECT_DOUBLE_EQ(point.at(4).GetDouble(), 40.);
}

TEST_F(LynxEventDispatcherTest, CancelledStreamDrainsAllPhysicalContacts) {
  Touch("touchstart", 1);
  Touch("touchstart", 2, 20);
  Touch("touchcancel", 1);
  Touch("touchmove", 2, 20);
  Touch("touchstart", 3, 30);
  Touch("touchend", 2, 20);
  Touch("touchmove", 3, 30);
  Touch("touchend", 3, 30);

  ASSERT_EQ(engine_->touches.size(), 3u);
  EXPECT_EQ(engine_->touches.back().name, "touchcancel");
  const auto& targets = engine_->touches.back().params.GetMap();
  ASSERT_EQ(targets.size(), 2u);
  EXPECT_TRUE(targets.at("10").GetArray().empty());
  EXPECT_TRUE(targets.at("20").GetArray().empty());

  Touch("touchstart", 4);
  Touch("touchend", 4);
  ASSERT_EQ(engine_->touches.size(), 5u);
  EXPECT_EQ(engine_->touches.at(3).name, "touchstart");
  EXPECT_EQ(Identifier(3, 10), 0);
  EXPECT_EQ(engine_->touches.at(4).name, "touchend");
  EXPECT_EQ(Identifier(4, 10), 0);
}

TEST_F(LynxEventDispatcherTest,
       CancellationDeduplicatesContactsOnTheSameTarget) {
  Touch("touchstart", 1);
  Touch("touchstart", 2);
  Touch("touchcancel", 1);

  ASSERT_EQ(engine_->touches.size(), 3u);
  const auto& targets = engine_->touches.back().params.GetMap();
  ASSERT_EQ(targets.size(), 1u);
  EXPECT_TRUE(targets.at("10").GetArray().empty());
}

TEST_F(LynxEventDispatcherTest, RepeatedCancellationDoesNotEndTheNextStream) {
  Touch("touchstart", 1);
  Touch("touchstart", 2, 20);
  Touch("touchcancel", 1);
  Touch("touchcancel", 2, 20);
  Touch("touchstart", 3);
  Touch("touchcancel", 1);
  Touch("touchend", 2, 20);
  Touch("touchmove", 3);
  Touch("touchend", 3);

  ASSERT_EQ(engine_->touches.size(), 6u);
  EXPECT_EQ(engine_->touches.at(2).name, "touchcancel");
  EXPECT_EQ(engine_->touches.at(3).name, "touchstart");
  EXPECT_EQ(engine_->touches.at(4).name, "touchmove");
  EXPECT_EQ(engine_->touches.at(5).name, "touchend");
  EXPECT_EQ(Identifier(3, 10), 0);
  EXPECT_EQ(Identifier(4, 10), 0);
  EXPECT_EQ(Identifier(5, 10), 0);
}

TEST_F(LynxEventDispatcherTest, SingleContactCancelRequiresNoFollowingUp) {
  Touch("touchstart", 1);
  Touch("touchcancel", 1);
  Touch("touchstart", 2);
  Touch("touchend", 2);

  ASSERT_EQ(engine_->touches.size(), 4u);
  EXPECT_EQ(engine_->touches.at(1).name, "touchcancel");
  EXPECT_EQ(engine_->touches.at(2).name, "touchstart");
  EXPECT_EQ(Identifier(2, 10), 0);
}

TEST_F(LynxEventDispatcherTest, SyntheticGesturesDoNotRecycleTouchIdentifiers) {
  Touch("touchstart", 1);
  Touch("tap", 1);
  Touch("touchstart", 2, 20);
  Touch("longpress", 2, 20);
  Touch("touchend", 1);
  Touch("touchend", 2, 20);

  ASSERT_EQ(engine_->touches.size(), 4u);
  EXPECT_EQ(Identifier(0, 10), 0);
  EXPECT_EQ(Identifier(1, 20), 1);
  EXPECT_EQ(Identifier(2, 10), 0);
  EXPECT_EQ(Identifier(3, 20), 1);
  ASSERT_EQ(engine_->single_touch_names.size(), 2u);
  EXPECT_EQ(engine_->single_touch_names.at(0), "tap");
  EXPECT_EQ(engine_->single_touch_names.at(1), "longpress");
}

TEST_F(LynxEventDispatcherTest, PageConfigurationResetsCancelledStream) {
  Touch("touchstart", 1);
  Touch("touchstart", 2, 20);
  Touch("touchcancel", 1);
  dispatcher_.SetEnableMultiTouch(true);
  Touch("touchstart", 3);

  ASSERT_EQ(engine_->touches.size(), 4u);
  EXPECT_EQ(engine_->touches.back().name, "touchstart");
  EXPECT_EQ(Identifier(3, 10), 0);
}

TEST_F(LynxEventDispatcherTest, SingleTouchModeUsesTheOriginalInterface) {
  dispatcher_.SetEnableMultiTouch(false);
  Touch("touchstart", 1);
  Touch("touchmove", 1);
  Touch("touchend", 1);

  EXPECT_TRUE(engine_->touches.empty());
  ASSERT_EQ(engine_->single_touch_names.size(), 3u);
  EXPECT_EQ(engine_->single_touch_names.at(0), "touchstart");
  EXPECT_EQ(engine_->single_touch_names.at(1), "touchmove");
  EXPECT_EQ(engine_->single_touch_names.at(2), "touchend");
}

TEST_F(LynxEventDispatcherTest, CancelUsesSurvivingTouchTarget) {
  Touch("touchstart", 17, 10);
  Touch("touchmove", 17, 20);
  Touch("touchcancel", 17, 20);
  ASSERT_EQ(engine_->touches.size(), 3u);
  EXPECT_EQ(Identifier(0, 10), 0);
  EXPECT_EQ(Identifier(1, 20), 0);
  const auto& targets = engine_->touches.at(2).params.GetMap();
  EXPECT_EQ(targets.count("10"), 0u);
  ASSERT_EQ(targets.count("20"), 1u);
  EXPECT_TRUE(targets.at("20").GetArray().empty());
  Touch("touchstart", 17, 30);
  EXPECT_EQ(Identifier(3, 30), 0);
}

TEST_F(LynxEventDispatcherTest, KeyEventPreservesKeyAndRepeat) {
  const struct {
    const char* name;
    bool repeat;
  } events[] = {{"keydown", false}, {"keydown", true}, {"keyup", false}};

  for (const auto& event : events) {
    SCOPED_TRACE(::testing::Message()
                 << event.name << " repeat=" << event.repeat);
    EXPECT_CALL(*engine_, SendBubbleEvent(event.name, 42, ::testing::_))
        .WillOnce(
            ::testing::WithArg<2>([event](const lynx::pub::Value& params) {
              const auto type = params.GetValueForKey("type");
              ASSERT_TRUE(type && type->IsString());
              EXPECT_EQ(type->str(), event.name);
              const auto key = params.GetValueForKey("key");
              ASSERT_TRUE(key && key->IsString());
              EXPECT_EQ(key->str(), "ArrowLeft");
              const auto repeated = params.GetValueForKey("repeat");
              ASSERT_TRUE(repeated && repeated->IsBool());
              EXPECT_EQ(repeated->Bool(), event.repeat);
            }));

    dispatcher_.OnKeyEvent(event.name, 42, "ArrowLeft", event.repeat);
  }
}

template <typename Dispatcher, typename = void>
constexpr bool kHasPointerDispatch = false;

template <typename Dispatcher>
constexpr bool kHasPointerDispatch<
    Dispatcher, std::void_t<decltype(&Dispatcher::OnPointerEvent)>> = true;

template <typename Dispatcher>
void ExpectPointerEventPayloads(Dispatcher& dispatcher,
                                RecordingEngineProxy& engine) {
  constexpr int64_t kTimestamp = 5'000'000'001;
  const struct {
    ClayPointerDeviceKind kind;
    const char* type;
    bool is_primary;
  } devices[] = {{kClayPointerDeviceKindMouse, "mouse", true},
                 {kClayPointerDeviceKindTrackpad, "mouse", true},
                 {kClayPointerDeviceKindStylus, "pen", true},
                 {kClayPointerDeviceKindTouch, "touch", false}};
  engine.density = 2.f;

  for (const auto& device : devices) {
    SCOPED_TRACE(device.kind);
    EXPECT_CALL(engine, SendBubbleEvent("pointerout", 42, ::testing::_))
        .WillOnce(
            ::testing::WithArg<2>([device](const lynx::pub::Value& params) {
              const auto type = params.GetValueForKey("type");
              ASSERT_TRUE(type && type->IsString());
              EXPECT_EQ(type->str(), "pointerout");
              const auto pointer_type = params.GetValueForKey("pointerType");
              ASSERT_TRUE(pointer_type && pointer_type->IsString());
              EXPECT_EQ(pointer_type->str(), device.type);
              const auto primary = params.GetValueForKey("isPrimary");
              ASSERT_TRUE(primary && primary->IsBool());
              EXPECT_EQ(primary->Bool(), device.is_primary);

              const std::initializer_list<std::pair<const char*, double>>
                  expected = {{"pointerId", 17},
                              {"button", -1},
                              {"buttons", 2},
                              {"width", 3},
                              {"height", 4},
                              {"pressure", 0.75},
                              {"timestamp", static_cast<double>(kTimestamp)},
                              {"__lynxRelatedTargetSign", 99},
                              {"x", 5},
                              {"y", 10},
                              {"pageX", 15},
                              {"pageY", 20},
                              {"clientX", 15},
                              {"clientY", 20}};
              for (const auto& [key, number] : expected) {
                const auto value = params.GetValueForKey(key);
                ASSERT_TRUE(value && value->IsNumber()) << key;
                EXPECT_DOUBLE_EQ(value->Number(), number) << key;
              }
            }));

    dispatcher.OnPointerEvent("pointerout", 42, 17, device.kind,
                              device.is_primary, -1, 2, 3.f, 4.f, 0.75f, 10.f,
                              20.f, 30.f, 40.f, kTimestamp, 99);
  }
}

TEST_F(LynxEventDispatcherTest,
       PointerEventPreservesInputAndReportsLogicalCoordinates) {
  if constexpr (kHasPointerDispatch<LynxEventDispatcher>) {
    ExpectPointerEventPayloads(dispatcher_, *engine_);
  } else {
    GTEST_SKIP() << "This platform has no dispatcher pointer event API";
  }
}

TEST_F(LynxEventDispatcherTest,
       WheelPreservesDeltasAndReportsLogicalCoordinates) {
  engine_->density = 2.f;
  EXPECT_CALL(*engine_, SendBubbleEvent("wheel", 42, ::testing::_))
      .WillOnce(::testing::WithArg<2>([](const lynx::pub::Value& params) {
        const std::initializer_list<std::pair<const char*, double>> expected = {
            {"x", 5},        {"y", 10},       {"pageX", 15},  {"pageY", 20},
            {"clientX", 15}, {"clientY", 20}, {"deltaX", -3}, {"deltaY", 7}};
        for (const auto& [key, number] : expected) {
          const auto value = params.GetValueForKey(key);
          ASSERT_TRUE(value && value->IsNumber()) << key;
          EXPECT_DOUBLE_EQ(value->Number(), number) << key;
        }
      }));

  dispatcher_.OnWheelEvent("wheel", 42, 10.f, 20.f, 30.f, 40.f, -3.f, 7.f);
}

TEST_F(LynxEventDispatcherTest, LayoutChangeForwardsGeometryInDetail) {
  EXPECT_CALL(*engine_,
              SendCustomEvent("layoutchange", 42, ::testing::_, "detail"))
      .WillOnce(::testing::WithArg<2>([](const lynx::pub::Value& params) {
        const auto id = params.GetValueForKey("id");
        ASSERT_TRUE(id && id->IsString());
        EXPECT_EQ(id->str(), "layout-target");
        const auto width = params.GetValueForKey("width");
        ASSERT_TRUE(width && width->IsNumber());
        EXPECT_DOUBLE_EQ(width->Number(), 40);
      }));

  Value::Map detail;
  detail.emplace("id", Value("layout-target"));
  detail.emplace("width", Value(40.f));
  dispatcher_.OnLayoutChanged(42, std::move(detail));
}

TEST_F(LynxEventDispatcherTest, AnimationCancelForwardsKeyframeIdentity) {
  EXPECT_CALL(*engine_,
              SendCustomEvent("animationcancel", 42, ::testing::_, "params"))
      .WillOnce(::testing::WithArg<2>([](const lynx::pub::Value& params) {
        const auto type = params.GetValueForKey("animation_type");
        ASSERT_TRUE(type && type->IsString());
        EXPECT_EQ(type->str(), "keyframe-animation");
        const auto name = params.GetValueForKey("animation_name");
        ASSERT_TRUE(name && name->IsString());
        EXPECT_EQ(name->str(), "fade-out");
      }));

  dispatcher_.OnAnimationEvent("animationcancel", "fade-out", 42, 0.0);
}

void ExpectStringField(const lynx::pub::Value& params, const char* name,
                       const char* expected) {
  const auto value = params.GetValueForKey(name);
  ASSERT_TRUE(value && value->IsString()) << name;
  EXPECT_EQ(value->str(), expected) << name;
}

void ExpectNumberFields(
    const lynx::pub::Value& params,
    std::initializer_list<std::pair<const char*, double>> expected) {
  for (const auto& [name, number] : expected) {
    const auto value = params.GetValueForKey(name);
    ASSERT_TRUE(value && value->IsNumber()) << name;
    EXPECT_DOUBLE_EQ(value->Number(), number) << name;
  }
}

auto PseudoChange(int view_id, uint32_t previous, uint32_t current) {
  using Change = lynx::event::PseudoStatusChange;
  return ::testing::ElementsAre(
      ::testing::AllOf(::testing::Field(&Change::id, view_id),
                       ::testing::Field(&Change::pre_status, previous),
                       ::testing::Field(&Change::current_status, current)));
}

TEST_F(LynxEventDispatcherTest, MouseEventPreservesButtonsAndLogicalPosition) {
  engine_->density = 2.f;
  EXPECT_CALL(*engine_, SendBubbleEvent("mousedown", 42, ::testing::_))
      .WillOnce(::testing::WithArg<2>([](const lynx::pub::Value& params) {
        ExpectStringField(params, "type", "mousedown");
        ExpectNumberFields(params, {{"button", 2},
                                    {"buttons", 3},
                                    {"scale", 1.5},
                                    {"x", 5},
                                    {"y", 10},
                                    {"pageX", 15},
                                    {"pageY", 20},
                                    {"clientX", 15},
                                    {"clientY", 20}});
        const auto identifier = params.GetValueForKey("identifier");
        ASSERT_TRUE(identifier && identifier->IsInt64());
        EXPECT_NE(identifier->Int64(), 0);
      }));

  dispatcher_.OnMouseEvent("mousedown", 42, 2, 3, 1.5f, 10.f, 20.f, 30.f, 40.f);
}

TEST_F(LynxEventDispatcherTest, GestureExtrasPreserveAuthoritativeCoordinates) {
  engine_->density = 2.f;
  ::testing::InSequence sequence;
  EXPECT_CALL(*engine_, SendGestureEvent(42, 17, "onUpdate", ::testing::_))
      .WillOnce(::testing::WithArg<3>([](const lynx::pub::Value& params) {
        ExpectStringField(params, "type", "onUpdate");
        ExpectNumberFields(params, {{"x", 5},
                                    {"y", 10},
                                    {"pageX", 15},
                                    {"pageY", 20},
                                    {"clientX", 15},
                                    {"clientY", 20},
                                    {"timestamp", 1234},
                                    {"scrollX", 2.5}});
      }));
  EXPECT_CALL(*engine_, SendGestureEvent(42, 17, "onEnd", ::testing::_))
      .WillOnce(::testing::WithArg<3>([](const lynx::pub::Value& params) {
        ExpectStringField(params, "type", "onEnd");
        ExpectNumberFields(params, {{"x", 6}, {"timestamp", 1250}});
        EXPECT_FALSE(params.Contains("scrollX"));
      }));

  Value::Map extra;
  // Gesture-specific data must not shadow the normalized event coordinates.
  extra.emplace("x", Value(999));
  extra.emplace("scrollX", Value(2.5f));
  Value update(std::move(extra));
  dispatcher_.OnGestureHandlerEvent("onUpdate", 42, 17, 10.f, 20.f, 30.f, 40.f,
                                    1234, update);
  Value end;
  dispatcher_.OnGestureHandlerEvent("onEnd", 42, 17, 12.f, 20.f, 30.f, 40.f,
                                    1250, end);
}

TEST_F(LynxEventDispatcherTest, ActiveFocusAndHoverReportBothStateDirections) {
  ::testing::InSequence sequence;
  const auto none = lynx::kPseudoStateNone;
  for (const auto& [change, state] :
       {std::make_pair(&LynxEventDispatcher::OnActiveChanged,
                       lynx::kPseudoStateActive),
        std::make_pair(&LynxEventDispatcher::OnFocusChanged,
                       lynx::kPseudoStateFocus)}) {
    EXPECT_CALL(*engine_,
                OnPseudoStatusesChanged(PseudoChange(42, none, state)));
    EXPECT_CALL(*engine_,
                OnPseudoStatusesChanged(PseudoChange(42, state, none)));
    (dispatcher_.*change)(42, true);
    (dispatcher_.*change)(42, false);
  }

  for (bool hover : {true, false}) {
    EXPECT_CALL(*engine_,
                SendCustomEvent("hoverchange", 42, ::testing::_, "detail"))
        .WillOnce(
            ::testing::WithArg<2>([hover](const lynx::pub::Value& params) {
              const auto value = params.GetValueForKey("hover");
              ASSERT_TRUE(value && value->IsBool());
              EXPECT_EQ(value->Bool(), hover);
            }));
    const auto hovered = lynx::kPseudoStateHover;
    EXPECT_CALL(*engine_,
                OnPseudoStatusesChanged(PseudoChange(42, hover ? none : hovered,
                                                     hover ? hovered : none)));
    dispatcher_.OnHoverChanged(42, hover);
  }
}

TEST_F(LynxEventDispatcherTest, DropEventPreservesFileMetadata) {
  EXPECT_CALL(*engine_, SendBubbleEvent("drop", 42, ::testing::_))
      .WillOnce(::testing::WithArg<2>([](const lynx::pub::Value& params) {
        ExpectStringField(params, "type", "drop");
        ExpectStringField(params, "dropEffect", "link");
        const auto files = params.GetValueForKey("files");
        ASSERT_TRUE(files && files->IsArray());
        ASSERT_EQ(files->Length(), 1);
        const auto file = files->GetValueAtIndex(0);
        ASSERT_TRUE(file && file->IsMap());
        ExpectStringField(*file, "name", "example.txt");
        ExpectNumberFields(*file, {{"size", 4096}});
      }));

  Value::Map file;
  file.emplace("name", Value("example.txt"));
  file.emplace("size", Value(int64_t{4096}));
  Value::Array files;
  files.emplace_back(std::move(file));
  Value::Map detail;
  detail.emplace("dropEffect", Value("link"));
  detail.emplace("files", Value(std::move(files)));
  dispatcher_.OnDragDropEvent("drop", 42, std::move(detail));
}

TEST_F(LynxEventDispatcherTest, CustomEventPreservesDetailAndUrgency) {
  using Options = lynx::shell::CustomEventDispatchOptions;
  ::testing::InSequence sequence;
  const auto expect_detail = [](const lynx::pub::Value& detail) {
    ExpectStringField(detail, "id", "target");
  };
  EXPECT_CALL(*engine_, SendCustomEventWithOptions(
                            "ready", 42, ::testing::_, "detail",
                            ::testing::Field(&Options::emergency, false)))
      .WillOnce(::testing::WithArg<2>(expect_detail));
  EXPECT_CALL(*engine_, SendCustomEventWithOptions(
                            "ready", 42, ::testing::_, "detail",
                            ::testing::Field(&Options::emergency, true)))
      .WillOnce(::testing::WithArg<2>(expect_detail));

  Value::Map detail;
  detail.emplace("id", Value("target"));
  dispatcher_.OnSendCustomEvent(42, "ready", std::move(detail));
  Value::Map urgent_detail;
  urgent_detail.emplace("id", Value("target"));
  EventDispatchOptions options;
  options.emergency = true;
  dispatcher_.OnSendCustomEventWithOptions(42, "ready",
                                           std::move(urgent_detail), options);
}

TEST_F(LynxEventDispatcherTest, OverlayEventPreservesCurrentIdAndStackOrder) {
  EXPECT_CALL(*engine_,
              SendCustomEvent("onRequestClose", 42, ::testing::_, "detail"))
      .WillOnce(::testing::WithArg<2>([](const lynx::pub::Value& params) {
        ASSERT_TRUE(params.IsArray());
        ASSERT_EQ(params.Length(), 1);
        const auto detail = params.GetValueAtIndex(0);
        ASSERT_TRUE(detail && detail->IsMap());
        ExpectStringField(*detail, "currentOverlayId", "top");
        const auto overlays = detail->GetValueForKey("overlays");
        ASSERT_TRUE(overlays && overlays->IsArray());
        ASSERT_EQ(overlays->Length(), 2);
        const auto bottom = overlays->GetValueAtIndex(0);
        const auto top = overlays->GetValueAtIndex(1);
        ASSERT_TRUE(bottom && bottom->IsString());
        ASSERT_TRUE(top && top->IsString());
        EXPECT_EQ(bottom->str(), "bottom");
        EXPECT_EQ(top->str(), "top");
      }));

  const char* overlays[] = {"bottom", "top"};
  dispatcher_.OnOverlayEvent(42, "top", 2, overlays, "onRequestClose");
}

}  // namespace
}  // namespace clay
