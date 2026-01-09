// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/nested_scroll/nested_scrollable.h"

#include <float.h>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

#include "clay/gfx/animation/cubic_bezier_interpolator.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/list_container/list_container_wrapper.h"
#include "clay/ui/component/list/list_wrapper.h"
#include "clay/ui/component/nested_scroll/nested_scroll_manager.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_wrapper.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/scrollable_direction.h"
#include "clay/ui/gesture/tap_gesture_recognizer.h"

namespace clay {

static constexpr int64_t kMillisecondsPerSecond = 1000;

const double kDurationDivisor = 60.0;

const double kInverseDeltaRampStartPx = 120.0;
const double kInverseDeltaRampEndPx = 480.0;
const double kInverseDeltaMinDuration = 6.0;
const double kInverseDeltaMaxDuration = 12.0;

const double kInverseDeltaSlope =
    (kInverseDeltaMinDuration - kInverseDeltaMaxDuration) /
    (kInverseDeltaRampEndPx - kInverseDeltaRampStartPx);

const double kInverseDeltaOffset =
    kInverseDeltaMaxDuration - kInverseDeltaRampStartPx * kInverseDeltaSlope;

static float MaximumDimension(const FloatPoint& delta) {
  return std::abs(delta.x()) > std::abs(delta.y()) ? delta.x() : delta.y();
}

bool PointEqual(const FloatPoint& point1, const FloatPoint& point2) {
  return std::abs(point1.x() - point2.x()) < FLT_EPSILON &&
         std::abs(point1.y() - point2.y()) < FLT_EPSILON;
}

NestedScrollable::NestedScrollable(int id, std::string tag,
                                   std::unique_ptr<RenderScroll> render_object,
                                   PageView* page_view,
                                   ScrollDirection direction,
                                   bool enable_nested_scroll)
    : WithTypeInfo(id, std::move(tag), std::move(render_object), page_view,
                   direction),
      enable_nested_scroll_(enable_nested_scroll) {
  ResetGestureRecognizers();
  wheel_animator_ = std::make_unique<ValueAnimator>();
  wheel_animator_->SetAnimationHandler(GetAnimationHandler());
  wheel_animator_->AddUpdateListener(this);
  wheel_animator_->AddListener(this);
  wheel_animator_->SetInterpolator(
      CubicBezierInterpolator::Create(0.42, 0.0, 0.58, 1.0));
}

NestedScrollable::~NestedScrollable() {
  if (wheel_animator_->IsRunning()) {
    wheel_animator_->End();
  }
  if (handle_signal_event_) {
    page_view()->gesture_manager()->EndMouseWheelTransactionByForce();
  }
  signal_scroll_started_ = false;
}

void NestedScrollable::SetScrollEnabled(bool enabled) {
  if (IsScrollEnabled() != enabled) {
    Scrollable::SetScrollEnabled(enabled);
    if (enabled) {
      ResetGestureRecognizers();
    } else {
#if defined(OS_ANDROID) || defined(OS_IOS) || defined(OS_HARMONY)
      RemoveGestureRecognizer(drag_recognizer_);
      drag_recognizer_ = nullptr;
#endif
    }
  }
}

bool NestedScrollable::CanDragScrollOnX() const {
  return scroll_direction_ == ScrollDirection::kHorizontal;
}

bool NestedScrollable::CanDragScrollOnY() const {
  return scroll_direction_ == ScrollDirection::kVertical;
}

std::tuple<FloatPoint, NestedScrollable*> NestedScrollable::HandleNestedScroll(
    FloatPoint delta, bool is_dragging) {
  std::tuple<FloatPoint, NestedScrollable*> result = {delta, this};
  if (delta.IsOrigin()) {
    return result;
  }

  auto handle_parent = [&] {
    if (auto parent_scrollable =
            nested_scroll_manager()->FindOuterScrollable(this)) {
      result = parent_scrollable->HandleNestedScroll(std::get<0>(result),
                                                     is_dragging);
    }
  };

  auto& unconsumed = std::get<0>(result);
  bool is_forward = (CanDragScrollOnX() ? delta.x() : delta.y()) >= 0;
  auto mode = is_forward ? scroll_forward_mode_ : scroll_backward_mode_;
  if (mode == NestedScrollMode::kParentFirst) {
    handle_parent();
    if (unconsumed.IsOrigin()) {
      return result;
    }
  }
  if (IsScrollEnabled()) {
    auto old_unconsumed = unconsumed;
    unconsumed = DoScroll(unconsumed);
    if (unconsumed != old_unconsumed) {
      nested_scroll_manager()->OnScrollableScroll(this);
    }
    if (unconsumed.IsOrigin()) {
      return result;
    }
    if (IsOverscrollEnabled(is_forward)) {
      // We only enter the overscroll state when dragging. Otherwise, the scroll
      // is triggered by fling animation. In that case, we should not do
      // overscroll here, and it will be handled by a bounce animator instead.
      if (is_dragging) {
        auto old_unconsumed = unconsumed;
        unconsumed = DoOverscroll(unconsumed);
        if (unconsumed != old_unconsumed) {
          nested_scroll_manager()->OnScrollableScroll(this);
        }
      }
      return result;
    }
  }

  if (mode == NestedScrollMode::kSelfFirst) {
    handle_parent();
  }
  return result;
}

void NestedScrollable::OnOverscroll(FloatPoint prev_overscroll_offset) {
  DidScroll();
}

void NestedScrollable::OnBounceEnd(bool canceled) {
  // We need to notify the manager to update the scroll status
  nested_scroll_manager()->OnScrollableBounceEnd(this);
}

void NestedScrollable::ResetGestureRecognizers() {
  RemoveGestureRecognizer(drag_recognizer_);
  drag_recognizer_ = nullptr;

  std::unique_ptr<DragGestureRecognizer> drag_recognizer =
      CreateDragRecognizerByDirection(scroll_direction_,
                                      page_view()->gesture_manager());
  drag_recognizer_ = drag_recognizer.get();
  drag_recognizer->SetDragStartCallback([this](const FloatPoint& point) {
    nested_scroll_manager()->DragStart(this);
  });
  drag_recognizer->SetDragUpdateCallback(
      [this](const FloatPoint& position, const FloatSize& delta) {
        nested_scroll_manager()->DragUpdate(this,
                                            {-delta.width(), -delta.height()});
      });
  drag_recognizer->SetDragEndCallback([this](const Velocity& velocity) {
    nested_scroll_manager()->DragEnd(this, -velocity.pixels_per_second());
    auto v = -velocity.pixels_per_second();
    last_drag_end_velocity_ = scroll_direction_ == ScrollDirection::kVertical
                                  ? v.height()
                                  : v.width();
  });
  drag_recognizer->SetDragCancelCallback(
      [this]() { nested_scroll_manager()->DragEnd(this, {}); });
  drag_recognizer->SetDelegate(this);
  AddGestureRecognizer(std::move(drag_recognizer));
}

NestedScrollManager* NestedScrollable::nested_scroll_manager() const {
  return page_view()->nested_scroll_manager();
}

NestedScrollable* NestedScrollable::FindAncestorNestedDraggableView() {
  auto parent = Parent();
  while (parent) {
    if (parent->Is<NestedScrollable>()) {
      return static_cast<NestedScrollable*>(parent);
    }
    parent = parent->Parent();
  }
  return nullptr;
}

NestedScrollable* NestedScrollable::FindBeginningScrollable() {
  std::vector<NestedScrollable*> visited = {this};
  while (true) {
    auto pre_scrollable = visited.back()->PreviousScrollable();
    if (pre_scrollable && std::find(visited.begin(), visited.end(),
                                    pre_scrollable) == visited.end()) {
      visited.push_back(pre_scrollable);
    } else {
      break;
    }
  }
  return visited.back();
}

NestedScrollable* NestedScrollable::FindNextScrollable() {
  return NextScrollable() ?: FindAncestorNestedDraggableView();
}

bool NestedScrollable::CanScrollBy(float delta_x, float delta_y) const {
  auto direction = GetScrollableDirection();
  if (delta_x > 0 && (direction & ScrollableDirection::kRightwards) !=
                         ScrollableDirection::kNone) {
    return true;
  }
  if (delta_x < 0 && (direction & ScrollableDirection::kLeftwards) !=
                         ScrollableDirection::kNone) {
    return true;
  }
  if (delta_y > 0 && (direction & ScrollableDirection::kDownwards) !=
                         ScrollableDirection::kNone) {
    return true;
  }
  if (delta_y < 0 && (direction & ScrollableDirection::kUpwards) !=
                         ScrollableDirection::kNone) {
    return true;
  }
  return false;
}

void NestedScrollable::HandleEvent(const PointerEvent& event) {
  if (!IsScrollEnabled()) {
    return;
  }
  BaseView::HandleEvent(event);
  if (event.type == PointerEvent::EventType::kSignalEvent) {
    page_view()->gesture_manager()->RegisterSignalRoute(
        [weak = this->GetWeakPtr()](const PointerEvent& event) {
          auto scrollable = static_cast<NestedScrollable*>(weak.get());
          if (!scrollable) {
            return;
          }
          switch (event.signal_kind) {
            case PointerEvent::SignalKind::kStartScroll: {
              scrollable->handle_signal_event_ = true;
              if (scrollable->CanScrollBy(event.scroll_delta_x,
                                          event.scroll_delta_y)) {
                if ((event.scroll_delta_x != 0.f && scrollable->CanScrollX()) ||
                    (event.scroll_delta_y != 0.f && scrollable->CanScrollY())) {
                  scrollable->signal_scroll_started_ = true;
                }
              }
              scrollable->DispatchMouseWheelEvent(event);
              break;
            }
            case PointerEvent::SignalKind::kScroll: {
              // TODO(liuguoliang): Support more nested hierarchy, currently
              // only one level is supported.
              scrollable->DispatchMouseWheelEvent(event);
              break;
            }
            case PointerEvent::SignalKind::kEndScroll:
              scrollable->DispatchMouseWheelEvent(event);
              scrollable->signal_scroll_started_ = false;
              scrollable->handle_signal_event_ = false;
              break;
            default:
              break;
          }
        });
  }
}

FloatPoint NestedScrollable::DoScroll(FloatPoint delta, bool by_user_input,
                                      bool ignore_repaint) {
  FloatPoint old_delta = delta;
  auto scroll = GetRenderScroll();
  auto max_scroll_width = scroll->MaxScrollWidth();
  auto max_scroll_height = scroll->MaxScrollHeight();
  if (delta.x() != 0) {
    float new_left = scroll_offset_.x() + delta.x();
    if (new_left < 0) {
      delta.SetX(new_left);
      new_left = 0;
    } else if (new_left > max_scroll_width) {
      // Avoid delta changing by float calculation.
      if (scroll_offset_.x() != max_scroll_width) {
        delta.SetX(new_left - max_scroll_width);
      }
      new_left = max_scroll_width;
    } else {
      delta.SetX(0);
    }
    scroll->SetScrollLeft(new_left, ignore_repaint);
  }
  if (delta.y() != 0) {
    float new_top = scroll_offset_.y() + delta.y();
    if (new_top < 0) {
      delta.SetY(new_top);
      new_top = 0;
    } else if (new_top > max_scroll_height) {
      // Avoid delta changing by float calculation.
      if (scroll_offset_.y() != max_scroll_height) {
        delta.SetY(new_top - max_scroll_height);
      }
      new_top = max_scroll_height;
    } else {
      delta.SetY(0);
    }
    scroll->SetScrollTop(new_top, ignore_repaint);
  }
  scroll_offset_ = scroll->ScrollOffset();

  if (!PointEqual(delta, old_delta)) {
    DidScroll();
    if (!ignore_repaint) {
      Invalidate();
    }
  }
  return delta;
}

void NestedScrollable::SetTouchSlop(float slop) {
  touch_slop_ = slop;
  UpdateTouchSlop();
}

void NestedScrollable::SetResolveDragImmediately(
    bool resolve_drag_immediately) {
  if (resolve_drag_immediately_ != resolve_drag_immediately) {
    resolve_drag_immediately_ = resolve_drag_immediately;
    UpdateTouchSlop();
  }
}

void NestedScrollable::UpdateTouchSlop() {
  float used_slop = resolve_drag_immediately_ ? -1 : FromLogical(touch_slop_);
  if (drag_recognizer_) {
    drag_recognizer_->SetTouchSlop(used_slop);
  }
}

double GetDuration(float delta) {
  auto duration = kInverseDeltaOffset + std::abs(delta) * kInverseDeltaSlope;
  duration =
      std::clamp(duration, kInverseDeltaMinDuration, kInverseDeltaMaxDuration);
  duration = duration / kDurationDivisor * kMillisecondsPerSecond;
  return duration;
}

double VelocityBasedDurationBound(FloatPoint delta, double velocity) {
  double delta_max_dimension = MaximumDimension(delta);
  if (std::abs(velocity) < FLT_EPSILON) {
    return std::numeric_limits<double>::max();
  }
  double bound = delta_max_dimension / velocity * 2.5f;
  return bound < 0 ? std::numeric_limits<double>::max()
                   : bound * kMillisecondsPerSecond;
}

void NestedScrollable::UpdateWheelAnimation(FloatPoint delta) {
  auto t = wheel_animator_->GetActivatedTime();
  if (t < FLT_EPSILON || t - last_retarget_ < FLT_EPSILON) {
    return;
  }
  auto new_delta = MaximumDimension(target_scroll_delta_) -
                   MaximumDimension(last_target_scroll_delta_) *
                       wheel_animator_->GetAnimatedFraction();
  auto velocity = CalculateVelocity(t);
  // Use the velocity-based duration bound when it is less than the constant
  // segment duration. This minimizes the "rubber-band" bouncing effect when
  // |velocity| is large and |new_delta| is small.
  double new_duration = std::min(GetDuration(new_delta),
                                 VelocityBasedDurationBound(delta, velocity));
  wheel_animator_->SetDuration(t + new_duration);
  double new_slope =
      velocity * ((new_duration / kMillisecondsPerSecond) / new_delta);
  new_slope = std::clamp(new_slope, -1000.0, 1000.0);
  wheel_animator_->SetInterpolator(
      CubicBezierInterpolator::Create(0.42, 0.42 * new_slope, 0.58, 1.0));
  last_retarget_ = t;
  last_target_scroll_delta_ = target_scroll_delta_;
}

void NestedScrollable::DoWheelScroll(FloatPoint delta) {
  if (need_scroll_animation_) {
    if (delta.IsOrigin()) {
      return;
    }
    if (!wheel_animator_->IsRunning()) {
      target_scroll_delta_ = delta;
      last_scroll_delta_ = 0;
      last_retarget_ = 0;
      last_target_scroll_delta_ = FloatPoint();
      wheel_animator_->SetDuration(GetDuration(MaximumDimension(delta)));
      wheel_animator_->Start();
      // ValueAnimator may not request frames on the start
      page_view()->RequestPaint();
    } else {
      if (delta.distance() > FLT_EPSILON) {
        // If reverse scrolling occurs, stop the current animation and restart a
        // new animation
        if (MaximumDimension(delta) * MaximumDimension(target_scroll_delta_) <
            0) {
          wheel_animator_->End();
          DoWheelScroll(delta);
        } else {
          target_scroll_delta_ += delta;
          UpdateWheelAnimation(delta);
        }
      }
    }
  } else {
    DoScroll(delta);
  }
}

double NestedScrollable::CalculateVelocity(float time) {
  auto duration = wheel_animator_->GetDuration() - last_retarget_;
  const double slope = wheel_animator_->GetInterpolator()->Velocity(
      (time - last_retarget_) / duration);
  auto delta = MaximumDimension(last_target_scroll_delta_) *
               (1 - wheel_animator_->GetAnimatedFraction());
  // Interpolator velocity just gives the slope of the curve. Convert it to
  // units of pixels per second.
  return slope * (delta / duration);
}

void NestedScrollable::OnAnimationEnd(Animator& animation) {
  SetScrollStatus(ScrollStatus::kIdle);
}

void NestedScrollable::OnAnimationCancel(Animator& animation) {
  SetScrollStatus(ScrollStatus::kIdle);
}

void NestedScrollable::OnAnimationUpdate(ValueAnimator& animation) {
  float fraction = wheel_animator_->GetAnimatedFraction();
  float scroll_delta = MaximumDimension(target_scroll_delta_) * fraction;
  auto delta = target_scroll_delta_.x()
                   ? FloatPoint(scroll_delta - last_scroll_delta_, 0)
                   : FloatPoint(0, scroll_delta - last_scroll_delta_);
  auto unconsumed = DoScroll(delta);
  if (!unconsumed.IsOrigin()) {
    wheel_animator_->Cancel();
  }
  last_scroll_delta_ = scroll_delta;
}

NestedScrollable* NestedScrollable::GetScrollable(BaseView* view) {
  if (view && view->Is<NestedScrollable>()) {
    return static_cast<NestedScrollable*>(view);
  } else if (view->Is<ScrollWrapper>()) {
    // TODO(liuguoliang): common method to find scrollable view
    return static_cast<ScrollWrapper*>(view)->GetScrollView();
  }
#ifndef ENABLE_CLAY_LITE
  else if (view->Is<ListWrapper>()) {
    return static_cast<ListWrapper*>(view)->GetListView();
  }
#endif
  else if (view->Is<ListContainerWrapper>()) {
    return static_cast<ListContainerWrapper*>(view)->GetListContainerView();
  }
  return nullptr;
}

void NestedScrollable::DispatchMouseWheelEvent(const PointerEvent& event) {
  NestedScrollable* s =
      signal_scroll_started_ ? this : FindAncestorNestedDraggableView();
  if (!s) {
    return;
  }
  FloatPoint delta = {event.scroll_delta_x, event.scroll_delta_y};
  if (event.is_precise_scroll) {
    s->DoScroll(delta);
  } else {
    s->DoWheelScroll(delta);
  }
  if (event.signal_kind == PointerEvent::SignalKind::kStartScroll) {
    s->SetScrollStatus(ScrollStatus::kDragging);
  }
  if (!need_scroll_animation_ &&
      event.signal_kind == PointerEvent::SignalKind::kEndScroll) {
    // Some unittests set `need_scroll_animation_` to false which causes mouse
    // wheel event triggering no animation. In such case, we need to set scroll
    // status in the end of mouse wheel event.
    s->SetScrollStatus(ScrollStatus::kIdle);
  }
}

bool NestedScrollable::IsPointerAllowed(
    const GestureRecognizer& gesture_recognizer, const PointerEvent& event) {
  return event.device == PointerEvent::DeviceType::kTouch ||
         event.device == PointerEvent::DeviceType::kTrackpad
#if defined(ENABLE_HEADLESS)
         || event.device == PointerEvent::DeviceType::kMouse
#endif
      ;
}

}  // namespace clay
