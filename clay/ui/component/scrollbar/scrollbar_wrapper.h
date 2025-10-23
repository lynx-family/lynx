// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLLBAR_SCROLLBAR_WRAPPER_H_
#define CLAY_UI_COMPONENT_SCROLLBAR_SCROLLBAR_WRAPPER_H_

#include <memory>
#include <string>
#include <vector>

#include "clay/gfx/scroll_direction.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/scrollbar/scrollbar_view.h"
#include "clay/ui/lynx_module/types.h"

namespace clay {

class ScrollbarWrapper : public BaseView, public ScrollbarView::Delegate {
 public:
  ScrollbarWrapper(int id, ScrollDirection direction, std::string tag,
                   PageView* page_view);
  ~ScrollbarWrapper() override;

  void AddChild(BaseView* child) override;
  void AddChild(BaseView* child, int index) override;
  void RemoveChild(BaseView* child) override;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;

  // Redirect the following settings to scroll view.
  void SetWidth(float width) override;
  void SetHeight(float height) override;
  void SetBound(float left, float top, float width, float height) override;

  void SetPaddings(float padding_left, float padding_top, float padding_right,
                   float padding_bottom) override;
  void SetBorder(const BordersData& data) override;
  void SetBorderStyle(BorderStyleType left, BorderStyleType top,
                      BorderStyleType right, BorderStyleType bottom) override;
  void SetBorderStyle(Side side, int style) override;
  void SetBorderWidth(float left_width, float top_width, float right_width,
                      float bottom_width) override;
  void SetBorderWidth(Side side, float width) override;
  void SetBorderColor(unsigned int left_color, unsigned int top_color,
                      unsigned int right_color,
                      unsigned int bottom_color) override;
  void SetBorderColor(Side side, uint32_t color) override;
  void SetBorderRadius(const FloatSize& left_top, const FloatSize& right_top,
                       const FloatSize& right_bottom,
                       const FloatSize& left_bottom) override;
  void SetBorderRadius(float radius_all) override;

  void SetBackground(const BackgroundData& background) override;
  void SetBackgroundColor(const Color& color) override;
  void SetBackgroundImage(const clay::Value::Array& array) override;
  void SetBackgroundClip(const clay::Value::Array& array) override;
  void SetBackgroundOrigin(const clay::Value::Array& array) override;
  void SetBackgroundPosition(
      const std::vector<BackgroundPosition>& positions) override;
  void SetBackgroundRepeat(const clay::Value::Array& array) override;
  void SetBackgroundSize(const std::vector<BackgroundSize>& sizes) override;

  void OnContentSizeChanged(const FloatRect& old_rect,
                            const FloatRect& new_rect) override;
  void OnViewportMetricsUpdated(int physical_width, int physical_height,
                                float device_pixel_ratio) override;

  void SetScrollbarEnabled(bool enabled);

  // Pass through all bound events to inner scroll-view.
  void AddEventCallback(const char* event) override;
  /*
   * We pass all bound events to inner `view_`, and when some events happen we
   * filter through `HasEvent` to decide to whether callback or not. Then
   * problem occurs. For keyframe animation, we use wrapper itself as a
   * listener, so animation related callback will call wrapper's
   * `OnAnimationEvent`. And `OnAnimationEvent` filters event using wrapper's
   * `events_`, which results in problem.
   */
  bool HasEvent(const std::string& event) const override;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 protected:
  void OnDestroy() override;

  const ScrollbarOrientationHelper& OrientationHelper() const {
    return scrollbar_->GetOrientationHelper();
  }

  virtual void WillUpdateScrollbar() = 0;
  virtual float GetScrollbarScrollOffset() = 0;
  virtual float GetTotalLength() = 0;

  // Update the dimension of scrollbar.
  void UpdateScrollbarBounds();

  void UpdateScrollbarLengths();
  void UpdateScrollbarPosition();

  void SetScrollbarVisible(bool visible);

  BaseView* view_;
  ScrollbarView* scrollbar_;

  bool scrollbar_visible_ = false;

  bool scrollbar_enabled_ = false;
  float scrollbar_width_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SCROLLBAR_SCROLLBAR_WRAPPER_H_
