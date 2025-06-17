// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_INPUT_UI_BASE_INPUT_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_INPUT_UI_BASE_INPUT_H_

#include <string>
#include <unordered_map>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

static constexpr int32_t INPUT_UNDEFINED_INT = 0xFFFFFFFF;
static constexpr float INPUT_UNDEFINED_FLOAT = 0xFFFFFFFF;
static constexpr int32_t INPUT_DEFAULT_COLOR = 0x4c3c433c;

namespace lynx {
namespace tasm {
namespace harmony {
class UIBaseInput : public UIView {
 public:
  static ArkUI_EnterKeyType ParseEnterKeyType(const lepus::Value& value);
  float width_for_measure_{INPUT_UNDEFINED_FLOAT};

 protected:
  void OnNodeEvent(ArkUI_NodeEvent* event) override;
  void OnFocusChange(bool has_focus, bool is_focus_transition) override;
  float OnArkUILayoutChanged();
  void SendInputEvent() const;
  void SendSelectionChangeEvent(int32_t start, int32_t end) const;
  void SendConfirmEvent() const;
  void SendFocusEvent() const;
  void SendBlurEvent() const;
  bool readonly_{false};
  bool show_soft_input_on_focus_{true};
  float computed_height_{INPUT_UNDEFINED_FLOAT};
  float max_height_{INPUT_UNDEFINED_FLOAT};
  float font_size_{INPUT_UNDEFINED_FLOAT};
  int32_t font_style_{INPUT_UNDEFINED_INT};
  int32_t font_weight_{INPUT_UNDEFINED_INT};
  std::string font_family_;
  float placeholder_font_size_{INPUT_UNDEFINED_FLOAT};
  int32_t placeholder_font_style_{INPUT_UNDEFINED_INT};
  int32_t placeholder_font_weight_{INPUT_UNDEFINED_INT};
  std::string placeholder_font_family_;
  std::string placeholder_;

  ArkUI_NodeHandle input_node_{nullptr};
  ArkUI_NodeHandle custom_keyboard_{nullptr};

  virtual ArkUI_NodeAttributeType GetTextAttributeType() const {
    return static_cast<ArkUI_NodeAttributeType>(-1);
  }
  virtual ArkUI_NodeAttributeType GetSelectionAttributeType() const {
    return static_cast<ArkUI_NodeAttributeType>(-1);
  };
  virtual ArkUI_NodeAttributeType GetEditingAttributeType() const {
    return static_cast<ArkUI_NodeAttributeType>(-1);
  }
  virtual ArkUI_NodeAttributeType GetPlaceholderAttributeType() const {
    return static_cast<ArkUI_NodeAttributeType>(-1);
  }
  virtual ArkUI_NodeAttributeType GetPlaceholderTextType() const {
    return static_cast<ArkUI_NodeAttributeType>(-1);
  }

  UIBaseInput(LynxContext* context, ArkUI_NodeType type, int sign, const std::string& tag,
              ArkUI_NodeType input_node_type);

  ~UIBaseInput() override;
  bool Focusable() override;
  void SetupFont();
  void SetupPlaceholderFont();
  void OnPropUpdate(const std::string& name, const lepus::Value& value) override;
  void OnNodeReady() override;
  void UpdateLayout(float left, float top, float width, float height, const float* paddings,
                    const float* margins, const float* sticky, float max_height,
                    uint32_t node_index) override;
  void OnMeasure(ArkUI_LayoutConstraint* layout_constraint) override;
  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) override;
  void Focus(const lepus::Value& args,
             base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void Blur(const lepus::Value& args,
            base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void SetValue(const lepus::Value& args,
                base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void GetValue(const lepus::Value& args,
                base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void SetSelectionRange(const lepus::Value& args,
                         base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

 private:
  using UIMethod = void (UIBaseInput::*)(
      const lepus::Value& args, base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  int32_t MeasureTextHeight(float font_size, float max_width, int font_weight, int font_style,
                            int line_spacing, std::string font_family, std::string value);
  static std::unordered_map<std::string, UIMethod> input_base_ui_method_map_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_INPUT_UI_BASE_INPUT_H_
