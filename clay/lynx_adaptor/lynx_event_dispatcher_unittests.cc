// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clay/lynx_adaptor/lynx_event_dispatcher.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/public/value.h"
#include "clay/ui/component/view.h"
#include "clay/ui/testing/ui_test.h"
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
  void SendCustomEvent(const std::string&, int32_t, const lynx::pub::Value&,
                       const std::string&) override {}
  void SendGestureEvent(int, int, std::string,
                        const lynx::pub::Value&) override {}
  void SendBubbleEvent(const std::string&, int32_t,
                       const lynx::pub::Value&) override {}
  void OnPseudoStatusesChanged(
      std::vector<lynx::event::PseudoStatusChange>) override {}
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
  float GetDensity() const override { return 1.f; }
  void OnFirstMeaningfulPaint() override {}
  void TriggerLayout() override {}

  std::vector<TouchRecord> touches;
  std::vector<std::string> single_touch_names;
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

}  // namespace
}  // namespace clay
