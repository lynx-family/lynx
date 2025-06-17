// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/input/ui_textarea.h"

#include <arkui/native_node.h>
#include <arkui/native_node_napi.h>

#include <cmath>
#include <string>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/dom/element_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"
#include "platform/harmony/lynx_xelement/input/input_shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIBase* UITextArea::Make(LynxContext* context, int sign,
                         const std::string& tag) {
  return new UITextArea(context, ARKUI_NODE_CUSTOM, sign, tag);
}

ArkUI_TextAreaType UITextArea::ParseTextAreaType(const lepus::Value& value) {
  ArkUI_TextAreaType type = ARKUI_TEXTAREA_TYPE_NORMAL;
  if (value.IsString()) {
    const auto& val = value.StdString();
    if (val == "text") {
      type = ARKUI_TEXTAREA_TYPE_NORMAL;
    } else if (val == "number" || val == "digit") {
      type = ARKUI_TEXTAREA_TYPE_NUMBER;
    } else if (val == "tel") {
      type = ARKUI_TEXTAREA_TYPE_PHONE_NUMBER;
    } else if (val == "email") {
      type = ARKUI_TEXTAREA_TYPE_EMAIL;
    } else {
      LOGE("x-textarea-ng can not recognize a undefined type")
    }
  }
  return type;
}

void UITextArea::OnNodeReady() { UIBaseInput::OnNodeReady(); }

UITextArea::UITextArea(LynxContext* context, ArkUI_NodeType type, int sign,
                       const std::string& tag)
    : UIBaseInput(context, type, sign, tag, ARKUI_NODE_TEXT_AREA) {
  NodeManager::Instance().RegisterNodeEvent(Node(), NODE_TOUCH_EVENT,
                                            NODE_TOUCH_EVENT, this);
  NodeManager::Instance().RegisterNodeEvent(
      input_node_, NODE_TEXT_AREA_ON_CHANGE, NODE_TEXT_AREA_ON_CHANGE, this);
  NodeManager::Instance().RegisterNodeEvent(
      input_node_, NODE_TEXT_AREA_ON_TEXT_SELECTION_CHANGE,
      NODE_TEXT_AREA_ON_TEXT_SELECTION_CHANGE, this);

  NodeManager::Instance().RegisterNodeEvent(
      input_node_, NODE_TEXT_AREA_ON_SUBMIT, NODE_TEXT_AREA_ON_SUBMIT, this);

  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_TEXT_AREA_PLACEHOLDER_COLOR, INPUT_DEFAULT_COLOR);
}

UITextArea::~UITextArea() {
  NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_TOUCH_EVENT);
  NodeManager::Instance().UnregisterNodeEvent(input_node_,
                                              NODE_TEXT_AREA_ON_CHANGE);
  NodeManager::Instance().UnregisterNodeEvent(
      input_node_, NODE_TEXT_AREA_ON_TEXT_SELECTION_CHANGE);
  NodeManager::Instance().UnregisterNodeEvent(input_node_,
                                              NODE_TEXT_AREA_ON_SUBMIT);
}

void UITextArea::FrameDidChanged() {
  UIBaseInput::FrameDidChanged();
  NodeManager::Instance().SetAttributeWithNumberValue(input_node_,
                                                      NODE_POSITION, 0, 0);
  NodeManager::Instance().SetAttributeWithNumberValue(input_node_, NODE_WIDTH,
                                                      width_);
}

void UITextArea::OnPropUpdate(const std::string& name,
                              const lepus::Value& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_TEXT_AREA_ON_PROP_UPDATE);
  UIBaseInput::OnPropUpdate(name, value);
  if (name == "type") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_AREA_TYPE,
        static_cast<int32_t>(ParseTextAreaType(value)));
  } else if (name == "caret-color") {
    if (value.IsNil()) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_AREA_CARET_COLOR, 0xFF000000);
    } else {
      CSSStringParser parser =
          CSSStringParser::FromLepusString(value, CSSParserConfigs());
      CSSValue color = parser.ParseCSSColor();
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_AREA_CARET_COLOR, color.GetValue().UInt32());
    }
  } else if (name == "placeholder-color") {
    if (value.IsNil()) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_AREA_PLACEHOLDER_COLOR, INPUT_DEFAULT_COLOR);
    } else {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_AREA_PLACEHOLDER_COLOR,
          static_cast<uint32_t>(value.Number()));
    }
  } else if (name == "maxlength") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_AREA_MAX_LENGTH,
        static_cast<int32_t>(value.Number()));
  } else if (name == "maxlines") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_MAX_LINES, static_cast<int32_t>(value.Number()));
  } else if (name == "confirm-type") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_AREA_ENTER_KEY_TYPE,
        static_cast<int32_t>(ParseEnterKeyType(value)));
  } else if (name == "show-soft-input-on-focus") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_AREA_ENABLE_KEYBOARD_ON_FOCUS,
        static_cast<int32_t>(value.Bool()));

    show_soft_input_on_focus_ = value.Bool();
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_AREA_ENABLE_KEYBOARD_ON_FOCUS,
        show_soft_input_on_focus_ ? 1 : 0);
    // Not exposed from ArkUI
    // if (show_soft_input_on_focus_) {
    //   ArkUI_AttributeItem item = {.object = nullptr};
    //   NodeManager::Instance().SetAttribute(
    //     input_node_, NODE_TEXT_AREA_CUSTOM_KEYBOARD,
    //     &item);
    // } else {
    //   ArkUI_AttributeItem item = {.object = custom_keyboard_};
    //   NodeManager::Instance().SetAttribute(
    //     input_node_, NODE_TEXT_AREA_CUSTOM_KEYBOARD,
    //     &item);
    // }
  } else if (name == "line-spacing") {
    float line_spacing = 0;
    float screen_size[2] = {0};
    const int32_t width_index = 0;
    context_->ScreenSize(screen_size);
    if (value.IsString()) {
      line_spacing = LynxUnitUtils::ToVPFromUnitValue(
          value.StdString(), screen_size[width_index],
          context_->DevicePixelRatio());
    } else if (value.IsNumber()) {
      line_spacing = static_cast<float>(value.Number());
    }
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_LINE_SPACING, line_spacing);
  } else if (name == "input-filter") {
    ArkUI_AttributeItem item = {.string = value.CString()};
    NodeManager::Instance().SetAttribute(input_node_,
                                         NODE_TEXT_AREA_INPUT_FILTER, &item);
  }
}

void UITextArea::OnNodeEvent(ArkUI_NodeEvent* event) {
  UIBaseInput::OnNodeEvent(event);
  const auto type = OH_ArkUI_NodeEvent_GetEventType(event);
  if (type == NODE_TOUCH_EVENT && !readonly_) {
    auto* input_event = OH_ArkUI_NodeEvent_GetInputEvent(event);
    const auto action = OH_ArkUI_UIInputEvent_GetAction(input_event);
    if (action == UI_TOUCH_EVENT_ACTION_UP) {
      NodeManager::Instance().SetAttributeWithNumberValue(input_node_,
                                                          NODE_FOCUS_STATUS, 1);
    }
  } else if (type == NODE_TEXT_AREA_ON_CHANGE) {
    SendInputEvent();
    const int32_t line = NodeManager::Instance().GetAttribute<int32_t>(
        input_node_, NODE_TEXT_AREA_CONTENT_LINE_COUNT);
    if (line != current_lines_) {
      current_lines_ = line;
      const auto param = lepus::Dictionary::Create();
      param->SetValue("line", current_lines_);
      CustomEvent event{Sign(), "line", "detail", lepus_value(param)};
      context_->SendEvent(event);
    }

  } else if (type == NODE_TEXT_AREA_ON_TEXT_SELECTION_CHANGE) {
    auto* component_event = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
    SendSelectionChangeEvent(component_event->data[0].i32,
                             component_event->data[1].i32);
  } else if (type == NODE_TEXT_AREA_ON_SUBMIT) {
    SendConfirmEvent();
  }
}

ArkUI_NodeAttributeType UITextArea::GetTextAttributeType() const {
  return NODE_TEXT_AREA_TEXT;
}

ArkUI_NodeAttributeType UITextArea::GetPlaceholderAttributeType() const {
  return NODE_TEXT_INPUT_PLACEHOLDER_FONT;
}

ArkUI_NodeAttributeType UITextArea::GetSelectionAttributeType() const {
  return NODE_TEXT_INPUT_TEXT_SELECTION;
}

ArkUI_NodeAttributeType UITextArea::GetEditingAttributeType() const {
  return NODE_TEXT_AREA_EDITING;
}

ArkUI_NodeAttributeType UITextArea::GetPlaceholderTextType() const {
  return NODE_TEXT_AREA_PLACEHOLDER;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
