// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/nested_scroll/nested_scroll_manager.h"

#include "clay/fml/logging.h"
#include "clay/ui/component/nested_scroll/nested_scrollable.h"
#include "clay/ui/component/nested_scroll/raster_fling_manager.h"
#include "clay/ui/component/page_view.h"

namespace clay {

NestedScrollManager::NestedScrollManager(PageView* page_view)
    : page_view_(page_view) {}

void NestedScrollManager::CancelAnimations() {
  if (fling_animator_ && fling_animator_->IsRunning()) {
    fling_animator_->Cancel();
  }
  for (auto& view : scrollable_chain_) {
    if (view) {
      view->StopAnimation();
    }
  }
  if (raster_fling_manager_) {
    raster_fling_manager_->StopAnimation();
  }
}

void NestedScrollManager::DragStart(NestedScrollable* scrollable) {
  FML_DCHECK(scrollable && scrollable->Is<NestedScrollable>());

  // We reset the touch slop for all scrollables before creating the new
  // scrollable chain.
  for (auto& scrollable : scrollable_chain_) {
    if (scrollable) {
      scrollable->SetResolveDragImmediately(false);
      scrollable->SetScrollStatus(Scrollable::ScrollStatus::kIdle);
    }
  }

  source_scrollable_ = scrollable;
  direction_ = source_scrollable_->GetScrollDirection();
  ConstructScrollableChain(scrollable);
  SetScrollStatus(Scrollable::ScrollStatus::kDragging);

  CancelAnimations();
}

void NestedScrollManager::DragUpdate(NestedScrollable* source_scrollable,
                                     FloatPoint delta) {
  if (source_scrollable_ != source_scrollable) {
    FML_DCHECK(false) << "source_scrollable is not match";
    return;
  }

  DispatchScroll(delta, true);
  current_scrollable_->DecodeImagesRecursively();
}

void NestedScrollManager::DragEnd(NestedScrollable* source_scrollable,
                                  FloatSize velocity) {
  if (source_scrollable_ != source_scrollable) {
    // FML_DCHECK(false) << "source_scrollable is not match";
    return;
  }

  auto v = source_scrollable->GetScrollDirection() == ScrollDirection::kVertical
               ? velocity.height()
               : velocity.width();
  if (ShouldTriggerFling(v)) {
    StartFling(v, IsSmoothScrollEnabled());
  } else if (auto overscroll_view = FindOverscrollView()) {
    TryStartBounce(overscroll_view, v);
  } else {
    SetScrollStatus(Scrollable::ScrollStatus::kIdle);
  }
  current_scrollable_->DecodeImagesRecursively();
}

NestedScrollable* NestedScrollManager::FindOuterScrollable(
    NestedScrollable* scrollable) {
  auto it = std::find(scrollable_chain_.rbegin(), scrollable_chain_.rend(),
                      scrollable->GetWeakPtr());
  if (it == scrollable_chain_.rend() || it == scrollable_chain_.rbegin()) {
    return nullptr;
  } else {
    return (--it)->get();
  }
}

void NestedScrollManager::ConstructScrollableChain(
    NestedScrollable* scrollable) {
  scrollable = scrollable->FindBeginningScrollable();
  current_scrollable_ = scrollable->GetWeakPtr();
  scrollable_chain_.clear();
  scrollable_chain_.push_back(scrollable->GetWeakPtr());

  while (scrollable->IsEnableNestedScroll()) {
    scrollable = scrollable->FindNextScrollable();
    if (scrollable && scrollable->GetScrollDirection() == direction_ &&
        std::find(scrollable_chain_.begin(), scrollable_chain_.end(),
                  scrollable->GetWeakPtr()) == scrollable_chain_.end()) {
      scrollable_chain_.push_back(scrollable->GetWeakPtr());
    } else {
      break;
    }
  }
}

std::tuple<FloatPoint, NestedScrollable*> NestedScrollManager::DispatchScroll(
    FloatPoint delta, bool is_dragging) {
  CleanUpScrollableChain();
  if (scrollable_chain_.size() == 0 || delta.IsOrigin()) {
    return {delta, nullptr};
  }

  // Handle the overscroll first if there is a view under overscroll.
  FloatPoint unconsumed = delta;
  if (auto overscroll_view = FindOverscrollView()) {
    unconsumed = overscroll_view->DoOverscroll(unconsumed);
  }

  // Capture phase
  for (auto it = scrollable_chain_.rbegin(); it != scrollable_chain_.rend();
       it++) {
    auto scrollable = it->get();
    if (scrollable->IsScrollEnabled() &&
        scrollable->CaptureScroll(unconsumed)) {
      auto old_unconsumed = unconsumed;
      unconsumed = scrollable->DoScroll(unconsumed);
      if (unconsumed != old_unconsumed) {
        OnScrollableScroll(scrollable);
      }
    }
  }

  auto scrollable = scrollable_chain_[0];
  if (!unconsumed.IsOrigin()) {
    return scrollable->HandleNestedScroll(unconsumed, is_dragging);
  }

  return {unconsumed, scrollable.get()};
}

bool NestedScrollManager::ShouldTriggerFling(float velocity) {
  float velocity_threshold = page_view_->FromLogical(50);
  if (std::abs(velocity) < velocity_threshold) {
    return false;
  }

  // In the overscroll state, we will check the target position to determine
  // whether to trigger fling. If the target position exceeds the scroll
  // boundary, we trigger a bounce animation instead of a fling.
  if (auto overscroll_view = FindOverscrollView()) {
    float overscroll_offset =
        overscroll_view->GetScrollDirection() == ScrollDirection::kVertical
            ? overscroll_view->OverscrollOffset().y()
            : overscroll_view->OverscrollOffset().x();
    if (overscroll_offset * velocity > 0) {
      return false;
    }

    // Use animator to calculate the target distance.
    FlingAnimator animator{};
    animator.SetStartVelocity(velocity);
    animator.SetFriction(1.0f);
    animator.SetDensity(
        page_view_->GetPixelRatio<kPixelTypeLogical, kPixelTypeClay>());
    animator.FlingInitialize();
    animator.GetDistance();
    return std::abs(animator.GetDistance()) > std::abs(overscroll_offset);
  }

  return true;
}

void NestedScrollManager::StartFling(float velocity, bool try_raster_fling) {
  if (try_raster_fling) {
    FML_DCHECK(IsSmoothScrollEnabled());
    if (current_scrollable_.get() == source_scrollable_ &&
        GetOrCreateRasterFlingManager()->StartAnimation(source_scrollable_,
                                                        velocity)) {
      // Run fling on raster successfully.
      FML_DLOG(INFO) << "run fling on raster velocity=" << velocity;
      SetScrollStatus(Scrollable::ScrollStatus::kFling);
      current_scrollable_->GetRenderScroll()->SetInScrollAnimation(true);
      return;
    } else {
      FML_DLOG(INFO) << "fling on raster failed and fallback to fling on UI.";
    }
  }
  if (!fling_animator_) {
    fling_animator_ = std::make_unique<FlingAnimator>();
    fling_animator_->SetAnimationHandler(page_view_->GetAnimationHandler());
    fling_animator_->AddListener(this);
    fling_animator_->SetFriction(1.0f);
    fling_animator_->SetDensity(
        page_view_->GetPixelRatio<kPixelTypeLogical, kPixelTypeClay>());
  } else {
    fling_animator_->Cancel();
  }
  fling_animator_->SetStartValue(0);
  fling_animator_->SetStartVelocity(velocity);
  fling_animator_->FlingInitialize();
  fling_animator_->Start();

  SetScrollStatus(Scrollable::ScrollStatus::kFling);
  current_scrollable_->GetRenderScroll()->SetInScrollAnimation(true);
}

bool NestedScrollManager::TryStartBounce(NestedScrollable* target,
                                         float velocity) {
  if (!target->DoBounce(velocity)) {
    SetScrollStatus(Scrollable::ScrollStatus::kIdle);
    return false;
  }

  current_scrollable_ = target->GetWeakPtr();
  SetScrollStatus(Scrollable::ScrollStatus::kBounce);
  return true;
}

void NestedScrollManager::OnDynamicAnimationUpdate(DynamicAnimator& animation,
                                                   float value,
                                                   float velocity) {
  FML_DCHECK(direction_ != ScrollDirection::kNone);
  if (&animation == fling_animator_.get()) {
    float delta = animation.GetDelta();
    FloatPoint delta_point = direction_ == ScrollDirection::kHorizontal
                                 ? FloatPoint{delta, 0}
                                 : FloatPoint{0, delta};
    auto [unconsumed, scrollable] = DispatchScroll(delta_point, false);

    if (!unconsumed.IsOrigin()) {
      // If there is unconsumed scroll, that means we meet the boundary of a
      // scrollable. Stop the fling and try to start bounce at that scrollable.
      animation.Cancel();
      if (scrollable) {
        TryStartBounce(scrollable, velocity);
      } else {
        SetScrollStatus(Scrollable::ScrollStatus::kIdle);
      }
    }
  }
}

void NestedScrollManager::OnDynamicAnimationEnd(DynamicAnimator& animation,
                                                bool canceled, float value,
                                                float velocity) {
  if (!canceled) {
    // For canceled animations, we should manage the state at the point
    // of cancellation, as the new state may not be idle.
    SetScrollStatus(Scrollable::ScrollStatus::kIdle);
  }
  if (current_scrollable_) {
    current_scrollable_->GetRenderScroll()->SetInScrollAnimation(false);
    current_scrollable_->DecodeImagesRecursively();
  }
}

void NestedScrollManager::SetScrollStatus(Scrollable::ScrollStatus status) {
  if (status_ != status) {
    status_ = status;
    if (current_scrollable_) {
      current_scrollable_->SetScrollStatus(status);
    }

    bool resolve_drag_immediately =
        status == Scrollable::ScrollStatus::kFling ||
        status == Scrollable::ScrollStatus::kBounce;
    for (auto& scrollable : scrollable_chain_) {
      if (scrollable) {
        scrollable->SetResolveDragImmediately(resolve_drag_immediately);
      }
    }
  }
}

void NestedScrollManager::OnScrollableScroll(NestedScrollable* scrollable) {
  if (std::find(scrollable_chain_.begin(), scrollable_chain_.end(),
                scrollable->GetWeakPtr()) == scrollable_chain_.end()) {
    return;
  }

  // The scroll is handed off to another scrollable. We should also hand off the
  // status.
  if (current_scrollable_.get() != scrollable) {
    if (current_scrollable_) {
      current_scrollable_->SetScrollStatus(Scrollable::ScrollStatus::kIdle);
    }
    current_scrollable_ = scrollable->GetWeakPtr();
    scrollable->SetScrollStatus(status_);
  }
}

void NestedScrollManager::OnScrollableBounceEnd(NestedScrollable* scrollable) {
  if (status_ == Scrollable::ScrollStatus::kBounce &&
      std::find(scrollable_chain_.begin(), scrollable_chain_.end(),
                scrollable->GetWeakPtr()) != scrollable_chain_.end()) {
    SetScrollStatus(Scrollable::ScrollStatus::kIdle);
  }
}

NestedScrollable* NestedScrollManager::FindOverscrollView() const {
  for (auto& scrollable : scrollable_chain_) {
    if (scrollable && scrollable->IsUnderOverscroll()) {
      return scrollable.get();
    }
  }

  return nullptr;
}

void NestedScrollManager::CleanUpScrollableChain() {
  scrollable_chain_.erase(
      std::remove_if(scrollable_chain_.begin(), scrollable_chain_.end(),
                     [](const auto& scrollable) { return !scrollable; }),
      scrollable_chain_.end());
}

void NestedScrollManager::OnFlingEndFromRaster(NestedScrollable* scrollable,
                                               float remaining_velocity) {
  if (scrollable != source_scrollable_) {
    return;
  }
  if (std::abs(remaining_velocity) > 0.f) {
    // The raster fling animation is finished, we try to use the remaining
    // velocity to start a new fling animation on UI to continue the nested
    // scroll if needed.
    FML_DLOG(INFO) << "run fling on ui with the remaining velocity="
                   << remaining_velocity;
    StartFling(remaining_velocity, false);
  } else {
    if (status_ == Scrollable::ScrollStatus::kFling) {
      SetScrollStatus(Scrollable::ScrollStatus::kIdle);
      current_scrollable_->GetRenderScroll()->SetInScrollAnimation(false);
    }
  }
}

RasterFlingManager* NestedScrollManager::GetOrCreateRasterFlingManager() {
  FML_DCHECK(IsSmoothScrollEnabled());
  if (!raster_fling_manager_) {
    raster_fling_manager_ = std::make_unique<RasterFlingManager>(this);
  }
  return raster_fling_manager_.get();
}

}  // namespace clay
