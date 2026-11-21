// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/input/ui_base_input.h"

#include <harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "base/include/string/string_utils.h"
#include "base/include/value/base_string.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_builder_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"
#include "platform/harmony/lynx_xelement/input/input_shadow_node.h"
namespace lynx {
namespace tasm {
namespace harmony {

namespace {
OH_Drawing_FontWeight ToDrawingFontWeight(int32_t weight) {
  // ArkUI keyword weights follow W900; Drawing only accepts W100 through W900.
  switch (weight) {
    case ARKUI_FONT_WEIGHT_BOLD:
      return FONT_WEIGHT_700;
    case ARKUI_FONT_WEIGHT_BOLDER:
      return FONT_WEIGHT_900;
    case ARKUI_FONT_WEIGHT_LIGHTER:
      return FONT_WEIGHT_100;
    case ARKUI_FONT_WEIGHT_MEDIUM:
      return FONT_WEIGHT_500;
    case ARKUI_FONT_WEIGHT_NORMAL:
    case ARKUI_FONT_WEIGHT_REGULAR:
      return FONT_WEIGHT_400;
    default:
      return static_cast<OH_Drawing_FontWeight>(weight);
  }
}
}  // namespace

ArkUI_EnterKeyType UIBaseInput::ParseEnterKeyType(const lepus::Value& value) {
  ArkUI_EnterKeyType type = ARKUI_ENTER_KEY_TYPE_DONE;
  if (value.IsString()) {
    const auto& val = value.StdString();
    if (val == "send") {
      type = ARKUI_ENTER_KEY_TYPE_SEND;
    } else if (val == "search") {
      type = ARKUI_ENTER_KEY_TYPE_SEARCH;
    } else if (val == "go") {
      type = ARKUI_ENTER_KEY_TYPE_GO;
    } else if (val == "done") {
      type = ARKUI_ENTER_KEY_TYPE_DONE;
    } else if (val == "next") {
      type = ARKUI_ENTER_KEY_TYPE_NEXT;
    } else {
      LOGE("x-input-ng can not recognize a undefined confirm-type")
    }
  }
  return type;
}

UIBaseInput::UIBaseInput(LynxContext* context, ArkUI_NodeType type, int sign,
                         const std::string& tag, ArkUI_NodeType input_node_type)
    : UIView(context, type, sign, tag) {
  const auto input_context = context_->GetNativePaintingContext();
  text_edit_enabled_ = input_context && input_context->IsRawInputEnabled();
  text_edit_vertical_ = input_node_type == ARKUI_NODE_TEXT_AREA;
  NodeManager::Instance().SetAttributeWithNumberValue(
      Node(), NODE_SCROLL_BAR_DISPLAY_MODE,
      static_cast<int32_t>(ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF));
  input_node_ = NodeManager::Instance().CreateNode(input_node_type);
  custom_keyboard_ = NodeManager::Instance().CreateNode(ARKUI_NODE_CUSTOM);

  NodeManager::Instance().RegisterNodeCustomEvent(
      Node(), ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE, this);

  NodeManager::Instance().AddNodeEventReceiver(input_node_,
                                               UIBase::EventReceiver);
  NodeManager::Instance().InsertNode(Node(), input_node_, 0);
  NodeManager::Instance().SetAttributeWithNumberValue(Node(), NODE_CLIP, 1);

  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_BACKGROUND_COLOR, 0x00000000);
  NodeManager::Instance().SetAttributeWithNumberValue(input_node_, NODE_PADDING,
                                                      0.0, 0.0, 0.0, 0.0);
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_BORDER_RADIUS, 0.0, 0.0, 0.0, 0.0);

  NodeManager::Instance().RegisterNodeEvent(input_node_,
                                            NODE_EVENT_ON_AREA_CHANGE, this);
  NodeManager::Instance().RegisterNodeEvent(input_node_, NODE_ON_FOCUS, this);
  NodeManager::Instance().RegisterNodeEvent(input_node_, NODE_ON_BLUR, this);
  NodeManager::Instance().RegisterNodeEvent(input_node_, NODE_EVENT_ON_APPEAR,
                                            this);
  if (text_edit_enabled_) {
    NodeManager::Instance().RegisterNodeCustomEvent(
        Node(), ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT, this);
    NodeManager::Instance().RegisterNodeEvent(
        input_node_,
        text_edit_vertical_ ? NODE_TEXT_AREA_ON_CONTENT_SIZE_CHANGE
                            : NODE_TEXT_INPUT_ON_CONTENT_SIZE_CHANGE,
        this);
  }
}

UIBaseInput::~UIBaseInput() {
  if (text_edit_enabled_) {
    NodeManager::Instance().UnregisterNodeCustomEvent(
        Node(), ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT);
    NodeManager::Instance().UnregisterNodeEvent(
        input_node_, text_edit_vertical_
                         ? NODE_TEXT_AREA_ON_CONTENT_SIZE_CHANGE
                         : NODE_TEXT_INPUT_ON_CONTENT_SIZE_CHANGE);
  }
  NodeManager::Instance().UnregisterNodeCustomEvent(
      Node(), ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE);
  NodeManager::Instance().UnregisterNodeEvent(input_node_,
                                              NODE_EVENT_ON_AREA_CHANGE);
  NodeManager::Instance().UnregisterNodeEvent(input_node_,
                                              NODE_EVENT_ON_APPEAR);

  NodeManager::Instance().UnregisterNodeEvent(input_node_, NODE_ON_FOCUS);
  NodeManager::Instance().UnregisterNodeEvent(input_node_, NODE_ON_BLUR);

  NodeManager::Instance().RemoveNodeEventReceiver(input_node_,
                                                  UIBase::EventReceiver);
  NodeManager::Instance().DisposeNode(input_node_);
  NodeManager::Instance().DisposeNode(custom_keyboard_);
  context_->UnsetFocusedTarget(weak_from_this());
}

bool UIBaseInput::Focusable() { return !readonly_; }

void UIBaseInput::UpdateLayout(float left, float top, float width, float height,
                               const float* paddings, const float* margins,
                               const float* sticky, float max_height,
                               uint32_t node_index) {
  const float previous_content_width = width_ - padding_left_ - padding_right_;
  const float previous_height = height_;
  const float previous_padding_top = padding_top_;
  const float previous_padding_bottom = padding_bottom_;
  UIView::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  if (background_drawable_) {
    padding_top_ = background_drawable_->GetBorderTopWidth() + padding_top_;
    padding_left_ = background_drawable_->GetBorderLeftWidth() + padding_left_;
    padding_bottom_ =
        background_drawable_->GetBorderBottomWidth() + padding_bottom_;
    padding_right_ =
        background_drawable_->GetBorderRightWidth() + padding_right_;
  }
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_PADDING, padding_top_, padding_right_, padding_bottom_,
      padding_left_);
  const bool text_layout_changed =
      previous_content_width != width_ - padding_left_ - padding_right_;
  if (text_layout_changed || previous_height != height_ ||
      previous_padding_top != padding_top_ ||
      previous_padding_bottom != padding_bottom_) {
    InvalidateTextEditGeometry(text_layout_changed);
  }
  context_->FindShadowNodeAndRunTask(Sign(), [this](ShadowNode* node) {
    InputShadowNode* shadow_node = reinterpret_cast<InputShadowNode*>(node);
    float max_height = shadow_node->ComputedMaxHeight();
    if (max_height == LayoutNodeStyle::UNDEFINED_MAX_SIZE) {
      max_height = INPUT_UNDEFINED_FLOAT;
    }
    if (max_height != this->max_height_) {
      this->max_height_ = max_height;
    }
    float computed_height = shadow_node->ComputedHeight();
    // TODO: We can not distinguish between computed_height == 0 and
    // computed_height == auto
    if (computed_height == 0) {
      computed_height = INPUT_UNDEFINED_FLOAT;
    }
    if (computed_height != this->computed_height_) {
      this->computed_height_ = computed_height;
    }
  });
  context_->GetUIOwner()->KeyboardAvoidingInputDidLayout(
      this, avoid_keyboard_in_lynx_view_, avoid_keyboard_spacing_in_lynx_view_);
}

void UIBaseInput::OnPropUpdate(const std::string& name,
                               const lepus::Value& value) {
  UIView::OnPropUpdate(name, value);
  if (name == "font-size" || name == "font-style" || name == "font-weight" ||
      name == "font-family" || name == "text-align" || name == "line-spacing" ||
      name == "type") {
    InvalidateTextEditGeometry();
  }
  if (!keyboard_event_observer_registered_) {
    keyboard_event_observer_registered_ = true;
    context_->GetUIOwner()->AddKeyboardEventObserver(Sign());
  }

  if (name == "disabled") {
    bool disabled = value.Bool();
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_HIT_TEST_BEHAVIOR,
        static_cast<int32_t>(disabled ? ARKUI_HIT_TEST_MODE_NONE
                                      : ARKUI_HIT_TEST_MODE_DEFAULT));
    NodeManager::Instance().SetAttributeWithNumberValue(
        node_, NODE_HIT_TEST_BEHAVIOR,
        static_cast<int32_t>(disabled ? ARKUI_HIT_TEST_MODE_NONE
                                      : ARKUI_HIT_TEST_MODE_DEFAULT));
    readonly_ = disabled;
    if (disabled) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, GetEditingAttributeType(), 0);
      NodeManager::Instance().SetAttributeWithNumberValue(input_node_,
                                                          NODE_FOCUS_STATUS, 0);
    }
  } else if (name == "readonly") {
    readonly_ = value.Bool();
  } else if (name == "default-value") {
    if (default_value_consumed_ && !text_edit_enabled_) {
      return;
    }
    if (value.IsNil() || value.IsString()) {
      const std::string text = value.IsString() ? value.StdString() : "";
      const std::string current_text =
          NodeManager::Instance().GetAttribute<std::string>(
              input_node_, GetTextAttributeType());
      if (current_text == text) {
        return;
      }
      should_suppress_default_value_events_ = true;
      SetTextValue(text);
      ScheduleClearDefaultValueEventSuppression();
    }
  } else if (name == "text-align") {
    int32_t align = static_cast<int32_t>(value.Number());
    if (align == 0 || align == 3) {  // left && start
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_ALIGN,
          static_cast<int32_t>(ARKUI_TEXT_ALIGNMENT_START));
    } else if (align == 2 || align == 4) {  // right && end
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_ALIGN,
          static_cast<int32_t>(ARKUI_TEXT_ALIGNMENT_END));
    } else if (align == 1) {  // center
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_ALIGN,
          static_cast<int32_t>(ARKUI_TEXT_ALIGNMENT_CENTER));
    } else if (align == 5) {  // justify
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_TEXT_ALIGN,
          static_cast<int32_t>(ARKUI_TEXT_ALIGNMENT_JUSTIFY));
    }
  } else if (name == "font-size") {
    if (value.IsNil()) {
      font_size_ = INPUT_UNDEFINED_FLOAT;
    } else {
      font_size_ = value.Number();
    }
  } else if (name == "placeholder-font-size" ||
             name == "-x-placeholder-font-size") {
    if (value.IsNil()) {
      placeholder_font_size_ = INPUT_UNDEFINED_FLOAT;
    } else {
      placeholder_font_size_ = value.Number();
    }
  } else if (name == "color") {
    if (value.IsNil()) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_FONT_COLOR, 0xFFFFFFFF);
    } else {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, NODE_FONT_COLOR, static_cast<uint32_t>(value.Number()));
    }
  } else if (name == "font-style") {
    int32_t intValue = static_cast<int32_t>(value.Number());
    if (intValue != 0 && intValue != 1) {
      font_style_ = INPUT_UNDEFINED_INT;
    } else {
      font_style_ = intValue;
    }
  } else if (name == "placeholder-font-style" ||
             name == "-x-placeholder-font-style") {
    int32_t intValue = static_cast<int32_t>(value.Number());
    if (intValue != 0 && intValue != 1) {
      placeholder_font_style_ = INPUT_UNDEFINED_INT;
    } else {
      placeholder_font_style_ = intValue;
    }
  } else if (name == "font-weight") {
    int32_t font_weight = static_cast<int32_t>(value.Number());
    int32_t ark_ui_font_weight = ARKUI_FONT_WEIGHT_NORMAL;
    if (font_weight == 0) {
      ark_ui_font_weight = ARKUI_FONT_WEIGHT_NORMAL;
    } else if (font_weight == 1) {
      ark_ui_font_weight = ARKUI_FONT_WEIGHT_BOLD;
    } else if (font_weight >= 2 && font_weight <= 10) {
      ark_ui_font_weight = font_weight - 2;
    } else {
      ark_ui_font_weight = INPUT_UNDEFINED_INT;
    }
    font_weight_ = ark_ui_font_weight;
  } else if (name == "placeholder-font-weight" ||
             name == "-x-placeholder-font-weight") {
    int32_t font_weight = static_cast<int32_t>(value.Number());
    int32_t ark_ui_font_weight = ARKUI_FONT_WEIGHT_NORMAL;
    if (font_weight == 0) {
      ark_ui_font_weight = ARKUI_FONT_WEIGHT_NORMAL;
    } else if (font_weight == 1) {
      ark_ui_font_weight = ARKUI_FONT_WEIGHT_BOLD;
    } else if (font_weight >= 2 && font_weight <= 10) {
      ark_ui_font_weight = font_weight - 2;
    } else {
      ark_ui_font_weight = INPUT_UNDEFINED_INT;
    }
    placeholder_font_weight_ = ark_ui_font_weight;
  } else if (name == "font-family") {
    font_family_ = value.StdString();
  } else if (name == "placeholder-font-family" ||
             name == "-x-placeholder-font-family") {
    placeholder_font_family_ = value.StdString();
  } else if (name == "placeholder") {
    if (value.IsNil()) {
      placeholder_ = "";
    } else {
      placeholder_ = value.StdString();
    }
  } else if (name == "confirm-enter") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, GetBlurOnSubmitAttributeType(),
        static_cast<uint32_t>(!value.Bool()));
  } else if (name == "set-soft-input-mode") {
    if (!value.IsString()) {
      return;
    }
    context_->GetUIOwner()->SetSoftInputMode(value.StdString());
  } else if (name == "avoid-keyboard") {
    avoid_keyboard_in_lynx_view_ = value.Bool();
    context_->GetUIOwner()->AvoidKeyboardPropsDidChangeForOwner(
        this, avoid_keyboard_in_lynx_view_,
        avoid_keyboard_spacing_in_lynx_view_);
  } else if (name == "avoid-keyboard-spacing") {
    if (value.IsString()) {
      float screen_size[2] = {0};
      const int32_t width_index = 0;
      context_->ScreenSize(screen_size);
      avoid_keyboard_spacing_in_lynx_view_ = LynxUnitUtils::ToVPFromUnitValue(
          value.StdString(), screen_size[width_index],
          context_->DevicePixelRatio());
    } else if (value.IsNumber()) {
      avoid_keyboard_spacing_in_lynx_view_ = static_cast<float>(value.Number());
    }
    context_->GetUIOwner()->AvoidKeyboardPropsDidChangeForOwner(
        this, avoid_keyboard_in_lynx_view_,
        avoid_keyboard_spacing_in_lynx_view_);
  } else if (name == "hold-keyboard") {
    hold_keyboard_ = value.Bool();
  }
}

void UIBaseInput::OnNodeReady() {
  UIView::OnNodeReady();
  SetupFont();
  SetupPlaceholderFont();
  default_value_consumed_ = true;
  bool focused = NodeManager::Instance().GetAttribute<int>(
                     input_node_, NODE_FOCUS_STATUS) == 1;
  if (focused) {
    context_->GetUIOwner()->KeyboardAvoidingInputDidFocus(
        this, avoid_keyboard_in_lynx_view_,
        avoid_keyboard_spacing_in_lynx_view_);
  }
}

void UIBaseInput::SetupFont() {
  float font_size = font_size_;
  if (font_size == INPUT_UNDEFINED_FLOAT) {
    font_size = 14.f;
  }
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_FONT_SIZE, font_size);

  int32_t font_style = font_style_;
  if (font_style == INPUT_UNDEFINED_INT) {
    font_style = 0;
  }

  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_FONT_STYLE, font_style);

  int32_t font_weight = font_weight_;
  if (font_weight == INPUT_UNDEFINED_INT) {
    font_weight = ARKUI_FONT_WEIGHT_NORMAL;
  }
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_FONT_WEIGHT, font_weight);

  ArkUI_AttributeItem item = {.string = font_family_.c_str()};
  NodeManager::Instance().SetAttribute(input_node_, NODE_FONT_FAMILY, &item);
}

void UIBaseInput::SetupPlaceholderFont() {
  float placeholder_font_size = placeholder_font_size_;
  if (placeholder_font_size == INPUT_UNDEFINED_FLOAT) {
    placeholder_font_size = NodeManager::Instance()
                                .GetAttribute(input_node_, NODE_FONT_SIZE)
                                ->value[0]
                                .f32;
  }
  int32_t placeholder_font_style = placeholder_font_style_;
  if (placeholder_font_style == INPUT_UNDEFINED_INT) {
    placeholder_font_style = NodeManager::Instance()
                                 .GetAttribute(input_node_, NODE_FONT_STYLE)
                                 ->value[0]
                                 .i32;
  }
  int32_t placeholder_font_weight = placeholder_font_weight_;
  if (placeholder_font_weight == INPUT_UNDEFINED_INT) {
    placeholder_font_weight = NodeManager::Instance()
                                  .GetAttribute(input_node_, NODE_FONT_WEIGHT)
                                  ->value[0]
                                  .i32;
  }
  std::string placeholder_font_family = placeholder_font_family_;
  if (placeholder_font_family.empty()) {
    placeholder_font_family = NodeManager::Instance()
                                  .GetAttribute(input_node_, NODE_FONT_FAMILY)
                                  ->string;
  }

  ArkUI_AttributeItem item{};
  ArkUI_NumberValue value[] = {{.f32 = placeholder_font_size},
                               {.i32 = placeholder_font_style},
                               {.i32 = placeholder_font_weight}};
  item.value = value;
  item.string = placeholder_font_family.c_str();
  item.size = sizeof(value) / sizeof(ArkUI_NumberValue);
  NodeManager::Instance().SetAttribute(input_node_,
                                       GetPlaceholderAttributeType(), &item);

  ArkUI_AttributeItem placeholder = {.string = placeholder_.c_str()};
  NodeManager::Instance().SetAttribute(
      input_node_, GetPlaceholderTextAttributeType(), &placeholder);
}

void UIBaseInput::OnMeasure(ArkUI_LayoutConstraint* layout_constraint) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UIBASE_INPUT_ON_MEASURE);

  ArkUI_LayoutConstraint* indefinite_constraint =
      OH_ArkUI_LayoutConstraint_Create();
  OH_ArkUI_LayoutConstraint_SetMinHeight(indefinite_constraint, 0);
  OH_ArkUI_LayoutConstraint_SetMaxHeight(indefinite_constraint, INT_MAX);

  OH_ArkUI_LayoutConstraint_SetMinWidth(indefinite_constraint,
                                        context_->ScaledDensity() * width_);
  OH_ArkUI_LayoutConstraint_SetMaxWidth(indefinite_constraint,
                                        context_->ScaledDensity() * width_);
  NodeManager::Instance().MeasureNode(input_node_, indefinite_constraint);
  OH_ArkUI_LayoutConstraint_Dispose(indefinite_constraint);

  float ui_height = OnArkUILayoutChanged();

  if (text_edit_enabled_) {
    // ArkUI already shapes the editor. Its content-size event drives expansion;
    // build the separate hit-test paragraph only when a pointer command needs
    // it.
    const float content_width = text_content_width_;
    const float content_height = text_content_height_;
    const float scale = context_->ScaledDensity();
    const float content_editor_width =
        text_edit_vertical_
            ? width_
            : std::max(width_, content_width + padding_left_ + padding_right_ +
                                   1.f / scale);
    const float content_editor_height =
        text_edit_vertical_
            ? std::max({height_, ui_height,
                        content_height + padding_top_ + padding_bottom_})
            : height_;
    const float editor_width = std::ceil(content_editor_width * scale) / scale;
    const float editor_height =
        std::ceil(content_editor_height * scale) / scale;
    ArkUI_LayoutConstraint* constraint = OH_ArkUI_LayoutConstraint_Create();
    OH_ArkUI_LayoutConstraint_SetMinWidth(constraint,
                                          std::ceil(editor_width * scale));
    OH_ArkUI_LayoutConstraint_SetMaxWidth(constraint,
                                          std::ceil(editor_width * scale));
    OH_ArkUI_LayoutConstraint_SetMinHeight(constraint,
                                           std::ceil(editor_height * scale));
    OH_ArkUI_LayoutConstraint_SetMaxHeight(constraint,
                                           std::ceil(editor_height * scale));
    NodeManager::Instance().MeasureNode(input_node_, constraint);
    OH_ArkUI_LayoutConstraint_Dispose(constraint);
    NodeManager::Instance().SetMeasuredSize(Node(), width_ * scale,
                                            height_ * scale);
    text_edit_maximum_ =
        text_edit_vertical_ ? editor_height - height_ : editor_width - width_;
    ApplyTextEditOffset(text_edit_offset_);
    ScheduleTextEditState();
    return;
  }

  ArkUI_LayoutConstraint* definite_constraint =
      OH_ArkUI_LayoutConstraint_Create();
  OH_ArkUI_LayoutConstraint_SetMinHeight(definite_constraint, 0);
  OH_ArkUI_LayoutConstraint_SetMaxHeight(definite_constraint,
                                         context_->ScaledDensity() * height_);

  if ((ui_height > height_ || height_ - ui_height < 1.0f) &&
      (max_height_ == INPUT_UNDEFINED_FLOAT || ui_height < max_height_)) {
    OH_ArkUI_LayoutConstraint_SetMaxHeight(
        definite_constraint, context_->ScaledDensity() * ui_height);
  }

  if (computed_height_ != INPUT_UNDEFINED_FLOAT) {
    OH_ArkUI_LayoutConstraint_SetMaxHeight(
        definite_constraint, context_->ScaledDensity() * computed_height_);
  } else if (max_height_ != INPUT_UNDEFINED_FLOAT && ui_height > max_height_) {
    OH_ArkUI_LayoutConstraint_SetMaxHeight(
        definite_constraint, context_->ScaledDensity() * max_height_);
  }

  OH_ArkUI_LayoutConstraint_SetMinWidth(definite_constraint,
                                        context_->ScaledDensity() * width_);
  OH_ArkUI_LayoutConstraint_SetMaxWidth(definite_constraint,
                                        context_->ScaledDensity() * width_);
  NodeManager::Instance().MeasureNode(input_node_, definite_constraint);
  OH_ArkUI_LayoutConstraint_Dispose(definite_constraint);

  NodeManager::Instance().SetMeasuredSize(Node(),
                                          context_->ScaledDensity() * width_,
                                          context_->ScaledDensity() * height_);
}

void UIBaseInput::OnNodeEvent(ArkUI_NodeEvent* event) {
  UIView::OnNodeEvent(event);
  auto type = OH_ArkUI_NodeEvent_GetEventType(event);
  if (text_edit_enabled_) {
    const auto change = text_edit_vertical_ ? NODE_TEXT_AREA_ON_CHANGE
                                            : NODE_TEXT_INPUT_ON_CHANGE;
    const auto selection = text_edit_vertical_
                               ? NODE_TEXT_AREA_ON_TEXT_SELECTION_CHANGE
                               : NODE_TEXT_INPUT_ON_TEXT_SELECTION_CHANGE;
    const auto content_size = text_edit_vertical_
                                  ? NODE_TEXT_AREA_ON_CONTENT_SIZE_CHANGE
                                  : NODE_TEXT_INPUT_ON_CONTENT_SIZE_CHANGE;
    if (type == content_size) {
      const auto* data = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
      if (text_content_width_ != data->data[0].f32 ||
          text_content_height_ != data->data[1].f32) {
        text_content_width_ = data->data[0].f32;
        text_content_height_ = data->data[1].f32;
        ++text_edit_cursor_revision_;
        NodeManager::Instance().RequestLayout(Node());
        ScheduleTextEditState();
      }
    } else if (type == change || type == selection || type == NODE_ON_FOCUS ||
               type == NODE_ON_BLUR) {
      if (type == change) {
        text_edit_layout_dirty_ = true;
        text_content_width_ = 0;
        text_content_height_ = 0;
        NodeManager::Instance().RequestLayout(Node());
      }
      ensure_caret_visible_ =
          type == change || type == NODE_ON_FOCUS ||
          (type == selection && selection_anchor_start_ < 0);
      ++text_edit_cursor_revision_;
      ScheduleTextEditState();
    } else if (type == NODE_EVENT_ON_AREA_CHANGE) {
      ScheduleTextEditState();
    }
  }
  if (type == NODE_EVENT_ON_APPEAR) {
  } else if (type == NODE_EVENT_ON_AREA_CHANGE) {
  } else if (type == NODE_ON_FOCUS) {
    context_->GetUIOwner()->KeyboardAvoidingInputDidFocus(
        this, avoid_keyboard_in_lynx_view_,
        avoid_keyboard_spacing_in_lynx_view_);
    SendFocusEvent();
    context_->SetFocusedTarget(weak_from_this());
  } else if (type == NODE_ON_BLUR) {
    context_->GetUIOwner()->KeyboardAvoidingInputDidBlur(this, false);
    SendBlurEvent();
    context_->UnsetFocusedTarget(weak_from_this());
  } else if (type == GetOnWillInsertEventType() ||
             type == GetOnWillDeleteEventType()) {
    ArkUI_NumberValue value[] = {
        {.i32 = readonly_ ? 0 : 1},
    };
    OH_ArkUI_NodeEvent_SetReturnNumberValue(event, value, 1);
  } else if (type == GetOnPasteEventType()) {
    is_pasting_ = true;
  }
}

void UIBaseInput::OnFocusChange(bool has_focus, bool is_focus_transition) {
  if (has_focus) {
    context_->GetUIOwner()->KeyboardAvoidingInputDidFocus(
        this, avoid_keyboard_in_lynx_view_,
        avoid_keyboard_spacing_in_lynx_view_);
  } else {
    context_->GetUIOwner()->KeyboardAvoidingInputDidBlur(this,
                                                         is_focus_transition);
    if (!is_focus_transition) {
      if (!hold_keyboard_) {
        NodeManager::Instance().SetAttributeWithNumberValue(
            input_node_, GetEditingAttributeType(), 0);
        NodeManager::Instance().SetAttributeWithNumberValue(
            input_node_, NODE_FOCUS_STATUS, 0);
      }
    }
  }
}

float UIBaseInput::OnArkUILayoutChanged() {
  int32_t ui_height =
      NodeManager::Instance().GetMeasuredSize(input_node_).height /
      GetContext()->ScaledDensity();

  float placeholder_font_size = placeholder_font_size_;
  if (placeholder_font_size == INPUT_UNDEFINED_FLOAT) {
    placeholder_font_size = NodeManager::Instance()
                                .GetAttribute(input_node_, NODE_FONT_SIZE)
                                ->value[0]
                                .f32;
  }

  std::string place_holder_font_family = placeholder_font_family_;
  if (place_holder_font_family.empty()) {
    place_holder_font_family = NodeManager::Instance()
                                   .GetAttribute(input_node_, NODE_FONT_FAMILY)
                                   ->string;
  }

  int32_t placeholder_font_style = placeholder_font_style_;
  if (placeholder_font_style == INPUT_UNDEFINED_INT) {
    placeholder_font_style = NodeManager::Instance()
                                 .GetAttribute(input_node_, NODE_FONT_STYLE)
                                 ->value[0]
                                 .i32;
  }
  int32_t placeholder_font_weight = placeholder_font_weight_;
  if (placeholder_font_weight == INPUT_UNDEFINED_INT) {
    placeholder_font_weight = NodeManager::Instance()
                                  .GetAttribute(input_node_, NODE_FONT_WEIGHT)
                                  ->value[0]
                                  .i32;
  }

  int32_t width_used_by_measure =
      width_for_measure_ != INPUT_UNDEFINED_FLOAT ? width_for_measure_ : width_;

  float placeholder_height = MeasureTextHeight(
      placeholder_font_size, width_used_by_measure, placeholder_font_weight,
      placeholder_font_style, 0, place_holder_font_family, placeholder_);

  float font_size = NodeManager::Instance()
                        .GetAttribute(input_node_, NODE_FONT_SIZE)
                        ->value[0]
                        .f32;
  int32_t font_style = NodeManager::Instance()
                           .GetAttribute(input_node_, NODE_FONT_STYLE)
                           ->value[0]
                           .i32;
  int32_t font_weight = NodeManager::Instance()
                            .GetAttribute(input_node_, NODE_FONT_WEIGHT)
                            ->value[0]
                            .i32;
  std::string font_family = NodeManager::Instance()
                                .GetAttribute(input_node_, NODE_FONT_FAMILY)
                                ->string;

  float min_content_height =
      MeasureTextHeight(font_size, width_used_by_measure, font_weight,
                        font_style, 0, font_family, " ");

  if (placeholder_height > ui_height) {
    ui_height = placeholder_height;
  }

  if (min_content_height > ui_height) {
    ui_height = min_content_height;
  }

  context_->FindShadowNodeAndRunTask(Sign(), [ui_height](ShadowNode* node) {
    InputShadowNode* shadow_node = reinterpret_cast<InputShadowNode*>(node);
    if (shadow_node->UIHeight() != ui_height) {
      shadow_node->SetUIHeight(ui_height);
      shadow_node->RequestLayout();
    }
  });

  return ui_height;
}

int32_t UIBaseInput::MeasureTextHeight(float font_size, float max_width,
                                       int font_weight, int font_style,
                                       int line_spacing,
                                       std::string font_family,
                                       std::string value) {
  ParagraphStyleHarmony paragraph_style;

  std::shared_ptr<FontFaceManager> font_face_manager =
      context_->GetFontFaceManager();
  std::shared_ptr<FontCollectionHarmony> font_collection = nullptr;
  if (font_face_manager != nullptr) {
    font_collection = font_face_manager->GetFontCollection();
  }
  if (font_collection == nullptr) {
    font_collection = FontCollectionHarmony::MakeSharedFontCollectionHarmony();
    ;
  }

  paragraph_style.SetLineSpacingScale((font_size + line_spacing) / font_size);

  std::unique_ptr<ParagraphBuilderHarmony> builder =
      std::make_unique<ParagraphBuilderHarmony>(&paragraph_style,
                                                font_collection.get());

  TextStyleHarmony text_style;
  text_style.SetFontSize(font_size);

  if (!font_family.empty()) {
    auto family_vec =
        font_face_manager->GetCustomFamiliesFromRawString(font_family);
    text_style.SetCustomFontFamilyVector(std::move(family_vec));
  }

  text_style.SetFontWeight(ToDrawingFontWeight(font_weight));
  text_style.SetFontStyle(static_cast<OH_Drawing_FontStyle>(font_style));
  builder->PushTextStyle(text_style);
  builder->AddText(value.c_str());
  builder->PopTextStyle();
  auto paragraph = builder->CreateParagraph(font_collection, 0);
  paragraph->Layout(max_width);

  float height = paragraph->GetHeight();

  return height;
}

void UIBaseInput::SendInputEvent() {
  const auto value = NodeManager::Instance().GetAttribute<std::string>(
      input_node_, GetTextAttributeType());

  bool focused = NodeManager::Instance().GetAttribute<int>(
                     input_node_, NODE_FOCUS_STATUS) == 1;

  auto cursor = NodeManager::Instance().GetAttribute(
      input_node_, GetSelectionAttributeType());

  auto selectionStart = focused ? cursor->value[0].i32 : -1;
  auto selectionEnd = focused ? cursor->value[1].i32 : -1;

  const auto param = lepus::Dictionary::Create();
  param->SetValue("value", value);
  param->SetValue("selectionStart", selectionStart);
  param->SetValue("selectionEnd", selectionEnd);
  param->SetValue("isComposing", false);
  param->SetValue("inputType", std::string(is_pasting_ ? "paste" : "normal"));
  CustomEvent event{Sign(), "input", "detail", lepus_value(param)};
  context_->SendEvent(event);
  is_pasting_ = false;
}

void UIBaseInput::SendSelectionChangeEvent(const int32_t start,
                                           const int32_t end) const {
  const auto param = lepus::Dictionary::Create();
  param->SetValue("selectionStart", start);
  param->SetValue("selectionEnd", end);
  // param->SetValue("direction", start <= end ? "forward" : "backward");
  CustomEvent event{Sign(), "selection", "detail", lepus_value(param)};
  context_->SendEvent(event);
}

void UIBaseInput::SendConfirmEvent() const {
  const std::string& value = NodeManager::Instance().GetAttribute<std::string>(
      input_node_, GetTextAttributeType());
  const auto param = lepus::Dictionary::Create();
  param->SetValue("value", value);
  CustomEvent event{Sign(), "confirm", "detail", lepus_value(param)};
  context_->SendEvent(event);
}

void UIBaseInput::SendFocusEvent() const {
  const std::string& value = NodeManager::Instance().GetAttribute<std::string>(
      input_node_, GetTextAttributeType());
  const auto param = lepus::Dictionary::Create();
  param->SetValue("value", value);
  CustomEvent event{Sign(), "focus", "detail", lepus_value(param)};
  context_->SendEvent(event);
}

void UIBaseInput::SendBlurEvent() const {
  const std::string& value = NodeManager::Instance().GetAttribute<std::string>(
      input_node_, GetTextAttributeType());
  const auto param = lepus::Dictionary::Create();
  param->SetValue("value", value);
  CustomEvent event{Sign(), "blur", "detail", lepus_value(param)};
  context_->SendEvent(event);
}

void UIBaseInput::SendKeyboardHeightChangedEvent(float height) const {
  const std::string& value = NodeManager::Instance().GetAttribute<std::string>(
      input_node_, GetTextAttributeType());
  const auto param = lepus::Dictionary::Create();
  param->SetValue("height", height);
  CustomEvent event{Sign(), "keyboardheightchange", "detail",
                    lepus_value(param)};
  context_->SendEvent(event);
}

std::unordered_map<std::string, UIBaseInput::UIMethod>
    UIBaseInput::input_base_ui_method_map_ = {
        {"focus", &UIBaseInput::Focus},
        {"blur", &UIBaseInput::Blur},
        {"setValue", &UIBaseInput::SetValue},
        {"getValue", &UIBaseInput::GetValue},
        {"textEditCommand", &UIBaseInput::ApplyTextEditCommand},
        {"setSelectionRange", &UIBaseInput::SetSelectionRange}};

void UIBaseInput::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto it = input_base_ui_method_map_.find(method);
      it != input_base_ui_method_map_.end()) {
    (this->*it->second)(args, std::move(callback));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UIBaseInput::Focus(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, GetEditingAttributeType(), 1);
  NodeManager::Instance().SetAttributeWithNumberValue(input_node_,
                                                      NODE_FOCUS_STATUS, 1);
  callback(LynxGetUIResult::SUCCESS, lepus::Value());
}

void UIBaseInput::Blur(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  context_->GetUIOwner()->KeyboardAvoidingInputDidBlur(this, false);
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, GetEditingAttributeType(), 0);
  NodeManager::Instance().SetAttributeWithNumberValue(input_node_,
                                                      NODE_FOCUS_STATUS, 0);
  callback(LynxGetUIResult::SUCCESS, lepus::Value());
}
void UIBaseInput::SetValue(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (args.IsTable()) {
    const auto params = args.Table();
    if (params->Contains("value")) {
      const auto& value = params->GetValue("value");
      if (value.IsString()) {
        SetTextValue(value.StdString());
        if (params->Contains("cursor")) {
          const auto& cursor = params->GetValue("cursor");
          if (!cursor.IsNumber()) {
            const auto ret = lepus::Dictionary::Create();
            ret->SetValue("err", "cursor must be a number");
            callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
            return;
          }
          const double cursor_value = cursor.Number();
          if (!std::isfinite(cursor_value) ||
              std::trunc(cursor_value) != cursor_value ||
              cursor_value > std::numeric_limits<int32_t>::max()) {
            const auto ret = lepus::Dictionary::Create();
            ret->SetValue(
                "err",
                "cursor must be a finite integer no greater than INT32_MAX");
            callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
            return;
          }
          if (cursor_value >= 0) {
            const auto cursor_index = static_cast<int32_t>(cursor_value);
            ArkUI_NumberValue caret[] = {{.i32 = cursor_index}};
            ArkUI_AttributeItem item = {.value = caret, .size = 1};
            const bool success = NodeManager::Instance().SetAttribute(
                input_node_, GetCaretAttributeType(), &item);
            if (!success) {
              const auto ret = lepus::Dictionary::Create();
              ret->SetValue("err", "failed to set caret position");
              callback(LynxGetUIResult::OPERATION_ERROR, lepus::Value(ret));
              return;
            }
          }
        }
        callback(LynxGetUIResult::SUCCESS, lepus::Value());
        return;
      }
    }
  }
  const auto ret = lepus::Dictionary::Create();
  ret->SetValue("err", "value is not assigned");
  callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
}

void UIBaseInput::SetTextValue(const std::string& value) {
  ArkUI_AttributeItem item = {.string = value.c_str()};
  NodeManager::Instance().SetAttribute(input_node_, GetTextAttributeType(),
                                       &item);
  InvalidateTextEditGeometry();
}

void UIBaseInput::OnLayout() {
  if (text_edit_enabled_) {
    NodeManager::Instance().LayoutNode(input_node_, 0, 0);
    ApplyTextEditOffset(text_edit_offset_);
    ScheduleTextEditState();
  }
}

void UIBaseInput::InvalidateTextEditGeometry(bool text_layout_changed) {
  if (!text_edit_enabled_) {
    return;
  }
  if (text_layout_changed) {
    text_edit_layout_dirty_ = true;
    text_content_width_ = 0;
    text_content_height_ = 0;
  }
  ++text_edit_cursor_revision_;
  NodeManager::Instance().RequestLayout(Node());
  ScheduleTextEditState();
}

void UIBaseInput::EnsureTextEditParagraph() {
  if (text_edit_paragraph_ && !text_edit_layout_dirty_) {
    return;
  }
  auto& manager = NodeManager::Instance();
  auto fonts = context_->GetFontFaceManager();
  auto collection = fonts ? fonts->GetFontCollection() : nullptr;
  if (!collection) {
    collection = FontCollectionHarmony::MakeSharedFontCollectionHarmony();
  }
  ParagraphStyleHarmony paragraph_style;
  paragraph_style.SetTextMaxLines(text_edit_vertical_ ? INT_MAX : 1);
  const int align = manager.GetAttribute<int>(input_node_, NODE_TEXT_ALIGN);
  const OH_Drawing_TextAlign alignments[] = {
      TEXT_ALIGN_START,   TEXT_ALIGN_CENTER, TEXT_ALIGN_END,
      TEXT_ALIGN_JUSTIFY, TEXT_ALIGN_START,  TEXT_ALIGN_START};
  if (align == ARKUI_TEXT_ALIGNMENT_RIGHT_TO_LEFT) {
    paragraph_style.SetTextDirection(TEXT_DIRECTION_RTL);
  }
  if (align >= 0 && align < 6) {
    paragraph_style.SetTextAlign(alignments[align]);
  }
  TextStyleHarmony style;
  const float font_size =
      manager.GetAttribute<float>(input_node_, NODE_FONT_SIZE);
  if (text_edit_vertical_ && font_size > 0) {
    const float spacing =
        manager.GetAttribute<float>(input_node_, NODE_TEXT_LINE_SPACING);
    paragraph_style.SetLineSpacingScale((font_size + spacing) / font_size);
  }
  style.SetFontSize(font_size);
  style.SetFontWeight(ToDrawingFontWeight(
      manager.GetAttribute<int>(input_node_, NODE_FONT_WEIGHT)));
  style.SetFontStyle(static_cast<OH_Drawing_FontStyle>(
      manager.GetAttribute<int>(input_node_, NODE_FONT_STYLE)));
  const auto family =
      manager.GetAttribute<std::string>(input_node_, NODE_FONT_FAMILY);
  if (fonts && !family.empty()) {
    style.SetCustomFontFamilyVector(
        fonts->GetCustomFamiliesFromRawString(family));
  }
  ParagraphBuilderHarmony builder(&paragraph_style, collection.get());
  builder.PushTextStyle(style);
  builder.AddText(
      manager.GetAttribute<std::string>(input_node_, GetTextAttributeType())
          .c_str());
  builder.PopTextStyle();
  text_edit_paragraph_ = builder.CreateParagraph(collection, 0);
  const float width =
      text_edit_vertical_
          ? std::max(0.f, width_ - padding_left_ - padding_right_)
          : std::numeric_limits<float>::max();
  text_edit_paragraph_->Layout(width);
  if (!text_edit_vertical_) {
    text_edit_paragraph_->Layout(
        std::max(width_ - padding_left_ - padding_right_,
                 text_edit_paragraph_->GetLongestLine()));
  }
  text_edit_layout_dirty_ = false;
}

void UIBaseInput::ApplyTextEditOffset(float offset) {
  text_edit_offset_ = std::clamp(offset, 0.f, text_edit_maximum_);
  // The editor measures its full content. Compose projects the visible slice
  // through this clipped container without starting a native scroll gesture.
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_TRANSLATE,
      text_edit_vertical_ ? 0.f : -text_edit_offset_,
      text_edit_vertical_ ? -text_edit_offset_ : 0.f, 0.f);
}

void UIBaseInput::ScheduleTextEditState() {
  if (!text_edit_enabled_ || text_edit_state_pending_) {
    return;
  }
  text_edit_state_pending_ = true;
  // ArkUI does not expose an updated caret until the next frame.
  base::HarmonyVsyncManager::GetInstance().RequestVSync(
      [weak = weak_from_this()](long long) {
        if (const auto ui = weak.lock()) {
          ui->GetContext()->PostTaskOnUIThread([weak] {
            const auto self =
                std::static_pointer_cast<UIBaseInput>(weak.lock());
            if (self) {
              self->text_edit_state_pending_ = false;
              self->SendTextEditState();
            }
          });
        }
      });
}

void UIBaseInput::SendTextEditState() {
  if (!text_edit_enabled_ || width_ <= 0 || height_ <= 0) {
    return;
  }
  const float scale = context_->ScaledDensity();
  auto& manager = NodeManager::Instance();
  const bool focused =
      manager.GetAttribute<int>(input_node_, NODE_FOCUS_STATUS) == 1;
  const auto* selection =
      manager.GetAttribute(input_node_, GetSelectionAttributeType());
  // A range has no insertion caret. Reporting ArkUI's selection end would
  // bring the inactive anchor back into view after a reverse selection.
  const bool collapsed = focused && selection && selection->value &&
                         selection->size >= 2 &&
                         selection->value[0].i32 == selection->value[1].i32;
  // ArkUI may reuse the attribute buffer on the next GetAttribute call.
  const auto* caret =
      collapsed ? manager.GetAttribute(input_node_, GetCaretAttributeType())
                : nullptr;
  const bool has_caret = caret && caret->value && caret->size >= 3;
  float caret_x = 0;
  float caret_y = 0;
  if (has_caret) {
    caret_x = caret->value[1].f32;
    caret_y = caret->value[2].f32;
    if (ensure_caret_visible_) {
      const float start = text_edit_vertical_ ? caret_y : caret_x;
      const float extent = text_edit_vertical_ ? height_ - padding_bottom_
                                               : width_ - padding_right_;
      const float leading = text_edit_vertical_ ? padding_top_ : padding_left_;
      const float caret_size =
          text_edit_vertical_
              ? manager.GetAttribute<float>(input_node_, NODE_FONT_SIZE)
              : 1.f / scale;
      float offset = text_edit_offset_;
      if (start < offset + leading) {
        offset = start - leading;
      } else if (start + caret_size > offset + extent) {
        offset = start + caret_size - extent;
      }
      ApplyTextEditOffset(offset);
    }
  }
  ensure_caret_visible_ = false;
  BASE_STATIC_STRING_DECL(kCommandId, "commandId");
  BASE_STATIC_STRING_DECL(kOffsetPx, "offsetPx");
  BASE_STATIC_STRING_DECL(kMaximumPx, "maximumPx");
  BASE_STATIC_STRING_DECL(kFocused, "focused");
  BASE_STATIC_STRING_DECL(kCursorRevision, "cursorRevision");
  BASE_STATIC_STRING_DECL(kCursorLeftPx, "cursorLeftPx");
  BASE_STATIC_STRING_DECL(kCursorTopPx, "cursorTopPx");
  BASE_STATIC_STRING_DECL(kCursorRightPx, "cursorRightPx");
  BASE_STATIC_STRING_DECL(kCursorBottomPx, "cursorBottomPx");
  const auto detail = lepus::Dictionary::Create();
  detail->SetValue(kCommandId, text_edit_command_id_);
  detail->SetValue(kOffsetPx, text_edit_offset_ * scale);
  detail->SetValue(kMaximumPx, text_edit_maximum_ * scale);
  detail->SetValue(kFocused, focused);
  detail->SetValue(kCursorRevision, text_edit_cursor_revision_);
  if (has_caret) {
    caret_x -= text_edit_vertical_ ? 0 : text_edit_offset_;
    caret_y -= text_edit_vertical_ ? text_edit_offset_ : 0;
    const float font_size =
        manager.GetAttribute<float>(input_node_, NODE_FONT_SIZE);
    detail->SetValue(kCursorLeftPx, caret_x * scale);
    detail->SetValue(kCursorTopPx, caret_y * scale);
    detail->SetValue(kCursorRightPx, caret_x * scale + 1);
    detail->SetValue(kCursorBottomPx, (caret_y + font_size) * scale);
  }
  // cspell:ignore texteditstate
  CustomEvent event{Sign(), "texteditstate", "detail", lepus::Value(detail)};
  context_->SendEvent(event);
}

void UIBaseInput::ApplyTextEditCommand(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (!text_edit_enabled_ || readonly_ || !args.IsTable()) {
    callback(LynxGetUIResult::OPERATION_ERROR, lepus::Value());
    return;
  }
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kCommandId, "commandId");
  BASE_STATIC_STRING_DECL(kXPx, "xPx");
  BASE_STATIC_STRING_DECL(kYPx, "yPx");
  BASE_STATIC_STRING_DECL(kOffsetPx, "offsetPx");
  const auto type = args.GetProperty(kType);
  const auto command = args.GetProperty(kCommandId);
  const auto x = args.GetProperty(kXPx);
  const auto y = args.GetProperty(kYPx);
  const auto offset = args.GetProperty(kOffsetPx);
  if (!type.IsString() || !command.IsNumber() || !x.IsNumber() ||
      !y.IsNumber() || !offset.IsNumber() || !std::isfinite(x.Number()) ||
      !std::isfinite(y.Number()) || !std::isfinite(offset.Number())) {
    callback(LynxGetUIResult::PARAM_INVALID, lepus::Value());
    return;
  }
  const auto& operation = type.StdString();
  if (operation != "scroll" && operation != "tap" && operation != "select" &&
      operation != "extendSelection" && operation != "endSelection") {
    callback(LynxGetUIResult::PARAM_INVALID, lepus::Value());
    return;
  }
  const double command_number = command.Number();
  if (!std::isfinite(command_number) ||
      command_number < std::numeric_limits<int32_t>::min() ||
      command_number > std::numeric_limits<int32_t>::max() ||
      std::trunc(command_number) != command_number) {
    callback(LynxGetUIResult::PARAM_INVALID, lepus::Value());
    return;
  }
  const auto command_id = static_cast<int32_t>(command_number);
  if (command_id <= text_edit_command_id_) {
    callback(LynxGetUIResult::SUCCESS, lepus::Value());
    return;
  }
  text_edit_command_id_ = command_id;
  auto& manager = NodeManager::Instance();
  if (operation == "scroll") {
    ApplyTextEditOffset(offset.Number() / context_->ScaledDensity());
  } else if (operation == "endSelection") {
    selection_anchor_start_ = -1;
    ensure_caret_visible_ = false;
  } else if (manager.GetAttribute<int>(input_node_, NODE_FOCUS_STATUS) == 1 &&
             (operation != "extendSelection" || selection_anchor_start_ >= 0)) {
    const float local_x = x.Number() / context_->ScaledDensity() -
                          padding_left_ +
                          (text_edit_vertical_ ? 0 : text_edit_offset_);
    const float local_y = y.Number() / context_->ScaledDensity() -
                          padding_top_ +
                          (text_edit_vertical_ ? text_edit_offset_ : 0);
    const bool password =
        !text_edit_vertical_ &&
        manager.GetAttribute<int>(input_node_, NODE_TEXT_INPUT_TYPE) ==
            ARKUI_TEXTINPUT_TYPE_NEW_PASSWORD;
    int position = 0;
    int text_length = 0;
    if (password) {
      text_length = base::SizeOfUtf16(manager.GetAttribute<std::string>(
          input_node_, GetTextAttributeType()));
      const auto* rect =
          manager.GetAttribute(input_node_, NODE_TEXT_INPUT_CONTENT_RECT);
      if (rect && rect->value && rect->value[2].f32 > 0) {
        // ArkUI masks each UTF-16 unit. Native bounds retain the actual mask
        // width and alignment; shaping the clear text would hit another index.
        position = std::clamp(
            static_cast<int>(
                std::round((local_x + padding_left_ - rect->value[0].f32) *
                           text_length / rect->value[2].f32)),
            0, text_length);
      }
    } else {
      EnsureTextEditParagraph();
      position = text_edit_paragraph_->GetPositionAndAffinity(local_x, local_y)
                     .GetPosition();
    }
    int start = position;
    int end = position;
    if (operation == "tap") {
      selection_anchor_start_ = -1;
    } else if (operation == "select") {
      if (password) {
        start = 0;
        end = text_length;
      } else {
        auto* word = OH_Drawing_TypographyGetWordBoundary(
            text_edit_paragraph_->GetRawStruct(), position);
        if (word) {
          start = OH_Drawing_GetStartFromRange(word);
          end = OH_Drawing_GetEndFromRange(word);
        }
      }
      selection_anchor_start_ = start;
      selection_anchor_end_ = end;
    } else if (operation == "extendSelection" && selection_anchor_start_ >= 0) {
      start = std::min(position, selection_anchor_start_);
      end = std::max(position, selection_anchor_end_);
    }
    if (operation == "tap") {
      // ArkUI's caret setter also clears an existing range.
      manager.SetAttributeWithNumberValue(input_node_, GetCaretAttributeType(),
                                          position);
    } else {
      manager.SetAttributeWithNumberValue(
          input_node_, GetSelectionAttributeType(), start, end);
    }
    // The gesture owner scrolls the active selection edge. ArkUI's caret
    // follows the selection end, which stays at the anchor when extending
    // backwards.
    ensure_caret_visible_ = operation == "tap";
    ++text_edit_cursor_revision_;
  }
  ScheduleTextEditState();
  callback(LynxGetUIResult::SUCCESS, lepus::Value());
}

bool UIBaseInput::ShouldSuppressDefaultValueEvent() const {
  return should_suppress_default_value_events_;
}

void UIBaseInput::ScheduleClearDefaultValueEventSuppression() {
  if (!context_ || !context_->GetUITaskRunner()) {
    should_suppress_default_value_events_ = false;
    return;
  }
  context_->PostTaskOnUIThread([weak_self = weak_from_this()] {
    auto self = weak_self.lock();
    if (!self) {
      return;
    }
    auto input = std::static_pointer_cast<UIBaseInput>(self);
    input->should_suppress_default_value_events_ = false;
  });
}

void UIBaseInput::GetValue(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  const std::string value = NodeManager::Instance().GetAttribute<std::string>(
      input_node_, GetTextAttributeType());
  const auto ret = lepus::Dictionary::Create();
  ret->SetValue("value", std::move(value));
  bool focused = NodeManager::Instance().GetAttribute<int>(
                     input_node_, NODE_FOCUS_STATUS) == 1;

  auto cursor = NodeManager::Instance().GetAttribute(
      input_node_, GetSelectionAttributeType());

  auto selectionStart = focused ? cursor->value[0].i32 : -1;
  auto selectionEnd = focused ? cursor->value[1].i32 : -1;

  ret->SetValue("selectionStart", selectionStart);
  ret->SetValue("selectionEnd", selectionEnd);
  ret->SetValue("isComposing", false);

  callback(LynxGetUIResult::SUCCESS, lepus::Value(ret));
}

void UIBaseInput::SetSelectionRange(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (args.IsTable()) {
    const auto params = args.Table();
    const auto& start = params->GetValue("selectionStart");
    const auto& end = params->GetValue("selectionEnd");
    if (start.IsNumber() && end.IsNumber()) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, GetSelectionAttributeType(),
          static_cast<int32_t>(start.Number()),
          static_cast<int32_t>(end.Number()));

      callback(LynxGetUIResult::SUCCESS, lepus::Value());
      return;
    }
  }
  const auto ret = lepus::Dictionary::Create();
  ret->SetValue("err", "selection is not assigned");
  callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
}

void UIBaseInput::OnKeyboardWillShow(float height) {
  bool focused = NodeManager::Instance().GetAttribute<int>(
                     input_node_, NODE_FOCUS_STATUS) == 1;
  if (focused) {
    context_->GetUIOwner()->KeyboardWillShowForOwner(
        this, height, avoid_keyboard_in_lynx_view_,
        avoid_keyboard_spacing_in_lynx_view_);
    SendKeyboardHeightChangedEvent(height);
  }
}

void UIBaseInput::OnKeyboardWillHide() {
  if (context_->GetUIOwner()->KeyboardWillHideForOwner(this)) {
    SendKeyboardHeightChangedEvent(0);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
