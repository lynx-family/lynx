// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/input/ui_input.h"

#include <arkui/native_node.h>
#include <arkui/native_node_napi.h>

#include <cfloat>
#include <cmath>
#include <string>
#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/dom/element_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIBase* UIInput::Make(LynxContext* context, int sign, const std::string& tag) {
  return new UIInput(context, ARKUI_NODE_CUSTOM, sign, tag);
}

ArkUI_TextInputType UIInput::ParseInputType(const lepus::Value& value) {
  ArkUI_TextInputType type = ARKUI_TEXTINPUT_TYPE_NORMAL;
  if (value.IsString()) {
    auto& val = value.StdString();
    if (val == "text") {
      type = ARKUI_TEXTINPUT_TYPE_NORMAL;
    } else if (val == "number") {
      type = ARKUI_TEXTINPUT_TYPE_NUMBER;
    } else if (val == "digit") {
      type = ARKUI_TEXTINPUT_TYPE_NUMBER_DECIMAL;
    } else if (val == "password") {
      type = ARKUI_TEXTINPUT_TYPE_NEW_PASSWORD;
    } else if (val == "tel") {
      type = ARKUI_TEXTINPUT_TYPE_PHONE_NUMBER;
    } else if (val == "email") {
      type = ARKUI_TEXTINPUT_TYPE_EMAIL;
    } else {
      LOGE("x-input-ng can not recognize a undefined type")
    }
  }
  return type;
}

void UIInput::OnPropUpdate(const std::string& name, const lepus::Value& value) {
  UIBaseInput::OnPropUpdate(name, value);
  if (name == "type") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_INPUT_TYPE,
        static_cast<int32_t>(ParseInputType(value)));
  } else if (name == "caret-color") {
    if (value.IsNil()) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_AREA_CARET_COLOR, 0xFF000000);
    } else {
      CSSStringParser parser =
          CSSStringParser::FromLepusString(value, CSSParserConfigs());
      CSSValue color = parser.ParseCSSColor();
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_INPUT_CARET_COLOR, color.GetValue().UInt32());
    }
  } else if (name == "placeholder-color") {
    if (value.IsNil()) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_INPUT_PLACEHOLDER_COLOR, INPUT_DEFAULT_COLOR);
    } else {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_INPUT_PLACEHOLDER_COLOR,
          static_cast<uint32_t>(value.Number()));
    }
  } else if (name == "maxlength") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_INPUT_MAX_LENGTH,
        static_cast<int32_t>(value.Number()));
  } else if (name == "confirm-type") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_INPUT_ENTER_KEY_TYPE,
        static_cast<int32_t>(ParseEnterKeyType(value)));
  } else if (name == "show-soft-input-on-focus") {
    show_soft_input_on_focus_ = value.Bool();
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_TEXT_INPUT_ENABLE_KEYBOARD_ON_FOCUS,
        show_soft_input_on_focus_ ? 1 : 0);
    if (show_soft_input_on_focus_) {
      ArkUI_AttributeItem item = {.object = nullptr};
      NodeManager::Instance().SetAttribute(
          input_node_, NODE_TEXT_INPUT_CUSTOM_KEYBOARD, &item);
    } else {
      ArkUI_AttributeItem item = {.object = custom_keyboard_};
      NodeManager::Instance().SetAttribute(
          input_node_, NODE_TEXT_INPUT_CUSTOM_KEYBOARD, &item);
    }
  } else if (name == "input-filter") {
    ArkUI_AttributeItem item = {.string = value.CString()};
    NodeManager::Instance().SetAttribute(input_node_,
                                         NODE_TEXT_INPUT_INPUT_FILTER, &item);
  }
}

void UIInput::OnNodeReady() { UIBaseInput::OnNodeReady(); }

UIInput::UIInput(LynxContext* context, ArkUI_NodeType type, int sign,
                 const std::string& tag)
    : UIBaseInput(context, type, sign, tag, ARKUI_NODE_TEXT_INPUT) {
  width_for_measure_ = FLT_MAX;
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_TEXT_INPUT_ENABLE_KEYBOARD_ON_FOCUS, 1);
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_TEXT_INPUT_SHOW_PASSWORD_ICON, 0);
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_TEXT_INPUT_MAX_LENGTH, 140);
  NodeManager::Instance().RegisterNodeEvent(Node(), NODE_TOUCH_EVENT,
                                            NODE_TOUCH_EVENT, this);
  NodeManager::Instance().RegisterNodeEvent(
      input_node_, NODE_TEXT_INPUT_ON_CHANGE, NODE_TEXT_INPUT_ON_CHANGE, this);

  NodeManager::Instance().RegisterNodeEvent(
      input_node_, NODE_TEXT_INPUT_ON_TEXT_SELECTION_CHANGE,
      NODE_TEXT_INPUT_ON_TEXT_SELECTION_CHANGE, this);

  NodeManager::Instance().RegisterNodeEvent(
      input_node_, NODE_TEXT_INPUT_ON_SUBMIT, NODE_TEXT_INPUT_ON_SUBMIT, this);

  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_TEXT_INPUT_PLACEHOLDER_COLOR, INPUT_DEFAULT_COLOR);
}

UIInput::~UIInput() {
  NodeManager::Instance().UnregisterNodeEvent(input_node_,
                                              NODE_EVENT_ON_AREA_CHANGE);
  NodeManager::Instance().UnregisterNodeEvent(input_node_,
                                              NODE_TEXT_INPUT_ON_CHANGE);
  NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_TOUCH_EVENT);
  NodeManager::Instance().UnregisterNodeEvent(
      input_node_, NODE_TEXT_INPUT_ON_TEXT_SELECTION_CHANGE);
  NodeManager::Instance().UnregisterNodeEvent(input_node_,
                                              NODE_TEXT_INPUT_ON_SUBMIT);
}

void UIInput::OnNodeEvent(ArkUI_NodeEvent* event) {
  UIBaseInput::OnNodeEvent(event);
  const auto type = OH_ArkUI_NodeEvent_GetEventType(event);
  if (type == NODE_TOUCH_EVENT && !readonly_) {
    auto* input_event = OH_ArkUI_NodeEvent_GetInputEvent(event);
    const auto action = OH_ArkUI_UIInputEvent_GetAction(input_event);
    if (action == UI_TOUCH_EVENT_ACTION_UP) {
      NodeManager::Instance().SetAttributeWithNumberValue(input_node_,
                                                          NODE_FOCUS_STATUS, 1);
    }
  } else if (type == NODE_TEXT_INPUT_ON_CHANGE) {
    SendInputEvent();
  } else if (type == NODE_TEXT_INPUT_ON_TEXT_SELECTION_CHANGE) {
    auto* component_event = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);

    SendSelectionChangeEvent(component_event->data[0].i32,
                             component_event->data[1].i32);
  } else if (type == NODE_TEXT_INPUT_ON_SUBMIT) {
    SendConfirmEvent();
  }
}

ArkUI_NodeAttributeType UIInput::GetTextAttributeType() const {
  return NODE_TEXT_INPUT_TEXT;
}

ArkUI_NodeAttributeType UIInput::GetPlaceholderAttributeType() const {
  return NODE_TEXT_INPUT_PLACEHOLDER_FONT;
}

ArkUI_NodeAttributeType UIInput::GetSelectionAttributeType() const {
  return NODE_TEXT_INPUT_TEXT_SELECTION;
}

ArkUI_NodeAttributeType UIInput::GetEditingAttributeType() const {
  return NODE_TEXT_INPUT_EDITING;
}

ArkUI_NodeAttributeType UIInput::GetPlaceholderTextType() const {
  return NODE_TEXT_INPUT_PLACEHOLDER;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
