// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/input/ui_base_input.h"

#include <memory>
#include <utility>

#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"
#include "platform/harmony/lynx_xelement/input/input_shadow_node.h"
namespace lynx {
namespace tasm {
namespace harmony {

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
  NodeManager::Instance().SetAttributeWithNumberValue(
      Node(), NODE_SCROLL_BAR_DISPLAY_MODE,
      static_cast<int32_t>(ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF));
  input_node_ = NodeManager::Instance().CreateNode(input_node_type);
  custom_keyboard_ = NodeManager::Instance().CreateNode(ARKUI_NODE_CUSTOM);

  NodeManager::Instance().RegisterNodeCustomEvent(
      Node(), ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE,
      ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE, this);

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

  NodeManager::Instance().RegisterNodeEvent(
      input_node_, NODE_EVENT_ON_AREA_CHANGE, Sign(), this);
  NodeManager::Instance().RegisterNodeEvent(input_node_, NODE_ON_FOCUS, Sign(),
                                            this);
  NodeManager::Instance().RegisterNodeEvent(input_node_, NODE_ON_BLUR, Sign(),
                                            this);
  NodeManager::Instance().RegisterNodeEvent(input_node_, NODE_EVENT_ON_APPEAR,
                                            Sign(), this);
}

UIBaseInput::~UIBaseInput() {
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
  UIView::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  NodeManager::Instance().SetAttributeWithNumberValue(
      input_node_, NODE_PADDING, padding_top_, padding_right_, padding_bottom_,
      padding_left_);
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
}

void UIBaseInput::OnPropUpdate(const std::string& name,
                               const lepus::Value& value) {
  UIView::OnPropUpdate(name, value);
  if (name == "readonly") {
    readonly_ = value.Bool();
    NodeManager::Instance().SetAttributeWithNumberValue(
        input_node_, NODE_HIT_TEST_BEHAVIOR,
        static_cast<int32_t>(readonly_ ? ARKUI_HIT_TEST_MODE_NONE
                                       : ARKUI_HIT_TEST_MODE_DEFAULT));
    NodeManager::Instance().SetAttributeWithNumberValue(
        node_, NODE_HIT_TEST_BEHAVIOR,
        static_cast<int32_t>(readonly_ ? ARKUI_HIT_TEST_MODE_NONE
                                       : ARKUI_HIT_TEST_MODE_DEFAULT));
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
  } else if (name == "placeholder-font-size") {
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
  } else if (name == "placeholder-font-style") {
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
  } else if (name == "placeholder-font-weight") {
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
  } else if (name == "placeholder-font-family") {
    placeholder_font_family_ = value.StdString();
  } else if (name == "placeholder") {
    if (value.IsNil()) {
      placeholder_ = "";
    } else {
      placeholder_ = value.StdString();
    }
  }
}

void UIBaseInput::OnNodeReady() {
  UIView::OnNodeReady();
  SetupFont();
  SetupPlaceholderFont();
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
  NodeManager::Instance().SetAttribute(input_node_, GetPlaceholderTextType(),
                                       &placeholder);
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
  if (type == NODE_EVENT_ON_APPEAR) {
  } else if (type == NODE_EVENT_ON_AREA_CHANGE) {
  } else if (type == NODE_ON_FOCUS) {
    SendFocusEvent();
    context_->SetFocusedTarget(weak_from_this());
  } else if (type == NODE_ON_BLUR) {
    SendBlurEvent();
    context_->UnsetFocusedTarget(weak_from_this());
  }
}

void UIBaseInput::OnFocusChange(bool has_focus, bool is_focus_transition) {
  if (!has_focus) {
    if (!is_focus_transition) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          input_node_, GetEditingAttributeType(), 0);
      NodeManager::Instance().SetAttributeWithNumberValue(input_node_,
                                                          NODE_FOCUS_STATUS, 0);
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
    font_collection = std::make_shared<FontCollectionHarmony>();
  }

  paragraph_style.SetLineSpacingScale((font_size + line_spacing) / font_size);

  std::unique_ptr<ParagraphBuilderHarmony> builder =
      std::make_unique<ParagraphBuilderHarmony>(&paragraph_style,
                                                font_collection.get());

  TextStyleHarmony text_style;
  text_style.SetFontSize(font_size);

  if (!font_family.empty()) {
    text_style.SetFontFamiliesToStyle(font_family);
  }

  text_style.SetFontWeight(static_cast<OH_Drawing_FontWeight>(font_weight));
  text_style.SetFontStyle(static_cast<OH_Drawing_FontStyle>(font_style));
  builder->PushTextStyle(text_style);
  builder->AddText(value.c_str());
  builder->PopTextStyle();
  auto paragraph = builder->CreateParagraph(font_collection, 0);
  paragraph->Layout(max_width);

  float height = paragraph->GetHeight();

  return height;
}

void UIBaseInput::SendInputEvent() const {
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
  CustomEvent event{Sign(), "input", "detail", lepus_value(param)};
  context_->SendEvent(event);
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

std::unordered_map<std::string, UIBaseInput::UIMethod>
    UIBaseInput::input_base_ui_method_map_ = {
        {"focus", &UIBaseInput::Focus},
        {"blur", &UIBaseInput::Blur},
        {"setValue", &UIBaseInput::SetValue},
        {"getValue", &UIBaseInput::GetValue},
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
        auto item = (ArkUI_AttributeItem){.string = std::move(value.CString())};
        NodeManager::Instance().SetAttribute(input_node_,
                                             GetTextAttributeType(), &item);
        callback(LynxGetUIResult::SUCCESS, lepus::Value());
        return;
      }
    }
  }
  const auto ret = lepus::Dictionary::Create();
  ret->SetValue("err", "value is not assigned");
  callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
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

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
