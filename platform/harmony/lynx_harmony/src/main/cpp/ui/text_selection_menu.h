// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_TEXT_SELECTION_MENU_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_TEXT_SELECTION_MENU_H_

#include <arkui/native_node.h>

#include <functional>
#include <string>

namespace lynx {
namespace tasm {
namespace harmony {

class TextSelectionMenu {
 public:
  struct Callbacks {
    std::function<std::string()> get_selected_text;
    std::function<void()> on_overlay_will_show;
    std::function<void()> on_hidden;
    std::function<void()> on_copy_succeeded;
    std::function<void()> on_select_all;
    std::function<void()> on_dismiss;
  };

  struct LayoutInfo {
    ArkUI_NodeHandle text_node{nullptr};
    ArkUI_NodeHandle root_node{nullptr};
    float density{1.f};
    float content_left_vp{0.f};
    float content_top_vp{0.f};
    float selection_left_px{0.f};
    float selection_right_px{0.f};
    float selection_top_px{0.f};
    float selection_bottom_px{0.f};
  };

  virtual ~TextSelectionMenu() = default;

  virtual void Show(const LayoutInfo& layout) = 0;
  virtual void Update(const LayoutInfo& layout) = 0;
  virtual void Hide() = 0;
  virtual bool IsVisible() const = 0;
  virtual ArkUI_NodeHandle OverlayNode() const = 0;
  virtual ArkUI_NodeHandle BackdropNode() const = 0;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_TEXT_SELECTION_MENU_H_
