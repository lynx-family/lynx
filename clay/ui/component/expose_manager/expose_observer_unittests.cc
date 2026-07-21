// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "clay/public/event_delegate.h"
#include "clay/public/value.h"
#include "clay/ui/component/intersection_observer_manager.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

std::vector<std::string> Events(std::initializer_list<std::string> names) {
  return std::vector<std::string>(names);
}

class RecordingEventDelegate final : public EventDelegate {
 public:
  const std::vector<std::string>& custom_events() const {
    return custom_events_;
  }
  const std::vector<std::string>& global_events() const {
    return global_events_;
  }
  void OnSendCustomEvent(int, const std::string& event_name,
                         Value::Map) override {
    custom_events_.push_back(event_name);
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
  std::vector<std::string> global_events_;
};

class ExposeObserverTest : public ::testing::Test {
 protected:
  void SetUp() override {
    page_ = std::make_unique<PageView>(0, nullptr, nullptr);
    page_->SetEventDelegate(&event_delegate_);
    page_->SetBound(0, 0, 1000, 1000);
  }

  void TearDown() override { page_->DestroyAllChildren(); }

  View* AddVisibleObservedView(int id) {
    auto* target = new View(id, page_.get());
    page_->AddChild(target);
    target->SetBound(0, 0, 100, 100);
    target->SetAttribute("exposure-id",
                         Value("visible-target-" + std::to_string(id)));
    target->AddEventCallback("uiappear");
    target->AddEventCallback("uidisappear");
    return target;
  }

  IntersectionObserverManager* manager() {
    return page_->intersection_observer_manager();
  }

  const std::vector<std::string>& custom_events() const {
    return event_delegate_.custom_events();
  }

  const std::vector<std::string>& global_events() const {
    return event_delegate_.global_events();
  }

  RecordingEventDelegate event_delegate_;
  std::unique_ptr<PageView> page_;
};

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

}  // namespace
}  // namespace clay
