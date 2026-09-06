// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scroll_coordinator/scroll_coordinator_scroll_view.h"

#include <algorithm>

#include "clay/fml/logging.h"
#include "clay/gfx/animation/viscous_fluid_interpolator.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_coordinator/scroll_coordinator_header.h"
#include "clay/ui/component/scroll_coordinator/scroll_coordinator_slot.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture/hit_test_responsive_result.h"
namespace clay {

ScrollCoordinatorScrollView::ScrollCoordinatorScrollView(int id,
                                                         int callback_id,
                                                         PageView* page_view)
    : WithTypeInfo(id, "ScrollCoordinatorScrollView",
                   std::make_unique<RenderScroll>(), page_view,
                   ScrollDirection::kVertical, true),
      callback_id_(callback_id) {
  // Anonymous View
  scroller_ =
      std::make_unique<Scroller>(this, GetAnimationHandler(),
                                 std::make_unique<ViscousFluidInterpolator>());
  SetOverflow(CSSProperty::OVERFLOW_HIDDEN);
  gesture_mediate_puppet_ =
      page_view->gesture_manager()->GetGestureMediatePuppet();
}

ScrollCoordinatorScrollView::~ScrollCoordinatorScrollView() {
  scroller_->AbortAnimation();
};

void ScrollCoordinatorScrollView::SetAttribute(const char* attr_c,
                                               const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kGranularity) {
    granularity_ = attribute_utils::GetNum(value);
  } else if (kw == KeywordID::kScrollEnable) {
    auto scroll_enabled = attribute_utils::GetBool(value, true);
    SetScrollEnabled(scroll_enabled);
  } else if (kw == KeywordID::kHeaderOverSlot) {
    SetHeadOverSlot(attribute_utils::GetBool(value, false));
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

void ScrollCoordinatorScrollView::SetFoldExpanded(float header_offset_px,
                                                  bool with_anim) {
  if (!GetHeader() || GetHeader()->Height() == 0) {
    return;
  }

  // Stop the animation if it is running, regardless of the new smooth flag.
  scroller_->AbortAnimation();
  float delta = header_offset_px - scroll_offset_.y();
  if (with_anim) {
    scroller_->StartScroll(GetScrollTop(), delta);
  } else {
    DoScroll({0, delta});
  }
}

void ScrollCoordinatorScrollView::SetHeadOverSlot(bool head_over_slot) {
  if (head_over_slot != head_over_slot_) {
    head_over_slot_ = head_over_slot;
    // try re-adding children to update index in view tree;
    if (header_) {
      auto header = header_;
      RemoveChild(header_);
      BaseView::AddChild(header);
    }
    if (slot_) {
      auto slot = slot_;
      RemoveChild(slot_);
      BaseView::AddChild(slot);
    }
  }
}

ScrollCoordinatorHeader* ScrollCoordinatorScrollView::GetHeader() const {
  return header_;
}

void ScrollCoordinatorScrollView::AddChild(BaseView* child, int index) {
  if (child->Is<ScrollCoordinatorHeader>() && !header_) {
    header_ = static_cast<ScrollCoordinatorHeader*>(child);
    BaseView::AddChild(child, GetHeaderIndex());
  } else if (child->Is<ScrollCoordinatorSlot>() && !slot_) {
    slot_ = static_cast<ScrollCoordinatorSlot*>(child);
    BaseView::AddChild(child, GetSlotIndex());
  } else {
    FML_DLOG(ERROR) << "scroll-coordinator accepts only "
                    << "scroll-coordinator-header or scroll-coordinator-slot.";
  }
  GetRenderScroll()->AddOverflowFromChildren();
}

void ScrollCoordinatorScrollView::OnLayout(LayoutContext* context) {
  BaseView::OnLayout(context);
  FML_DCHECK(header_);
  FML_DCHECK(slot_);
  if (header_ == nullptr || slot_ == nullptr) {
    return;
  }
  header_->SetY(Top());
  // We assume that the height of the header must be greater than that of the
  // toolbar. If not, we need to ensure compatibility for this situation.
  slot_->SetY(std::max(header_->Height() + header_->Top(), toolbar_height_));
  max_scroll_offset_ = std::max(0.f, header_->Height() - toolbar_height_);
  GetRenderScroll()->AddOverflowFromChildren();
}

void ScrollCoordinatorScrollView::RemoveChild(BaseView* child) {
  if (child->Is<ScrollCoordinatorHeader>()) {
    header_ = nullptr;
  }
  if (child->Is<ScrollCoordinatorSlot>()) {
    slot_ = nullptr;
  }
  BaseView::RemoveChild(child);
  GetRenderScroll()->AddOverflowFromChildren();
}

void ScrollCoordinatorScrollView::HandleEvent(
    const PointerEvent& pointer_event) {
  NestedScrollable::HandleEvent(pointer_event);
  GestureManager* gesture_manager = page_view()->gesture_manager();
  if (!gesture_manager) return;
  gesture_manager->UpdateCxxFoldViewState(true, state_ == State::FULLY_FOLD,
                                          state_ == State::FULLY_OPEN);
}

void ScrollCoordinatorScrollView::AddEventCallback(const char* event) {
  BaseView::AddEventCallback(event);
}

void ScrollCoordinatorScrollView::OnScrollUpdate(float offset) {
  float delta = offset - scroll_offset_.y();
  DoScroll({0.f, delta});
}

ScrollCoordinatorScrollView::State ScrollCoordinatorScrollView::GetState()
    const {
  return state_;
}

void ScrollCoordinatorScrollView::SyncState() {
  if (scroll_offset_.y() >= max_scroll_offset_) {
    state_ = State::FULLY_FOLD;
  } else if (scroll_offset_.y() == 0.f) {
    state_ = State::FULLY_OPEN;
  } else {
    state_ = State::OPEN;
  }
}

bool ScrollCoordinatorScrollView::CaptureScroll(FloatPoint delta) {
  bool is_forward = delta.y() > 0;
  return is_forward;
}

void ScrollCoordinatorScrollView::OnChildSizeChanged(BaseView* child) {
  GetRenderScroll()->AddOverflowFromChildren();
}

FloatPoint ScrollCoordinatorScrollView::DoScroll(FloatPoint delta,
                                                 bool by_user_input,
                                                 bool ignore_repaint) {
  bool res = false;
  if (gesture_mediate_puppet_) {
    gesture_mediate_puppet_.Act(
        [&](auto& actor) { res = actor.NativeFoldViewSlotCanScroll(); });
  }
  if (delta.y() < 0 && res) {
    return {0, 0};
  }
  return NestedScrollable::DoScroll(delta, by_user_input, ignore_repaint);
}

void ScrollCoordinatorScrollView::DidScroll() {
  SyncState();
  float offset_y = GetScrollOffset().y();
  bool exceeds_threshold = std::abs(offset_y - last_sent_offset_y_) >
                           granularity_ * max_scroll_offset_;
  if (exceeds_threshold && HasEvent(event_attr::kEventOffset)) {
    // Use the parent ScrollCoordinator's id as the event target.
    page_view()->SendEvent(callback_id_, event_attr::kEventOffset,
                           {"offset", "height"}, offset_y, max_scroll_offset_);
    last_sent_offset_y_ = offset_y;
  }
}

}  // namespace clay
