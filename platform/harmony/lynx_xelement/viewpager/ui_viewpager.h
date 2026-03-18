// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_VIEWPAGER_UI_VIEWPAGER_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_VIEWPAGER_UI_VIEWPAGER_H_

#include <string>
#include <unordered_map>

#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "base/include/value/base_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIViewPager : public UIBase {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag);
  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;
  bool IsScrollable() override;
  float ScrollX() override;

 protected:
  UIViewPager(LynxContext* context, ArkUI_NodeType type, int sign,
              const std::string& tag);
  void InsertNode(UIBase* child, int index) override;
  void RemoveNode(UIBase* child) override;
  void UpdateLayout(float left, float top, float width, float height,
                    const float* paddings, const float* margins,
                    const float* sticky, float max_height,
                    uint32_t node_index) override;
  void OnNodeReady() override;
  void FinishLayoutOperation() override;
  void OnNodeEvent(ArkUI_NodeEvent* event) override;
  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&>
                        callback) override;
  void EnableNestedScroll(bool enable) const;
  ~UIViewPager() override;
  void OnMeasure(ArkUI_LayoutConstraint* layout_constraint) override;
  void OnLayout() override;

 private:
  bool layout_changed_{false};
  bool should_reload_data_{false};
  bool enable_nested_scroll_{false};
  ArkUI_NodeHandle scroll_{nullptr};
  ArkUI_NodeHandle row_{nullptr};
  bool first_render_{true};
  bool enable_scroll_{true};
  bool tab_was_selected_before_touch_{false};
  int32_t initial_select_index_{0};
  // legacy API compatiable
  int32_t viewpager_width_changed_with_marked_index_{-1};
  int32_t index_after_reload_{-1};
  int32_t pending_scroll_index_{-1};
  bool pending_scroll_animated_{false};
  int32_t index_{0};
  int32_t emit_target_changes_only_during_scroll_{-1};
  int last_offset_on_touch_up_{-1};
  using UIMethod = void (UIViewPager::*)(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  static std::unordered_map<std::string, UIMethod> viewpager_ui_method_map_;
  void ScrollToIndex(int32_t index, bool animated);
  int32_t ClampIndex(int32_t index) const;
  void SelectTab(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_VIEWPAGER_UI_VIEWPAGER_H_
