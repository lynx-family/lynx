// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_OVERLAY_UI_OVERLAY_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_OVERLAY_UI_OVERLAY_H_

#include <arkui/native_dialog.h>
#include <arkui/native_type.h>
#include <node_api.h>

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIOverlay : public UIBase {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag);
  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;
  void FrameDidChanged() override;
  void onRequestClose();
  void OnNodeReady() override;
  bool IsVisible() override;

 protected:
  UIOverlay(LynxContext* context, int sign, const std::string& tag);

  ~UIOverlay() override;

  void UpdateLayout(float left, float top, float width, float height,
                    const float* paddings, const float* margins,
                    const float* sticky, float max_height,
                    uint32_t node_index) override;

  void InsertNode(UIBase* child, int index) override;

  void RemoveNode(UIBase* child) override;

  bool ShouldHitTest() override;

  bool EventThrough(float point[2]) override;

  bool IgnoreFocus() override;

  LynxPointerEventsValue PointerEvents() override;

  void OnNodeEvent(ArkUI_NodeEvent* event) override;

  ArkUI_NodeHandle RootNode() override { return stack_; }

  bool IsOverlayContent() const override { return true; }

 private:
  bool are_gestures_attached_{false};
  bool is_root_attached_{false};
  bool is_visible_props_{false};
  bool is_show_{false};
  bool is_event_pass_through_{false};
  bool consume_event_self_{false};
  bool child_event_through_{false};
  ArkUI_LevelMode harmony_level_mode_{ARKUI_LEVEL_MODE_OVERLAY};
  ArkUI_NodeHandle stack_{nullptr};
  ArkUI_NativeDialog* native_dialog_{nullptr};

  void ShowDialog(bool is_show);
  void ApplyLevelMode();
  void RestoreRootTarget();
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_OVERLAY_UI_OVERLAY_H_
