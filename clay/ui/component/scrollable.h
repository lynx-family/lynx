// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLLABLE_H_
#define CLAY_UI_COMPONENT_SCROLLABLE_H_

#include <memory>
#include <string>

#include "clay/gfx/animation/bounce_animator.h"
#include "clay/gfx/scroll_direction.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/nested_scroll/overscroll_effect.h"

namespace clay {
class Scrollable : public WithTypeInfo<Scrollable, BaseView>,
                   public DynamicAnimator::AnimationListener {
 public:
  class Listener {
   public:
    virtual void OnScrollableScrolled() = 0;
  };

  enum class ScrollStatus {
    // The scrollable is still
    kIdle,
    // The scrollable is dragging by user's finger
    kDragging,
    // The scrollable is in fling animation
    kFling,
    // The scrollable is in bounce animation
    kBounce,
    // The scrollable is in other animations, such as smooth scrolling triggered
    // by the `scrollTo` UI method
    kAnimating,
  };

  Scrollable(int id, std::string tag,
             std::unique_ptr<RenderScroll> render_object, PageView* page_view,
             ScrollDirection direction);

  virtual void SetScrollEnabled(bool enabled) { scroll_enabled_ = enabled; }
  bool IsScrollEnabled() const { return scroll_enabled_; }
  void SetUpperOverscrollEnabled(bool enabled) {
    upper_overscroll_enabled_ = enabled;
  }
  void SetLowerOverscrollEnabled(bool enabled) {
    lower_overscroll_enabled_ = enabled;
  }
  void SetOverscrollEnabled(bool enabled) {
    SetUpperOverscrollEnabled(enabled);
    SetLowerOverscrollEnabled(enabled);
  }
  bool IsOverscrollEnabled(bool is_lower) const {
    return is_lower ? lower_overscroll_enabled_ : upper_overscroll_enabled_;
  }
  void AddScrollListener(Listener* listener);
  void RemoveScrollListener(Listener* listener);
  void RemoveAllScrollListener();
  FloatSize GetScrollOffset() const override { return scroll_offset_; }

  bool IsUnderOverscroll() const;
  FloatPoint DoOverscroll(FloatPoint delta);

  void SetOverscrollOffset(FloatPoint offset);
  void SetClampedOverscrollOffset(FloatPoint offset);
  FloatPoint OverscrollOffset() const { return overscroll_offset_; }
  FloatPoint ClampedOverscrollOffset() const {
    return clamped_overscroll_offset_;
  }
  RenderScroll* GetRenderScroll() const {
    return static_cast<RenderScroll*>(render_object());
  }

  virtual void OnOverscroll(FloatPoint prev_overscroll_offset) {}

  bool DoBounce(float velocity);

  // Stop scroll animations
  virtual void StopAnimation();

  ScrollStatus GetScrollStatus() const { return status_; }

  virtual void ResetScroll() {}
  virtual void ResetGestureRecognizers() {}

  void SetScrollDirection(ScrollDirection direction);
  ScrollDirection GetScrollDirection() const;

  ScrollableDirection GetScrollableDirection() const override {
    if (!IsScrollEnabled()) {
      return ScrollableDirection::kNone;
    }
    return GetRenderScroll()->GetScrollableDirection();
  }

 protected:
  void SetScrollStatus(ScrollStatus status);

  virtual void OnScrollStatusChange(ScrollStatus old_status) {}
  virtual void OnBounceEnd(bool canceled) {}

  void NotifyScrolled();

  FloatSize scroll_offset_;
  ScrollDirection scroll_direction_ = ScrollDirection::kVertical;
  ScrollStatus status_ = Scrollable::ScrollStatus::kIdle;

  // DynamicAnimator::AnimationListener
  void OnDynamicAnimationUpdate(DynamicAnimator& animation, float value,
                                float velocity) override;
  void OnDynamicAnimationEnd(DynamicAnimator& animation, bool canceled,
                             float value, float velocity) override;

 private:
  std::forward_list<Listener*> listeners_;
  bool scroll_enabled_ = true;
  bool upper_overscroll_enabled_ = false;
  bool lower_overscroll_enabled_ = false;
  FloatPoint overscroll_offset_ = {0, 0};

  // The difference between overscroll_offset_ and clamped_overscroll_offset_ is
  // that overscroll_offset_ is used on dragging, and clamped_overscroll_offset_
  // is used on bouncing animation. For dragging, rubber band effect is applied
  // to the dragged offset. For bouncing animation, we directly animate the
  // clamped_overscroll_offset_ to avoid the rubber band effect being applied.
  FloatPoint clamped_overscroll_offset_ = {0, 0};
  std::unique_ptr<OverscrollEffect> overscroll_effect_;

  std::unique_ptr<BounceAnimator> bounce_animator_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SCROLLABLE_H_
