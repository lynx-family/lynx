// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/nested_scroll/nested_scroll_manager.h"
#include "clay/ui/component/nested_scroll/nested_scrollable.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/testing/ui_test.h"

namespace clay {

class TestScrollable : public NestedScrollable {
 public:
  explicit TestScrollable(PageView* page_view, float max_scroll_offset)
      : NestedScrollable(-1, "", std::make_unique<RenderScroll>(), page_view,
                         ScrollDirection::kVertical),
        max_scroll_offset_(max_scroll_offset) {
    SetEnableNestedScroll(true);
    SetBound(0, 0, max_scroll_offset, max_scroll_offset);
  }

  FloatPoint DoScroll(FloatPoint delta, bool is_user_input,
                      bool ignore_repaint) override {
    if (delta.y() != 0) {
      float new_offset = scroll_offset_ + delta.y();
      if (new_offset < 0) {
        delta.SetY(new_offset);
        new_offset = 0;
      } else if (new_offset > max_scroll_offset_) {
        delta.SetY(new_offset - max_scroll_offset_);
        new_offset = max_scroll_offset_;
      } else {
        delta.SetY(0);
      }
      scroll_offset_ = new_offset;
    }
    return delta;
  }

  void OnScrollStatusChange(ScrollStatus old_status) override {
    NestedScrollable::OnScrollStatusChange(old_status);
    if (status_ == ScrollStatus::kBounce) {
      bounced_ = true;
    }
  }

  float max_scroll_offset_;
  float scroll_offset_ = 0;
  bool bounced_ = false;
};

class NestedScrollableTest : public UITest {
  void UISetUp() override {}
};

TEST_F_UI(NestedScrollableTest, Chain) {
  NestedScrollable* scrollable1 =
      new ScrollView(-1, ScrollDirection::kVertical, page_.get());
  page_->AddChild(scrollable1);
  NestedScrollable* scrollable2 =
      new ScrollView(-1, ScrollDirection::kHorizontal, page_.get());
  scrollable1->AddChild(scrollable2);
  NestedScrollable* scrollable3 =
      new ScrollView(-1, ScrollDirection::kVertical, page_.get());
  scrollable2->AddChild(scrollable3);
  NestedScrollable* scrollable4 =
      new ScrollView(-1, ScrollDirection::kVertical, page_.get());
  scrollable3->AddChild(scrollable4);
  NestedScrollable* scrollable5 =
      new ScrollView(-1, ScrollDirection::kVertical, page_.get());
  scrollable4->AddChild(scrollable5);

  // The scrollable chain will stop on encountering a scrollable oriented in a
  // different direction
  page_->nested_scroll_manager()->DragStart(scrollable5);
  EXPECT_EQ(page_->nested_scroll_manager()->FindOuterScrollable(scrollable5),
            scrollable4);
  EXPECT_EQ(page_->nested_scroll_manager()->FindOuterScrollable(scrollable4),
            scrollable3);
  EXPECT_EQ(page_->nested_scroll_manager()->FindOuterScrollable(scrollable3),
            nullptr);

  // The scrollable chain will stop at the scrollable with nested scrolling
  // disabled
  scrollable4->SetEnableNestedScroll(false);
  page_->nested_scroll_manager()->DragStart(scrollable5);
  EXPECT_EQ(page_->nested_scroll_manager()->FindOuterScrollable(scrollable5),
            scrollable4);
  EXPECT_EQ(page_->nested_scroll_manager()->FindOuterScrollable(scrollable4),
            nullptr);
}

TEST_F_UI(NestedScrollableTest, SimpleScroll) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable);

  // No fling
  DispatchDragEvent({50, 50}, {50, 0}, false);
  EXPECT_FALSE(scrollable->nested_scroll_manager()->IsFlingRunning());
  EXPECT_EQ(scrollable->scroll_offset_, 50);
  DoAnimation(1000);
  EXPECT_EQ(scrollable->scroll_offset_, 50);
}

TEST_F_UI(NestedScrollableTest, NestedDrag) {
  TestScrollable* scrollable1 = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable1);
  TestScrollable* scrollable2 = new TestScrollable(page_.get(), 100);
  scrollable1->AddChild(scrollable2);

  page_->nested_scroll_manager()->DragStart(scrollable2);

  // First scroll the inner scrollable
  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, 50));
  EXPECT_EQ(scrollable2->scroll_offset_, 50);
  EXPECT_EQ(scrollable1->scroll_offset_, 0);

  // Pass the unconsumed delta to the outer scrollable
  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, 100));
  EXPECT_EQ(scrollable2->scroll_offset_, 100);
  EXPECT_EQ(scrollable1->scroll_offset_, 50);

  // The inner scrollable can't scroll any more
  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, 20));
  EXPECT_EQ(scrollable2->scroll_offset_, 100);
  EXPECT_EQ(scrollable1->scroll_offset_, 70);

  // Still scroll the inner first
  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, -50));
  EXPECT_EQ(scrollable2->scroll_offset_, 50);
  EXPECT_EQ(scrollable1->scroll_offset_, 70);

  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, -100));
  EXPECT_EQ(scrollable2->scroll_offset_, 0);
  EXPECT_EQ(scrollable1->scroll_offset_, 20);
}

TEST_F_UI(NestedScrollableTest, NestedDisabled) {
  TestScrollable* scrollable1 = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable1);
  TestScrollable* scrollable2 = new TestScrollable(page_.get(), 100);
  scrollable2->SetEnableNestedScroll(false);
  scrollable1->AddChild(scrollable2);

  page_->nested_scroll_manager()->DragStart(scrollable2);

  // Outer scrollable will not scroll as nested scroll is disabled
  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, 150));
  EXPECT_EQ(scrollable2->scroll_offset_, 100);
  EXPECT_EQ(scrollable1->scroll_offset_, 0);

  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, -70));
  EXPECT_EQ(scrollable2->scroll_offset_, 30);
  EXPECT_EQ(scrollable1->scroll_offset_, 0);
  page_->nested_scroll_manager()->DragEnd(scrollable2, {});

  // Fling only works on the inner scrollable
  DispatchDragEvent({50, 50}, {50, 0}, true, 20, 2);
  DoAnimation();
  EXPECT_EQ(scrollable2->scroll_offset_, 100);
  EXPECT_EQ(scrollable1->scroll_offset_, 0);
}

TEST_F_UI(NestedScrollableTest, RubberBandEffectAndBounce) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  scrollable->SetOverscrollEnabled(true);
  page_->AddChild(scrollable);

  // Drag to trigger the rubber band effect
  page_->nested_scroll_manager()->DragStart(scrollable);
  page_->nested_scroll_manager()->DragUpdate(scrollable, {0, -100});
  EXPECT_EQ(scrollable->OverscrollOffset().y(), -100);
  EXPECT_GT(scrollable->ClampedOverscrollOffset().y(), -60);
  EXPECT_LT(scrollable->ClampedOverscrollOffset().y(), -30);

  // Release to trigger the bounce animation
  float bounce_start_offset = scrollable->ClampedOverscrollOffset().y();
  page_->nested_scroll_manager()->DragEnd(scrollable, {0, 0});
  DoAnimation(1);
  EXPECT_GT(scrollable->ClampedOverscrollOffset().y(), bounce_start_offset);
  EXPECT_LT(scrollable->ClampedOverscrollOffset().y(), 0);
  DoAnimation(1000);
  EXPECT_EQ(scrollable->OverscrollOffset().y(), 0);
}

TEST_F_UI(NestedScrollableTest, Fling) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable);

  // Already at top, drag down should not trigger fling
  DispatchDragEvent({50, 50}, {50, 100}, true);
  EXPECT_EQ(scrollable->scroll_offset_, 0);
  EXPECT_TRUE(scrollable->nested_scroll_manager()->IsFlingRunning());
  DoAnimation(1);
  EXPECT_FALSE(scrollable->nested_scroll_manager()->IsFlingRunning());

  // Drag to trigger fling animation
  DispatchDragEvent({50, 50}, {50, 0}, true);
  EXPECT_TRUE(scrollable->nested_scroll_manager()->IsFlingRunning());
  EXPECT_GT(scrollable->scroll_offset_, 0);
  EXPECT_LE(scrollable->scroll_offset_, 50);
  DoAnimation(1);
  EXPECT_TRUE(scrollable->nested_scroll_manager()->IsFlingRunning());

  // Wait a second to finish the fling animation
  DoAnimation(1000);
  EXPECT_GT(scrollable->scroll_offset_, 50);
}

TEST_F_UI(NestedScrollableTest, FlingToBounce) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  scrollable->SetOverscrollEnabled(true);
  page_->AddChild(scrollable);

  // Drag to trigger the bounce animation
  DispatchDragEvent({50, 50}, {50, 0}, true, 10, 1);
  EXPECT_GT(scrollable->scroll_offset_, 0);
  EXPECT_LE(scrollable->scroll_offset_, 50);

  // Wait a second to finish the bounce animation
  DoAnimation();
  EXPECT_EQ(scrollable->bounced_, true);
}

TEST_F_UI(NestedScrollableTest, NestedFling) {
  TestScrollable* scrollable1 = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable1);
  TestScrollable* scrollable2 = new TestScrollable(page_.get(), 100);
  scrollable1->AddChild(scrollable2);

  DispatchDragEvent({50, 50}, {50, 0}, true, 10, 1);
  EXPECT_LT(scrollable2->scroll_offset_, 100);
  EXPECT_EQ(scrollable1->scroll_offset_, 0);

  // The outer scrollable also scrolled by the fling animation
  DoAnimation();
  EXPECT_EQ(scrollable2->scroll_offset_, 100);
  EXPECT_EQ(scrollable1->scroll_offset_, 100);
}

TEST_F_UI(NestedScrollableTest, RestoreOverscrollFirst) {
  TestScrollable* scrollable1 = new TestScrollable(page_.get(), 100);
  scrollable1->SetOverscrollEnabled(true);
  page_->AddChild(scrollable1);
  TestScrollable* scrollable2 = new TestScrollable(page_.get(), 100);
  scrollable1->AddChild(scrollable2);

  // The outer scrollable is overscrolled
  page_->nested_scroll_manager()->DragStart(scrollable2);
  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, -50));
  EXPECT_EQ(scrollable2->scroll_offset_, 0);
  EXPECT_EQ(scrollable1->scroll_offset_, 0);
  EXPECT_EQ(scrollable1->OverscrollOffset().y(), -50);

  // First restore the overscroll, then scroll the inner scrollable
  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, 20));
  EXPECT_EQ(scrollable2->scroll_offset_, 0);
  EXPECT_EQ(scrollable1->scroll_offset_, 0);
  EXPECT_EQ(scrollable1->OverscrollOffset().y(), -30);
  page_->nested_scroll_manager()->DragUpdate(scrollable2, FloatPoint(0, 50));
  EXPECT_EQ(scrollable2->scroll_offset_, 20);
  EXPECT_EQ(scrollable1->scroll_offset_, 0);
  EXPECT_EQ(scrollable1->OverscrollOffset().y(), 0);
}

TEST_F_UI(NestedScrollableTest, StopFlingOnTouch) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable);

  // Trigger fling animation
  DispatchDragEvent({50, 50}, {50, 0}, true, 10, 1);
  EXPECT_GT(scrollable->scroll_offset_, 0);
  EXPECT_LE(scrollable->scroll_offset_, 50);

  // The fling animation has not been finished
  DoAnimation(10);
  EXPECT_LT(scrollable->scroll_offset_, 100);

  // Stop the fling animation by touch
  float current_offset = scrollable->scroll_offset_;
  DispatchTapEvent({50, 50});
  DoAnimation();
  EXPECT_EQ(scrollable->scroll_offset_, current_offset);
}

TEST_F_UI(NestedScrollableTest, StopBounceOnTouch) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  scrollable->SetOverscrollEnabled(true);
  page_->AddChild(scrollable);

  // Drag to overscroll
  DispatchDragEvent({50, 50}, {50, 100}, false);
  EXPECT_EQ(scrollable->scroll_offset_, 0);
  EXPECT_LT(scrollable->OverscrollOffset().y(), 0);
  EXPECT_GE(scrollable->OverscrollOffset().y(), -50);

  // The bounce animation has not been finished
  DoAnimation(10);
  EXPECT_LT(scrollable->OverscrollOffset().y(), 0);

  // Touch to stop the bounce animation
  float current_offset = scrollable->OverscrollOffset().y();
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kDownEvent, {50, 50})});
  DoAnimation();
  EXPECT_EQ(scrollable->OverscrollOffset().y(), current_offset);

  // Release the touch to trigger bounce again
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kUpEvent, {50, 50})});
  DoAnimation();
  EXPECT_EQ(scrollable->OverscrollOffset().y(), 0);
}

TEST_F_UI(NestedScrollableTest, ScrollMode) {
  TestScrollable* outer = new TestScrollable(page_.get(), 100);
  page_->AddChild(outer);
  TestScrollable* inner = new TestScrollable(page_.get(), 100);
  outer->AddChild(inner);

  inner->SetForwardMode(NestedScrollMode::kSelfOnly);
  page_->nested_scroll_manager()->DragStart(inner);
  // Only the inner scrollable can scroll
  page_->nested_scroll_manager()->DragUpdate(inner, FloatPoint(0, 150));
  EXPECT_EQ(inner->scroll_offset_, 100);
  EXPECT_EQ(outer->scroll_offset_, 0);

  inner->SetForwardMode(NestedScrollMode::kSelfFirst);
  page_->nested_scroll_manager()->DragUpdate(inner, FloatPoint(0, -50));
  EXPECT_EQ(inner->scroll_offset_, 50);
  // The inner scrollable scrolls first, then the outer scrollable
  page_->nested_scroll_manager()->DragUpdate(inner, FloatPoint(0, 100));
  EXPECT_EQ(inner->scroll_offset_, 100);
  EXPECT_EQ(outer->scroll_offset_, 50);

  inner->SetBackwardMode(NestedScrollMode::kParentFirst);
  // The outer scrollable scrolls first, then the inner scrollable
  page_->nested_scroll_manager()->DragUpdate(inner, FloatPoint(0, -100));
  EXPECT_EQ(inner->scroll_offset_, 50);
  EXPECT_EQ(outer->scroll_offset_, 0);
}

TEST_F_UI(NestedScrollableTest, DestroyOnDrag) {
  auto outer = new ScrollView(-1, ScrollDirection::kVertical, page_.get());
  outer->SetBound(0, 0, 100, 100);
  page_->AddChild(outer);

  auto inner = new ScrollView(-1, ScrollDirection::kVertical, page_.get());
  inner->SetBound(0, 0, 100, 300);
  inner->SetTouchSlop(-1);
  outer->AddChild(inner, 0);

  auto box_view = new View(-1, page_.get());
  box_view->SetBound(0, 0, 100, 500);
  inner->AddChild(box_view, 0);
  // Make sure recalculate the overflow rect
  inner->OnLayoutUpdated();
  outer->OnLayoutUpdated();
  EXPECT_EQ(200, inner->GetRenderScroll()->MaxScrollHeight());
  EXPECT_EQ(200, outer->GetRenderScroll()->MaxScrollHeight());

  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kDownEvent, {50, 50})});
  page_->DispatchPointerEvent({CreatePointer(
      0, PointerEvent::EventType::kMoveEvent, {50, 20}, {0, -30})});
  EXPECT_EQ(inner->GetScrollOffset().height(), 30);
  page_->DestroyAllChildren();
  page_->DispatchPointerEvent({CreatePointer(
      0, PointerEvent::EventType::kMoveEvent, {50, 0}, {0, -20})});
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kUpEvent, {50, 20}, {0, 0})});

  // Just ensure there is no crash
}

TEST_F_UI(NestedScrollableTest, DestroyOnFling) {
  auto outer = new ScrollView(-1, ScrollDirection::kVertical, page_.get());
  outer->SetBound(0, 0, 100, 100);
  page_->AddChild(outer);

  auto inner = new ScrollView(-1, ScrollDirection::kVertical, page_.get());
  inner->SetBound(0, 0, 100, 300);
  inner->SetTouchSlop(-1);
  outer->AddChild(inner, 0);

  auto box_view = new View(-1, page_.get());
  box_view->SetBound(0, 0, 100, 500);
  inner->AddChild(box_view, 0);
  // Make sure recalculate the overflow rect
  inner->OnLayoutUpdated();
  outer->OnLayoutUpdated();
  EXPECT_EQ(200, inner->GetRenderScroll()->MaxScrollHeight());
  EXPECT_EQ(200, outer->GetRenderScroll()->MaxScrollHeight());

  DispatchDragEvent({50, 50}, {50, 0}, true);
  EXPECT_EQ(inner->GetScrollOffset().height(), 50);
  DoAnimation(20);
  EXPECT_GT(inner->GetScrollOffset().height(), 50);
  page_->DestroyAllChildren();
  DoAnimation(100);

  // Just ensure there is no crash
}

TEST_F_UI(NestedScrollableTest, ManagerStatusDrag) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable);

  page_->nested_scroll_manager()->DragStart(scrollable);
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kDragging);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kDragging);
  page_->nested_scroll_manager()->DragEnd(scrollable, {0, 0});
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
}

TEST_F_UI(NestedScrollableTest, ManagerStatusDragWithFling) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable);

  page_->nested_scroll_manager()->DragStart(scrollable);
  page_->nested_scroll_manager()->DragEnd(scrollable, {0, 500});
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kFling);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kFling);
  DoAnimation(1000);
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  EXPECT_GT(scrollable->scroll_offset_, 0);
}

TEST_F_UI(NestedScrollableTest, ManagerStatusDragWithBounce) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  scrollable->SetOverscrollEnabled(true);
  page_->AddChild(scrollable);

  page_->nested_scroll_manager()->DragStart(scrollable);
  page_->nested_scroll_manager()->DragUpdate(scrollable, {0, -10});
  EXPECT_EQ(scrollable->OverscrollOffset().y(), -10);
  page_->nested_scroll_manager()->DragEnd(scrollable, {0, 0});
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kBounce);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kBounce);
  DoAnimation(1000);
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(scrollable->scroll_offset_, 0);
  EXPECT_EQ(scrollable->OverscrollOffset().y(), 0);
}

TEST_F_UI(NestedScrollableTest, ManagerStatusDragWithBounceDisabled) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable);

  page_->nested_scroll_manager()->DragStart(scrollable);
  page_->nested_scroll_manager()->DragUpdate(scrollable, {0, -10});
  EXPECT_EQ(scrollable->scroll_offset_, 0);
  EXPECT_EQ(scrollable->OverscrollOffset().y(), 0);
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kDragging);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kDragging);
  page_->nested_scroll_manager()->DragEnd(scrollable, {0, 0});
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
}

TEST_F_UI(NestedScrollableTest, ManagerStatusFlingWithBounce) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  scrollable->SetOverscrollEnabled(true);
  page_->AddChild(scrollable);

  page_->nested_scroll_manager()->DragStart(scrollable);
  page_->nested_scroll_manager()->DragUpdate(scrollable, {0, 30});
  page_->nested_scroll_manager()->DragEnd(scrollable, {0, -1000});
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kFling);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kFling);
  bool bounced = false;
  for (int i = 0; i < 200; i++) {
    DoAnimation(5);
    if (scrollable->nested_scroll_manager()->GetScrollStatus() ==
        Scrollable::ScrollStatus::kBounce) {
      bounced = true;
    }
  }
  EXPECT_TRUE(bounced);
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(scrollable->scroll_offset_, 0);
  EXPECT_EQ(scrollable->OverscrollOffset().y(), 0);
}

TEST_F_UI(NestedScrollableTest, ManagerStatusFlingWithBounceDisabled) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable);

  page_->nested_scroll_manager()->DragStart(scrollable);
  page_->nested_scroll_manager()->DragUpdate(scrollable, {0, 30});
  page_->nested_scroll_manager()->DragEnd(scrollable, {0, -1000});
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kFling);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kFling);
  for (int i = 0; i < 200; i++) {
    DoAnimation(5);
    if (scrollable->scroll_offset_ == 0) {
      break;
    }
  }
  EXPECT_EQ(scrollable->scroll_offset_, 0);
  // Verify immediate transition to idle status upon reaching the boundary
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
}

TEST_F_UI(NestedScrollableTest, ManagerStatusTouchOnFling) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  page_->AddChild(scrollable);

  page_->nested_scroll_manager()->DragStart(scrollable);
  page_->nested_scroll_manager()->DragEnd(scrollable, {0, 500});
  DoAnimation(10);
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kFling);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kFling);
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kDownEvent, {50, 50})});
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kDragging);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kDragging);
}

TEST_F_UI(NestedScrollableTest, ManagerStatusTouchOnBounce) {
  TestScrollable* scrollable = new TestScrollable(page_.get(), 100);
  scrollable->SetOverscrollEnabled(true);
  page_->AddChild(scrollable);

  page_->nested_scroll_manager()->DragStart(scrollable);
  page_->nested_scroll_manager()->DragEnd(scrollable, {0, -1000});
  DoAnimation(10);
  // The fling animation should be changed to a bounce animation immediately.
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kBounce);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kBounce);
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kDownEvent, {50, 50})});
  EXPECT_EQ(scrollable->nested_scroll_manager()->GetScrollStatus(),
            Scrollable::ScrollStatus::kDragging);
  EXPECT_EQ(scrollable->GetScrollStatus(), Scrollable::ScrollStatus::kDragging);
}

TEST_F_UI(NestedScrollableTest, ScrollableStatusDrag) {
  TestScrollable* outer = new TestScrollable(page_.get(), 100);
  page_->AddChild(outer);
  TestScrollable* inner = new TestScrollable(page_.get(), 100);
  outer->AddChild(inner);

  page_->nested_scroll_manager()->DragStart(inner);
  EXPECT_EQ(inner->GetScrollStatus(), Scrollable::ScrollStatus::kDragging);
  EXPECT_EQ(outer->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  page_->nested_scroll_manager()->DragUpdate(inner, {0, 50});
  EXPECT_EQ(inner->GetScrollStatus(), Scrollable::ScrollStatus::kDragging);
  EXPECT_EQ(outer->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  page_->nested_scroll_manager()->DragUpdate(inner, {0, 60});
  // The drag status has been transferred to the outer scrollable
  EXPECT_EQ(inner->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(outer->GetScrollStatus(), Scrollable::ScrollStatus::kDragging);
  page_->nested_scroll_manager()->DragEnd(inner, {0, 0});
  EXPECT_EQ(outer->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
}

TEST_F_UI(NestedScrollableTest, ScrollableStatusFling) {
  TestScrollable* outer = new TestScrollable(page_.get(), 100);
  page_->AddChild(outer);
  TestScrollable* inner = new TestScrollable(page_.get(), 100);
  outer->AddChild(inner);

  page_->nested_scroll_manager()->DragStart(inner);
  EXPECT_EQ(inner->GetScrollStatus(), Scrollable::ScrollStatus::kDragging);
  EXPECT_EQ(outer->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  page_->nested_scroll_manager()->DragUpdate(inner, {0, 99});
  EXPECT_EQ(inner->GetScrollStatus(), Scrollable::ScrollStatus::kDragging);
  EXPECT_EQ(outer->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  page_->nested_scroll_manager()->DragEnd(inner, {0, 2000});
  EXPECT_EQ(inner->GetScrollStatus(), Scrollable::ScrollStatus::kFling);
  EXPECT_EQ(outer->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  DoAnimation(10);
  // The fling status has been transferred to the outer scrollable
  EXPECT_EQ(inner->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  EXPECT_EQ(outer->GetScrollStatus(), Scrollable::ScrollStatus::kFling);
  DoAnimation(1000);
  EXPECT_EQ(outer->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
}

}  // namespace clay
