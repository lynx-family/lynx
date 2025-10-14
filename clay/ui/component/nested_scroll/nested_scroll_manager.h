// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_NESTED_SCROLL_NESTED_SCROLL_MANAGER_H_
#define CLAY_UI_COMPONENT_NESTED_SCROLL_NESTED_SCROLL_MANAGER_H_

#include <memory>
#include <tuple>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/gfx/animation/fling_animator.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/scroll_direction.h"
#include "clay/ui/component/nested_scroll/nested_scrollable.h"
#include "clay/ui/component/nested_scroll/raster_fling_manager.h"

namespace clay {

class PageView;

// NestedScrollManager handles the scroll behavior of nested scrollable views.
class NestedScrollManager : public DynamicAnimator::AnimationListener {
 public:
  explicit NestedScrollManager(PageView* page_view);

  void DragStart(NestedScrollable* source_scrollable);
  void DragUpdate(NestedScrollable* source_scrollable, FloatPoint delta);
  void DragEnd(NestedScrollable* source_scrollable, FloatSize velocity);

  NestedScrollable* FindOuterScrollable(NestedScrollable* scrollable);

  void OnScrollableScroll(NestedScrollable* scrollable);
  void OnScrollableBounceEnd(NestedScrollable* scrollable);

  void CancelAnimations();

  float GetFlingVelocity() const {
    if (fling_animator_ && fling_animator_->IsRunning()) {
      return fling_animator_->GetCurrentVelocity();
    } else {
      return 0.f;
    }
  }

  Scrollable::ScrollStatus GetScrollStatus() const { return status_; }

  // Return true if raster fling animation is enabled.
  // TODO: will be removed once smooth scroll is stable.
  bool IsSmoothScrollEnabled() const {
#if OS_IOS || OS_ANDROID
    // Disable it on iOS for better bounce effect. If the compositor animation
    // is enabled, frame drops occur when scrolling to the container's edge and
    // switching to the bounce animation.
    return false;
#else
    return true;
#endif
  }

  RasterFlingManager* raster_fling_manager() const {
    return raster_fling_manager_.get();
  }

 private:
  void ConstructScrollableChain(NestedScrollable* scrollable);
  std::tuple<FloatPoint, NestedScrollable*> DispatchScroll(FloatPoint delta,
                                                           bool is_dragging);
  bool ShouldTriggerFling(float velocity);
  void StartFling(float velocity, bool try_raster_fling = false);
  bool TryStartBounce(NestedScrollable* target, float velocity);

  // DynamicAnimator::AnimationListener
  void OnDynamicAnimationUpdate(DynamicAnimator& animation, float value,
                                float velocity) override;
  void OnDynamicAnimationEnd(DynamicAnimator& animation, bool canceled,
                             float value, float velocity) override;

  void SetScrollStatus(Scrollable::ScrollStatus status);

  bool IsFlingRunning() const {
    return fling_animator_ && fling_animator_->IsRunning();
  }

  NestedScrollable* FindOverscrollView() const;

  // Remove invalid WeakPtr from scrollable_chain_.
  void CleanUpScrollableChain();

  void OnFlingEndFromRaster(NestedScrollable* scrollable,
                            float remaining_velocity);
  RasterFlingManager* GetOrCreateRasterFlingManager();

  std::vector<fml::WeakPtr<NestedScrollable>> scrollable_chain_;
  fml::WeakPtr<NestedScrollable> current_scrollable_;
  NestedScrollable* source_scrollable_ = nullptr;
  ScrollDirection direction_ = ScrollDirection::kNone;
  PageView* page_view_;
  std::unique_ptr<FlingAnimator> fling_animator_;
  Scrollable::ScrollStatus status_ = NestedScrollable::ScrollStatus::kIdle;

  std::unique_ptr<RasterFlingManager> raster_fling_manager_;
  friend class RasterFlingManager;

  FRIEND_TEST(NestedScrollableTest, SimpleScroll);
  FRIEND_TEST(NestedScrollableTest, Fling);
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_NESTED_SCROLL_NESTED_SCROLL_MANAGER_H_
