// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scrollbar/scrollbar_view.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "clay/gfx/animation/value_animator.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
#include "clay/ui/rendering/render_container.h"

namespace clay {

const double kScrollbarFadeOutDuration = 300;
const double kScrollbarFadeInDuration = 100;
const double kDefaultThumbWidth = 8.0f;  // in logical pixels
const double kThumbMinLength = 18.0f;    // in logical pixels
const double kThumbRadius = 4.0f;        // in logical pixels

namespace {

inline bool NearlyZeroFloat(float value) {
  return std::abs(value) < std::numeric_limits<float>::epsilon();
}

using OnDragCallback = std::function<void(const FloatSize& delta)>;

}  // namespace

namespace internal {

class ScrollbarThumb : public BaseView {
 public:
  ScrollbarThumb(PageView* page_view, ScrollDirection direction)
      : BaseView(-1, "scrollbar-thumb", std::make_unique<RenderBox>(),
                 page_view) {
    SetScrollDirection(direction);
  }

  ~ScrollbarThumb() override = default;

  RenderBox* GetRenderBox() {
    return static_cast<RenderBox*>(render_object_.get());
  }

  void SetOnDrag(OnDragCallback on_drag) { on_drag_ = on_drag; }
  void SetOnStatusChange(
      std::function<void(ScrollbarView::ThumbStatus)> on_status_change) {
    on_status_change_ = on_status_change;
  }

  void OnMouseHoverChange() override {
    if (status_ != ScrollbarView::ThumbStatus::kDragging) {
      SetStatus(is_mouse_hover_ ? ScrollbarView::ThumbStatus::kHovered
                                : ScrollbarView::ThumbStatus::kIdle);
    }
  }

  void SetScrollDirection(ScrollDirection direction) {
    direction_ = direction;
    ResetDragRecognizer();
  }

 private:
  void SetStatus(ScrollbarView::ThumbStatus status) {
    if (status_ != status) {
      status_ = status;
      if (on_status_change_) {
        on_status_change_(status);
      }
    }
  }
  void ResetDragRecognizer() {
    ClearGestureRecognizers();
    std::unique_ptr<DragGestureRecognizer> drag_recognizer;
    if (direction_ == ScrollDirection::kVertical) {
      drag_recognizer = std::make_unique<VerticalDragGestureRecognizer>(
          page_view_->gesture_manager());
    } else {
      drag_recognizer = std::make_unique<HorizontalDragGestureRecognizer>(
          page_view_->gesture_manager());
    }
    drag_recognizer->SetTouchSlop(-1.f);
    drag_recognizer->SetDragUpdateCallback(
        [this](const FloatPoint& position, const FloatSize& delta) {
          if (on_drag_) {
            on_drag_(delta);
          }
        });
    drag_recognizer->SetDragStartCallback([this](const FloatPoint&) {
      SetStatus(ScrollbarView::ThumbStatus::kDragging);
    });
    drag_recognizer->SetDragEndCallback([this](const Velocity&) {
      SetStatus(is_mouse_hover_ ? ScrollbarView::ThumbStatus::kHovered
                                : ScrollbarView::ThumbStatus::kIdle);
    });
    drag_recognizer->SetDragCancelCallback([this]() {
      SetStatus(is_mouse_hover_ ? ScrollbarView::ThumbStatus::kHovered
                                : ScrollbarView::ThumbStatus::kIdle);
    });
    AddGestureRecognizer(std::move(drag_recognizer));
  }

  ScrollbarView::ThumbStatus status_;
  OnDragCallback on_drag_;
  std::function<void(ScrollbarView::ThumbStatus)> on_status_change_;
  ScrollDirection direction_ = ScrollDirection::kVertical;
};

}  // namespace internal

ScrollbarView::ScrollbarView(PageView* page_view)
    : BaseView(-1, "scrollbar", std::make_unique<RenderContainer>(), page_view),
      thumb_width_(FromLogical(kDefaultThumbWidth)),
      thumb_min_length_(FromLogical(kThumbMinLength)),
      thumb_radius_(FromLogical(kThumbRadius)) {
  thumb_ = new internal::ScrollbarThumb(page_view,
                                        orientation_helper_.GetDirection());
  thumb_->SetOnDrag([this](auto&& delta) { OnThumbDrag(delta); });
  thumb_->SetOnStatusChange(
      [this](auto status) { OnThumbStatusChange(status); });
  AddChild(thumb_);

  InitStyle();

  ResetGestureRecognizer();

  fadeout_animator_ = std::make_unique<ValueAnimator>();
  fadeout_animator_->SetDuration(kScrollbarFadeOutDuration);
  fadeout_animator_->SetAnimationHandler(GetAnimationHandler());
  fadeout_animator_->AddUpdateListener(this);
}

ScrollbarView::~ScrollbarView() {
  if (fadeout_timer_) {
    fadeout_timer_->Stop();
  }
}

void ScrollbarView::SetDirection(int type) {
  layout_direction_ = static_cast<DirectionType>(type);
}

/**
FIXME(ZhuChengcheng): currently scrollview's rtl is achieved by manuly convert
offset from offset to length - offset in painting only. However we cannot do
this in scrollbar by the reason of different render object. Therefore we covert
it here. This is tricky both for ScrollView and Scrollbar,We should refactor
them both.
**/
float ScrollbarView::GetConvertedPosition(float origin) const {
  if (layout_direction_ == DirectionType::kLynxRtl ||
      layout_direction_ == DirectionType::kRtl) {
    return 1 - origin;
  } else {
    return origin;
  }
}

void ScrollbarView::HandleTrackTapDown(const PointerEvent& event) {
  double scroll_increment = visible_length_ * 0.8;
  auto tap_location =
      orientation_helper_.GetLocation(GetPointBySelf(event.position));
  if (tap_location < GetThumbLocation()) {
    scroll_increment = -scroll_increment;
  } else if (tap_location <= GetThumbLocation() + GetThumbLength()) {
    // Do nothing when tapping on the thumb area
    return;
  }

  const float old_position = position_;
  position_ = std::clamp(
      position_ + scroll_increment / (total_length_ - visible_length_), 0.0,
      1.0);
  UpdateThumb();
  if (delegate_ && old_position != position_) {
    delegate_->OnScrollbarScrolled(GetConvertedPosition(old_position),
                                   GetConvertedPosition(position_), true, true);
  }
}

void ScrollbarView::OnContentSizeChanged(const FloatRect& old_rect,
                                         const FloatRect& new_rect) {
  BaseView::OnContentSizeChanged(old_rect, new_rect);
  UpdateThumb();
}

void ScrollbarView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kScrollBarThumbColor) {
    thumb_color_ = attribute_utils::GetColor(value);
    InitStyle();
  } else if (kw == KeywordID::kScrollBarThumbActiveColor) {
    thumb_active_color_ = attribute_utils::GetColor(value);
  } else if (kw == KeywordID::kScrollBarThumbHoverColor) {
    thumb_hover_color_ = attribute_utils::GetColor(value);
  } else if (kw == KeywordID::kScrollBarTrackColor) {
    track_color_ = attribute_utils::GetColor(value);
    InitStyle();
  } else if (kw == KeywordID::kScrollBarThumbRadius) {
    thumb_radius_ = FromLogical(attribute_utils::GetNum(value));
    InitStyle();
  } else if (kw == KeywordID::kScrollBarThumbWidth) {
    thumb_width_ = FromLogical(attribute_utils::GetNum(value));
    UpdateThumb();
  } else if (kw == KeywordID::kScrollBarThumbMinLength) {
    thumb_min_length_ = FromLogical(attribute_utils::GetNum(value));
    UpdateThumb();
  } else if (kw == KeywordID::kScrollBarAutoHide) {
    SetAutoHide(attribute_utils::GetBool(value));
  } else if (kw == KeywordID::kScrollBarAutoHideDelay) {
    auto_hide_delay_ = attribute_utils::GetNum(value);
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

void ScrollbarView::SetVisibleLength(float length) {
  if (visible_length_ != length) {
    visible_length_ = length;
    UpdateThumb();
  }
}

void ScrollbarView::SetTotalLength(float length) {
  if (total_length_ != length) {
    total_length_ = length;
    UpdateThumb();
  }
}

void ScrollbarView::SetPosition(float position) {
  auto position_converted = GetConvertedPosition(position);
  if (position_ != position_converted) {
    const float old_position = position_;
    position_ = position_converted;
    UpdateThumb();
    if (delegate_) {
      delegate_->OnScrollbarScrolled(GetConvertedPosition(old_position),
                                     GetConvertedPosition(position_), false,
                                     false);
    }
  }
}

void ScrollbarView::SetScrollDirection(ScrollDirection direction) {
  orientation_helper_.SetDirection(direction);
  thumb_->SetScrollDirection(direction);
}

void ScrollbarView::SetAutoHide(bool value) {
  if (auto_hide_ == value) {
    return;
  }

  auto_hide_ = value;
  if (auto_hide_) {
    StartFadeOutTimer();
  } else {
    StartFadeIn();
  }
}

void ScrollbarView::NotifyScrollViewScrolled() {
  if (!Parent()) {
    return;
  }
  StartFadeIn();
  StartFadeOutTimer();
}

void ScrollbarView::OnDestroy() {
  // Finalize animation before being removed.
  if (fadeout_animator_->IsRunning()) {
    fadeout_animator_->End();
  }
  // Memory resource will be released in `DestroyChildrenRecursively`.
  thumb_ = nullptr;
}

void ScrollbarView::InitStyle() {
  thumb_->SetBackgroundColor(thumb_color_);
  SetBackgroundColor(track_color_);
  thumb_->SetBorderRadius(thumb_radius_);

  if (auto_hide_) {
    SetOpacity(0);
  }
}

float ScrollbarView::GetThumbLocation() const {
  return position_ * (orientation_helper_.GetLength(*this) - GetThumbLength());
}

float ScrollbarView::GetThumbLength() const {
  const float self_length = orientation_helper_.GetLength(*this);

  if (NearlyZeroFloat(total_length_)) {
    return 0.f;
  }

  auto safe_min_length = std::min(thumb_min_length_, GetVisibleLength());
  return std::max(safe_min_length,
                  visible_length_ / total_length_ * self_length);
}

void ScrollbarView::UpdateThumb() {
  const float self_length = orientation_helper_.GetLength(*this);
  const float thumb_length = std::ceil(GetThumbLength());
  float thumb_location = std::floor(GetThumbLocation());

  // if thumb_length + thumb_location is very close to the end of the scrollbar,
  // make them align.
  if (std::abs(thumb_location + thumb_length - self_length) < 1.0) {
    thumb_location = self_length - thumb_length;
  }
  orientation_helper_.SetBound(
      thumb_, thumb_location,
      (orientation_helper_.GetSecondaryLength(*this) - thumb_width_) / 2,
      thumb_length, thumb_width_);
}

void ScrollbarView::HandleEvent(const PointerEvent& event) {
  BaseView::HandleEvent(event);
  StartFadeIn();
}

void ScrollbarView::OnMouseHoverChange() {
  BaseView::OnMouseHoverChange();

  if (is_mouse_hover_) {
    StartFadeIn();
  } else {
    StartFadeOutTimer();
  }
}

void ScrollbarView::UpdatePosition(float offset_in_pixel) {
  const float thumb_length = GetThumbLength();
  const float half = thumb_length / 2.f;
  const float self_length = orientation_helper_.GetLength(*this);
  position_ = (std::clamp(offset_in_pixel, half, self_length - half) - half) /
              (self_length - thumb_length);
  UpdateThumb();
}

void ScrollbarView::StartFadeOutTimer() {
  if (fadeout_timer_) {
    fadeout_timer_->Stop();
  }
  if (!auto_hide_) {
    return;
  }
  fadeout_timer_ =
      std::make_unique<fml::Timer>(page_view()->GetTaskRunner(), false);
  fadeout_timer_->Start(fml::TimeDelta::FromMilliseconds(auto_hide_delay_),
                        [this] { StartFadeOut(); });
}

void ScrollbarView::StartFadeOut() {
  if (!auto_hide_ || is_mouse_hover_ || thumb_status_ != ThumbStatus::kIdle) {
    return;
  }
  fadeout_animator_->SetDuration(kScrollbarFadeOutDuration);
  fadeout_animator_->Reverse();
  // ValueAnimator may not request frames on the start
  page_view()->RequestPaint();
}

void ScrollbarView::StartFadeIn() {
  if (!fadeout_animator_->IsReversing() && !fadeout_animator_->IsRunning() &&
      fadeout_animator_->GetAnimatedFraction() != 1) {
    fadeout_animator_->SetDuration(kScrollbarFadeInDuration);
    fadeout_animator_->Start();
    // ValueAnimator may not request frames on the start
    page_view()->RequestPaint();
  }
}

void ScrollbarView::OnAnimationUpdate(ValueAnimator& animation) {
  SetOpacity(animation.GetAnimatedFraction());
}

void ScrollbarView::OnThumbDrag(const FloatSize& delta) {
  const float old_position = position_;
  float offset = GetThumbLocation() + GetThumbLength() / 2.f;
  offset += orientation_helper_.GetLength(delta);
  UpdatePosition(offset);
  if (delegate_ && old_position != position_) {
    delegate_->OnScrollbarScrolled(GetConvertedPosition(old_position),
                                   GetConvertedPosition(position_), true,
                                   false);
  }
  thumb_->SetBackgroundColor(thumb_active_color_);
}

void ScrollbarView::OnThumbStatusChange(ThumbStatus thumb_status) {
  thumb_status_ = thumb_status;
  switch (thumb_status) {
    case ThumbStatus::kIdle:
      thumb_->SetBackgroundColor(thumb_color_);
      StartFadeOutTimer();
      break;
    case ThumbStatus::kHovered:
      thumb_->SetBackgroundColor(thumb_hover_color_);
      break;
    case ThumbStatus::kDragging:
      thumb_->SetBackgroundColor(thumb_active_color_);
      break;
  }
}

void ScrollbarView::ResetGestureRecognizer() {
  RemoveGestureRecognizer(tap_recognizer_);
  tap_recognizer_ = nullptr;
  std::unique_ptr<TapGestureRecognizer> tap_recognizer =
      std::make_unique<TapGestureRecognizer>(page_view()->gesture_manager());
  tap_recognizer_ = tap_recognizer.get();
  tap_recognizer->SetTapUpCallback([this](auto&& PH1) {
    HandleTrackTapDown(std::forward<decltype(PH1)>(PH1));
  });
  AddGestureRecognizer(std::move(tap_recognizer));
}

}  // namespace clay
