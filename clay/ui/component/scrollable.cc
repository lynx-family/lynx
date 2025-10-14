// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scrollable.h"

#include <utility>

#include "clay/ui/component/page_view.h"
#include "clay/ui/component/rubberband_distance.h"
#include "clay/ui/rendering/render_scroll.h"

namespace clay {

Scrollable::Scrollable(int id, std::string tag,
                       std::unique_ptr<RenderScroll> render_object,
                       PageView* page_view, ScrollDirection direction)
    : WithTypeInfo(id, std::move(tag), std::move(render_object), page_view),
      scroll_direction_(direction),
      overscroll_effect_(
          std::make_unique<OffsetOverscrollEffect>(GetRenderScroll())) {
  GetRenderScroll()->SetScrollDirection(scroll_direction_);
}

void Scrollable::AddScrollListener(Listener* listener) {
  if (!listener) {
    return;
  }
  listeners_.push_front(listener);
}

void Scrollable::RemoveScrollListener(Listener* listener) {
  if (!listener) {
    return;
  }
  listeners_.remove(listener);
}

void Scrollable::RemoveAllScrollListener() { listeners_.clear(); }

void Scrollable::NotifyScrolled() {
  for (auto& listener : listeners_) {
    listener->OnScrollableScrolled();
  }
}

bool Scrollable::IsUnderOverscroll() const {
  return !overscroll_offset_.IsOrigin();
}

FloatPoint Scrollable::DoOverscroll(FloatPoint delta) {
  auto offset = overscroll_offset_;
  if (delta.x() != 0) {
    if (offset.x() * delta.x() >= 0 && !IsOverscrollEnabled(delta.x() > 0)) {
      // The direction is not allowed to overscroll, do nothing.
    } else {
      float new_offset = offset.x() + delta.x();
      if (new_offset * offset.x() < 0) {
        // The direction of the overscroll has been altered, indicating that the
        // former overscrolled offset is fully consumed. To maintain consistency
        // in the scrolling behavior, the direction is kept unchanged, and the
        // new_offset represents the exact amount of the overscroll offset that
        // remains unconsumed.
        delta.SetX(new_offset);
        new_offset = 0;
      } else {
        delta.SetX(0);
      }
      offset.SetX(new_offset);
    }
  }
  if (delta.y() != 0) {
    if (offset.y() * delta.y() >= 0 && !IsOverscrollEnabled(delta.y() > 0)) {
      // The direction is not allowed to overscroll, do nothing.
    } else {
      float new_offset = offset.y() + delta.y();
      if (new_offset * offset.y() < 0) {
        delta.SetY(new_offset);
        new_offset = 0;
      } else {
        delta.SetY(0);
      }
      offset.SetY(new_offset);
    }
  }
  SetOverscrollOffset(offset);
  return delta;
}

void Scrollable::SetOverscrollOffset(FloatPoint offset) {
  if (overscroll_offset_ != offset) {
    auto old_offset = clamped_overscroll_offset_;
    overscroll_offset_ = offset;
    clamped_overscroll_offset_ = RubberBandDistance(offset, Width(), Height());
    overscroll_effect_->OnOverscroll(clamped_overscroll_offset_, old_offset);
    OnOverscroll(old_offset);
  }
}

void Scrollable::SetClampedOverscrollOffset(FloatPoint offset) {
  if (clamped_overscroll_offset_ != offset) {
    auto old_offset = clamped_overscroll_offset_;
    clamped_overscroll_offset_ = offset;
    overscroll_offset_ =
        RubberBandDistance(clamped_overscroll_offset_, Width(), Height());
    overscroll_effect_->OnOverscroll(clamped_overscroll_offset_, old_offset);
    OnOverscroll(old_offset);
  }
}

void Scrollable::SetScrollStatus(ScrollStatus status) {
  if (status_ != status) {
    auto old_status = status_;
    status_ = status;
    OnScrollStatusChange(old_status);
  }
}

bool Scrollable::DoBounce(float velocity) {
  float start_offset = scroll_direction_ == ScrollDirection::kVertical
                           ? ClampedOverscrollOffset().y()
                           : ClampedOverscrollOffset().x();
  if (start_offset == 0 && velocity == 0) {
    return false;
  }

  bool is_lower = start_offset > 0 || (start_offset == 0 && velocity > 0);
  if (!IsOverscrollEnabled(is_lower)) {
    return false;
  }
  if (!bounce_animator_) {
    bounce_animator_ = std::make_unique<BounceAnimator>();
    bounce_animator_->SetAnimationHandler(page_view_->GetAnimationHandler());
    bounce_animator_->AddListener(this);
  } else {
    bounce_animator_->Cancel();
  }
  bounce_animator_->SetStartVelocity(velocity);
  bounce_animator_->SetStartValue(start_offset);
  bounce_animator_->SetTargetValue(0);
  bounce_animator_->Init();
  bounce_animator_->Start();
  return true;
}

void Scrollable::OnDynamicAnimationUpdate(DynamicAnimator& animation,
                                          float value, float velocity) {
  if (&animation == bounce_animator_.get()) {
    FML_DCHECK(scroll_direction_ != ScrollDirection::kNone);
    SetClampedOverscrollOffset(scroll_direction_ == ScrollDirection::kVertical
                                   ? FloatPoint(0, value)
                                   : FloatPoint(value, 0));
  }
}

void Scrollable::OnDynamicAnimationEnd(DynamicAnimator& animation,
                                       bool canceled, float value,
                                       float velocity) {
  if (&animation == bounce_animator_.get()) {
    if (!canceled) {
      // If canceled, the status is handled by the NestedScrollManager.
      SetScrollStatus(ScrollStatus::kIdle);
    }
    OnBounceEnd(canceled);
  }
}

void Scrollable::StopAnimation() {
  if (bounce_animator_ && bounce_animator_->IsRunning()) {
    bounce_animator_->Cancel();
  }
}

void Scrollable::SetScrollDirection(ScrollDirection direction) {
  if (scroll_direction_ != direction) {
    ResetScroll();
    scroll_direction_ = direction;
    ResetGestureRecognizers();
  }
}

ScrollDirection Scrollable::GetScrollDirection() const {
  return scroll_direction_;
}

}  // namespace clay
