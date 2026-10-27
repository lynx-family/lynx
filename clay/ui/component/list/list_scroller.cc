// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_scroller.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>

#include "base/include/fml/time/time_point.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/layout_types.h"
#include "clay/ui/component/list/list_children_helper.h"
#include "clay/ui/component/list/list_layout_manager.h"
#include "clay/ui/component/list/list_orientation_helper.h"
#include "clay/ui/component/scroller_animator.h"
#include "clay/ui/component/view_context.h"

namespace clay {
namespace {

static float GetInterpolation(float t) {
  t -= 1;
  return t * t * t * t * t + 1.0f;
}

constexpr char kAlignNone[] = "none";
constexpr char kAlignTop[] = "top";
constexpr char kAlignBottom[] = "bottom";
constexpr char kAlignMiddle[] = "middle";

inline bool IsNear(float l, float r) {
  return std::abs(l - r) < std::numeric_limits<float>::epsilon();
}

}  // namespace

ListScroller::ListScroller(BaseListView* list_view) : list_view_(list_view) {
  FML_DCHECK(list_view_);
}

ListScroller::~ListScroller() = default;

// static
AlignTo ListScroller::StringToAlign(const std::string& str) {
  if (str == kAlignNone) {
    return AlignTo::kNone;
  } else if (str == kAlignTop) {
    return AlignTo::kStart;
  } else if (str == kAlignMiddle) {
    return AlignTo::kMiddle;
  } else if (str == kAlignBottom) {
    return AlignTo::kEnd;
  } else {
    return AlignTo::kNone;
  }
}

int32_t ListScroller::CalculatePxPerFrame() const {
  return list_view_->page_view()->DeviceDpi() / 4;
}

void ListScroller::ScrollToPosition(
    bool smooth, int position, float offset, AlignTo align_to,
    const std::string& id, const std::optional<FloatRect> target_rect,
    std::function<void(uint32_t, const std::string&)> callback) {
  position_ = position;
  id_ = id;
  offset_ = offset;
  align_to_ = align_to;
  target_rect_ = target_rect;

  if (!smooth) {
    auto result = ScrollImmediately();
    if (callback) {
      callback(result ? static_cast<uint32_t>(LynxUIMethodResult::kSuccess)
                      : static_cast<uint32_t>(LynxUIMethodResult::kUnknown),
               "");
    }
    return;
  }

  // reset final location and duration time when animation start
  target_distance_ = std::nullopt;
  duration_time_ = std::nullopt;
  last_distance_ = 0;
  start_time_ = fml::TimePoint::Now().ToEpochDelta().ToMillisecondsF();

  if (Scrolling()) {
    if (callback_) {
      callback_(static_cast<uint32_t>(LynxUIMethodResult::kUnknown), "");
      callback_ = nullptr;
    }
  }

  callback_ = callback;

  if (!Scrolling()) {
    StartAnimator();
  }
}

bool ListScroller::Scrolling() const {
  if (!animator_) {
    return false;
  }
  return animator_->Running();
}

void ListScroller::StopScroll(bool result) {
  if (Scrolling()) {
    StopAnimator();
    if (callback_) {
      callback_(result ? static_cast<uint32_t>(LynxUIMethodResult::kSuccess)
                       : static_cast<uint32_t>(LynxUIMethodResult::kUnknown),
                "");
      callback_ = nullptr;
    }
  }
}

ListLayoutManager* ListScroller::GetLayoutManager() const {
  return list_view_->GetLayoutManager();
}

ListAdapter* ListScroller::GetAdapter() const { return list_view_->adapter_; }

ListChildrenHelper* ListScroller::GetChildrenHelper() const {
  return list_view_->children_helper_.get();
}

AnimationHandler* ListScroller::GetAnimationHandler() const {
  return list_view_->GetAnimationHandler();
}

void ListScroller::StartAnimator() {
  if (Scrolling()) {
    return;
  }
  if (list_view_) {
    list_view_->NotifyScrollAnimationStart();
  }

  if (!animator_) {
    animator_ = std::make_unique<internal::ScrollerAnimator>(
        [this](int64_t frame_time) { return OnAnimation(frame_time); },
        GetAnimationHandler());
  }
  animator_->Start();
}

void ListScroller::StopAnimator() {
  if (!Scrolling()) {
    return;
  }
  if (list_view_) {
    list_view_->NotifyScrollAnimationEnd();
  }
  animator_->Stop();
}

bool ListScroller::ScrollImmediately() {
  if (Scrolling()) {
    StopAnimator();
  }

  // First, we move the target item to visible area (if needed)
  auto layout_manager = GetLayoutManager();
  auto target_item = GetChildrenHelper()->FindChildByPosition(position_);
  if (!target_item) {
    layout_manager->ScrollToPosition(position_, align_to_);
    list_view_->Layout();
    target_item = GetChildrenHelper()->FindChildByPosition(position_);
    if (!target_item) {
      return false;
    }
  }

  // Then we can calculate the final location
  auto distance = DistanceToTarget(target_item);
  if (distance != 0) {
    FloatSize px_to_scroll;
    if (layout_manager->CanScrollHorizontally()) {
      px_to_scroll = {-distance, 0.f};
    } else {
      px_to_scroll = {0.f, -distance};
    }
    list_view_->OnScrollBy(px_to_scroll);
  }
  return true;
}

bool ListScroller::OnAnimation(int64_t frame_time) {
  TRACE_EVENT("LIST", "ListScroller::OnAnimation");
  if (list_view_->NeedsLayout()) {
    // This will consume the pending ops
    list_view_->Layout();
  }

  const int child_count = GetChildrenHelper()->GetChildCount();
  if (child_count == 0) {
    StopScroll();
    return true;
  }

  ListLayoutManager* layout_manager = GetLayoutManager();
  UpdatePositions();

  // For an iteration, we either scroll to a target position OR an offset in px.
  int target_pos = ListItemViewHolder::kNoPosition;
  float target_offset = 0.f;
  bool should_stop = false;
  bool is_horizontal = layout_manager->CanScrollHorizontally();

  auto target_item = GetChildrenHelper()->FindChildByPosition(position_);
  if (target_item == nullptr && !target_distance_.has_value()) {
    const float list_height = list_view_->Height();
    // target view is not visible
    const bool scroll_to_start = start_position_ > position_;
    int item_to_scroll = scroll_to_start ? (start_position_ - position_)
                                         : (position_ - end_position_);

    float estimate_distance = item_to_scroll * list_height / child_count;
    const bool far_away =
        IsFarAwayFromTarget(item_to_scroll, estimate_distance);
    if (far_away) {
      // scroll to near target by middle point
      target_pos = scroll_to_start
                       ? (start_position_ - position_) / 2 + position_
                       : (end_position_ - position_) / 2 + position_;
    } else {
      // scroll slower when near to target
      target_offset =
          std::min(list_height,
                   static_cast<float>(CalculatePxPerFrame() * item_to_scroll)) *
          (scroll_to_start ? -1.f : 1.f);
    }
  } else {
    // target view is visible, adjust to offset
    if (!target_distance_.has_value()) {
      target_distance_ = DistanceToTarget(target_item);
      if (!is_horizontal) {
        duration_time_ = ComputeScrollDuration(0, target_distance_.value());
      } else {
        duration_time_ = ComputeScrollDuration(target_distance_.value(), 0);
      }
    }

    if (!ComputeScrollOffset(target_distance_.value(), frame_time,
                             target_offset)) {
      should_stop = true;
    }
  }

  if (target_pos != ListItemViewHolder::kNoPosition) {
    layout_manager->ScrollToPosition(target_pos, align_to_);
    list_view_->MarkNeedsLayout();
  } else if (!IsNear(target_offset, 0.f)) {
    FloatSize px_to_scroll;
    if (is_horizontal) {
      px_to_scroll = {-target_offset, 0.f};
    } else {
      px_to_scroll = {0.f, -target_offset};
    }
    if (!list_view_->OnScrollBy(px_to_scroll)) {
      should_stop = true;
    }
  }

  if (should_stop) {
    StopScroll(true);
  }

  return should_stop;
}

void ListScroller::UpdatePositions() {
  position_ = std::min(position_, GetAdapter()->GetItemCount() - 1);
  position_ = std::max(position_, 0);

  ListChildrenHelper* children_helper = GetChildrenHelper();
  const int child_count = children_helper->GetChildCount();
  FML_DCHECK(child_count);

  // FIXME(Xietong): Lynx use GetLayoutPosition here.
  start_position_ = children_helper->GetChildAt(0)->GetPosition();
  end_position_ = children_helper->GetChildAt(child_count - 1)->GetPosition();
}

bool ListScroller::IsFarAwayFromTarget(int to_scroll,
                                       float estimated_distance) {
  return to_scroll > 30 && estimated_distance > 10.f * list_view_->Height();
}

float ListScroller::DistanceToTarget(ListItemViewHolder* target_item) {
  FML_DCHECK(target_item);

  BaseView* target_view = nullptr;
  if (!id_.empty()) {
    target_view =
        ViewContext::FindViewByIdSelector(id_, target_item->GetView());
  }

  ListLayoutManager* layout_manager = GetLayoutManager();
  ListOrientationHelper* orientation_helper =
      layout_manager->GetOrientationHelper();

  float target_start = orientation_helper->GetDecoratedStart(target_item);
  float target_height;
  if (target_view) {
    auto view_rect = target_view->BoundsRelativeTo(target_item->GetView());
    target_start += orientation_helper->GetRectStart(view_rect);
    target_height = orientation_helper->GetRectEnd(view_rect) -
                    orientation_helper->GetRectStart(view_rect);
  } else if (target_rect_) {
    target_start += orientation_helper->GetRectStart(target_rect_.value());
    target_height = orientation_helper->GetRectEnd(target_rect_.value()) -
                    orientation_helper->GetRectStart(target_rect_.value());
  } else {
    target_height = orientation_helper->GetDecoratedMeasure(target_item);
  }

  // height of visible area
  int available_height = orientation_helper->GetEnd();

  // return delta of view Top and target-Y
  float res = target_start - offset_;
  if (align_to_ == AlignTo::kMiddle) {
    res -= round((available_height - target_height) / 2.f);
  } else if (align_to_ == AlignTo::kEnd) {
    res -= available_height - target_height;
  } else if (align_to_ == AlignTo::kNone) {
    if (0 <= res && res <= available_height - target_height) {
      // cell need not scrolling when totally in visible area
      res = 0;
    } else if (res > available_height - target_height) {
      // calculate as AlignTo::kBottom when scrolling to head
      res -= available_height - target_height;
    }
  }
  return res;
}

/**
 * Call this when you want to know the new location. If it returns true, the
 * animation is not yet finished.
 */
bool ListScroller::ComputeScrollOffset(const float& distance,
                                       const int64_t& now_time,
                                       float& target_offset) {
  // Any scroller can be used for time, since they were started
  // together in scroll mode. We use X here.
  int64_t elapsed_time = std::max<int64_t>(0, now_time - start_time_);
  float target_distance;
  if (elapsed_time < duration_time_) {
    float q = GetInterpolation(static_cast<float>(elapsed_time) /
                               duration_time_.value());
    // Temporarily does not support x-axis movement
    target_distance = std::round(q * distance);
    target_offset = target_distance - last_distance_;
    if (std::abs(distance - target_distance) < 1.0) {
      target_offset = distance - last_distance_;
      return false;
    }
  } else {
    target_offset = distance - last_distance_;
    return false;
  }
  last_distance_ = target_distance;
  return true;
}

int ListScroller::ComputeScrollDuration(int dx, int dy) {
  int abs_dx = std::abs(dx);
  int abs_dy = std::abs(dy);
  bool horizontal = abs_dx > abs_dy;
  int container_size = horizontal ? list_view_->Width() : list_view_->Height();

  float abs_delta = static_cast<float>(horizontal ? abs_dx : abs_dy);
  int duration = static_cast<int>(((abs_delta / container_size) + 1) * 300);
  return std::min(duration, 2000);
}

}  // namespace clay
