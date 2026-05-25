// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/editable_view.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "base/include/string/string_utils.h"
#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/editing_misc.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/editable/ime_utils.h"
#include "clay/ui/component/editable/text_editing_controller.h"
#include "clay/ui/component/editable/text_span.h"
#include "clay/ui/component/editable/text_utils.h"
#include "clay/ui/component/layout_controller.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/text_paragraph_builder.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/lynx_module/type_utils.h"
#include "clay/ui/painter/gradient.h"
#include "clay/ui/platform/keyboard_types.h"
#include "clay/ui/rendering/editable/render_editable.h"
#include "clay/ui/resource/font_collection.h"
#if defined(CLAY_ENABLE_TTTEXT)
#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"
#endif

namespace clay {
namespace {

// All in ARGB
constexpr uint32_t kDefaultPlaceholderColor = 0xffaaaaaa;

FontWeight ToPlaceHolderWeight(const std::string& font_weight_val) {
  FontWeight font_weight = FontWeight::kNormal;
  if (font_weight_val == "normal") {
    font_weight = FontWeight::kNormal;
  } else if (font_weight_val == "bold") {
    font_weight = FontWeight::kBold;
  } else if (font_weight_val == "100") {
    font_weight = FontWeight::k100;
  } else if (font_weight_val == "200") {
    font_weight = FontWeight::k200;
  } else if (font_weight_val == "300") {
    font_weight = FontWeight::k300;
  } else if (font_weight_val == "400") {
    font_weight = FontWeight::k400;
  } else if (font_weight_val == "500") {
    font_weight = FontWeight::k500;
  } else if (font_weight_val == "600") {
    font_weight = FontWeight::k600;
  } else if (font_weight_val == "700") {
    font_weight = FontWeight::k700;
  } else if (font_weight_val == "800") {
    font_weight = FontWeight::k800;
  } else if (font_weight_val == "900") {
    font_weight = FontWeight::k900;
  }
  return font_weight;
}

LYNX_UI_METHOD_BEGIN(EditableView) {
  LYNX_UI_METHOD(EditableView, focus);
  LYNX_UI_METHOD(EditableView, blur);
  LYNX_UI_METHOD(EditableView, setValue);
  LYNX_UI_METHOD(EditableView, getValue);
  LYNX_UI_METHOD(EditableView, setSelectionRange);
}
LYNX_UI_METHOD_END(EditableView);

// All in pixels.
constexpr float kDefaultFontSizeInDip = 14.f;
constexpr float kDefaultLineHeight = 1.2f;

// All in ARGB
constexpr uint32_t kDefaultEditableTextColor = 0xff000000;

constexpr uint64_t kCaretTwinkleIntervalMs = 500;

// Hot key tags.
constexpr uint32_t kTagCommand = 1;
constexpr uint32_t kTagControl = 1 << 1;
constexpr uint32_t kTagShift = 1 << 2;

uint32_t AddTag(uint32_t value, uint32_t tag) { return value | tag; }

uint32_t RemoveTag(uint32_t value, uint32_t tag) { return value & ~tag; }

FontWeight ToFontWeight(int font_weight_val) {
  return static_cast<FontWeight>(font_weight_val);
}

}  // namespace

// Returns units filtered. If nothing changed, returns 0.
size_t EditableView::FilterInput(TextEditingValue* value,
                                 const std::string& pattern) {
  auto input = value->GetText();
  auto origin_length = input.length();
  text_input_controller_->InputFilterAsync(
      input, pattern,
      [weak_self = weak_factory_.GetWeakPtr(), origin_length,
       edit_value =
           TextEditingValue(*value)](const std::string& result) mutable {
        if (!weak_self) {
          return;
        }
        auto editable_view = static_cast<EditableView*>(weak_self.get());
        edit_value.SetTextAndReserveSelectionState(result);
        if (origin_length - result.length()) {
          editable_view->UpdateRemoteStateIfNeeded(edit_value);
        }
      });
  return 0;
}

size_t EditableView::FilterNewLine(TextEditingValue* value) {
  // May costs time.
  static auto pattern = std::string("\n");
  return FilterInput(value, pattern);
}

size_t EditableView::FilterMaxLength(TextEditingValue* value) {
  size_t new_value_length = value->GetU16Length();
  const auto& text_editing_value = GetTextEditingValue();
  size_t old_value_length = text_editing_value.GetU16Length();
  bool is_composing = value->composing();

  if (!is_composing && new_value_length > max_length_) {
    text_editing_controller_->SetNeedUpdateRemote(true);
    if (old_value_length == max_length_) {
      *value = GetTextEditingValue();
      UpdateRemoteStateIfNeeded(*value);
      return 0;
    } else {
      // We need to consider truncating the surrogate(such as emoji) in the
      // middle.
      auto u16_str = value->GetU16Text().substr(0, max_length_);
      auto u32_str = lynx::base::U16StringToU32(u16_str);
      u16_str = lynx::base::U32StringToU16(u32_str);
      value->SetText(u16_str, TextRange(u16_str.length()));
      UpdateRemoteStateIfNeeded(*value);
      return new_value_length - max_length_;
    }
  }
  return 0;
}

size_t EditableView::FilterInputTextByType(TextEditingValue* value) {
  switch (keyboard_input_type_) {
    case KeyboardInputType::kClassNumber: {
      if (max_lines_ > 1) {
        static auto pattern = std::string("[^0-9\\.\n]");
        return FilterInput(value, pattern);
      } else {
        static auto pattern = std::string("[^0-9\\.]");
        return FilterInput(value, pattern);
      }
    }
    case KeyboardInputType::kClassPhone: {
      if (max_lines_ > 1) {
        static auto pattern = std::string("[^0-9\n]");
        return FilterInput(value, pattern);
      } else {
        static auto pattern = std::string("[^0-9]");
        return FilterInput(value, pattern);
      }
    }
    default:
      return 0;
  }
}

size_t EditableView::FilterInputTextByUser(TextEditingValue* value) {
  return FilterInput(value, input_filter_pattern_);
}

EditableView::EditableView(int id, std::string tag, PageView* page_view,
                           bool is_multiline, bool layout_root_candidate)
    : EditableView(id, id, tag, page_view, is_multiline,
                   layout_root_candidate) {}

EditableView::EditableView(int id, int callback_id, std::string tag,
                           PageView* page_view, bool is_multiline,
                           bool layout_root_candidate)
    : WithTypeInfo(id, tag, std::make_unique<RenderEditable>(), page_view),
      callback_id_(callback_id),
      is_multiline_(is_multiline),
      layout_root_candidate_(layout_root_candidate) {
  text_editing_controller_ = std::make_unique<TextEditingController>();
  text_editing_controller_->AddObserver(this);

  GetRenderEditable()->SetTextEditingController(text_editing_controller_.get());
  GetRenderEditable()->SetTextInputControllerDelegate(this);
  GetRenderEditable()->SetMultiline(is_multiline_);

  SetFocusable(true);

  InitDefaultStyle();

  ResetGestureRecognizers();

  text_input_controller_ =
      std::make_unique<TextInputController>(page_view, callback_id_, this);
  text_editing_history_state_ = std::make_unique<TextEditingHistoryState>(this);

#if defined(OS_WIN) || defined(OS_MAC)
  SetCursor({"text"});
#endif
}

EditableView::~EditableView() {
  page_view()->StopInput(this);
  if (text_editing_controller_) {
    text_editing_controller_->RemoveObserver(this);
  }
}

void EditableView::InitDefaultStyle() {
  text_style_.text_align = TextAlignment::kLeft;
  text_style_.text_color = Color(kDefaultEditableTextColor);
  text_style_.font_size = GetDefaultFontSize();
  text_style_.strut_font_size = GetDefaultFontSize();
  text_style_.strut_height = kDefaultLineHeight;
  text_style_.strut_enabled = true;
}

void EditableView::OnContentSizeChanged(const FloatRect& old_rect,
                                        const FloatRect& new_rect) {
  MarkNeedsLayout();
}

LayoutContextMeasure EditableView::CreateLayoutContext(
    const MeasureConstraint& constraint) {
  LayoutContextMeasure context;
  switch (constraint.width_mode) {
    case TextMeasureMode::kIndefinite:
      context.layout_width = std::numeric_limits<float>::infinity();
      break;
    case TextMeasureMode::kAtMost:
    case TextMeasureMode::kDefinite:
      context.layout_width = *constraint.width;
      break;
  }
  return context;
}

void EditableView::OnLayout(LayoutContext* context) {
  MeasureConstraint constraint = {Width(), MeasureMode::kDefinite, Height(),
                                  MeasureMode::kDefinite};
  auto layout_context = CreateLayoutContext(constraint);
  if (!context) {
    context = &layout_context;
  }
  const auto& text_editing_value = GetTextEditingValue();
  if (text_editing_value.empty() && !GetPlaceholder().empty()) {
    // Layout placeholder
    TextStyle temp_style = text_style_;
    temp_style.strut_enabled = std::nullopt;
#if defined(CLAY_ENABLE_TTTEXT) && (defined(OS_WIN) || defined(OS_MAC))
    temp_style.font_size =
        placeholder_font_size_.value_or(GetDefaultFontSize());
    temp_style.text_color =
        placeholder_color_.value_or(Color(kDefaultPlaceholderColor));
    temp_style.font_weight =
        placeholder_font_weight_.value_or(FontWeight::kNormal);
    if (temp_style.font_size.has_value() &&
        temp_style.line_height.has_value()) {
      temp_style.line_height = *temp_style.font_size * *temp_style.line_height;
    }
    auto builder = std::make_unique<TextParagraphBuilder>(true, temp_style);
#else
    auto builder = std::make_unique<TextParagraphBuilder>(true, temp_style);
    temp_style.font_size =
        placeholder_font_size_.value_or(GetDefaultFontSize());
    temp_style.text_color =
        placeholder_color_.value_or(Color(kDefaultPlaceholderColor));
    temp_style.font_weight =
        placeholder_font_weight_.value_or(FontWeight::kNormal);
#endif
    builder->PushStyle(temp_style);
    builder->AddText(lynx::base::U8StringToU16(GetPlaceholder()));
    builder->Pop();
    paragraph_ = Build(std::move(builder));
    GetRenderEditable()->SetPlaceholderLineHeight(
        *temp_style.font_size *
        text_style_.line_height.value_or(kDefaultLineHeight));
    LayoutText(context);
    SetPlaceholderHeight(paragraph_->GetHeight());
  } else {
    // Layout content
    TextStyle temp_style = text_style_;
#if defined(CLAY_ENABLE_TTTEXT) && (defined(OS_WIN) || defined(OS_MAC))
    float line_height = *temp_style.font_size *
                        text_style_.line_height.value_or(kDefaultLineHeight);
    if (text_style_.strut_enabled.value_or(false) &&
        text_style_.strut_height.has_value()) {
      line_height =
          text_style_.strut_font_size.value_or(*temp_style.font_size) *
          static_cast<float>(*text_style_.strut_height);
    }
    temp_style.line_height = line_height;
#endif
    auto builder = std::make_unique<TextParagraphBuilder>(true, temp_style);
    BuildTextSpan(temp_style)->Build(*builder);
    paragraph_ = Build(std::move(builder));
#if defined(CLAY_ENABLE_TTTEXT)
    auto* impl = static_cast<txt::ParagraphTTText*>(paragraph_.get());
    impl->SetNeedTrimSpace(true);
#endif
    LayoutText(context);
  }

  GetRenderEditable()->SetParagraph(paragraph_.get());
  GetRenderEditable()->SetRoughTextLineHeight(
      *text_style_.font_size *
      text_style_.line_height.value_or(kDefaultLineHeight));
}

float EditableView::EstimateHeightWithMaxLines() {
  // TODO(yulitao): Simply use template paragraph to do it.
  // This can be refined to be more accurate.
  bool infinite = max_lines_ == std::numeric_limits<uint32_t>::max();
  // Note: treat unlimited lines as one line height.
  float text_height;
  if (!paragraph_) {
    if (!template_paragraph_ || GetTextEditingValue().empty()) {
      auto builder = std::make_unique<TextParagraphBuilder>(true, text_style_);
      builder->PushStyle(text_style_);
      builder->AddText(u" ");
      builder->Pop();
      template_paragraph_ = Build(std::move(builder));
      template_paragraph_->Layout(std::numeric_limits<float>::infinity());
      GetRenderEditable()->SetDefaultLineHeight(
          template_paragraph_->GetHeight());
    }
    text_height = template_paragraph_ ? template_paragraph_->GetHeight() *
                                            (infinite ? 1 : max_lines_)
                                      : 0;
  } else {
    text_height = paragraph_->GetHeight();
  }
  return text_height;
}

// Different from text measure, editable has some special logics:
// - If height/width is definite, use it;
// - If width is at_most or infinite, use it.
// - If height is at_most or infinite, estimate height according to first line
// height of current style.
void EditableView::Measure(const MeasureConstraint& constraint,
                           MeasureResult& result) {
  auto context = CreateLayoutContext(constraint);
  Layout(&context);
  if (constraint.height_mode == MeasureMode::kDefinite) {
    result.height = *constraint.height;
  } else {
    auto estimated_height = EstimateHeightWithMaxLines();
    result.height = (constraint.height_mode == MeasureMode::kIndefinite ||
                     !constraint.height.has_value())
                        ? estimated_height
                        : std::min(estimated_height, *constraint.height);
  }

  if (constraint.width_mode == MeasureMode::kIndefinite ||
      !constraint.width.has_value()) {
    result.width = std::numeric_limits<float>::infinity();
  } else {
    result.width = *constraint.width;
  }
  const auto& text_editing_value = GetTextEditingValue();
  if (constraint.height_mode != MeasureMode::kDefinite &&
      text_editing_value.empty()) {
    // Empty content should at least strut a height by default style.
    result.height =
        std::max(result.height, GetRenderEditable()->GetRoughTextLineHeight() +
                                    VerticalThickness());
  }
}

void EditableView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  switch (kw) {
    case KeywordID::kShowSoftInputOnfocus:
      SetShowSoftInputOnFocus(attribute_utils::GetBool(value));
      break;
    case KeywordID::kType:
      keyboard_input_type_ =
          ConvertInputType(attribute_utils::GetCString(value));
      ApplyFilter(&EditableView::FilterInputTextByType, true);
      break;
    case KeywordID::kDisabled:
      SetDisabled(attribute_utils::GetBool(value));
      break;
    case KeywordID::kConfirmType:
      keyboard_action_ = ConvertConfirmType(
          attribute_utils::GetCString(value),
          max_lines_ == 1 ? KeyboardAction::kDone : KeyboardAction::kMultiLine);
      break;
    case KeywordID::kColor:
      SetFontColor(attribute_utils::GetInt(value));
      break;
    case KeywordID::kFontSize: {
      double font_size = 0.0;
      if (attribute_utils::TryGetNum(value, font_size)) {
        SetFontSize(static_cast<float>(font_size));
      }
    } break;
    case KeywordID::kFontWeight: {
      double font_weight_val;
      if (attribute_utils::TryGetNum(value, font_weight_val)) {
        SetFontWeight(ToFontWeight(font_weight_val));
      }
    } break;
    case KeywordID::kLetterSpacing: {
      double letter_spacing = 0.0;
      if (attribute_utils::TryGetNum(value, letter_spacing)) {
        SetLetterSpacing(letter_spacing);
      }
    } break;
    case KeywordID::kTextAlign: {
      double text_align = 0.0;
      if (attribute_utils::TryGetNum(value, text_align)) {
        SetTextAlign(static_cast<TextAlignment>(text_align));
      }
    } break;
    case KeywordID::kFontFamily:
      SetFontFamily(attribute_utils::GetCString(value));
      break;
    case KeywordID::kCaretColor: {
      if (value.IsUint()) {
        GetRenderEditable()->SetCaretColor(
            Color(attribute_utils::GetUint(value)));
        break;
      }
      Color color;
      const auto caret_color = lynx::base::TrimString(
          attribute_utils::GetCString(value), " \t\n\r\f\v",
          lynx::base::TrimPositions::TRIM_ALL);
      if (Color::Parse(caret_color, &color)) {
        GetRenderEditable()->SetCaretColor(color);
      } else {
        GetRenderEditable()->SetCaretColor(std::nullopt);
      }
    } break;
    case KeywordID::kCaretGradient: {
      auto gradient_value = lynx::base::TrimString(
          attribute_utils::GetCString(value), " \t\n\r\f\v",
          lynx::base::TrimPositions::TRIM_ALL);
      if (gradient_value.empty() || gradient_value == "none") {
        GetRenderEditable()->SetCaretGradient(std::nullopt);
      } else if (auto gradient = Gradient::Create(gradient_value)) {
        GetRenderEditable()->SetCaretGradient(std::move(gradient));
      } else {
        GetRenderEditable()->SetCaretGradient(std::nullopt);
      }
    } break;
    case KeywordID::kCaretWidth: {
      double caret_width = 0.0;
      if (attribute_utils::TryGetNum(value, caret_width)) {
        GetRenderEditable()->SetCaretWidth(static_cast<float>(caret_width));
      } else {
        GetRenderEditable()->SetCaretWidth(0.0f);
      }
    } break;
    case KeywordID::kCaretHeight: {
      double caret_height = 0.0;
      if (attribute_utils::TryGetNum(value, caret_height)) {
        GetRenderEditable()->SetCaretHeight(static_cast<float>(caret_height));
      } else {
        GetRenderEditable()->SetCaretHeight(0.0f);
      }
    } break;
    case KeywordID::kCaretRadius: {
      double caret_radius = 0.0;
      if (attribute_utils::TryGetNum(value, caret_radius)) {
        GetRenderEditable()->SetCaretRadius(static_cast<float>(caret_radius));
      } else {
        GetRenderEditable()->SetCaretRadius(0.0f);
      }
    } break;
    case KeywordID::kMaxlength: {
      double max_length;
      if (attribute_utils::TryGetNum(value, max_length) && max_length > 0) {
        max_length_ = max_length;
        ApplyFilter(&EditableView::FilterMaxLength, max_length_ > 0);
      }
    } break;
    case KeywordID::kReadonly:
      SetReadOnly(attribute_utils::GetBool(value));
      break;
    case KeywordID::kMaxlines: {
      double max_lines;
      if (attribute_utils::TryGetNum(value, max_lines) && max_lines > 0) {
        SetMaxLines(max_lines);
      }
    } break;
    case KeywordID::kPlaceholder:
      SetPlaceholder(attribute_utils::GetCString(value));
      break;
    case KeywordID::kPlaceholderFontSize: {
      attribute_utils::Length val_with_unit{0.0, attribute_utils::Unit::kNone};
      if (attribute_utils::TryGetLength(value, val_with_unit)) {
        SetPlaceholderFontSize(val_with_unit.val);
      }
    } break;
    case KeywordID::kPlaceholderFontWeight: {
      std::string font_weight_val;
      if (attribute_utils::TryGetString(value, font_weight_val)) {
        SetPlaceholderFontWeight(ToPlaceHolderWeight(font_weight_val));
      }
    } break;
    case KeywordID::kPlaceholderColor: {
      Color color;
      if (value.IsUint()) {
        color = attribute_utils::GetUint(value, 0xff000000);
        SetPlaceholderColor(color);
      }
    } break;
    default:
      BaseView::SetAttribute(attr_c, value);
      break;
  }
}

void EditableView::SetFontFamily(const std::string& font_family) {
  std::string new_font_family = font_family;
  if (new_font_family.length() > 1) {
    new_font_family = lynx::base::TrimString(
        new_font_family, "\'", lynx::base::TrimPositions::TRIM_ALL);
    if (new_font_family.length() > 1) {
      new_font_family = lynx::base::TrimString(
          new_font_family, "\"", lynx::base::TrimPositions::TRIM_ALL);
    }
  }
  if (text_style_.font_family != new_font_family) {
    text_style_.font_family = new_font_family;
    text_style_.strut_font_families.clear();
    text_style_.strut_font_families.emplace_back(new_font_family);
    MarkDirty();
    RelayoutWhenSetFontFamily(new_font_family);
  }
}

void EditableView::SetPlaceholder(const std::string& placeholder) {
  if (placeholder != placeholder_) {
    placeholder_ = placeholder;
    MarkNeedsLayout();
  }
}

void EditableView::SetPlaceholderFontSize(float font_size) {
  if (placeholder_font_size_ != font_size) {
    placeholder_font_size_ = font_size;
    MarkNeedsLayout();
  }
}

void EditableView::SetPlaceholderFontWeight(FontWeight font_weight) {
  if (placeholder_font_weight_ != font_weight) {
    placeholder_font_weight_ = font_weight;
    MarkNeedsLayout();
  }
}

void EditableView::SetPlaceholderColor(const Color& color) {
  if (placeholder_color_ != color) {
    placeholder_color_ = color;
    MarkNeedsLayout();
  }
}

void EditableView::RelayoutWhenSetFontFamily(const std::string& font_family) {
  auto font_collection = Isolate::Instance().GetFontCollection();

  if (font_collection) {
    font_collection->RegisterCallback(font_family,
                                      [self = weak_factory_.GetWeakPtr()]() {
                                        if (!self) {
                                          return;
                                        }
                                        self->MarkDirty();
                                      });
  }
}

void EditableView::UpdateCaretPosition() {
  auto point = AbsoluteLocationWithScroll();
  auto caret_rect = GetRenderEditable()->ComputeCaretRect();
  text_input_controller_->UpdateCaretPosition(
      point.x() + caret_rect.x() + ContentInsetLeft(),
      point.y() + caret_rect.y() + ContentInsetTop(), caret_rect.width(),
      caret_rect.height());
}

void EditableView::OnValueChanged(const TextEditingValue& value,
                                  const TextEditingController*) {
  TwinkleCaretPeriodically();
  if (!value.composing()) {
    UpdateRemoteStateIfNeeded(value);
  }
  MarkNeedsLayout();
}

void EditableView::OnSelectionChanged(const TextEditingValue& value,
                                      const TextEditingController*) {
  TwinkleCaretPeriodically();
  UpdateCaretPosition();
  page_view()->SendEvent(callback_id_, event_attr::kEventSelectionChange,
                         {"start", "end"},
                         static_cast<int>(value.selection().start()),
                         static_cast<int>(value.selection().end()));
}

void EditableView::OnUserInput(const TextEditingValue& value,
                               const TextEditingController*) {
  const auto& text_editing_value = GetTextEditingValue();
  // Keep string alive to transfer c_str.
  auto text_editing_str = text_editing_value.GetText();
  page_view()->SendEvent(
      callback_id_, event_attr::kEventInput,
      {"value", "selectionStart", "selectionEnd", "isComposing"},
      text_editing_str.c_str(),
      static_cast<int>(text_editing_value.selection().start()),
      static_cast<int>(text_editing_value.selection().end()),
      text_editing_value.composing());
}

void EditableView::OnCommitText(std::string text) {
  auto text_editing_value = GetTextEditingValue();
  if (text_editing_value.composing()) {
    text_editing_value.UpdateComposingText("");
    text_editing_value.EndComposing();
  }
  text_editing_value.AddText(text);
  FilterInputIfNeeded(&text_editing_value);
  SetTextEditingValue(text_editing_value);
  MarkNeedsLayout();
}

void EditableView::OnComposingText(std::string text) {
  auto text_editing_value = GetTextEditingValue();
  if (!text_editing_value.composing()) {
    text_editing_value.BeginComposing();
  }
  text_editing_value.AddText(text);
  FilterInputIfNeeded(&text_editing_value);
  SetTextEditingValue(text_editing_value, true);
  MarkNeedsLayout();
}

void EditableView::ApplyFilter(FilterFunc func, bool take_effect) {
  if (take_effect) {
    filter_funcs_.push_back(func);
  } else {
    for (auto iter = filter_funcs_.begin(); iter != filter_funcs_.end();
         iter++) {
      if (*iter == func) {
        filter_funcs_.erase(iter);
      }
    }
  }
}

void EditableView::FilterInputIfNeeded(TextEditingValue* input) {
  if (filter_funcs_.empty()) {
    return;
  }

  for (FilterFunc func : filter_funcs_) {
    auto filter_number = (this->*func)(input);
    if (filter_number > 0) {
      UpdateRemoteStateIfNeeded(*input);
    }
  }
}

void EditableView::OnKeyboardEvent(std::unique_ptr<KeyEvent> key_event) {
  OnKeyEventInternal(key_event.get());
}

bool EditableView::OnKeyEvent(const KeyEvent* event) {
  return OnKeyEventInternal(event);
}

void EditableView::OnPerformAction(KeyboardAction action) {
  // TODO(yulitao): Deal with action other than |kDone|.
  auto text_editing_str = GetTextEditingValue().GetText();
  switch (action) {
    case KeyboardAction::kMultiLine:
      break;
    case KeyboardAction::kGo:
    case KeyboardAction::kNext:
    case KeyboardAction::kSearch:
    case KeyboardAction::kSend:
    case KeyboardAction::kDone:
      page_view()->SendEvent(callback_id_, event_attr::kEventConfirm, {"value"},
                             text_editing_str.c_str());
    case KeyboardAction::kPrevious:
      ClearFocus();
  }
}

void EditableView::OnFinishInput() {
  if (editing_) {
    QuitEditing();
  }
}

bool EditableView::OnKeyEventInternal(const KeyEvent* key_event) {
  if (!key_event || disabled_) {
    return false;
  }
  // Note(yulitao): Capture keycode use allowlist in case that some special
  // keycodes may be swallowed by us.
  bool is_down = key_event->GetType() == KeyEventType::kDown;
  bool is_repeat = key_event->GetType() == KeyEventType::kRepeat;
  KeyCode keycode = key_event->GetLogical();

  if (!editing_ && keycode != KeyCode::kSelect && keycode != KeyCode::kEnter) {
    return false;
  }

  if (ApplyHotKey(key_event)) {
    return true;
  }
  switch (keycode) {
    case KeyCode::kGoBack: {
      if (editing_) {
        if (!is_down) {
          QuitEditing();
        }
        return true;
      } else {
        return false;
      }
    }
    case KeyCode::kEscape: {
      if (!is_down) {
        QuitEditing();
      }
      return true;
    }
    case KeyCode::kSelect: {
      if (is_down) {
        BeginEditingIfNeeded();
      }
      return true;
    }
    case KeyCode::kDelete: {
      if (is_down || is_repeat) {
        DoDelete();
      }
      return true;
    }
    case KeyCode::kBackspace: {
      if (is_down || is_repeat) {
        DoBackspace();
      }
      return true;
    }
    case KeyCode::kClear: {
      if (is_down) {
        SetContent("");
      }
      return true;
    }
    case KeyCode::kArrowLeft:
    case KeyCode::kArrowRight:
    case KeyCode::kArrowUp:
    case KeyCode::kArrowDown: {
      if (is_down || is_repeat) {
        MoveCaret(keycode);
      }
      return true;
    }
    case KeyCode::kSpace: {
#if defined(OS_ANDROID) || defined(OS_HARMONY)
      // On some android IM client, space may be sent by keyevent.
      if (is_down) {
        OnCommitText(" ");
      }
      return true;
#else
      if (key_event->GetSynthesized()) {
        // For PC headless mode
        return HandleSynthesizedKeyEvent(key_event);
      }
      return false;
#endif
    }
    case KeyCode::kEnter: {
      if (!editing_) {
        if (!is_down) {
          BeginEditingIfNeeded();
        }
        return true;
      } else if (is_down && key_event->GetSynthesized() &&
                 key_event->GetCharacter() == "\r" && max_lines_ > 1) {
        // Treat as line break in multi-line mode for PC headless mode.
        OnCommitText("\n");
        return true;
      }
    }
    default: {
#if defined(OS_ANDROID) || defined(OS_IOS) || \
    (defined(OS_HARMONY) && !defined(ENABLE_HEADLESS))
      // In case a keyboard is used on a mobile system.
      if (!key_event->GetCharacter().empty()) {
        // Treat as unicode character.
        if (is_down) {
          OnCommitText(key_event->GetCharacter());
        }
        return true;
      }
#else
      // We using synthesized key event for PC headless mode
      if (key_event->GetSynthesized()) {
        return HandleSynthesizedKeyEvent(key_event);
      }
      // PC IME will handle it.
      return false;
#endif
    }
  }
  return false;
}

bool EditableView::HandleSynthesizedKeyEvent(const KeyEvent* key_event) {
  if (!key_event->GetSynthesized()) {
    return false;
  }
  if (key_event->GetType() == KeyEventType::kDown &&
      !key_event->GetCharacter().empty()) {
    // Treat as utf8 character.
    OnCommitText(key_event->GetCharacter());
    return true;
  }
  return false;
}

// TODO(wangyanyi): Currently, rtl language input zwj is not supported and
// using the getwordboundary method is a more trick method.
TextRange EditableView::FindNextOrPrevCharacterSelection(bool is_forward) {
  auto text_editing_value = GetTextEditingValue();
  const auto& text = text_editing_value.GetU16Text();
  auto current_selection = TextRange(text_editing_value.selection());
  if (is_forward) {
    if (TextUtils::JudgeIfZWJCharacter(text_editing_value.GetU16Text(),
                                       current_selection.extent(), true)) {
      auto current_word =
          GetRenderEditable()->GetWordBoundary(current_selection.extent());
      auto prev_word = GetRenderEditable()->GetWordBoundary(
          std::max(static_cast<int>(current_word.base() - 1), 0));
      return prev_word;
    } else {
      size_t position = current_selection.extent();
      if (position > 0) {
        int count = IsTrailingSurrogate(text.at(position - 1)) ? 2 : 1;
        return TextRange(current_selection.extent(),
                         current_selection.extent() - count);
      }
    }
  } else {
    if (TextUtils::JudgeIfZWJCharacter(text_editing_value.GetU16Text(),
                                       current_selection.extent(), false)) {
      auto current_word =
          GetRenderEditable()->GetWordBoundary(current_selection.extent());
      return current_word;
    } else {
      size_t position = current_selection.extent();
      if (position < text.length()) {
        int count = IsLeadingSurrogate(text.at(position)) ? 2 : 1;
        return TextRange(current_selection.extent(),
                         current_selection.extent() + count);
      }
    }
  }
  return text_editing_value.selection();
}

void EditableView::MoveCaret(KeyCode keycode) {
  // TODO(yulitao): Need determine how many utf16 units there is.
  auto text_editing_value = GetTextEditingValue();
  // If we are in a composing state, caret behaviors are managed remotely
  // (i.e., by the platform layer) on MacOS.
  // TODO(haochen): Currently we cannot handle left/right key in composing state
  // on windows.
  // TODO(haocehn): Check TV.
#if defined(OS_MACOSX)
  if (text_editing_value.composing()) {
    return;
  }
#endif
  if (keycode == KeyCode::kArrowLeft) {
    auto prev = FindNextOrPrevCharacterSelection(true);
    text_editing_value.SetSelection(TextRange(prev.start(), prev.start()));
    SetTextEditingValue(text_editing_value);
    GetRenderEditable()->MarkNeedsPaint();
  } else if (keycode == KeyCode::kArrowRight) {
    auto next = FindNextOrPrevCharacterSelection(false);
    text_editing_value.SetSelection(TextRange(next.end(), next.end()));
    SetTextEditingValue(text_editing_value);
    GetRenderEditable()->MarkNeedsPaint();
  } else if (keycode == KeyCode::kArrowUp) {
    GetRenderEditable()->MoveCaretUpDown(VerticalDirection::kUp);
  } else if (keycode == KeyCode::kArrowDown) {
    GetRenderEditable()->MoveCaretUpDown(VerticalDirection::kDown);
  }
}

void EditableView::OnDeleteSurroundingText(int before_length,
                                           int after_length) {
  DeleteSurroundingText(before_length, after_length);
}

void EditableView::DeleteSurroundingText(int before_length, int after_length) {
  auto text_editing_value = GetTextEditingValue();
  bool deleted = text_editing_value.DeleteSurrounding(
      -before_length, before_length + after_length);
  if (deleted) {
    SetTextEditingValue(text_editing_value);
    MarkNeedsLayout();
  }
}

void EditableView::DoDelete() {
  auto text_editing_value = GetTextEditingValue();
  auto text = text_editing_value.GetU16Text();
  text_editing_value.StopComposingIfNecessary();
  auto& selection = text_editing_value.selection();
  if (!selection.collapsed()) {
    size_t start = selection.start();
    text_editing_value.SetText(text.erase(start, selection.length()),
                               TextRange(start));
    if (text_editing_value.composing()) {
      // This occurs only immediately after composing has begun with a
      // selection.
      text_editing_value.SetComposingRange(selection, 0);
    }
  } else {
    auto next = FindNextOrPrevCharacterSelection(false);
    text_editing_value.SetText(text.erase(next.start(), next.length()),
                               TextRange(next.start(), next.start()));
    if (text_editing_value.composing()) {
      auto composing_range = text_editing_value.composing_range();
      composing_range.set_end(composing_range.end() - next.length());
      text_editing_value.SetComposingRange(composing_range, 0);
    }
  }
  SetTextEditingValue(text_editing_value);
  MarkNeedsLayout();
}

void EditableView::DoBackspace() {
  auto text_editing_value = GetTextEditingValue();
  auto text = text_editing_value.GetU16Text();
  text_editing_value.StopComposingIfNecessary();
  auto& selection = text_editing_value.selection();
  if (!selection.collapsed()) {
    size_t start = selection.start();
    text_editing_value.SetText(text.erase(start, selection.length()),
                               TextRange(start));
    if (text_editing_value.composing()) {
      // This occurs only immediately after composing has begun with a
      // selection.
      text_editing_value.SetComposingRange(selection, 0);
    }
  } else {
    auto prev = FindNextOrPrevCharacterSelection(true);
    text_editing_value.SetText(text.erase(prev.start(), prev.length()),
                               TextRange(prev.start(), prev.start()));
  }
  SetTextEditingValue(text_editing_value);
  GetRenderEditable()->MarkNeedsPaint();
}

RenderEditable* EditableView::GetRenderEditable() {
  return static_cast<RenderEditable*>(render_object_.get());
}

void EditableView::OnGestureTap(const PointerEvent& pointer) {
  if (last_tap_pointer_ == pointer.pointer_id) {
    // Tap gesture would always comes later than the first tap of double-tap.
    // Avoid redundant handling because finding offset by coordinate is a bit
    // expensive.
    return;
  }
  last_tap_pointer_ = pointer.pointer_id;

  BeginEditingIfNeeded();
  GetRenderEditable()->UpdateCaretByCoordinate(
      GetPointBySelf(pointer.position));
}

void EditableView::OnGestureDoubleTap(const PointerEvent& pointer) {
  GetRenderEditable()->SelectWord(GetPointBySelf(pointer.position));
}
void EditableView::OnGestureTripleTap(const PointerEvent& pointer) {
  GetRenderEditable()->SelectLine(GetPointBySelf(pointer.position));
}

void EditableView::OnDragStart(const FloatPoint& position) {
  BeginEditingIfNeeded();
  GetRenderEditable()->UpdateCaretByCoordinate(GetPointBySelf(position));
}

void EditableView::OnDragUpdate(const FloatPoint& position,
                                const FloatSize& delta) {
  GetRenderEditable()->UpdateCaretByCoordinate(GetPointBySelf(position), true);
}

txt::Paragraph* EditableView::GetParagraph() { return paragraph_.get(); }

FloatRect EditableView::ComputeCaretRect() {
  return GetRenderEditable()->ComputeCaretRect();
}

void EditableView::SetDisabled(bool disabled) {
  if (disabled_ == disabled) {
    return;
  }

  disabled_ = disabled;
  GetRenderEditable()->SetDisabled(disabled);
  page_view()->StopInput(this);
}

void EditableView::BeginEditingIfNeeded() {
  if (readonly_ || disabled_) {
    RequestFocus();
  } else {
    BeginEditing();
  }
}

void EditableView::BeginEditing() {
  if (editing_) {
    text_input_controller_->Show();
    RequestKeyboard();
    return;
  }
  editing_ = true;
  RequestFocus();
  TwinkleCaretPeriodically();
  text_input_controller_->SetClient(callback_id_, keyboard_action_,
                                    KeyboardInputType::kClassText);
  text_input_controller_->SetEditableTransform(ToGlobalTransform());
  text_input_controller_->SetEditingState(GetTextEditingValue());
  text_input_controller_->Show();
  RequestKeyboard();
  GetRenderEditable()->MarkNeedsPaint();
}

void EditableView::QuitEditing() {
  FML_DCHECK(editing_);
  editing_ = false;
  hot_key_tag_ = 0;
  page_view()->StopInput(this);
  auto text_editing_value = GetTextEditingValue();
  if (text_editing_value.composing()) {
    text_editing_value.CommitComposing();
    text_editing_value.EndComposing();
    FilterInputIfNeeded(&text_editing_value);
    SetTextEditingValue(text_editing_value);
    MarkNeedsLayout();
  }
  if (caret_timer_) {
    caret_timer_.reset();
    GetRenderEditable()->SetCaretDisplay(false);
  }
  text_input_controller_->Hide();
  text_input_controller_->ClearClient();
}

void EditableView::UpdateCaretRectIfNeeded() {
  if (!text_input_controller_->ConnectKeyboard()) {
    return;
  }
  text_input_controller_->SetCaretRect(
      GetRenderEditable()->ComputeCaretRectRelativeToCanvas());
}

void EditableView::UpdateComposingRectIfNeeded() {
  if (!text_input_controller_->ConnectKeyboard()) {
    return;
  }
  const auto& text_editing_value = GetTextEditingValue();
  const TextRange composing_range = text_editing_value.composing_range();
  text_input_controller_->SetComposingRect(
      GetRenderEditable()->ComputeComposingRect(composing_range));
}

void EditableView::RequestKeyboard() {
  if (show_soft_input_on_focus_) {
    page_view()->RequestInput(this, keyboard_input_type_, keyboard_action_);
  }
}

void EditableView::FocusHasChanged(bool focused, bool is_leaf) {
  if (focused && !editing_) {
    BeginEditingIfNeeded();
  }

  if (focused && text_editing_controller_->MoveSelectionToEndIfNeeded()) {
    UpdateRemoteStateIfNeeded(GetTextEditingValue());
  }
  auto text_editing_value = GetTextEditingValue();

  // collapse selection when losing focus.
  if (!focused) {
    if (!text_editing_value.selection().collapsed()) {
      text_editing_value.SetSelection(
          TextRange(text_editing_value.selection().end(),
                    text_editing_value.selection().end()));
      SetTextEditingValue(text_editing_value);
    }
  }

  if (!focused && editing_) {
    QuitEditing();
  }

  auto str = text_editing_value.GetText();
  if (focused) {
    UpdateCaretPosition();
  }
  page_view()->SendEvent(
      callback_id_, focused ? event_attr::kEventFocus : event_attr::kEventBlur,
      {"value"}, str.c_str());
}

void EditableView::TwinkleCaretPeriodically() {
  if (!editing_) {
    return;
  }
  twinkle_flag_ = true;
  GetRenderEditable()->SetCaretDisplay(true);

  if (!caret_timer_) {
    caret_timer_ =
        std::make_unique<fml::RepeatingTimer>(page_view()->GetTaskRunner());
  }
  caret_timer_->Start(fml::TimeDelta::FromMilliseconds(kCaretTwinkleIntervalMs),
                      [this] { ToggleCaret(); });
}

void EditableView::ToggleCaret() {
  twinkle_flag_ = !twinkle_flag_;
  GetRenderEditable()->SetCaretDisplay(twinkle_flag_);
}

void EditableView::SetDirection(int type) {
  SetTextDirection(static_cast<TextDirection>(type));
}

void EditableView::SetMaxLines(uint32_t max_lines) {
  if (max_lines == max_lines_) {
    return;
  }

  max_lines_ = max_lines;
  ApplyFilter(&EditableView::FilterNewLine, max_lines_ == 1);
  GetRenderEditable()->SetMaxLines(max_lines_);
  if (max_lines_ > 1) {
    text_input_controller_->SetMultiline(true);
  } else {
    text_input_controller_->SetMultiline(false);
  }
  MarkNeedsLayout();
}

void EditableView::SetShowSoftInputOnFocus(bool show_soft_input_on_focus) {
  show_soft_input_on_focus_ = show_soft_input_on_focus;
  if (!show_soft_input_on_focus_) {
    page_view()->StopInput(this);
  }
}

void EditableView::SetContent(const std::string& content) {
  auto text_editing_value = GetTextEditingValue();
  text_editing_value.SetTextAndReserveSelectionState(content);
  FilterInputIfNeeded(&text_editing_value);
  SetTextEditingValue(text_editing_value);
  MarkNeedsLayout();
}

void EditableView::SetFontColor(const Color& color) {
  GetRenderEditable()->SetCaretFallbackColor(color);
  if (text_style_.text_color != color) {
    text_style_.text_color = color;
    MarkNeedsLayout();
  }
}

void EditableView::SetFontSize(float font_size) {
  if (text_style_.font_size != font_size) {
    text_style_.font_size = font_size;
    if (!placeholder_font_size_.has_value()) {
      placeholder_font_size_ = font_size;
    }
    text_style_.strut_font_size = font_size;
    MarkNeedsLayout();
  }
}

void EditableView::SetFontWeight(FontWeight font_weight) {
  if (text_style_.font_weight != font_weight) {
    text_style_.font_weight = font_weight;
    text_style_.strut_font_weight = font_weight;
    MarkNeedsLayout();
  }
}

void EditableView::SetLetterSpacing(float letter_spacing) {
  if (text_style_.letter_spacing != letter_spacing) {
    text_style_.letter_spacing = letter_spacing;
    MarkNeedsLayout();
  }
}

void EditableView::SetTextAlign(TextAlignment text_alignment) {
  GetRenderEditable()->SetTextAlign(text_alignment);
  if (is_multiline_ && text_style_.text_align != text_alignment) {
    text_style_.text_align = text_alignment;
  }
  MarkNeedsLayout();
}

void EditableView::SetTextDirection(TextDirection text_direction) {
  if (text_direction == TextDirection::kNormal) {
    text_direction = TextDirection::kLtr;
  }
  if (text_style_.text_direction != text_direction) {
    text_style_.text_direction = text_direction;
    GetRenderEditable()->SetTextDirection(text_direction);
    MarkNeedsLayout();
  }
}

void EditableView::SetLineSpacing(float line_spacing) {
  if (text_style_.line_spacing != line_spacing) {
    text_style_.line_spacing = line_spacing;
    MarkNeedsLayout();
  }
}

void EditableView::SetReadOnly(bool read_only) {
  if (editing_) {
    QuitEditing();
  }
  readonly_ = read_only;
  if (is_focused_) {
    BeginEditingIfNeeded();
  }
}

float EditableView::LayoutWidth() {
  if (max_lines_ == 1) {
    // Single line means no soft wrap.
    return std::numeric_limits<float>::infinity();
  } else {
    return ContentWidth();
  }
}

void EditableView::setValue(const LynxModuleValues& args,
                            const LynxUIMethodCallback& callback) {
  std::string content;
  int index = -1;
  CastNamedLynxModuleArgs({"value", "index"}, args, content, index);
  auto text_editing_value = GetTextEditingValue();
  text_editing_value.SetText(
      content,
      TextRange(
          index >= 0
              ? std::min<size_t>(static_cast<size_t>(index), content.size())
              : std::min(content.size(),
                         text_editing_value.selection().extent())));
  FilterInputIfNeeded(&text_editing_value);
  SetTextEditingValue(text_editing_value);
  MarkNeedsLayout();
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void EditableView::blur(const LynxModuleValues&,
                        const LynxUIMethodCallback& callback) {
  ClearFocus();
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void EditableView::focus(const LynxModuleValues& args,
                         const LynxUIMethodCallback& callback) {
  RequestFocus();
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void EditableView::setSelectionRange(const LynxModuleValues& args,
                                     const LynxUIMethodCallback& callback) {
  int begin = 0, end = 0;
  auto text_editing_value = GetTextEditingValue();
  CastNamedLynxModuleArgs({"selectionStart", "selectionEnd"}, args, begin, end);
  text_editing_value.SetSelection(TextRange(begin, end));
  SetTextEditingValue(text_editing_value);
  MarkNeedsLayout();
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void EditableView::getValue(const LynxModuleValues&,
                            const LynxUIMethodCallback& callback) {
  auto str = GetTextEditingValue().GetText();
  callback(LynxUIMethodResult::kSuccess, clay::Value(str));
}

bool EditableView::ApplyHotKey(const KeyEvent* key_event) {
  auto key_code = key_event->GetLogical();
  bool is_up = key_event->GetType() == KeyEventType::kUp;
  UpdateHotKeyTag(key_code, is_up);
  if (hot_key_tag_ == 0) {
    // Not in hot key mode.
    return false;
  }
  if (!is_up) {
    // Only handle second hot key down and repeat.
    if (hot_key_tag_ == kTagCommand) {
      HandleCommandHotKey(key_code);
    } else if (hot_key_tag_ == kTagControl) {
      HandleCtrlHotKey(key_code);
    } else if (hot_key_tag_ == kTagShift) {
      return HandleShiftHotKey(key_code);
    }
    // Do not respond to multiple modifier key combination, like 'command' +
    // 'control' + key.
  }
  return true;
}

void EditableView::UpdateHotKeyTag(LogicalKeyboardKey key_code, bool is_up) {
  switch (key_code) {
    case KeyCode::kMetaLeft:
    case KeyCode::kMetaRight:
    case KeyCode::kMeta:
      hot_key_tag_ = is_up ? RemoveTag(hot_key_tag_, kTagCommand)
                           : AddTag(hot_key_tag_, kTagCommand);
      break;
    case KeyCode::kControlLeft:
    case KeyCode::kControlRight:
    case KeyCode::kControl:
      hot_key_tag_ = is_up ? RemoveTag(hot_key_tag_, kTagControl)
                           : AddTag(hot_key_tag_, kTagControl);
      break;
    case KeyCode::kShiftLeft:
    case KeyCode::kShiftRight:
    case KeyCode::kShift:
      hot_key_tag_ = is_up ? RemoveTag(hot_key_tag_, kTagShift)
                           : AddTag(hot_key_tag_, kTagShift);
      break;
    default:
      break;
  }
}

void EditableView::HandleCommandHotKey(LogicalKeyboardKey key_code) {
  HandleWinCtrlAndMacCommandHotKey(key_code);
}

void EditableView::HandleCtrlHotKey(LogicalKeyboardKey key_code) {
#if defined(WIN32) || defined(ENABLE_HEADLESS)
  HandleWinCtrlAndMacCommandHotKey(key_code);
#endif
}

void EditableView::HandleWinCtrlAndMacCommandHotKey(
    LogicalKeyboardKey key_code) {
  const auto& text_editing_value = GetTextEditingValue();
  switch (key_code) {
    case KeyCode::kKeyV: {
      // Paste from clipboard.
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
      auto clip_board_data = page_view()->GetClipboardData();
      OnCommitText(lynx::base::U16StringToU8(clip_board_data));
#endif
      break;
    }
    case KeyCode::kKeyC:
    case KeyCode::kKeyX: {
      // Copy to clipboard.
      std::u16string selected = u"";
      const auto& selection = text_editing_value.selection();
      if (!selection.collapsed()) {
        selected = text_editing_value.GetU16Text().substr(
            selection.start(), selection.end() - selection.start());
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
        page_view()->SetClipboardData(selected);
#endif
        if (key_code == KeyCode::kKeyX) {
          DoDelete();
        }
      }
      break;
    }
    case KeyCode::kKeyA: {
      // Select all.
      auto text_editing_value = GetTextEditingValue();
      text_editing_value.SetSelection(text_editing_value.text_range());
      SetTextEditingValue(text_editing_value);
      break;
    }
    case KeyCode::kKeyZ: {
      // Revoke.
      // SetRawContent(revoke_cache_, true);
      text_editing_history_state_->Undo();
      break;
    }
    default:
      break;
  }
}

bool EditableView::HandleShiftHotKey(LogicalKeyboardKey key_code) {
  // Move 'extent' value left/right/up/down.
  auto text_editing_value = GetTextEditingValue();
  auto current_selection = TextRange(text_editing_value.selection());
  switch (key_code) {
    case KeyCode::kArrowLeft: {
      if (text_editing_value.selection().extent() ==
          text_editing_value.text_range().start()) {
        break;
      }
      auto prev = FindNextOrPrevCharacterSelection(true);
      text_editing_value.SetSelection(
          TextRange(current_selection.base(), prev.start()));
      SetTextEditingValue(text_editing_value);
      break;
    }
    case KeyCode::kArrowRight: {
      if (text_editing_value.selection().extent() ==
          text_editing_value.text_range().end()) {
        break;
      }
      auto next = FindNextOrPrevCharacterSelection(false);
      text_editing_value.SetSelection(
          TextRange(current_selection.base(), next.end()));
      SetTextEditingValue(text_editing_value);
      break;
    }
    case KeyCode::kArrowUp:
      GetRenderEditable()->MoveCaretUpDown(VerticalDirection::kUp, true);
      break;
    case KeyCode::kArrowDown:
      GetRenderEditable()->MoveCaretUpDown(VerticalDirection::kDown, true);
      break;

    default:
      // other cases should be handled as normal key, e.g:
      // shift + '2' -> '@'
      // shift + 'a' -> 'A'
      return false;
  }
  return true;
}

void EditableView::ResetGestureRecognizers() {
  RemoveGestureRecognizer(multi_tap_recognizer_);
  RemoveGestureRecognizer(drag_recognizer_);
  multi_tap_recognizer_ = nullptr;
  drag_recognizer_ = nullptr;
  std::unique_ptr<MultiTapGestureRecognizer> multi_tap_recognizer =
      std::make_unique<MultiTapGestureRecognizer>(
          page_view()->gesture_manager());
  multi_tap_recognizer->SetDelegate(this);
  multi_tap_recognizer_ = multi_tap_recognizer.get();
  std::unique_ptr<DragGestureRecognizer> drag_recognizer =
      std::make_unique<HorizontalDragGestureRecognizer>(
          page_view()->gesture_manager());
  drag_recognizer->SetDelegate(this);
  drag_recognizer_ = drag_recognizer.get();
  multi_tap_recognizer->SetMultiTapCallback([this](auto&& PH1, int count) {
    OnGestureTap(std::forward<decltype(PH1)>(PH1));
    if (count == 2) {
      OnGestureDoubleTap(std::forward<decltype(PH1)>(PH1));
    }
    if (count >= 3) {
      OnGestureTripleTap(std::forward<decltype(PH1)>(PH1));
    }
  });
  drag_recognizer->SetDragStartCallback(
      [this](auto&& PH1) { OnDragStart(std::forward<decltype(PH1)>(PH1)); });
  drag_recognizer->SetDragUpdateCallback([this](auto&& PH1, auto&& PH2) {
    OnDragUpdate(std::forward<decltype(PH1)>(PH1),
                 std::forward<decltype(PH2)>(PH2));
  });
  AddGestureRecognizer(std::move(drag_recognizer));
  AddGestureRecognizer(std::move(multi_tap_recognizer));
}

bool EditableView::IsMultiline() { return is_multiline_; }

void EditableView::LayoutText(LayoutContext* context) {
  const double available_max_width =
      std::max(0.0f, static_cast<LayoutContextMeasure*>(context)->layout_width -
                         GetRenderEditable()->PaddingLeft() -
                         GetRenderEditable()->PaddingRight() -
                         GetRenderEditable()->CaretWidth());
  const double text_max_width =
      IsMultiline() ? available_max_width : std::numeric_limits<double>::max();
  if (paragraph_) {
    paragraph_->Layout(text_max_width);
  }
}

std::shared_ptr<TextSpan> EditableView::BuildTextSpan(TextStyle style) {
  const auto& text_editing_value = GetTextEditingValue();
  TextStyle composingStyle = style;
#if defined(OS_MACOSX) || defined(OS_ANDROID)
  composingStyle.text_decoration =
      TextDecoration{TextDecorationLine::kTextDecorationLineUnderline,
                     TextDecorationStyle::kSolid};
#endif
  std::u16string text_submit =
      keyboard_input_type_ != KeyboardInputType::kPassword
          ? text_editing_value.GetU16Text()
          : std::u16string(text_editing_value.GetU16Length(), u'•');
#if defined(CLAY_ENABLE_TTTEXT)
  text_submit += u"\n";
#endif
  auto selection = text_editing_value.selection();
  auto composing = text_editing_value.composing_range();
  std::vector<std::shared_ptr<TextSpan>> text_spans;
  text_spans.push_back(std::make_shared<TextSpan>(
      text_submit.substr(0, composing.start()), style));
  text_spans.push_back(std::make_shared<TextSpan>(
      text_submit.substr(composing.start(), composing.length()),
      composingStyle));
  text_spans.push_back(
      std::make_shared<TextSpan>(text_submit.substr(composing.end()), style));
  return std::make_shared<TextSpan>(std::u16string(), style, text_spans);
}

const TextEditingValue& EditableView::GetTextEditingValue() {
  return text_editing_controller_->GetValue();
}

void EditableView::SetTextEditingValue(const TextEditingValue& value,
                                       bool is_composing,
                                       bool need_update_remote) {
  bool text_changed = value.GetU16Text() != GetTextEditingValue().GetU16Text();
  bool trigger_input_event =
      !is_composing && pre_text_value_ != value.GetU16Text();
  if (trigger_input_event) {
    pre_text_value_ = value.GetU16Text();
  }
  text_editing_controller_->SetValue(value, trigger_input_event,
                                     need_update_remote);
  if (text_editing_history_state_ && text_changed) {
    text_editing_history_state_->Push();
  }
  UpdateCursorPositionIfNeeded(value.selection());
}

void EditableView::UpdateEditingState(std::string text, TextSelection selection,
                                      TextRange composing, Affinity affinity) {
  bool is_composing = !(composing.collapsed());
  const auto& old_value = GetTextEditingValue();
  auto new_value = TextEditingValue(
      text, TextRange(selection.base_offset(), selection.extent_offset()),
      composing, is_composing, affinity);
  if (old_value == new_value) {
    return;
  }
  if (!is_composing) {
    FilterInputIfNeeded(&new_value);
  }
  SetTextEditingValue(new_value, is_composing, true);
  MarkNeedsLayout();
}

void EditableView::PerformAction() { OnPerformAction(keyboard_action_); }

Transform EditableView::ToGlobalTransform() const {
  return BaseView::LocalToGlobalTransform();
}

void EditableView::UpdateRemoteStateIfNeeded(
    const TextEditingValue& text_editing_value) {
  if (text_input_controller_) {
    if (!text_input_controller_->ConnectKeyboard() ||
        !text_editing_controller_->NeedUpdateRemote()) {
      return;
    }
    text_input_controller_->SetEditingState(text_editing_value);
  }
}

bool EditableView::MatchAttrSettings(KeywordID attr) {
  switch (attr) {
    case KeywordID::kShowSoftInputOnfocus:
    case KeywordID::kConfirmType:
    case KeywordID::kColor:
    case KeywordID::kFontSize:
    case KeywordID::kLineHeight:
    case KeywordID::kTextAlign:
    case KeywordID::kFontFamily:
    case KeywordID::kCaretColor:
    case KeywordID::kCaretGradient:
    case KeywordID::kCaretWidth:
    case KeywordID::kCaretHeight:
    case KeywordID::kCaretRadius:
    case KeywordID::kMaxlength:
    case KeywordID::kReadonly:
    case KeywordID::kMaxlines:
    case KeywordID::kValue:
    case KeywordID::kDisabled:
    case KeywordID::kPlaceholder:
    case KeywordID::kPlaceholderColor:
    case KeywordID::kPlaceholderFontSize:
    case KeywordID::kPlaceholderFontWeight:
      return true;
    default:
      break;
  }
  return false;
}

bool EditableView::MatchNGAttrSettings(KeywordID attr) {
  switch (attr) {
    case KeywordID::kShowSoftInputOnfocus:
    case KeywordID::kType:
    case KeywordID::kConfirmType:
    case KeywordID::kColor:
    case KeywordID::kFontSize:
    case KeywordID::kTextAlign:
    case KeywordID::kFontFamily:
    case KeywordID::kCaretColor:
    case KeywordID::kCaretGradient:
    case KeywordID::kCaretWidth:
    case KeywordID::kCaretHeight:
    case KeywordID::kCaretRadius:
    case KeywordID::kMaxlength:
    case KeywordID::kReadonly:
    case KeywordID::kMaxlines:
    case KeywordID::kDisabled:
    case KeywordID::kPlaceholder:
    case KeywordID::kPlaceholderColor:
    case KeywordID::kPlaceholderFontSize:
    case KeywordID::kPlaceholderFontWeight:
      return true;
    default:
      break;
  }
  return false;
}

void EditableView::PostPaint() {
  UpdateCaretRectIfNeeded();
  UpdateComposingRectIfNeeded();
}

float EditableView::GetDefaultFontSize() const {
  return FromLogical(kDefaultFontSizeInDip);
}

bool EditableView::IsPointerAllowed(const GestureRecognizer& gesture_recognizer,
                                    const PointerEvent& event) {
  if (&gesture_recognizer == drag_recognizer_ ||
      &gesture_recognizer == multi_tap_recognizer_) {
    if (event.device == PointerEvent::DeviceType::kMouse) {
      return event.buttons == PointerEvent::MouseButton::kPrimary;
    }
    return true;
  } else {
    FML_DLOG(ERROR) << "unrecognized gesture recognizer: "
                    << gesture_recognizer.GetMemberTag();
    return true;
  }
}

void EditableView::UpdateCursorPositionIfNeeded(const TextRange& range) {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  if (range.collapsed()) {
    page_view_->SetCursorPosition(range.base());
  }
#endif
}

TextEditingHistoryState::TextEditingHistoryState(EditableView* editable_view)
    : editable_view_(editable_view) {
  Push();
}

void TextEditingHistoryState::Redo() { Update(*stack_.Redo()); }

void TextEditingHistoryState::Undo() {
  if (throttle_timer_ && !throttle_timer_->Stopped()) {
    throttle_timer_->Stop();
    Update(stack_.CurrentValue());
  } else {
    Update(*stack_.Undo());
  }
}

void TextEditingHistoryState::Push() {
  if (!throttle_timer_) {
    throttle_timer_ = ThrottledPush();
  }
  if (throttle_timer_->Stopped()) {
    throttle_timer_->Start(duration_, [=]() {
      if (editable_view_) {
        auto value = editable_view_->GetTextEditingValue();
        if (!value.composing()) {
          stack_.Push(value);
        }
      }
    });
  }
}

void TextEditingHistoryState::Update(
    std::optional<TextEditingValue> new_value) {
  if (new_value.has_value() &&
      *new_value == editable_view_->GetTextEditingValue()) {
    return;
  }
  editable_view_->SetTextEditingValue(*new_value);
}

std::unique_ptr<fml::Timer> TextEditingHistoryState::ThrottledPush() {
  return std::make_unique<fml::Timer>(
      editable_view_->page_view()->GetTaskRunner(), false);
}

}  // namespace clay
