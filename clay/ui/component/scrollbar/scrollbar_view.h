// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLLBAR_SCROLLBAR_VIEW_H_
#define CLAY_UI_COMPONENT_SCROLLBAR_SCROLLBAR_VIEW_H_

#include <memory>

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/scrollbar/scrollbar_orientation_helper.h"
#include "clay/ui/gesture/tap_gesture_recognizer.h"

namespace clay {

namespace internal {

class ScrollbarThumb;

}  // namespace internal

class ScrollbarView : public BaseView, AnimatorUpdateListener {
 public:
  class Delegate {
   public:
    virtual void OnScrollbarScrolled(float old_position, float new_position,
                                     bool by_interaction, bool smooth) = 0;
  };

  enum class ThumbStatus {
    kIdle,
    kHovered,
    kDragging,
  };

  explicit ScrollbarView(PageView* page_view);
  ~ScrollbarView() override;

  void OnContentSizeChanged(const FloatRect& old_rect,
                            const FloatRect& new_rect) override;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;

  void SetVisibleLength(float length);
  float GetVisibleLength() const { return visible_length_; }
  void SetTotalLength(float length);
  float GetTotalLength() const { return total_length_; }
  void SetPosition(float position);

  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

  const ScrollbarOrientationHelper& GetOrientationHelper() const {
    return orientation_helper_;
  }

  void SetDirection(int type) override;

  float GetConvertedPosition(float origin) const;

  void SetScrollDirection(ScrollDirection direction);

  void SetAutoHide(bool value);

  void NotifyScrollViewScrolled();

  void ResetGestureRecognizer();

 protected:
  void OnMouseHoverChange() override;

 private:
  void OnDestroy() override;

  void InitStyle();

  // Instead of returning the view dimension of thumb, the ideal value
  // (calculated from visible length, total length and position) is returned.
  // The view dimension of thumb may have minor difference from this value
  // because the view dimension is either ceiled or floored.
  float GetThumbLocation() const;
  float GetThumbLength() const;

  void UpdateThumb();

  void HandleEvent(const PointerEvent& event) override;
  void UpdatePosition(float offset_in_pixel);

  void HandleTrackTapDown(const PointerEvent& event);

  void StartFadeOutTimer();

  void StartFadeOut();
  void StartFadeIn();

  void OnAnimationUpdate(ValueAnimator& animation) override;

  void OnThumbDrag(const FloatSize& delta);
  void OnThumbStatusChange(ThumbStatus thumb_status);

  internal::ScrollbarThumb* thumb_;

  float visible_length_ = 0.f;
  float total_length_ = 1.f;
  float position_ = 0.f;  // in percentage
  ThumbStatus thumb_status_ = ThumbStatus::kIdle;

  float thumb_width_ = 8;        // in logical pixels
  float thumb_min_length_ = 18;  // in logical pixels
  float thumb_radius_ = 4;       // in logical pixels
  Color thumb_color_ = Color::RGBOColor(0, 0, 0, 0.4);
  Color thumb_active_color_ = Color::RGBOColor(0, 0, 0, 0.8);
  Color thumb_hover_color_ = Color::RGBOColor(0, 0, 0, 0.8);
  Color track_color_ = Color::RGBOColor(0, 0, 0, 0);
  bool auto_hide_ = true;
  float auto_hide_delay_ = 1000;  // in milliseconds

  Delegate* delegate_ = nullptr;
  std::unique_ptr<fml::Timer> fadeout_timer_;
  std::unique_ptr<ValueAnimator> fadeout_animator_;

  ScrollbarOrientationHelper orientation_helper_;
  TapGestureRecognizer* tap_recognizer_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SCROLLBAR_SCROLLBAR_VIEW_H_
