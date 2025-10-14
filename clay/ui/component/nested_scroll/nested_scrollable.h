// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_NESTED_SCROLL_NESTED_SCROLLABLE_H_
#define CLAY_UI_COMPONENT_NESTED_SCROLL_NESTED_SCROLLABLE_H_

#include <memory>
#include <string>
#include <tuple>

#include "clay/gfx/animation/value_animator.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/scrollable.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

class NestedScrollManager;
class BaseView;
class PageView;

enum class NestedScrollMode {
  kSelfOnly,
  kSelfFirst,
  kParentFirst,
};

class NestedScrollable : public WithTypeInfo<NestedScrollable, Scrollable>,
                         public AnimatorUpdateListener,
                         public GestureRecognizer::Delegate {
 public:
  NestedScrollable(int id, std::string tag,
                   std::unique_ptr<RenderScroll> render_object,
                   PageView* page_view, ScrollDirection direction,
                   bool enable_nested_scroll = false);
  ~NestedScrollable() override;

  void SetScrollEnabled(bool enabled) override;

  // TODO(liuguoliang.lgl): Consider moving this to NestedScrollManager.
  /**
   * Handle the scroll distance along the scrollable chain.
   * By default, the scroll offset will be passed from the innermost scrollable
   * view to outer.
   *
   * @param delta: the scroll distance that needs to be consumed
   * @param is_dragging is this scroll distance resulting from dragging
   * @return unconsumed distance and the last scrollable that consumes scroll
   */
  std::tuple<FloatPoint, NestedScrollable*> HandleNestedScroll(
      FloatPoint delta, bool is_dragging = true);

  // The scrollable has been scrolled.
  // Note that you should call this method if you override the DoScroll.
  virtual void DidScroll() { NotifyScrolled(); }

  // Capture the scroll offset of its descendants. By default, the scroll offset
  // is passed from child to parent, the inner scrollable will consume the
  // scroll offset first. But sometimes (like the fold view) we want the outer
  // scrollable to consume the scroll offset first, regardless of the descendant
  // scrollables.
  virtual bool CaptureScroll(FloatPoint delta) { return false; }

  /*
   * Scrolling should be handled by current view.
   *
   * @param delta: scroll by delta offset
   * @ret: unconsumed distance, unconsumed equals delta minus consumed_delta
   */
  virtual FloatPoint DoScroll(FloatPoint delta, bool by_user_input = true,
                              bool ignore_repaint = false);

  void DoWheelScroll(FloatPoint delta);

  NestedScrollable* FindAncestorNestedDraggableView();
  NestedScrollManager* nested_scroll_manager() const;
  bool IsEnableNestedScroll() const { return enable_nested_scroll_; }

  void SetEnableNestedScroll(bool enable_nested_scroll) {
    enable_nested_scroll_ = enable_nested_scroll;
  }

  NestedScrollable* FindBeginningScrollable();
  NestedScrollable* FindNextScrollable();

  virtual NestedScrollable* NextScrollable() { return next_scrollable_.get(); }
  virtual NestedScrollable* PreviousScrollable() {
    return pre_scrollable_.get();
  }

  void HandleEvent(const PointerEvent& event) override;

  bool CanScrollBy(float delta_x, float delta_y) const;

  bool CanScrollX() const {
    return static_cast<int>(scroll_direction_) &
           static_cast<int>(ScrollDirection::kHorizontal);
  }
  bool CanScrollY() const {
    return static_cast<int>(scroll_direction_) &
           static_cast<int>(ScrollDirection::kVertical);
  }

  void SetNextScrollable(NestedScrollable* scrollable) {
    next_scrollable_.reset();
    if (scrollable) {
      next_scrollable_ = scrollable->GetWeakPtr();
    }
  }
  void SetPreScrollable(NestedScrollable* scrollable) {
    pre_scrollable_.reset();
    if (scrollable) {
      pre_scrollable_ = scrollable->GetWeakPtr();
    }
  }

  void SetForwardMode(NestedScrollMode mode) { scroll_forward_mode_ = mode; }

  void SetBackwardMode(NestedScrollMode mode) { scroll_backward_mode_ = mode; }

  // For now this is only used for unit tests.
  void SetTouchSlop(float slop);

  void SetResolveDragImmediately(bool resolve_drag_immediately);

  bool IsRtlDirection() const {
    auto layout_direction = GetRenderScroll()->GetLayoutDirection();
    return layout_direction == DirectionType::kLynxRtl ||
           layout_direction == DirectionType::kRtl;
  }

  static NestedScrollable* GetScrollable(BaseView* view);

  bool IsPointerAllowed(const GestureRecognizer& gesture_recognizer,
                        const PointerEvent& event) override;

 protected:
  // check can scroll in direction
  virtual bool CanDragScrollOnX() const;
  virtual bool CanDragScrollOnY() const;

  void ResetGestureRecognizers() override;

  float GetScrollTop() const {
    RenderBox* box = static_cast<RenderBox*>(render_object_.get());
    return box->ScrollTop();
  }
  float GetScrollLeft() const {
    RenderBox* box = static_cast<RenderBox*>(render_object_.get());
    return box->ScrollLeft();
  }
  void OnOverscroll(FloatPoint prev_overscroll_offset) override;

  void OnBounceEnd(bool canceled) override;

  void OnAnimationUpdate(ValueAnimator& animation) override;

  double CalculateVelocity(float time);

  float last_drag_end_velocity_ = 0.f;  // pixels/second

  DragGestureRecognizer* drag_recognizer_ = nullptr;

  NestedScrollMode scroll_forward_mode_ = NestedScrollMode::kSelfFirst;
  NestedScrollMode scroll_backward_mode_ = NestedScrollMode::kSelfFirst;
  fml::WeakPtr<NestedScrollable> next_scrollable_;
  fml::WeakPtr<NestedScrollable> pre_scrollable_;

 private:
  FRIEND_TEST(ScrollViewTest, NestedScrollGestureOnPC);
  FRIEND_TEST(ScrollViewTest, ScrollEvent);
  void UpdateTouchSlop();

  void UpdateWheelAnimation(FloatPoint delta);
  void DispatchMouseWheelEvent(const PointerEvent& event);

  bool enable_nested_scroll_ = false;
  float touch_slop_ = kTouchSlop;
  // We ensure to resolve the drag gesture immediately. This results in a better
  // user experience and reasonable state transition.
  //   1. Avoid triggering the tap gesture when the scrollable is scrolling.
  //   2. Immediately resolve the drag gesture when the scrollable is scrolling,
  //   so that there is no delay for scrolling again.
  //   3. Immediately transition to drag state without a period of idle state,
  //   which is weird.
  bool resolve_drag_immediately_ = false;

  BaseView* parent_scroll_view_ = nullptr;

  friend NestedScrollManager;

  std::unique_ptr<ValueAnimator> wheel_animator_;
  // unittests only
  bool need_scroll_animation_ = true;
  // Wheel animator related variables
  FloatPoint target_scroll_delta_;
  FloatPoint last_target_scroll_delta_;
  float last_scroll_delta_ = 0.f;
  float last_retarget_ = 0.f;

  // indicate current scroll-view to handle scroll event, instead of dispatching
  // to parent
  bool signal_scroll_started_ = false;
  // indicate current scroll-view to handle signal event
  bool handle_signal_event_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_NESTED_SCROLL_NESTED_SCROLLABLE_H_
