// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clay/public/event_delegate.h"
#include "clay/public/value.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/intersection_observer_manager.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

std::vector<std::string> Events(std::initializer_list<std::string> names) {
  return std::vector<std::string>(names);
}

class ExposureTestTaskRunner final : public fml::TaskRunner {
 public:
  static fml::RefPtr<ExposureTestTaskRunner> Create() {
    return fml::AdoptRef(new ExposureTestTaskRunner());
  }

  void PostTask(lynx::base::closure task) override {
    immediate_tasks_.push_back(std::move(task));
  }

  void PostDelayedTask(lynx::base::closure task,
                       fml::TimeDelta delay) override {
    delayed_tasks_.push_back({std::move(task), delay});
  }

  bool RunsTasksOnCurrentThread() override { return true; }

  fml::TaskQueueId GetTaskQueueId() override { return fml::TaskQueueId(0); }

  size_t DelayedTaskCount() const { return delayed_tasks_.size(); }

  void AdvanceBy(fml::TimeDelta delta) {
    std::vector<lynx::base::closure> ready_tasks;
    for (auto it = delayed_tasks_.begin(); it != delayed_tasks_.end();) {
      it->delay = it->delay - delta;
      if (it->delay <= fml::TimeDelta::Zero()) {
        ready_tasks.push_back(std::move(it->task));
        it = delayed_tasks_.erase(it);
      } else {
        ++it;
      }
    }
    for (auto& task : ready_tasks) {
      task();
    }
  }

 private:
  struct DelayedTask {
    lynx::base::closure task;
    fml::TimeDelta delay;
  };

  ExposureTestTaskRunner() : TaskRunner(fml::RefPtr<fml::MessageLoopImpl>()) {}

  std::vector<lynx::base::closure> immediate_tasks_;
  std::vector<DelayedTask> delayed_tasks_;
};

class RecordingEventDelegate final : public EventDelegate {
 public:
  using CustomEvent = std::pair<int, std::string>;

  const std::vector<std::string>& custom_events() const {
    return custom_events_;
  }
  const std::vector<CustomEvent>& custom_events_by_target() const {
    return custom_events_by_target_;
  }
  const std::vector<EventDispatchOptions>& custom_event_options() const {
    return custom_event_options_;
  }
  const std::vector<std::string>& global_events() const {
    return global_events_;
  }
  void OnSendCustomEvent(int callback_id, const std::string& event_name,
                         Value::Map) override {
    custom_events_.push_back(event_name);
    custom_events_by_target_.emplace_back(callback_id, event_name);
  }
  void OnSendCustomEventWithOptions(
      int callback_id, const std::string& event_name, Value::Map,
      const EventDispatchOptions& options) override {
    custom_events_.push_back(event_name);
    custom_events_by_target_.emplace_back(callback_id, event_name);
    custom_event_options_.push_back(options);
  }
  void OnSendGlobalEvent(const std::string& event_name, Value) override {
    global_events_.push_back(event_name);
  }

  void OnTouchEvent(const std::string&, int, float, float, float,
                    float) override {}
  void OnMouseEvent(const std::string&, int, int, int, float, float, float,
                    float, float) override {}
  void OnWheelEvent(const std::string&, int, float, float, float, float, float,
                    float) override {}
  void OnKeyEvent(const std::string&, int, const char*, bool) override {}
  void OnGestureHandlerEvent(const std::string&, int, uint32_t, float, float,
                             float, float, int64_t, Value&) override {}
  void OnAnimationEvent(const std::string&, const char*, int) override {}
  void OnTransitionEvent(const std::string&, const char*, int,
                         ClayAnimationPropertyType) override {}
  void OnFocusChanged(int, bool) override {}
  void OnHoverChanged(int, bool) override {}
  void OnDragDropEvent(const std::string&, int, Value::Map) override {}
  void OnViewportMetricsChanged(double, double, double, double, double, double,
                                double, bool) override {}
  void OnDrawEndEvent() override {}
  void OnFirstMeaningfulPaint() override {}
  void OnOverlayEvent(int, const char*, int, const char**,
                      const char*) override {}
  void OnLayoutChanged(int, Value::Map) override {}
  void OnIntersectionEvent(int, Value::Map) override {}
  void OnCallJSApiCallback(int, Value) override {}
  void CallJSIntersectionObserver(int, int, Value) override {}

 private:
  std::vector<std::string> custom_events_;
  std::vector<CustomEvent> custom_events_by_target_;
  std::vector<EventDispatchOptions> custom_event_options_;
  std::vector<std::string> global_events_;
};

class ExposeObserverTest : public ::testing::Test {
 protected:
  void SetUp() override {
    task_runner_ = ExposureTestTaskRunner::Create();
    page_ = std::make_unique<PageView>(0, nullptr, task_runner_);
    page_->SetEventDelegate(&event_delegate_);
    page_->SetBound(0, 0, 1000, 1000);
  }

  void TearDown() override { page_->DestroyAllChildren(); }

  View* AddObservedView(BaseView* parent, int id, float left, float top,
                        float width, float height) {
    auto* target = new View(id, page_.get());
    parent->AddChild(target);
    target->SetBound(left, top, width, height);
    target->SetAttribute("exposure-id",
                         Value("visible-target-" + std::to_string(id)));
    target->AddEventCallback("uiappear");
    target->AddEventCallback("uidisappear");
    return target;
  }

  View* AddVisibleObservedView(int id) {
    return AddObservedView(page_.get(), id, 0, 0, 100, 100);
  }

  // A stop/resume pair marks the next notification as a new frame without
  // emitting a synthetic disappear event.
  void NotifyObserversOnNextFrame() {
    manager()->StopExposure(false);
    manager()->ResumeExposure();
    manager()->NotifyObservers();
  }

  IntersectionObserverManager* manager() {
    return page_->intersection_observer_manager();
  }

  void NotifyTargetReady(BaseView* target) {
    manager()->ReconcileExposureForTarget(target);
    page_->SendGlobalExposureEvent();
  }

  const std::vector<std::string>& custom_events() const {
    return event_delegate_.custom_events();
  }

  const std::vector<std::string>& global_events() const {
    return event_delegate_.global_events();
  }

  size_t CustomEventCount(const char* event_name) const {
    return static_cast<size_t>(
        std::count(custom_events().begin(), custom_events().end(), event_name));
  }

  size_t CustomEventCount(int callback_id, const char* event_name) const {
    return static_cast<size_t>(std::count_if(
        event_delegate_.custom_events_by_target().begin(),
        event_delegate_.custom_events_by_target().end(),
        [callback_id, event_name](const auto& event) {
          return event.first == callback_id && event.second == event_name;
        }));
  }

  const std::vector<EventDispatchOptions>& custom_event_options() const {
    return event_delegate_.custom_event_options();
  }

  RecordingEventDelegate event_delegate_;
  fml::RefPtr<ExposureTestTaskRunner> task_runner_;
  std::unique_ptr<PageView> page_;
};

TEST(ExposureFrequencyTest, ConvertsFrequencyToMicrosecondInterval) {
  EXPECT_EQ(IntersectionObserverManager::CalculateExposureIntervalMicros(-1),
            0);
  EXPECT_EQ(IntersectionObserverManager::CalculateExposureIntervalMicros(0), 0);
  EXPECT_EQ(IntersectionObserverManager::CalculateExposureIntervalMicros(1),
            1000000);
  EXPECT_EQ(IntersectionObserverManager::CalculateExposureIntervalMicros(20),
            50000);
  EXPECT_EQ(IntersectionObserverManager::CalculateExposureIntervalMicros(60),
            16666);
  EXPECT_EQ(IntersectionObserverManager::CalculateExposureIntervalMicros(120),
            16666);
}

TEST_F(ExposeObserverTest, NodeReadyImmediatelyExposesVisibleTarget) {
  View* target = AddVisibleObservedView(1);

  NotifyTargetReady(target);

  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  ASSERT_EQ(custom_event_options().size(), 1u);
  EXPECT_TRUE(custom_event_options()[0].emergency);
  EXPECT_EQ(global_events(), Events({"exposure"}));

  manager()->NotifyObservers();
  page_->SendGlobalExposureEvent();
  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  EXPECT_EQ(global_events(), Events({"exposure"}));
}

TEST_F(ExposeObserverTest,
       DirectPageChildWithExposureAreaSkipsFastPathWithoutGeometry) {
  auto* target = new View(1, page_.get());
  target->SetBound(2000, 2000, 100, 100);
  target->SetAttribute("exposure-id", Value("offscreen-target"));
  target->SetAttribute("exposure-area", Value("100%"));
  target->AddEventCallback("uiappear");
  target->AddEventCallback("uidisappear");

  page_->AddChild(target);

  EXPECT_TRUE(custom_events().empty());
  EXPECT_TRUE(global_events().empty());

  target->SetBound(0, 0, 100, 100);
  NotifyObserversOnNextFrame();
  page_->SendGlobalExposureEvent();
  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  EXPECT_EQ(global_events(), Events({"exposure"}));
}

TEST_F(ExposeObserverTest,
       DirectPageChildWithDefaultExposureAreaUsesFastPathWithoutGeometry) {
  auto* target = new View(1, page_.get());
  target->SetBound(2000, 2000, 100, 100);
  target->SetAttribute("exposure-id", Value("offscreen-target"));
  target->AddEventCallback("uiappear");
  target->AddEventCallback("uidisappear");

  page_->AddChild(target);

  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  EXPECT_EQ(global_events(), Events({"exposure"}));
  ASSERT_EQ(custom_event_options().size(), 1u);
  EXPECT_TRUE(custom_event_options()[0].emergency);
}

TEST_F(ExposeObserverTest, StructuralAttestationRejectsLaterPageChildren) {
  page_->AddChild(new View(99, page_.get()));
  View* target = AddVisibleObservedView(1);

  NotifyTargetReady(target);

  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  ASSERT_EQ(custom_event_options().size(), 1u);
  EXPECT_FALSE(custom_event_options()[0].emergency);
}

TEST_F(ExposeObserverTest, NodeReadyWaitsForValidBounds) {
  View* target = AddObservedView(page_.get(), 1, 0, 0, 0, 0);

  NotifyTargetReady(target);

  EXPECT_TRUE(custom_events().empty());
  EXPECT_TRUE(global_events().empty());

  target->SetBound(0, 0, 100, 100);
  manager()->NotifyObservers();
  page_->SendGlobalExposureEvent();
  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  EXPECT_EQ(global_events(), Events({"exposure"}));
}

TEST_F(ExposeObserverTest, NodeReadyIgnoresDetachedTarget) {
  auto target = std::make_unique<View>(1, page_.get());
  target->SetBound(0, 0, 100, 100);
  target->SetAttribute("exposure-id", Value("detached-target"));
  target->AddEventCallback("uiappear");

  NotifyTargetReady(target.get());

  EXPECT_TRUE(custom_events().empty());
  EXPECT_TRUE(global_events().empty());
}

TEST_F(ExposeObserverTest, NodeReadyRespectsExposureGates) {
  manager()->SetExposureHostVisible(false);
  View* target = AddVisibleObservedView(1);

  NotifyTargetReady(target);
  EXPECT_TRUE(custom_events().empty());
  EXPECT_TRUE(global_events().empty());

  manager()->SetExposureHostVisible(true);
  manager()->StopExposure(false);
  NotifyTargetReady(target);
  EXPECT_TRUE(custom_events().empty());
  EXPECT_TRUE(global_events().empty());

  manager()->ResumeExposure();
  NotifyTargetReady(target);
  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  EXPECT_EQ(global_events(), Events({"exposure"}));
}

TEST_F(ExposeObserverTest, StopWithoutEventPreservesVisibleState) {
  AddVisibleObservedView(1);
  manager()->NotifyObservers();
  ASSERT_EQ(custom_events(), Events({"uiappear"}));
  page_->SendGlobalExposureEvent();
  ASSERT_EQ(global_events(), Events({"exposure"}));

  for (int i = 0; i < 15; ++i) {
    manager()->StopExposure(false);
    manager()->ResumeExposure();
  }

  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear"}));
}

TEST_F(ExposeObserverTest, StopWithEventStormCoalescesUntilNextFrame) {
  AddVisibleObservedView(1);
  manager()->NotifyObservers();
  ASSERT_EQ(custom_events(), Events({"uiappear"}));

  for (int i = 0; i < 15; ++i) {
    manager()->StopExposure(true);
    manager()->ResumeExposure();
  }

  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear"}));
  EXPECT_EQ(global_events(), Events({"exposure", "disexposure"}));
  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear", "uiappear"}));
  page_->SendGlobalExposureEvent();
  EXPECT_EQ(global_events(), Events({"exposure", "disexposure", "exposure"}));
}

TEST_F(ExposeObserverTest, StopWithEventFlushesPairedEventsInSameTask) {
  AddVisibleObservedView(1);
  manager()->NotifyObservers();

  manager()->StopExposure(true);

  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear"}));
  ASSERT_EQ(custom_event_options().size(), 2u);
  EXPECT_TRUE(custom_event_options()[0].emergency);
  EXPECT_FALSE(custom_event_options()[1].emergency);
  EXPECT_EQ(global_events(), Events({"exposure", "disexposure"}));
}

TEST_F(ExposeObserverTest, ObserverAddedWhileStoppedWaitsForResume) {
  manager()->StopExposure(true);
  AddVisibleObservedView(1);

  manager()->NotifyObservers();
  EXPECT_TRUE(custom_events().empty());

  manager()->ResumeExposure();
  EXPECT_TRUE(custom_events().empty());
  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear"}));
}

TEST_F(ExposeObserverTest, StopWithoutEventThenStopWithEventTransitionsOnce) {
  AddVisibleObservedView(1);
  manager()->NotifyObservers();
  ASSERT_EQ(custom_events(), Events({"uiappear"}));

  manager()->StopExposure(false);
  manager()->StopExposure(true);
  manager()->StopExposure(true);
  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear"}));
  EXPECT_EQ(global_events(), Events({"exposure", "disexposure"}));
}

TEST_F(ExposeObserverTest, StopWithoutEventReconcilesGeometryAfterResume) {
  View* target = AddVisibleObservedView(1);
  manager()->NotifyObservers();
  ASSERT_EQ(custom_events(), Events({"uiappear"}));
  page_->SendGlobalExposureEvent();

  manager()->StopExposure(false);
  target->SetBound(2000, 2000, 100, 100);
  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear"}));

  manager()->ResumeExposure();
  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear"}));
  page_->SendGlobalExposureEvent();
  EXPECT_EQ(global_events(), Events({"exposure", "disexposure"}));
}

TEST_F(ExposeObserverTest, HostWindowGateDefersAndReconcilesExposure) {
  manager()->SetExposureHostVisible(false);
  View* target = AddVisibleObservedView(1);

  manager()->NotifyObservers();
  EXPECT_TRUE(custom_events().empty());

  manager()->SetExposureHostVisible(true);
  manager()->NotifyObservers();
  ASSERT_EQ(custom_events(), Events({"uiappear"}));

  manager()->SetExposureHostVisible(false);
  target->SetBound(2000, 2000, 100, 100);
  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear"}));

  manager()->SetExposureHostVisible(true);
  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear"}));
}

TEST_F(ExposeObserverTest, RealDetachClosesExposureWhileHostWindowIsDetached) {
  View* target = AddVisibleObservedView(1);
  manager()->NotifyObservers();
  ASSERT_EQ(custom_events(), Events({"uiappear"}));

  manager()->SetExposureHostVisible(false);
  manager()->NotifyTargetDetached(target);
  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear"}));
}

TEST_F(ExposeObserverTest, StableGeometryDoesNotAmplifyAppearEvents) {
  View* target = AddVisibleObservedView(1);

  // Repeated frame notifications must not turn a stable state into duplicate
  // appear or disappear events.
  for (int i = 0; i < 10; ++i) {
    NotifyObserversOnNextFrame();
  }
  EXPECT_EQ(CustomEventCount("uiappear"), 1u);
  EXPECT_EQ(CustomEventCount("uidisappear"), 0u);

  target->SetBound(2000, 2000, 100, 100);
  for (int i = 0; i < 10; ++i) {
    NotifyObserversOnNextFrame();
  }
  EXPECT_EQ(CustomEventCount("uiappear"), 1u);
  EXPECT_EQ(CustomEventCount("uidisappear"), 1u);

  target->SetBound(0, 0, 100, 100);
  for (int i = 0; i < 10; ++i) {
    NotifyObserversOnNextFrame();
  }
  EXPECT_EQ(CustomEventCount("uiappear"), 2u);
  EXPECT_EQ(CustomEventCount("uidisappear"), 1u);
}

TEST_F(ExposeObserverTest, EarlyFrameSchedulesSingleFrequencyRetry) {
  View* target = AddVisibleObservedView(1);
  manager()->SetExposureFrequency(1);
  manager()->NotifyObservers();
  ASSERT_EQ(custom_events(), Events({"uiappear"}));

  target->SetBound(2000, 2000, 100, 100);
  manager()->NotifyObservers();
  manager()->NotifyObservers();
  manager()->NotifyObservers();

  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  ASSERT_EQ(task_runner_->DelayedTaskCount(), 1u);

  task_runner_->AdvanceBy(fml::TimeDelta::FromMilliseconds(1000));
  EXPECT_EQ(task_runner_->DelayedTaskCount(), 0u);

  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear"}));
}

TEST_F(ExposeObserverTest, HorizontalScrollViewOnlyExposesIntersectingCards) {
  auto* scroll_view =
      new ScrollView(1, ScrollDirection::kHorizontal, page_.get());
  page_->AddChild(scroll_view);
  scroll_view->SetBound(100, 100, 300, 100);

  // Child bounds are local to the 300-pixel-wide scroll viewport. The third
  // card starts beyond its right edge.
  AddObservedView(scroll_view, 2, 0, 0, 100, 100);
  AddObservedView(scroll_view, 3, 150, 0, 100, 100);
  AddObservedView(scroll_view, 4, 350, 0, 100, 100);

  manager()->NotifyObservers();
  EXPECT_EQ(CustomEventCount(2, "uiappear"), 1u);
  EXPECT_EQ(CustomEventCount(3, "uiappear"), 1u);
  EXPECT_EQ(CustomEventCount(4, "uiappear"), 0u);

  scroll_view->OnScrollUpdate(100);
  EXPECT_EQ(scroll_view->GetScrollOffset().x(), 100);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount(2, "uidisappear"), 1u);
  EXPECT_EQ(CustomEventCount(3, "uidisappear"), 0u);
  EXPECT_EQ(CustomEventCount(4, "uiappear"), 1u);

  scroll_view->OnScrollUpdate(0);
  EXPECT_EQ(scroll_view->GetScrollOffset().x(), 0);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount(2, "uiappear"), 2u);
  EXPECT_EQ(CustomEventCount(3, "uidisappear"), 0u);
  EXPECT_EQ(CustomEventCount(4, "uidisappear"), 1u);
}

TEST_F(ExposeObserverTest, ExposureAreaRequiresConfiguredVisibleRatio) {
  View* target = AddObservedView(page_.get(), 1, 999, 0, 100, 100);
  target->SetAttribute("exposure-area", Value("100%"));

  manager()->NotifyObservers();
  EXPECT_EQ(CustomEventCount("uiappear"), 0u);

  target->SetBound(900, 0, 100, 100);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount("uiappear"), 1u);

  target->SetBound(901, 0, 100, 100);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount("uidisappear"), 1u);

  target->SetAttribute("exposure-area", Value("invalid"));
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount("uiappear"), 2u);
}

TEST_F(ExposeObserverTest, ExposureAreaChecksEachClippingBoundary) {
  page_->SetBound(0, 0, 100, 60);

  auto* clipping_parent = new View(1, page_.get());
  page_->AddChild(clipping_parent);
  clipping_parent->SetBound(0, 0, 60, 100);
  clipping_parent->SetOverflow(CSSProperty::OVERFLOW_HIDDEN);

  View* target = AddObservedView(clipping_parent, 2, 0, 0, 100, 100);
  target->SetAttribute("exposure-area", Value("50%"));

  manager()->NotifyObservers();
  EXPECT_EQ(CustomEventCount("uiappear"), 1u);

  clipping_parent->SetBound(0, 0, 50, 100);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount("uiappear"), 1u);
  EXPECT_EQ(CustomEventCount("uidisappear"), 0u);

  clipping_parent->SetBound(0, 0, 49, 100);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount("uidisappear"), 1u);
}

TEST_F(ExposeObserverTest,
       EmptyClippingParentKeepsOrdinaryIntersectionBehavior) {
  auto* clipping_parent = new View(1, page_.get());
  page_->AddChild(clipping_parent);
  clipping_parent->SetBound(0, 0, 0, 0);
  clipping_parent->SetOverflow(CSSProperty::OVERFLOW_HIDDEN);

  auto* target = new View(2, page_.get());
  clipping_parent->AddChild(target);
  target->SetBound(0, 0, 100, 100);

  EXPECT_TRUE(IsViewIntersecting(target, page_.get(), false));
}

TEST_F(ExposeObserverTest, ScreenMarginLeftDoesNotExpandRightSide) {
  View* left_target = AddObservedView(page_.get(), 1, -75, 0, 25, 25);
  left_target->SetAttribute("exposure-screen-margin-left", Value("100px"));

  View* right_target = AddObservedView(page_.get(), 2, 1050, 0, 25, 25);
  right_target->SetAttribute("exposure-screen-margin-left", Value("100px"));

  manager()->NotifyObservers();
  EXPECT_EQ(CustomEventCount(1, "uiappear"), 1u);
  EXPECT_EQ(CustomEventCount(2, "uiappear"), 0u);
}

TEST_F(ExposeObserverTest, HiddenPreloadedPageDoesNotExposeItsContent) {
  // Model two page containers that occupy the same viewport. Only the active
  // container may contribute exposure events.
  auto* current_page = new View(1, page_.get());
  page_->AddChild(current_page);
  current_page->SetBound(0, 0, 1000, 1000);
  AddObservedView(current_page, 2, 0, 0, 100, 100);

  auto* preloaded_page = new View(3, page_.get());
  page_->AddChild(preloaded_page);
  preloaded_page->SetBound(0, 0, 1000, 1000);
  preloaded_page->SetVisible(false);
  AddObservedView(preloaded_page, 4, 0, 0, 100, 100);

  manager()->NotifyObservers();
  EXPECT_EQ(CustomEventCount(2, "uiappear"), 1u);
  EXPECT_EQ(CustomEventCount(4, "uiappear"), 0u);

  current_page->SetVisible(false);
  preloaded_page->SetVisible(true);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount(2, "uidisappear"), 1u);
  EXPECT_EQ(CustomEventCount(4, "uiappear"), 1u);

  for (int i = 0; i < 10; ++i) {
    NotifyObserversOnNextFrame();
  }
  EXPECT_EQ(CustomEventCount(2, "uiappear"), 1u);
  EXPECT_EQ(CustomEventCount(2, "uidisappear"), 1u);
  EXPECT_EQ(CustomEventCount(4, "uiappear"), 1u);
  EXPECT_EQ(CustomEventCount(4, "uidisappear"), 0u);
}

TEST_F(ExposeObserverTest, OverflowHiddenViewportClipsOffscreenListItem) {
  auto* list_viewport = new View(1, page_.get());
  page_->AddChild(list_viewport);
  list_viewport->SetBound(100, 100, 200, 200);
  list_viewport->SetOverflow(CSSProperty::OVERFLOW_HIDDEN);
  // The item is inside the page but outside its parent's clipping rectangle.
  View* list_item = AddObservedView(list_viewport, 2, 250, 0, 100, 100);

  manager()->NotifyObservers();
  EXPECT_EQ(CustomEventCount("uiappear"), 0u);

  list_item->SetBound(150, 0, 100, 100);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount("uiappear"), 1u);

  list_item->SetBound(250, 0, 100, 100);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount("uidisappear"), 1u);
}

TEST_F(ExposeObserverTest, RecycledListItemReconcilesBeforeReappearing) {
  auto* list_viewport = new View(1, page_.get());
  page_->AddChild(list_viewport);
  list_viewport->SetBound(0, 0, 300, 300);
  list_viewport->SetOverflow(CSSProperty::OVERFLOW_HIDDEN);
  View* list_item = AddObservedView(list_viewport, 2, 0, 0, 100, 100);

  manager()->NotifyObservers();
  ASSERT_EQ(CustomEventCount("uiappear"), 1u);

  list_viewport->RemoveChild(list_item);
  EXPECT_EQ(CustomEventCount("uidisappear"), 1u);

  // Reattaching a recycled item offscreen must not reuse its previous visible
  // state. It can appear again only after moving back into the viewport.
  list_item->SetBound(0, 400, 100, 100);
  list_viewport->AddChild(list_item);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount("uiappear"), 1u);

  list_item->SetBound(0, 100, 100, 100);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(CustomEventCount("uiappear"), 2u);
  EXPECT_EQ(CustomEventCount("uidisappear"), 1u);
}

TEST_F(ExposeObserverTest, StableTargetDoesNotReappearAfterHostForeground) {
  AddVisibleObservedView(1);
  manager()->NotifyObservers();
  ASSERT_EQ(CustomEventCount("uiappear"), 1u);

  // Host visibility gates observation; it does not imply that a stable target
  // left and re-entered the viewport.
  for (int i = 0; i < 10; ++i) {
    manager()->SetExposureHostVisible(false);
    manager()->NotifyObservers();
    manager()->SetExposureHostVisible(true);
    manager()->NotifyObservers();
  }

  EXPECT_EQ(CustomEventCount("uiappear"), 1u);
  EXPECT_EQ(CustomEventCount("uidisappear"), 0u);
}

TEST_F(ExposeObserverTest, RootViewportClipsDetachedTarget) {
  auto detached_parent = std::make_unique<View>(1, page_.get());
  auto target = std::make_unique<View>(2, page_.get());
  detached_parent->AddChild(target.get());
  ASSERT_NE(target->Parent(), page_.get());

  target->SetBound(1391, 4, 70, 24);
  target->SetAttribute("exposure-id", Value("detached-target"));
  target->AddEventCallback("uiappear");
  target->AddEventCallback("uidisappear");

  manager()->NotifyObservers();
  EXPECT_TRUE(custom_events().empty());
  page_->SendGlobalExposureEvent();
  EXPECT_TRUE(global_events().empty());

  target->SetBound(950, 0, 100, 100);
  manager()->StopExposure(false);
  manager()->ResumeExposure();
  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear"}));
  page_->SendGlobalExposureEvent();
  EXPECT_EQ(global_events(), Events({"exposure"}));

  page_->SetVisible(false);
  manager()->StopExposure(false);
  manager()->ResumeExposure();
  manager()->NotifyObservers();
  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear"}));
  page_->SendGlobalExposureEvent();
  EXPECT_EQ(global_events(), Events({"exposure", "disexposure"}));

  detached_parent->RemoveChild(target.get());
}

TEST_F(ExposeObserverTest, ExposureAreaChecksRootOutsideTargetParentChain) {
  auto detached_parent = std::make_unique<View>(1, page_.get());
  auto target = std::make_unique<View>(2, page_.get());
  detached_parent->AddChild(target.get());
  ASSERT_NE(target->Parent(), page_.get());

  target->SetBound(950, 0, 100, 100);
  target->SetAttribute("exposure-id", Value("detached-target"));
  target->SetAttribute("exposure-area", Value("100%"));
  target->AddEventCallback("uiappear");
  target->AddEventCallback("uidisappear");

  manager()->NotifyObservers();
  EXPECT_TRUE(custom_events().empty());

  target->SetBound(900, 0, 100, 100);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(custom_events(), Events({"uiappear"}));

  page_->SetVisible(false);
  NotifyObserversOnNextFrame();
  EXPECT_EQ(custom_events(), Events({"uiappear", "uidisappear"}));

  detached_parent->RemoveChild(target.get());
}

}  // namespace
}  // namespace clay
