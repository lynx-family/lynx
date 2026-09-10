// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "base/include/fml/thread.h"
#include "clay/ui/common/overlay_manager.h"
#include "clay/ui/component/overlay_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/rendering/render_object.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

PointerEvent CreateDownPointer(float x, float y) {
  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.position = {x, y};
  return event;
}

class OffsetOverlayView : public OverlayView {
 public:
  OffsetOverlayView(uint32_t id, PageView* page_view, Point touch_offset,
                    bool is_system_overlay)
      : OverlayView(id, page_view),
        touch_offset_(touch_offset),
        is_system_overlay_(is_system_overlay) {}

  bool ShouldChangeOffset() const override { return true; }
  bool IsSystemOverlay() const override { return is_system_overlay_; }
  Point GetTouchOffset() const override { return touch_offset_; }

 private:
  Point touch_offset_;
  bool is_system_overlay_;
};

void ExpectHitTestResult(const HitTestResult& result,
                         std::initializer_list<BaseView*> expected) {
  ASSERT_EQ(result.size(), expected.size());
  auto result_it = result.begin();
  for (auto* expected_target : expected) {
    EXPECT_EQ(result_it->get(), expected_target);
    ++result_it;
  }
}

}  // namespace

TEST(OverlayViewTest, ShowAndHide) {
  auto thread = std::make_unique<fml::Thread>("ui");
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, thread->GetTaskRunner());
  page_view->SetBound(0, 0, 1000, 1000);
  std::unique_ptr<OverlayView> overlay1 =
      std::make_unique<OverlayView>(1, page_view.get());
  std::unique_ptr<OverlayView> overlay2 =
      std::make_unique<OverlayView>(2, page_view.get());
  overlay1->SetBound(0, 0, 1000, 1000);
  overlay2->SetBound(0, 0, 1000, 1000);

  auto child1 = std::make_unique<View>(3, page_view.get());
  child1->SetBound(0, 0, 100, 100);
  overlay1->AddChild(child1.get());

  auto child2 = std::make_unique<View>(4, page_view.get());
  child2->SetBound(100, 100, 200, 200);
  overlay2->AddChild(child2.get());

  FloatPoint unused;
  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(10, 10), &unused),
            page_view.get());

  EXPECT_EQ(page_view->overlay_manager()->overlays_.size(), (size_t)0);

  page_view->AddChild(overlay1.get());
  page_view->AddChild(overlay2.get());

  page_view->Layout();

  EXPECT_EQ(page_view->overlay_manager()->overlays_.size(), (size_t)2);

  overlay2->Hide();

  EXPECT_EQ(page_view->overlay_manager()->overlays_.size(), (size_t)1);

  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(10, 10), &unused),
            child1.get());
  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(110, 10), &unused),
            page_view.get());

  overlay2->SetPassEventsThrough(false);
  overlay2->Show();

  EXPECT_EQ(page_view->overlay_manager()->overlays_.size(), (size_t)2);
  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(110, 10), &unused),
            overlay2.get());
  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(110, 110), &unused),
            child2.get());
}

TEST(OverlayViewTest, Level) {
  auto thread = std::make_unique<fml::Thread>("ui");
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(-1, nullptr, thread->GetTaskRunner());
  page_view->SetBound(0, 0, 1000, 1000);
  std::unique_ptr<OverlayView> overlay1 =
      std::make_unique<OverlayView>(-1, page_view.get());
  std::unique_ptr<OverlayView> overlay2 =
      std::make_unique<OverlayView>(-1, page_view.get());

  page_view->AddChild(overlay1.get());
  page_view->AddChild(overlay2.get());

  page_view->Layout();

  // The larger the level, the lower it will be displayed
  overlay1->Hide();
  overlay2->Hide();
  overlay1->SetLevel(1);
  overlay2->SetLevel(2);
  overlay1->Show();
  overlay2->Show();
  EXPECT_EQ(page_view->overlay_manager()->overlays_.back(), overlay1.get());
  overlay1->Hide();
  overlay2->Hide();
  overlay1->SetLevel(2);
  overlay2->SetLevel(1);
  overlay1->Show();
  overlay2->Show();
  EXPECT_EQ(page_view->overlay_manager()->overlays_.back(), overlay2.get());

  // For overlays with the same level, the newly added ones are at the front (to
  // user).
  overlay1->Hide();
  overlay2->Hide();
  overlay1->SetLevel(1);
  overlay2->SetLevel(1);
  overlay1->Show();
  overlay2->Show();
  EXPECT_EQ(page_view->overlay_manager()->overlays_.back(), overlay2.get());
  overlay1->Hide();
  overlay2->Hide();
  overlay1->SetLevel(1);
  overlay2->SetLevel(1);
  overlay2->Show();
  overlay1->Show();
  EXPECT_EQ(page_view->overlay_manager()->overlays_.back(), overlay1.get());
}

class OverlayViewEventThroughTest : public UITest {};

TEST_F_UI(OverlayViewEventThroughTest,
          PassThroughChildContinuesHitTestingUnderlyingPage) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto overlay = std::make_unique<OverlayView>(2, page_.get());
  auto overlay_child = std::make_unique<View>(3, page_.get());
  page_->AddChild(underlying.get());
  page_->AddChild(overlay.get());
  overlay->AddChild(overlay_child.get());
  underlying->SetBound(0, 0, 100, 100);
  overlay->SetBound(0, 0, 100, 100);
  overlay_child->SetBound(0, 0, 100, 100);
  overlay_child->SetEventThrough(true);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(
      result, {overlay_child.get(), nullptr, underlying.get(), page_.get()});
  EXPECT_TRUE(result.front()->ShouldPassEventToNative());

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      underlying.get());
  EXPECT_EQ(relative_position, FloatPoint(50, 50));
}

TEST_F_UI(OverlayViewEventThroughTest,
          RegularOverlayChildStopsHitTestingUnderlyingPage) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto overlay = std::make_unique<OverlayView>(2, page_.get());
  auto overlay_child = std::make_unique<View>(3, page_.get());
  page_->AddChild(underlying.get());
  page_->AddChild(overlay.get());
  overlay->AddChild(overlay_child.get());
  underlying->SetBound(0, 0, 100, 100);
  overlay->SetBound(0, 0, 100, 100);
  overlay_child->SetBound(0, 0, 100, 100);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(result, {overlay_child.get(), nullptr});
  EXPECT_FALSE(result.front()->ShouldPassEventToNative());

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      overlay_child.get());
}

TEST_F_UI(OverlayViewEventThroughTest, OverlayEventThroughIsInheritedByChild) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto overlay = std::make_unique<OverlayView>(2, page_.get());
  auto overlay_child = std::make_unique<View>(3, page_.get());
  page_->AddChild(underlying.get());
  page_->AddChild(overlay.get());
  overlay->AddChild(overlay_child.get());
  underlying->SetBound(0, 0, 100, 100);
  overlay->SetBound(0, 0, 100, 100);
  overlay_child->SetBound(0, 0, 100, 100);
  overlay->SetEventThrough(true);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(
      result, {overlay_child.get(), nullptr, underlying.get(), page_.get()});
  EXPECT_TRUE(result.front()->ShouldPassEventToNative());

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      underlying.get());
}

TEST_F_UI(OverlayViewEventThroughTest,
          ExplicitFalseChildStopsInheritedOverlayEventThrough) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto overlay = std::make_unique<OverlayView>(2, page_.get());
  auto overlay_child = std::make_unique<View>(3, page_.get());
  page_->AddChild(underlying.get());
  page_->AddChild(overlay.get());
  overlay->AddChild(overlay_child.get());
  underlying->SetBound(0, 0, 100, 100);
  overlay->SetBound(0, 0, 100, 100);
  overlay_child->SetBound(0, 0, 100, 100);
  overlay->SetEventThrough(true);
  overlay_child->SetEventThrough(false);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(result, {overlay_child.get(), nullptr});
  EXPECT_FALSE(result.front()->ShouldPassEventToNative());

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      overlay_child.get());
}

TEST_F_UI(OverlayViewEventThroughTest,
          BlockingOverlayStopsEventThroughChildAtOverlayBoundary) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto overlay = std::make_unique<OverlayView>(2, page_.get());
  auto overlay_child = std::make_unique<View>(3, page_.get());
  page_->AddChild(underlying.get());
  page_->AddChild(overlay.get());
  overlay->AddChild(overlay_child.get());
  underlying->SetBound(0, 0, 100, 100);
  overlay->SetBound(0, 0, 100, 100);
  overlay_child->SetBound(0, 0, 100, 100);
  overlay->SetPassEventsThrough(false);
  overlay_child->SetEventThrough(true);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(result, {overlay_child.get(), overlay.get(), nullptr});
  EXPECT_TRUE(result.front()->ShouldPassEventToNative());
  EXPECT_EQ(result.back().get(), nullptr);

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      overlay.get());
}

TEST_F_UI(OverlayViewEventThroughTest,
          PassThroughOverlaySurfaceFallsThroughWithoutBoundary) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto overlay = std::make_unique<OverlayView>(2, page_.get());
  page_->AddChild(underlying.get());
  page_->AddChild(overlay.get());
  underlying->SetBound(0, 0, 100, 100);
  overlay->SetBound(0, 0, 100, 100);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(result, {underlying.get(), page_.get()});

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      underlying.get());
}

TEST_F_UI(OverlayViewEventThroughTest,
          LowerBlockingOverlayStopsTopEventThroughOverlay) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto blocking_overlay = std::make_unique<OverlayView>(2, page_.get());
  auto pass_through_overlay = std::make_unique<OverlayView>(3, page_.get());
  auto pass_through_child = std::make_unique<View>(4, page_.get());
  page_->AddChild(underlying.get());
  page_->AddChild(blocking_overlay.get());
  page_->AddChild(pass_through_overlay.get());
  pass_through_overlay->AddChild(pass_through_child.get());
  underlying->SetBound(0, 0, 100, 100);
  blocking_overlay->SetBound(0, 0, 100, 100);
  pass_through_overlay->SetBound(0, 0, 100, 100);
  pass_through_child->SetBound(0, 0, 100, 100);
  blocking_overlay->SetPassEventsThrough(false);
  pass_through_child->SetEventThrough(true);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(result, {pass_through_child.get(), nullptr,
                               blocking_overlay.get(), nullptr});

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      blocking_overlay.get());
}

TEST_F_UI(OverlayViewEventThroughTest,
          MultipleEventThroughOverlayChildrenReachUnderlyingPage) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto lower_overlay = std::make_unique<OverlayView>(2, page_.get());
  auto lower_child = std::make_unique<View>(3, page_.get());
  auto upper_overlay = std::make_unique<OverlayView>(4, page_.get());
  auto upper_child = std::make_unique<View>(5, page_.get());
  page_->AddChild(underlying.get());
  page_->AddChild(lower_overlay.get());
  page_->AddChild(upper_overlay.get());
  lower_overlay->AddChild(lower_child.get());
  upper_overlay->AddChild(upper_child.get());
  underlying->SetBound(0, 0, 100, 100);
  lower_overlay->SetBound(0, 0, 100, 100);
  lower_child->SetBound(0, 0, 100, 100);
  upper_overlay->SetBound(0, 0, 100, 100);
  upper_child->SetBound(0, 0, 100, 100);
  lower_child->SetEventThrough(true);
  upper_child->SetEventThrough(true);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(result, {upper_child.get(), nullptr, lower_child.get(),
                               nullptr, underlying.get(), page_.get()});

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      underlying.get());
}

TEST_F_UI(OverlayViewEventThroughTest,
          PageEventThroughIsInheritedAcrossOverlayBoundary) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto overlay = std::make_unique<OverlayView>(2, page_.get());
  auto overlay_child = std::make_unique<View>(3, page_.get());
  page_->AddChild(underlying.get());
  page_->AddChild(overlay.get());
  overlay->AddChild(overlay_child.get());
  underlying->SetBound(0, 0, 100, 100);
  overlay->SetBound(0, 0, 100, 100);
  overlay_child->SetBound(0, 0, 100, 100);
  page_->SetEventThrough(true);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(
      result, {overlay_child.get(), nullptr, underlying.get(), page_.get()});
  for (const auto& target : result) {
    if (target) {
      EXPECT_TRUE(target->ShouldPassEventToNative());
    }
  }

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      nullptr);
}

TEST_F_UI(OverlayViewEventThroughTest,
          OverlayPassThroughUsesOffsetOnlyForInPageOverlay) {
  auto underlying = std::make_unique<View>(1, page_.get());
  auto overlay =
      std::make_unique<OffsetOverlayView>(2, page_.get(), Point(20, 30), false);
  page_->AddChild(underlying.get());
  page_->AddChild(overlay.get());
  underlying->SetBound(60, 70, 20, 20);
  overlay->SetBound(0, 0, 100, 100);
  Layout();

  HitTestResult result;
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(result, {underlying.get(), page_.get()});

  FloatPoint relative_position;
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      underlying.get());
  EXPECT_EQ(relative_position, FloatPoint(10, 10));

  page_->RemoveChild(overlay.get());
  auto system_overlay =
      std::make_unique<OffsetOverlayView>(3, page_.get(), Point(20, 30), true);
  page_->AddChild(system_overlay.get());
  system_overlay->SetBound(0, 0, 100, 100);
  Layout();

  result.clear();
  EXPECT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
  ExpectHitTestResult(result, {page_.get()});
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      page_.get());
  EXPECT_EQ(relative_position, FloatPoint(50, 50));
}

}  // namespace clay
