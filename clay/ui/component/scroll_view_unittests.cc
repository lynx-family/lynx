// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define CLAY_UNIT_TESTS 1

#include <algorithm>
#include <memory>

#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/scroll_wrapper.h"
#include "clay/ui/component/view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/rendering/render_scroll.h"
#include "clay/ui/testing/ui_test.h"

namespace clay {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

class ScrollViewTest : public UITest {
 protected:
  void UISetUp() override {
    outer_view_ = std::make_unique<ScrollView>(-1, ScrollDirection::kVertical,
                                               page_.get());
    page_->AddChild(outer_view_.get());
    inner_view_ = std::make_unique<ScrollView>(-1, ScrollDirection::kHorizontal,
                                               page_.get());
    outer_view_->AddChild(inner_view_.get(), 0);
    inner_view_->SetBound(0, 0, 300, 300);
    inner_view_->SetEnableNestedScroll(true);
    outer_view_->SetBound(0, 0, 100, 100);
    outer_view_->SetEnableNestedScroll(true);
    box_view_ =
        std::make_unique<ScrollView>(-1, ScrollDirection::kNone, page_.get());
    box_view_->SetScrollEnabled(false);
    box_view_->SetBound(0, 0, 600, 600);
    inner_view_->AddChild(box_view_.get(), 0);
  }

  void UITearDown() override {
    outer_view_.reset();
    inner_view_.reset();
    box_view_.reset();
  }

  std::unique_ptr<ScrollView> outer_view_;
  std::unique_ptr<ScrollView> inner_view_;
  std::unique_ptr<ScrollView> box_view_;
};

TEST_F_UI(ScrollViewTest, ScrollToId) {
  std::unique_ptr<View> view_a = std::make_unique<View>(-1, page_.get());
  view_a->SetBound(0, 0, 100, 100);
  std::unique_ptr<View> view_b = std::make_unique<View>(-1, page_.get());
  view_b->SetBound(0, 100, 100, 100);
  view_b->SetComponentName("view_b");
  std::unique_ptr<View> view_c = std::make_unique<View>(-1, page_.get());
  view_c->SetBound(0, 200, 100, 100);
  view_c->SetComponentName("view_c");
  outer_view_->AddChild(view_a.get(), 1);
  outer_view_->AddChild(view_b.get(), 2);
  outer_view_->AddChild(view_c.get(), 3);
  outer_view_->OnLayoutUpdated();
  outer_view_->SetScrollToId("view_c", false);
  EXPECT_EQ(200, outer_view_->GetScrollOffset().height());
  view_a.reset();
  view_b.reset();
  view_c.reset();
}

TEST_F_UI(ScrollViewTest, NestedScrollGestureOnPC) {
  outer_view_->need_scroll_animation_ = false;
  inner_view_->need_scroll_animation_ = false;
  outer_view_->OnLayoutUpdated();
  inner_view_->OnLayoutUpdated();
  auto inner_offset = inner_view_->GetScrollOffset();
  auto outer_offset = outer_view_->GetScrollOffset();
  // the main movement on y axis, so outer will moved
  EXPECT_EQ(0.0, inner_offset.width());
  EXPECT_EQ(0.0, outer_offset.height());
  inner_view_->HandleEvent(PointerEvent(PointerEvent::EventType::kSignalEvent));
  outer_view_->HandleEvent(PointerEvent(PointerEvent::EventType::kSignalEvent));
  DispatchTouchPadEvent({0, 0}, {50, 100}, 5);

  inner_offset = inner_view_->GetScrollOffset();
  outer_offset = outer_view_->GetScrollOffset();
  EXPECT_EQ(0.0, inner_offset.width());
  EXPECT_EQ(100.0, outer_offset.height());

  // move within 500ms continue with prev move
  DispatchTouchPadEvent({0, 0}, {50, 10}, 5);
  inner_offset = inner_view_->GetScrollOffset();
  outer_offset = outer_view_->GetScrollOffset();
  EXPECT_EQ(0.0, inner_offset.width());
  EXPECT_EQ(110.0, outer_offset.height());

  // simulate 500ms passed and start a new scroll.
  page_->gesture_manager()->EndMouseWheelTransactionByForce();
  DispatchTouchPadEvent({0, 0}, {50, 10}, 5);
  inner_offset = inner_view_->GetScrollOffset();
  outer_offset = outer_view_->GetScrollOffset();
  EXPECT_EQ(50.0, inner_offset.width());
  EXPECT_EQ(110.0, outer_offset.height());
}

TEST_F_UI(ScrollViewTest, ScrollIntoView) {
  auto scroll_view =
      std::make_unique<ScrollView>(-1, ScrollDirection::kVertical, page_.get());
  scroll_view->SetBound(0, 0, 300, 100);
  scroll_view->SetScrollDirection(ScrollDirection::kHorizontal);
  scroll_view->SetPaddings(50, 0, 50, 0);

  auto view1 = std::make_unique<View>(-1, page_.get());
  view1->SetBound(50, 0, 500, 100);
  scroll_view->AddChild(view1.get(), 0);

  auto view2 = std::make_unique<View>(-1, page_.get());
  view2->SetBound(450, 0, 100, 100);
  scroll_view->AddChild(view2.get(), 0);

  scroll_view->OnLayoutUpdated();

  auto* render_scroll =
      static_cast<RenderScroll*>(scroll_view->render_object());
  EXPECT_EQ(render_scroll->MaxScrollWidth(), 300);
  EXPECT_EQ(view1->BoundsRelativeTo(scroll_view.get()).location().x(), 50);
  scroll_view->StartScrollInto(view1.get(), "", "start", "");
  EXPECT_EQ(view1->BoundsRelativeTo(scroll_view.get()).location().x(), 0);
  scroll_view->StartScrollInto(view1.get(), "", "end", "");
  EXPECT_EQ(view1->BoundsRelativeTo(scroll_view.get()).location().x(), -200);
  scroll_view->StartScrollInto(view1.get(), "", "center", "");
  EXPECT_EQ(view1->BoundsRelativeTo(scroll_view.get()).location().x(), -100);

  scroll_view->StartScrollInto(view2.get(), "", "end", "");
  EXPECT_EQ(view2->BoundsRelativeTo(scroll_view.get()).location().x(), 200);
  scroll_view->StartScrollInto(view2.get(), "", "center", "");
  // This view is not centered because of reaching the max scroll width.
  EXPECT_EQ(view2->BoundsRelativeTo(scroll_view.get()).location().x(), 150);
}

TEST_F_UI(ScrollViewTest, AppendChild) {
  auto scroll_view = std::make_unique<ScrollWrapper>(
      -1, ScrollDirection::kVertical, page_.get());

  auto view0 = std::make_unique<View>(-1, page_.get());
  scroll_view->AddChild(view0.get());

  auto view1 = std::make_unique<View>(-1, page_.get());
  scroll_view->AddChild(view1.get());

  auto view2 = std::make_unique<View>(-1, page_.get());
  scroll_view->AddChild(view2.get());

  auto inner = scroll_view->GetChildren()[0];
  EXPECT_EQ(inner->GetChildren()[0], view0.get());
  EXPECT_EQ(inner->GetChildren()[1], view1.get());
  EXPECT_EQ(inner->GetChildren()[2], view2.get());
}

TEST_F_UI(ScrollViewTest, ScrollTop) {
  auto scroll_view = std::make_unique<ScrollView>(
      -1, ScrollDirection::kHorizontal, page_.get());
  scroll_view->SetBound(0, 0, 300, 100);
  page_->AddChild(scroll_view.get());

  auto view0 = std::make_unique<View>(-1, page_.get());
  view0->SetBound(0, 0, 500, 100);
  scroll_view->AddChild(view0.get(), 0);

  auto view1 = std::make_unique<View>(-1, page_.get());
  view1->SetBound(0, 100, 500, 100);
  scroll_view->AddChild(view1.get(), 1);

  auto view2 = std::make_unique<View>(-1, page_.get());
  view2->SetBound(0, 200, 500, 100);
  scroll_view->AddChild(view2.get(), 2);

  scroll_view->SetAttribute("scroll-top", clay::Value(100));
  scroll_view->SetAttribute("scroll-x", clay::Value(false));
  scroll_view->SetAttribute("scroll-y", clay::Value(true));

  scroll_view->OnLayoutUpdated();
  Layout();

  EXPECT_EQ(100, scroll_view->GetScrollOffset().height());
}

TEST_F_UI(ScrollViewTest, ScrollToIndex) {
  auto scroll_view = std::make_unique<ScrollView>(
      -1, ScrollDirection::kHorizontal, page_.get());
  scroll_view->SetBound(0, 0, 300, 100);
  page_->AddChild(scroll_view.get());

  auto view0 = std::make_unique<View>(-1, page_.get());
  view0->SetBound(0, 0, 500, 100);
  scroll_view->AddChild(view0.get(), 0);

  auto view1 = std::make_unique<View>(-1, page_.get());
  view1->SetBound(0, 100, 500, 100);
  scroll_view->AddChild(view1.get(), 1);

  auto view2 = std::make_unique<View>(-1, page_.get());
  view2->SetBound(0, 200, 500, 100);
  scroll_view->AddChild(view2.get(), 2);

  scroll_view->SetAttribute("scroll-to-index", clay::Value(2));
  scroll_view->SetAttribute("scroll-x", clay::Value(false));
  scroll_view->SetAttribute("scroll-y", clay::Value(true));

  scroll_view->OnLayoutUpdated();
  Layout();

  EXPECT_EQ(200, scroll_view->GetScrollOffset().height());
}

// TODO(liuguoliang): Fix scrollWidth/scrollHeight and add test case
TEST_F_UI(ScrollViewTest, ScrollEvent) {
  auto scroll_view = std::make_unique<ScrollWrapper>(
      -1, ScrollDirection::kVertical, page_.get());
  scroll_view->GetScrollView()->need_scroll_animation_ = false;
  scroll_view->SetBound(0, 0, 100, 100);
  scroll_view->SetAttribute("scroll-y", Value(true));
  scroll_view->GetScrollView()->SetOverscrollEnabled(true);
  scroll_view->AddEventCallback(event_attr::kEventScroll);
  scroll_view->AddEventCallback(event_attr::kEventScrollEnd);
  page_->AddChild(scroll_view.get());
  auto content_view = std::make_unique<View>(-1, page_.get());
  content_view->SetBound(0, 0, 100, 500);
  scroll_view->AddChild(content_view.get());

  // By attribute
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(
      *this,
      OnCustomEvent(
          event_attr::kEventScroll,
          UnorderedElementsAre(
              Pair("deltaX", Value(0.f)), Pair("deltaY", Value(200.f)),
              Pair("scrollLeft", Value(0.f)), Pair("scrollTop", Value(200.f)),
              Pair("scrollWidth", Value(100.f)), Pair("scrollHeight", _),
              Pair("isDragging", _))))
      .Times(1);

  scroll_view->SetAttribute("scroll-top", Value(200));

  // By dragging
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScroll, _))
      .Times(::testing::AtLeast(5));
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollEnd, _)).Times(1);
  DispatchDragEvent({50, 50}, {50, 100}, false);

  // By fling
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScroll, _))
      .Times(::testing::AtLeast(5));
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollEnd, _)).Times(0);
  DispatchDragEvent({50, 50}, {50, 100}, true);
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScroll, _))
      .Times(AtLeast(2));
  EXPECT_CALL(
      *this,
      OnCustomEvent(event_attr::kEventScrollEnd,
                    UnorderedElementsAre(
                        Pair("deltaX", _), Pair("deltaY", _),
                        Pair("scrollLeft", Value(0.f)), Pair("scrollTop", _),
                        Pair("scrollWidth", Value(100.f)),
                        Pair("scrollHeight", _), Pair("isDragging", _))))
      .Times(1);
  DoAnimation(10);
  DoAnimation(1000);
  EXPECT_GT(scroll_view->GetScrollView()->GetScrollOffset().height(), 0);

  // By bounce
  ::testing::Mock::VerifyAndClearExpectations(this);
  scroll_view->SetAttribute("scroll-top", Value(0));
  EXPECT_EQ(scroll_view->GetScrollView()->GetScrollOffset().height(), 0);
  DispatchDragEvent({50, 50}, {50, 100});
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScroll, _))
      .Times(AtLeast(2));
  EXPECT_CALL(
      *this, OnCustomEvent(event_attr::kEventScrollEnd,
                           UnorderedElementsAre(
                               Pair("deltaX", _), Pair("deltaY", _),
                               Pair("scrollLeft", Value(0.f)),
                               Pair("scrollTop", Value(0.f)),
                               Pair("scrollWidth", Value(100.f)),
                               Pair("scrollHeight", _), Pair("isDragging", _))))
      .Times(1);
  ResetAnimationTime();
  DoAnimation(10);
  DoAnimation(1000);
  EXPECT_EQ(scroll_view->GetScrollView()->GetScrollOffset().height(), 0);

  // By fling and bounce
  ::testing::Mock::VerifyAndClearExpectations(this);
  DispatchDragEvent({50, 50}, {50, -200}, true, 10, 1);
  EXPECT_GT(scroll_view->GetScrollView()->GetScrollOffset().height(), 200);
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScroll, _))
      .Times(AtLeast(2));
  EXPECT_CALL(
      *this, OnCustomEvent(event_attr::kEventScrollEnd,
                           UnorderedElementsAre(
                               Pair("deltaX", _), Pair("deltaY", _),
                               Pair("scrollLeft", Value(0.f)),
                               Pair("scrollTop", Value(400.f)),
                               Pair("scrollWidth", Value(100.f)),
                               Pair("scrollHeight", _), Pair("isDragging", _))))
      .Times(1);
  ResetAnimationTime();
  DoAnimation(10);
  DoAnimation(1000);
  DoAnimation(1000);
  EXPECT_EQ(scroll_view->GetScrollView()->GetScrollOffset().height(), 400);

  // By signal
  ::testing::Mock::VerifyAndClearExpectations(this);
  scroll_view->SetAttribute("scroll-top", Value(0));
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScroll, _)).Times(5);
  EXPECT_CALL(
      *this, OnCustomEvent(event_attr::kEventScrollEnd,
                           UnorderedElementsAre(
                               Pair("deltaX", _), Pair("deltaY", _),
                               Pair("scrollLeft", Value(0.f)),
                               Pair("scrollTop", Value(50.f)),
                               Pair("scrollWidth", Value(100.f)),
                               Pair("scrollHeight", _), Pair("isDragging", _))))
      .Times(1);
  DispatchTouchPadEvent({50, 50}, {50, 100}, 5);
  // Ensure to immediately fire the end signal event for `scrollend`.
  page_->gesture_manager()->EndMouseWheelTransactionByForce();

  // By scrollTo (smooth: false)
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(
      *this,
      OnCustomEvent(
          event_attr::kEventScroll,
          UnorderedElementsAre(
              Pair("deltaX", Value(0.f)), Pair("deltaY", Value(-50.f)),
              Pair("scrollLeft", Value(0.f)), Pair("scrollTop", Value(0.f)),
              Pair("scrollWidth", Value(100.f)), Pair("scrollHeight", _),
              Pair("isDragging", _))))
      .Times(1);
  scroll_view->scrollTo(CreateLynxModuleValues({"offset"}, {Value(0)}));

  // By scrollTo (smooth: true)
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScroll, _))
      .Times(AtLeast(2));
  EXPECT_CALL(
      *this, OnCustomEvent(event_attr::kEventScrollEnd,
                           UnorderedElementsAre(
                               Pair("deltaX", _), Pair("deltaY", _),
                               Pair("scrollLeft", Value(0.f)),
                               Pair("scrollTop", Value(100.f)),
                               Pair("scrollWidth", Value(100.f)),
                               Pair("scrollHeight", _), Pair("isDragging", _))))
      .Times(1);
  scroll_view->scrollTo(
      CreateLynxModuleValues({"offset", "smooth"}, {Value(100), Value(true)}));
  ResetAnimationTime();
  DoAnimation(10);
  DoAnimation(1000);
}

TEST_F_UI(ScrollViewTest, ScrollToUpperLowerEvent) {
  auto scroll_view = std::make_unique<ScrollWrapper>(
      -1, ScrollDirection::kVertical, page_.get());
  scroll_view->SetBound(0, 0, 100, 100);
  scroll_view->SetAttribute("scroll-y", Value(true));
  scroll_view->SetAttribute("lower-threshold", Value(50));
  scroll_view->SetAttribute("upper-threshold", Value(50));
  scroll_view->AddEventCallback(event_attr::kEventScrollToUpper);
  scroll_view->AddEventCallback(event_attr::kEventScrollToLower);
  page_->AddChild(scroll_view.get());
  auto content_view = std::make_unique<View>(-1, page_.get());
  content_view->SetBound(0, 0, 100, 500);
  scroll_view->AddChild(content_view.get());

  // To lower
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollToLower, _))
      .Times(1);
  scroll_view->SetAttribute("scroll-top", Value(360));
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollToLower, _))
      .Times(0);
  scroll_view->SetAttribute("scroll-top", Value(380));
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollToLower, _))
      .Times(1);
  scroll_view->SetAttribute("scroll-top", Value(340));
  scroll_view->SetAttribute("scroll-top", Value(380));

  // To upper
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollToUpper, _))
      .Times(1);
  scroll_view->SetAttribute("scroll-top", Value(40));
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollToUpper, _))
      .Times(0);
  scroll_view->SetAttribute("scroll-top", Value(20));
  ::testing::Mock::VerifyAndClearExpectations(this);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollToUpper, _))
      .Times(1);
  scroll_view->SetAttribute("scroll-top", Value(60));
  scroll_view->SetAttribute("scroll-top", Value(20));
}

TEST_F_UI(ScrollViewTest, BounceView) {
  auto scroll_view = std::make_unique<ScrollWrapper>(
      -1, ScrollDirection::kHorizontal, page_.get());
  scroll_view->SetBound(0, 0, 100, 100);
  scroll_view->SetAttribute("bounce", Value(true));
  scroll_view->AddEventCallback(event_attr::kEventScrollToBounce);

  auto content_view = std::make_unique<View>(-1, page_.get());
  content_view->SetBound(0, 0, 200, 100);
  scroll_view->AddChild(content_view.get());

  auto bounce_view = std::make_unique<BounceView>(-1, page_.get());
  bounce_view->SetBound(0, 0, 20, 20);
  bounce_view->SetAttribute("direction", Value("right"));
  scroll_view->AddChild(bounce_view.get());

  page_->AddChild(scroll_view.get());

  Layout();

  EXPECT_EQ(bounce_view->BoundsRelativeTo(nullptr).x(), 200);
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollToBounce, _))
      .Times(0);

  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kDownEvent, {50, 50})});
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kMoveEvent, {50, 50})});
  page_->DispatchPointerEvent({CreatePointer(
      0, PointerEvent::EventType::kMoveEvent, {-150, 50}, {-200, 0})});
  EXPECT_LT(bounce_view->BoundsRelativeTo(nullptr).x(), 80);
  EXPECT_GT(bounce_view->BoundsRelativeTo(nullptr).x(), 20);
  ::testing::Mock::VerifyAndClearExpectations(this);

  // scrolltobounce should be called after touch up
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollToBounce, _))
      .Times(1);
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kUpEvent, {-150, 50})});
  ::testing::Mock::VerifyAndClearExpectations(this);

  DoAnimation();
  DispatchDragEvent({50, 50}, {200, 50}, false);
  // Fling should not trigger scrolltobounce event
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScrollToBounce, _))
      .Times(0);

  DoAnimation();

  DispatchDragEvent({50, 50}, {-40, 50}, true, 5, 5);
  float max_scroll_offset = 0;
  ResetAnimationTime();
  for (int i = 0; i < 50; i++) {
    DoAnimation(16);
    max_scroll_offset =
        std::max(max_scroll_offset,
                 scroll_view->GetScrollView()->TotalScrollOffset().width());
  }
  EXPECT_GT(max_scroll_offset, 120);
  EXPECT_EQ(scroll_view->GetScrollView()->TotalScrollOffset().width(), 100);
}

}  // namespace clay
