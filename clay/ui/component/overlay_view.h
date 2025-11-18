// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_OVERLAY_VIEW_H_
#define CLAY_UI_COMPONENT_OVERLAY_VIEW_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "clay/ui/component/base_view.h"

namespace clay {

class GestureManager;
class OverlayManager;
class OverlayView : public WithTypeInfo<OverlayView, BaseView> {
 public:
  OverlayView(uint32_t id, PageView* page_view);
  OverlayView(uint32_t id, const std::string& tag, PageView* page_view);
  ~OverlayView() override;

  void SetPassEventsThrough(bool pass_events_through);

  bool IsFullScreen() const { return full_screen_; }
  void SetFullScreen(bool full_screen);

  void SetAttribute(const char* attr, const clay::Value& value) override;

  const std::string_view GetOverlayId() const { return overlay_id_; }
  void SetOverlayId(std::string_view new_id) { overlay_id_ = new_id; }

  void SetBound(float left, float top, float width, float height) override;

  bool HitTest(const PointerEvent& event, HitTestResult& result,
               bool& is_pass_through);
  bool HitTest(const PointerEvent& event, HitTestResult& result) override;

  void OnAttachToTree() override;
  void OnDetachFromTree() override;

  void SetLevel(int level) { level_ = level; }
  int Level() const { return level_; }

  virtual bool ShouldChangeOffset() const { return false; }

  bool CanEventsPassThroughToViewsBehind() const override {
    return pass_events_through_;
  }

  virtual Point GetTouchOffset() const { return Point(); }

 protected:
  void Show();
  void Hide();
  int level_ = 1;
  OverlayManager* overlay_manager_;
  std::string overlay_id_;
  // Handle touch/focus event in overlay or not.
  bool pass_events_through_ = true;
  bool full_screen_ = false;

  bool IsIndependentSubViewTree() const override { return true; }

 private:
  FRIEND_TEST(OverlayViewTest, ShowAndHide);
  FRIEND_TEST(OverlayViewTest, Level);

  // Control the display order of overlays. The larger the level, the lower it
  // will be displayed. You can only adjust the level when the overlay is not
  // visible.
};
}  // namespace clay

#endif  // CLAY_UI_COMPONENT_OVERLAY_VIEW_H_
