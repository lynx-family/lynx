// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_ROOT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_ROOT_H_

#include <memory>
#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIRoot : public UIView {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIRoot(context, sign, tag);
  }
  ~UIRoot() override;

  void AttachToNodeContent(NativeNodeContent* content) override;

  void UpdateLayout(float left, float top, float width, float height, const float* paddings,
                    const float* margins, const float* sticky, float max_height,
                    uint32_t node_index) override;
  void OnNodeEvent(ArkUI_NodeEvent* event) override;
  ArkUI_NodeHandle GetProxyNode();
  bool IsRoot() override;
  ArkUI_NodeHandle RootNode() override { return node_; }
  void GetOffsetToScreen(float offset_screen[2]);
  bool IsVisible() override;
  void OnNodeReady() override;
  bool EventThrough(float point[2]) override;
  std::weak_ptr<EventTarget> ParentLynxPageUI() override;
  void SetParentLynxPageUI(std::weak_ptr<EventTarget> parent) override;
  EventTarget::LynxPageUIMap* ChildrenLynxPageUI() override;
  void SetChildrenLynxPageUI(EventTarget::LynxPageUIMap children) override;
  bool IsChildLynxPageUI() const { return is_child_lynx_page_ui_; }
  void EnsurePositionChangeObservation();
  bool IsAttachedToViewTree() const { return is_root_attached_; }

 protected:
  UIRoot(LynxContext* context, int sign, const std::string& tag);

 private:
  bool are_gestures_attached_{false};
  bool is_child_lynx_page_ui_{false};
  bool is_root_attached_{false};
  bool is_root_visible_{false};
  bool is_position_change_observation_started_{false};
  ArkUI_NodeHandle root_proxy_{nullptr};
  ArkUI_NodeHandle normal_sibling_{nullptr};
  ArkUI_NodeHandle transparent_sibling_{nullptr};
  std::weak_ptr<EventTarget> parent_lynx_page_ui_;
  std::unique_ptr<EventTarget::LynxPageUIMap> children_lynx_page_ui_map_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_ROOT_H_
