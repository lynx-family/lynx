// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/nested_scroll/raster_fling_manager.h"

#include <memory>
#include <utility>

#include "clay/flow/animation/scroll_offset_animation.h"
#include "clay/fml/logging.h"
#include "clay/ui/component/nested_scroll/nested_scroll_manager.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/rendering/render_scroll.h"

namespace clay {

RasterFlingManager::RasterFlingManager(
    NestedScrollManager* nested_scroll_manager)
    : nested_scroll_manager_(nested_scroll_manager) {
  FML_DCHECK(nested_scroll_manager->IsSmoothScrollEnabled());
}

bool RasterFlingManager::StartAnimation(NestedScrollable* scrollable,
                                        float velocity) {
  // Stop the current raster fling animation if exists.
  StopAnimation();
  // Only support ScrollView now.
  if (!scrollable->Is<ScrollView>()) {
    FML_DLOG(INFO) << "only ScrollView support raster fling now.";
    return false;
  }
  if (scrollable->IsUnderOverscroll()) {
    // The raster fling animation is not supported when target scrollable is
    // under overscroll, fallback to fling on UI.
    FML_DLOG(INFO) << "the target scrollable is under overscroll.";
    return false;
  }
  RenderScroll* render_scroll =
      static_cast<RenderScroll*>(scrollable->render_object());
  if (!render_scroll->HasValidID()) {
    FML_DLOG(INFO) << "the target scrollable has no valid id.";
    return false;
  }

  current_scrollable_ = scrollable->GetWeakPtr();
  ScrollDirection direction = scrollable->GetScrollDirection();
  float start_value;
  if (direction == ScrollDirection::kVertical) {
    start_value = render_scroll->ScrollTop();
  } else {
    start_value = render_scroll->ScrollLeft();
  }
  auto animator = std::make_unique<FlingAnimator>();
  animator->SetFriction(1.0f);
  animator->SetDensity(
      nested_scroll_manager_->page_view_
          ->GetPixelRatio<kPixelTypeLogical, kPixelTypeClay>());
  animator->SetStartValue(start_value);
  animator->SetStartVelocity(velocity);
  // Construct the `ScrollOffsetAnimation` with fling animator and bind to
  // RenderScroll.
  auto scroll_offset_animation = std::make_unique<clay::ScrollOffsetAnimation>(
      current_session_id_, direction, std::move(animator));
  render_scroll->StartRasterFling(std::move(scroll_offset_animation));
  return true;
}

void RasterFlingManager::StopAnimation() {
  current_session_id_++;
  if (current_scrollable_) {
    RenderScroll* render_scroll =
        static_cast<RenderScroll*>(current_scrollable_->render_object());
    // Mark fling animation end for RenderScroll;
    render_scroll->StopRasterFling();
    current_scrollable_.reset();
  }
}

void RasterFlingManager::OnAnimationEnd(NestedScrollable* scrollable,
                                        int32_t session_id, float value,
                                        float velocity) {
  RenderScroll* render_scroll =
      static_cast<RenderScroll*>(scrollable->render_object());
  // Mark fling animation end for RenderScroll;
  render_scroll->StopRasterFling();
  // Session dismatch, ignore the update opration.
  if (session_id != current_session_id_) {
    return;
  }
  nested_scroll_manager_->OnFlingEndFromRaster(scrollable, velocity);
}

void RasterFlingManager::OnAnimationUpdate(NestedScrollable* scrollable,
                                           int32_t session_id,
                                           float scroll_offset,
                                           bool ignore_ui_repaint) {
  // Session dismatch, ignore the update opration.
  if (session_id != current_session_id_) {
    return;
  }
  // Do scroll and update the scroll offset of the ScrollView.
  ScrollView* scroll_view = static_cast<ScrollView*>(scrollable);
  // Should ignore repaint if `ignore_ui_repaint`.
  scroll_view->DoScrollFromRaster(scroll_offset, ignore_ui_repaint);
}

}  // namespace clay
