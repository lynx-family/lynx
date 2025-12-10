// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/base_text_shadow_node.h"

#include <algorithm>
#include <cmath>
#include <codecvt>
#include <cstdint>
#include <limits>
#include <memory>
#include <string_view>
#include <utility>

#include "base/include/string/string_number_convert.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/component/text/text_paragraph_builder.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/resource/font_collection.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/inline_view_shadow_node.h"
#include "clay/ui/shadow/measure_utils.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/vertical_align_style.h"

namespace clay {
namespace utils = attribute_utils;
static constexpr float kDefaultFontSizeInDip = 14.f;
static const Color kDefaultTextColor = Color(0xFF000000);
static constexpr TextAlignment kDefaultAlignment = TextAlignment::kStart;

BaseTextShadowNode::BaseTextShadowNode(ShadowNodeOwner* owner, std::string tag,
                                       int id)
    : ShadowNode(owner, tag, id), weak_factory_(this) {}

void BaseTextShadowNode::AddChild(ShadowNode* node) {
  AddChild(node, ChildCount());
}

void BaseTextShadowNode::AddChild(ShadowNode* child, int index) {
  ShadowNode::AddChild(child, index);
  MarkNeedsUpdate(TextUpdateFlag::kUpdateFlagChildren);
}

void BaseTextShadowNode::RemoveChild(ShadowNode* child) {
  ShadowNode::RemoveChild(child);
  MarkNeedsUpdate(TextUpdateFlag::kUpdateFlagChildren);
}

void BaseTextShadowNode::OnLayout(float width, TextMeasureMode width_mode,
                                  float height, TextMeasureMode height_mode,
                                  const std::array<float, 4>& paddings,
                                  const std::array<float, 4>& borders) {
  ShadowNode::OnLayout(width, width_mode, height, height_mode, paddings,
                       borders);
  for (auto child : children_) {
    child->OnLayout(width, width_mode, height, height_mode, paddings, borders);
  }
}

void BaseTextShadowNode::SetAttribute(const char* attr_c,
                                      const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  SetAttribute(kw, attr_c, value);
}

void BaseTextShadowNode::SetAttribute(KeywordID kw, const char* attr_c,
                                      const clay::Value& value) {
  switch (kw) {
    case KeywordID::kFontSize:
      SetFontSize(utils::GetDouble(
          value, kDefaultFontSizeInDip * Logical2ClayPixelRatio()));
      break;
    case KeywordID::kColor:
      ResetTextColorAndGradient();
      if (value.IsUint()) {
        SetTextColor(Color(utils::GetUint(value, 0xff000000)));
      } else if (value.IsArray()) {
        const auto& array = utils::GetArray(value);
        if (array.size() == 0) {
          return;
        }
        const auto& type =
            static_cast<ClayBackgroundImageType>(utils::GetUint(array[0]));
        if (type == ClayBackgroundImageType::kLinearGradient &&
            array.size() > 1) {
          const auto& linear_array = utils::GetArray(array[1]);
          std::optional<Gradient> g = Gradient::CreateLinear(linear_array);
          if (g.has_value()) {
            SetTextGradient(g.value());
          }
        }
      }
      break;
    case KeywordID::kTextOverflow:
      SetTextOverflow(static_cast<TextOverflow>(utils::GetInt(value)));
      break;
    case KeywordID::kFontWeight:
      SetFontWeight(static_cast<FontWeight>(utils::GetInt(value)));
      break;
    case KeywordID::kFontStyle:
      SetFontStyle(static_cast<FontStyle>(utils::GetInt(value)));
      break;
    case KeywordID::kLineHeight: {
      double line_height = 0.f;
      if (utils::TryGetNum(value, line_height)) {
        SetLineHeight(line_height);
      }
    } break;
    case KeywordID::kLineSpacing:
      SetLineSpacing(utils::GetDouble(value));
      break;
    case KeywordID::kLetterSpacing:
      SetLetterSpacing(utils::GetDouble(value));
      break;
    case KeywordID::kFontFamily:
      SetFontFamily(utils::GetCString(value));
      break;
    case KeywordID::kTextAlign:
      SetTextAlign(static_cast<TextAlignment>(utils::GetInt(value)));
      break;
    case KeywordID::kTextDecoration: {
      const auto& decoration_array = utils::GetArray(value);
      if (decoration_array.size() != 3) {
        return;
      }
      auto decoration_type = utils::GetInt(decoration_array[0]);
      auto decoration_style = utils::GetInt(decoration_array[1]);
      auto decoration_color = utils::GetInt(decoration_array[2]);
      auto decoration = GetTextDecoration().value_or(TextDecoration());
      int decoration_line = 0;
      if (decoration_type &
          static_cast<int>(ClayTextDecorationType::kUnderLine)) {
        decoration_line |= static_cast<int>(kTextDecorationLineUnderline);
      }
      if (decoration_type &
          static_cast<int>(ClayTextDecorationType::kLineThrough)) {
        decoration_line |= static_cast<int>(kTextDecorationLineLineThrough);
      }
      decoration.style = static_cast<TextDecorationStyle>(decoration_style);
      decoration.line = decoration_line;
      decoration.color = decoration_color;
      SetTextDecoration(decoration);
    } break;
    case KeywordID::kDirection: {
      double text_direction = 0.0;
      if (attribute_utils::TryGetNum(value, text_direction)) {
        SetTextDirection(static_cast<TextDirection>(text_direction));
      }
    } break;
    case KeywordID::kTextShadow: {
      const auto& array = utils::GetArray(value);
      std::vector<Shadow> shadows(array.size());
      for (size_t i = 0; i < array.size(); i++) {
        const auto& arr = utils::GetArray(array[i]);
        shadows[i].offset_x = utils::GetDouble(arr[0]);
        shadows[i].offset_y = utils::GetDouble(arr[1]);
        shadows[i].blur_radius = utils::GetDouble(arr[2]);
        shadows[i].spread_radius = utils::GetDouble(arr[3]);
        auto option = static_cast<ClayShadowOption>(utils::GetInt(arr[4]));
        shadows[i].inset = (option == ClayShadowOption::kInset);
        shadows[i].color = Color(utils::GetUint(arr[5]));
      }
      SetTextShadows(std::move(shadows));
    } break;
    case KeywordID::kTextStrokeColor: {
      auto color = Color(utils::GetUint(value, 0xff000000));
      SetTextStrokeColor(color);
    } break;
    case KeywordID::kTextStrokeWidth: {
      auto width = utils::GetDouble(value);
      SetTextStrokeWidth(width);
    } break;
    case KeywordID::kEnableFontScaling:
      break;
    case KeywordID::kTextMaxline: {
      int max_lines = 0;
      if (lynx::base::StringToInt(utils::GetCString(value), &max_lines)) {
        SetTextMaxLine(static_cast<uint32_t>(max_lines) > 0
                           ? static_cast<uint32_t>(max_lines)
                           : std::numeric_limits<uint32_t>::max());
      } else {
        SetTextMaxLine(std::numeric_limits<uint32_t>::max());
      }
    } break;
    case KeywordID::kWordBreak:
      SetWordBreak(static_cast<WordBreak>(utils::GetInt(value)));
      break;
    case KeywordID::kWhiteSpace:
      SetWhiteSpaceType(static_cast<WhiteSpace>(utils::GetInt(value)));
      break;
    case KeywordID::kTextIndent: {
      const auto& array = utils::GetArray(value);
      if (array.size() < 2) {
        return;
      }
      text_indent_use_percent_ = utils::GetInt(array[1]);
      SetTextIndent(static_cast<double>(utils::GetDouble(array[0])));
    } break;
    case KeywordID::kTextMaxlength:
      max_length_ = utils::GetInt(value);
      break;
    case KeywordID::kXAutoFontSize: {
      const auto& array = utils::GetArray(value);
      FML_DCHECK(array.size() == 4);
      if (array.size() != 4) {
        return;
      }
      enable_auto_font_size_ = utils::GetBool(array[0]);
      auto_font_size_min_size_ = utils::GetDouble(array[1]);
      auto_font_size_max_size_ = utils::GetDouble(array[2]);
      auto_font_size_step_granularity_ = utils::GetDouble(array[3]);
    } break;
    case KeywordID::kXAutoFontSizePresetSizes: {
      const auto& array = utils::GetArray(value);
      for (size_t i = 0; i < array.size(); i++) {
        auto_font_size_preset_sizes_.push_back(utils::GetDouble(array[i]));
      }
      std::sort(auto_font_size_preset_sizes_.begin(),
                auto_font_size_preset_sizes_.end());
    } break;
    case clay::KeywordID::kTextSingleLineVerticalAlign: {
      SetTextSingleLineVerticalAlign(value);
    }
    default:
      if (attr_c) {
        auto kw = GetKeywordID(attr_c);
        if (kw == KeywordID::kText) {
          CreateRawTextNodeIfNeed(attribute_utils::GetCString(value));
        }
        ShadowNode::SetAttribute(attr_c, value);
      }
      break;
  }
}

void BaseTextShadowNode::CreateRawTextNodeIfNeed(std::string text) {
  if (IsTextShadowNode() || IsInlineTextShadowNode()) {
    for (auto child : GetChildren()) {
      if (child->IsRawTextShadowNode()) {
        if (child->id() > 0) {
          return;
        } else {
          static_cast<RawTextShadowNode*>(child)->SetText(text);
          return;
        }
      }
    }
    auto raw_text = new RawTextShadowNode(owner_, "raw-text", -1);
    raw_text->SetText(text);
    MarkNeedsUpdate(TextUpdateFlag::kUpdateFlagChildren);
    AddChild(raw_text);
  }
}

void BaseTextShadowNode::EnsureDefaultStyle() {
  if (text_style_) {
    return;
  }
  text_style_ = std::make_optional<TextStyle>();
  text_style_->font_size = kDefaultFontSizeInDip * Logical2ClayPixelRatio();
  text_style_->text_color = kDefaultTextColor;
  text_style_->text_align = kDefaultAlignment;
}

void BaseTextShadowNode::SetFontSize(float font_size) {
  EnsureDefaultStyle();
  if (text_style_->font_size != font_size) {
    text_style_->font_size = font_size;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetLineHeight(float line_height) {
  EnsureDefaultStyle();
  if (line_height_.has_value() && line_height == *line_height_) {
    return;
  }
  if (MeasureUtils::isUndefined(line_height)) {
    line_height_.reset();
  } else {
    line_height_ = line_height;
  }

  MarkDirty();
}

void BaseTextShadowNode::SetLineSpacing(float line_spacing) {
  EnsureDefaultStyle();
  if (text_style_->line_spacing != line_spacing) {
    text_style_->line_spacing = line_spacing;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetLetterSpacing(float letter_spacing) {
  EnsureDefaultStyle();
  if (text_style_->letter_spacing != letter_spacing) {
    text_style_->letter_spacing = letter_spacing;
    MarkDirty();
  }
}
void BaseTextShadowNode::SetTextAlign(TextAlignment text_align) {
  EnsureDefaultStyle();
  if (text_style_->text_align != text_align) {
    text_style_->text_align = text_align;
    MarkDirty();
  }
}
void BaseTextShadowNode::SetFontWeight(FontWeight font_weight) {
  EnsureDefaultStyle();
  if (text_style_->font_weight != font_weight) {
    text_style_->font_weight = font_weight;
    MarkDirty();
  }
}
void BaseTextShadowNode::SetFontStyle(FontStyle font_style) {
  EnsureDefaultStyle();
  if (text_style_->font_style != font_style) {
    text_style_->font_style = font_style;
    MarkDirty();
  }
}
void BaseTextShadowNode::SetTextColor(const Color& text_color) {
  EnsureDefaultStyle();
  if (text_style_->text_color != text_color) {
    text_style_->text_color = text_color;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetTextBackgroundColor(const Color& color) {
  EnsureDefaultStyle();
  if (text_style_->background_color != color) {
    text_style_->background_color = color;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetFontFamily(const std::string& font_family) {
  EnsureDefaultStyle();
  auto font_collection = Isolate::Instance().GetFontCollection();
  std::string_view str_view = font_family;
  std::vector<std::string> font_families;
  lynx::base::SplitString(str_view, ',', font_families);
  for (auto& new_font_family : font_families) {
    if (new_font_family.length() > 1) {
      new_font_family = lynx::base::TrimString(
          new_font_family, " ", lynx::base::TrimPositions::TRIM_ALL);
      if (new_font_family.length() > 1) {
        new_font_family = lynx::base::TrimString(
            new_font_family, "\'", lynx::base::TrimPositions::TRIM_ALL);
      }
      if (new_font_family.length() > 1) {
        new_font_family = lynx::base::TrimString(
            new_font_family, "\"", lynx::base::TrimPositions::TRIM_ALL);
      }
    }
    if (font_collection->HasFontResourceLoading(new_font_family)) {
      RelayoutWhenSetFontFamily(new_font_family);
    } else if (font_collection->HasFontResource(new_font_family)) {
      text_style_->font_family = new_font_family;
      break;
    } else if (font_collection->IfSystemFontFamily(new_font_family)) {
      text_style_->font_family = new_font_family;
      break;
    }
  }
  MarkDirty();
}

void BaseTextShadowNode::SetTextDecoration(
    const TextDecoration& text_decoration) {
  EnsureDefaultStyle();
  if (text_style_->text_decoration != text_decoration) {
    text_style_->text_decoration = text_decoration;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetTextDirection(TextDirection text_direction) {
  EnsureDefaultStyle();
  if (text_direction == TextDirection::kNormal) {
    text_direction = TextDirection::kLtr;
  }
  if (text_style_->text_direction != text_direction) {
    text_style_->text_direction = text_direction;
    MarkDirty();
  }
}

void BaseTextShadowNode::AppendTextShadow(Shadow&& text_shadow) {
  if (!text_style_) {
    text_style_ = std::make_optional<TextStyle>();
  }
  if (!text_style_->text_shadows) {
    text_style_->text_shadows = std::vector<Shadow>();
  }

  text_style_->text_shadows->emplace_back(std::move(text_shadow));
  MarkDirty();
}

void BaseTextShadowNode::SetTextShadows(std::vector<Shadow>&& text_shadows) {
  EnsureDefaultStyle();
  if (text_style_->text_shadows != text_shadows) {
    text_style_->text_shadows = std::move(text_shadows);
    MarkDirty();
  }
}

void BaseTextShadowNode::SetTextStrokeColor(Color color) {
  EnsureDefaultStyle();
  if (stroke_color_ != color) {
    stroke_color_ = color;
    MarkDirty();
  }
  text_style_->foreground_id = id_;
}
void BaseTextShadowNode::SetTextStrokeWidth(double width) {
  EnsureDefaultStyle();
  if (stroke_width_ != width) {
    stroke_width_ = width;
    MarkDirty();
  }
  text_style_->foreground_id = id_;
}

void BaseTextShadowNode::SetTextGradient(const Gradient& gradient) {
  EnsureDefaultStyle();
  if (text_style_->text_gradient != gradient) {
    text_style_->text_gradient = gradient;
    text_style_->foreground_id = id_;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetTextMaxLine(uint32_t max_lines) {
  EnsureDefaultStyle();
  if (text_style_->max_lines != max_lines) {
    text_style_->max_lines = max_lines;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetTextOverflow(TextOverflow overflow) {
  EnsureDefaultStyle();
  if (text_style_->overflow != overflow) {
    text_style_->overflow = overflow;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetTextEllipsis(std::u16string ellipsis) {
  EnsureDefaultStyle();
  if (text_style_->ellipsis != ellipsis) {
    text_style_->ellipsis = std::move(ellipsis);
    MarkDirty();
  }
}

void BaseTextShadowNode::SetWordBreak(WordBreak break_type) {
  EnsureDefaultStyle();
  if (text_style_->word_break != break_type) {
    text_style_->word_break = break_type;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetWhiteSpaceType(WhiteSpace white_space) {
  EnsureDefaultStyle();
  if (text_style_->white_space != white_space) {
    text_style_->white_space = white_space;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetTextIndent(double text_indent) {
  EnsureDefaultStyle();
  if (text_indent_ != text_indent) {
    text_indent_ = text_indent;
    MarkDirty();
  }
}

void BaseTextShadowNode::SetTextSingleLineVerticalAlign(
    const clay::Value& value) {
  EnsureDefaultStyle();
  auto type = attribute_utils::GetCString(value);
  if (type == "bottom") {
    text_style_->align_type = VerticalAlignType::kVerticalAlignBottom;
  } else if (type == "center") {
    text_style_->align_type = VerticalAlignType::kVerticalAlignCenter;
  } else if (type == "top") {
    text_style_->align_type = VerticalAlignType::kVerticalAlignTop;
  }
  text_style_->enable_text_bounds = true;
}

void BaseTextShadowNode::PreLayout(PreLayoutContext* context) {
  auto* context_text = static_cast<PreLayoutContextText*>(context);
  for (ShadowNode* child : children_) {
    child->PreLayout(context_text);
  }
}

void BaseTextShadowNode::TextLayout(LayoutContext* context) {
  TRACE_EVENT("clay", "BaseTextShadowNode::TextLayout");
  TextParagraphBuilder* builder =
      static_cast<LayoutContextText*>(context)->builder();

  EnsureDefaultStyle();
  builder->PushStyle(text_style_.value());
  ProcessChildLayout(context);
  if (text_style_) {
    builder->Pop();
  }
}

void BaseTextShadowNode::ProcessChildLayout(LayoutContext* context) {
  for (ShadowNode* child : children_) {
    if (child->IsInlineTextShadowNode() || child->IsInlineImageShadowNode()) {
      child->UpdateLayoutStylesFromLynx();
    }
    if (child->MarginLeft() > 0.f || child->PaddingLeft() > 0.f) {
      static_cast<LayoutContextText*>(context)->AddFakePlaceholder(
          child->MarginLeft() + child->PaddingLeft());
    }

    child->TextLayout(context);

    if (child->MarginRight() > 0.f || child->PaddingRight() > 0.f) {
      static_cast<LayoutContextText*>(context)->AddFakePlaceholder(
          child->MarginRight() + child->PaddingRight());
    }
  }
}

void BaseTextShadowNode::RelayoutWhenSetFontFamily(
    const std::string& font_family) {
  auto font_collection = Isolate::Instance().GetFontCollection();

  if (font_collection) {
    font_collection->RegisterCallback(
        font_family, [self = weak_factory_.GetWeakPtr(), font_family]() {
          if (!self) {
            return;
          }
          self->text_style_->font_family = font_family;
          self->MarkDirty();
        });
  }
}

void BaseTextShadowNode::MeasureInlineView(
    const MeasureConstraint& constraint) {
  for (auto* child : children_) {
    if (child->IsInlineViewShadowNode()) {
      auto result =
          static_cast<InlineViewShadowNode*>(child)->MeasureNativeNode(
              constraint);
      child->SetWidth(result.width);
      child->SetHeight(result.height);
    } else if (child->IsBaseTextShadowNode()) {
      static_cast<BaseTextShadowNode*>(child)->MeasureInlineView(constraint);
    }
  }
}

void BaseTextShadowNode::AlignNativeNode(txt::Paragraph* paragraph) {
  for (auto* child : children_) {
    if (child->IsInlineViewShadowNode()) {
      auto inline_view_shadow_node = std::make_shared<InlineViewShadowNode>(
          *static_cast<InlineViewShadowNode*>(child));
      auto text_box =
          paragraph->GetRectsForRange(inline_view_shadow_node->StartGlyph(),
                                      inline_view_shadow_node->EndGlyph(),
                                      txt::Paragraph::RectHeightStyle::kTight,
                                      txt::Paragraph::RectWidthStyle::kTight);
      if (child->GetEndIndex() > 0 && text_box.size() > 0) {
        // When sets text-overflow: ellipsis, when the view and ellipsis
        // are adjacent, the textbox will return two values ​​(a box of
        // the image and a box of ellipsis, which may be a bug of skia).
        // Here we use text-direction to avoid this problem.
        if (text_box.back().direction == txt::TextDirection::rtl) {
          if (width_mode_ != TextMeasureMode::kDefinite) {
            inline_view_shadow_node->AlignNativeNode(
                text_box.back().rect.Top(),
                text_box.back().rect.Left() -
                    std::max(paragraph->GetMaxWidth() -
                                 paragraph->GetMaxIntrinsicWidth(),
                             0.0));
          } else {
            inline_view_shadow_node->AlignNativeNode(
                text_box.back().rect.Top(), text_box.back().rect.Left());
          }
        } else {
          inline_view_shadow_node->AlignNativeNode(
              text_box.front().rect.Top(), text_box.front().rect.Left());
        }
      } else {
        inline_view_shadow_node->SetEndIndex(0);
      }
    } else {
      static_cast<BaseTextShadowNode*>(child)->AlignNativeNode(paragraph);
    }
  }
}

void BaseTextShadowNode::CollectMaxLineHeight(float& line_height,
                                              float& font_size) {
  if (line_height_.has_value() && *line_height_ > line_height) {
    line_height = *line_height_;
  }
  if (text_style_.has_value() && text_style_->font_size.has_value() &&
      text_style_->font_size.value() > font_size) {
    font_size = text_style_->font_size.value();
  }

  for (auto* child : children_) {
    if (child->IsBaseTextShadowNode()) {
      static_cast<BaseTextShadowNode*>(child)->CollectMaxLineHeight(line_height,
                                                                    font_size);
    }
  }
}

void BaseTextShadowNode::ResetTextColorAndGradient() {
  EnsureDefaultStyle();
  if (text_style_->text_color.has_value()) {
    text_style_->text_color = std::nullopt;
  }
  if (text_style_->text_gradient.has_value()) {
    text_style_->text_gradient = std::nullopt;
  }
}

std::u16string BaseTextShadowNode::GetRawText() {
  if (GetChildren().empty()) {
    return std::u16string();
  } else {
    auto result = std::u16string();
    for (auto* child : children_) {
      if (child->IsRawTextShadowNode()) {
        result += static_cast<RawTextShadowNode*>(child)->Text();
      } else if (child->IsInlineTextShadowNode()) {
        result += static_cast<InlineTextShadowNode*>(child)->GetRawText();
      }
    }
    return result;
  }
}

}  // namespace clay
