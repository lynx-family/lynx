// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_INPUT_UI_TEXTAREA_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_INPUT_UI_TEXTAREA_H_

#include <string>

#include "platform/harmony/lynx_xelement/input/ui_base_input.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UITextArea : public UIBaseInput {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag);
  static ArkUI_TextAreaType ParseTextAreaType(const lepus::Value& value);

 protected:
  UITextArea(LynxContext* context, ArkUI_NodeType type, int sign,
             const std::string& tag);
  ~UITextArea() override;
  void FrameDidChanged() override;
  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;
  void OnNodeEvent(ArkUI_NodeEvent* event) override;
  void OnNodeReady() override;

  ArkUI_NodeAttributeType GetTextAttributeType() const override;
  ArkUI_NodeAttributeType GetPlaceholderAttributeType() const override;
  ArkUI_NodeAttributeType GetSelectionAttributeType() const override;
  ArkUI_NodeAttributeType GetEditingAttributeType() const override;
  ArkUI_NodeAttributeType GetPlaceholderTextType() const override;

 private:
  int32_t current_lines_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_INPUT_UI_TEXTAREA_H_
