// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/base_text_shadow_node.h"

#include <utility>
#include <vector>

#include "base/include/string/quickjs_dtoa.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/harmony/props_constant.h"
#include "core/renderer/tasm/config.h"
#include "core/style/color.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/font/font_face_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/raw_text_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/utils/unicode_decode_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_linear_gradient_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_radial_gradient_layer.h"

namespace lynx {
namespace tasm {
namespace harmony {

BaseTextShadowNode::BaseTextShadowNode(int sign, const std::string& tag)
    : ShadowNode(sign, tag) {}

void BaseTextShadowNode::OnContextReady() {
  const auto font_manager = context_->GetFontFaceManager();
  style_.SetDefaultFontFamily(
      font_manager ? font_manager->GetDefaultFontFamily() : "");
  style_.SetColor(starlight::DefaultColor::DEFAULT_TEXT_COLOR);
  style_.SetFontSize(context_->DefaultFontSize() * ScaleDensity());
}

void BaseTextShadowNode::OnPropsUpdate(const std::string& name,
                                       const lepus::Value& value) {
  char const* attr = name.c_str();
  PrepareTextProps();
  if (base::StringEqual(attr, kColor)) {
    SetColor(value);
  } else if (base::StringEqual(attr, kFontSize)) {
    text_props_->font_size = value.Number() * ScaleDensity();
    style_.SetFontSize(text_props_->font_size);
  } else if (base::StringEqual(attr, kLineHeight)) {
    text_props_->line_height = value.Number() * ScaleDensity();
  } else if (base::StringEqual(attr, kFontFamily)) {
    text_props_->font_family = value.StdString();
    SetRawFontFamilies(text_props_->font_family);
    style_.ClearCustomFontFamilies();
  } else if (base::StringEqual(attr, kFontWeight)) {
    text_props_->font_weight = static_cast<FontWeight>(value.Number());
    style_.SetFontWeightToStyle(text_props_->font_weight);
  } else if (base::StringEqual(attr, kFontStyle)) {
    text_props_->font_style = static_cast<FontStyle>(value.Number());
    style_.SetFontStyleToStyle(text_props_->font_style);
  } else if (base::StringEqual(attr, kLetterSpacing)) {
    text_props_->letter_spacing = value.Number() * ScaleDensity();
    style_.SetLetterSpacing(text_props_->letter_spacing);
  } else if (base::StringEqual(attr, kVerticalAlign)) {
    const auto* val_array = value.Array().get();
    if (val_array->size() == 2) {
      text_props_->vertical_align_style_.vertical_align_type =
          static_cast<starlight::VerticalAlignType>(val_array->get(0).Number());
      text_props_->vertical_align_style_.baseline_offset =
          val_array->get(1).Number();
    }
  } else if (base::StringEqual(attr, kTextDecoration)) {
    const auto val_array = value.Array();
    if (value.IsArray() && val_array->size() == 3) {
      int decoration_line = static_cast<int>(val_array->get(0).Number());
      int decoration_style = static_cast<int>(val_array->get(1).Number());
      uint32_t decoration_color = val_array->get(2).UInt32();
      text_props_->text_decoration_color = decoration_color;
      text_props_->text_decoration_line = decoration_line;
      text_props_->text_decoration_style = decoration_style;
      style_.SetTextDecoration(decoration_line);
      style_.SetTextDecorationColor(decoration_color);
      style_.SetTextDecorationStyle(decoration_style);
    } else {
      text_props_->text_decoration_color = 0;
      text_props_->text_decoration_line = 0;
      text_props_->text_decoration_style = 0;
      style_.SetTextDecoration(0);
      style_.SetTextDecorationColor(0);
      style_.SetTextDecorationStyle(0);
    }
  } else if (base::StringEqual(attr, kWordBreak)) {
    word_break_ = static_cast<starlight::WordBreakType>(value.Number());
    text_props_->word_break = word_break_;
  } else if (base::StringEqual(attr, kTextStrokeWidth)) {
    text_props_->stroke_width = value.Number() * ScaleDensity();
    style_.SetStrokeWidth(text_props_->stroke_width);
  } else if (base::StringEqual(attr, kTextStrokeColor)) {
    text_props_->stroke_color = value.UInt32();
    style_.SetStrokeColor(text_props_->stroke_color);
  } else if (base::StringEqual(attr, kTextShadow)) {
    SetTextShadow(value);
  } else if (base::StringEqual(attr, kTextAttr)) {
    if (value.IsString()) {
      text_ = value.ToString();
    } else if (value.IsNumber()) {
      char tmp[128];
      js_dtoa(tmp, value.Number());
      text_ = tmp;
    } else if (value.IsBool()) {
      text_ = value.Bool() ? "true" : "false";
    }
  } else {
    ShadowNode::OnPropsUpdate(name, value);
  }
  MarkDirty();
}

void BaseTextShadowNode::SetCustomFontFamiliesToStyle() {
  if (!raw_font_families_.empty() && style_.GetCustomFontFamilies().empty()) {
    std::vector<std::string> new_custom_families =
        context_->GetFontFaceManager()->GetCustomFamiliesFromRawVector(
            raw_font_families_);
    style_.SetCustomFontFamilyVector(std::move(new_custom_families));
  }
}

void BaseTextShadowNode::AppendToParagraph(ParagraphBuilderHarmony& builder,
                                           float width, float height) {
  if (!text_props_.has_value() ||
      static_cast<int64_t>(text_props_->line_height) == kLineHeightNormal) {
    style_.SetFontHeight(1.4f);
  } else {
    double font_size = text_props_->font_size != -1
                           ? text_props_->font_size
                           : context_->DefaultFontSize() * ScaleDensity();
    style_.SetFontHeight(font_size ? text_props_->line_height / font_size : 0);
  }

  style_.SetHalfLeading(true);
  style_.UpdateTextPaint(width, height, context_->ScaledDensity());

  // we need to set font families before PushTextStyle!
  if (!LynxEnv::GetInstance().EnableGlobalFontCollection()) {
    style_.SetCustomFontFamilyVector(raw_font_families_);
  } else {
    SetCustomFontFamiliesToStyle();
  }

  builder.PushTextStyle(style_);
  OnAppendToParagraph(builder, width, height);
  builder.PopTextStyle();
}

void BaseTextShadowNode::OnAppendToParagraph(ParagraphBuilderHarmony& builder,
                                             float width, float height) {
  if (GetChildren().empty() && !text().empty()) {
    AddTextToBuilder(builder, text());
  }
  for (const auto& child : GetChildren()) {
    if (static_cast<BaseTextShadowNode*>(child)->IsInlineTruncation()) {
      // skip inline truncation
      continue;
    }
    if (auto* b = child->AsParagraphContent()) {
      b->AppendToParagraph(builder, width, height);
    }
  }
}

void BaseTextShadowNode::LoadFontFamilyIfNeeded(
    const std::vector<std::string>& raw_font_families,
    FontCollectionHarmony* font_collection) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, BASE_TEXT_SHADOW_NODE_LOAD_FONT_FAMILY);
  // try to load font from loader or pre-register
  if (!context_) {
    return;
  }
  auto font_face_manager = context_->GetFontFaceManager();
  if (!font_face_manager) {
    return;
  }
  for (const auto& raw_font_family : raw_font_families) {
    auto it = font_face_manager->GetFontFaces().find(raw_font_family);
    if (it != font_face_manager->GetFontFaces().end()) {
      // has custom font families
      const std::vector<FontFace::FontSrcData>& font_src_data =
          it->second.GetSrcData();
      for (auto& font_src : font_src_data) {
        const auto& font_family = font_src.unique_custom_font_family;
        if (font_family.empty() ||
            font_collection->GetFontLoadingState(font_family) ==
                FontCollectionHarmony::FontLoadingState::kLoaded) {
          continue;
        }

        text_props_->loading_font_sign++;
        auto weak_font_manager = std::weak_ptr(font_face_manager);
        auto sign = Signature();
        font_face_manager->LoadFontWithUrl(
            sign, font_family, font_src.src, font_src.type,
            [this, font_collection, weak_font_manager, sign](
                const std::string& font_family, int32_t ret_code,
                std::vector<uint8_t>& data, size_t length) {
              auto shared_font_manager = weak_font_manager.lock();
              text_props_->loading_font_sign--;
              if (length > 0 && shared_font_manager) {
                if ((font_collection->GetFontLoadingState(font_family) !=
                     FontCollectionHarmony::FontLoadingState::kLoaded)) {
                  font_collection->SetLoadingFontState(
                      font_family,
                      FontCollectionHarmony::FontLoadingState::kLoaded);

                  shared_font_manager->AddLoadingShadowNodes(
                      sign, const_cast<BaseTextShadowNode*>(this));

                  font_collection->RegisterFontBuffer(
                      font_family.c_str(), data, length,
                      [weak_font_manager, sign]() {
                        auto shared_font_manager = weak_font_manager.lock();
                        if (shared_font_manager) {
                          shared_font_manager->TryMarkDirtyOnLayoutThread(sign);
                        }
                      });
                }
              }
            });
      }
    }
  }
}

bool BaseTextShadowNode::BuildAttributedString(AttributedString& out) {
  TextFragment fragment;
  if (IsPlaceholder()) {
    // if contains placeholder return false
    return false;
  }
  if (IsBindEvent()) {
    return false;
  }
  if (IsRawText()) {
    fragment.text_content = static_cast<RawTextShadowNode*>(this)->text();
  } else {
    fragment.text_props = text_props_;
  }
  out.emplace_back(std::move(fragment));

  for (const auto& child : GetChildren()) {
    if (static_cast<BaseTextShadowNode*>(child)->IsInlineTruncation()) {
      // skip inline truncation
      continue;
    }
    if (!static_cast<BaseTextShadowNode*>(child)->BuildAttributedString(out)) {
      return false;
    }
  }
  return true;
}

void BaseTextShadowNode::SetTextShadow(const lepus::Value& shadow) {
  if (!shadow.IsArray() || shadow.Array()->size() == 0) {
    text_props_->shadow_data = std::nullopt;
    style_.SetShadowLayer(std::nullopt);
    return;
  }

  const auto& shadow_array = shadow.Array()->get(0).Array();
  if (shadow_array->size() < 6) {
    text_props_->shadow_data = std::nullopt;
  } else {
    text_props_->shadow_data = {
        .blur_radius =
            static_cast<float>(shadow_array->get(2).Number() * ScaleDensity()),
        .x = static_cast<float>(shadow_array->get(0).Number() * ScaleDensity()),
        .y = static_cast<float>(shadow_array->get(1).Number() * ScaleDensity()),
        .color = shadow_array->get(5).UInt32()};
  }
  style_.SetShadowLayer(text_props_->shadow_data);
}

void BaseTextShadowNode::SetColor(const lepus::Value& color) {
  if (color.IsUInt32()) {
    text_props_->color = color.UInt32();
    style_.SetColor(text_props_->color);
    style_.SetGradientColor(nullptr);
  } else if (color.IsArray()) {
    text_props_->color = 0;
    // text with gradient color should set the color to transparent here.
    style_.SetColor(0);
    // TODO(renzhongyue): Add gradient to text_props_ to gen hash key.
    const auto& val_array = color.Array();
    if (val_array->size() != 2 || !val_array->get(1).IsArray()) {
      style_.SetGradientColor(nullptr);
      return;
    }

    if (const auto type = static_cast<starlight::BackgroundImageType>(
            val_array->get(0).Number());
        type == starlight::BackgroundImageType::kLinearGradient) {
      style_.SetGradientColor(std::unique_ptr<BackgroundGradientLayer>(
          new BackgroundLinearGradientLayer(val_array->get(1))));
    } else if (type == starlight::BackgroundImageType::kRadialGradient) {
      style_.SetGradientColor(std::unique_ptr<BackgroundGradientLayer>(
          new BackgroundRadialGradientLayer(val_array->get(1))));
    }
    text_props_->gradient_color = style_.GradientColor();
  }
}

void BaseTextShadowNode::UpdateWordBreakType(
    starlight::WordBreakType word_break) {
  for (const auto& child : GetChildren()) {
    auto* text_node = static_cast<BaseTextShadowNode*>(child);
    // override word break
    text_node->UpdateWordBreakType(word_break_);
  }
}

void BaseTextShadowNode::AddTextToBuilder(ParagraphBuilderHarmony& builder,
                                          const std::string& text) {
  if (!text.empty()) {
    auto property = UnicodeDecodeProperty::kDefault;
    if (word_break_ == starlight::WordBreakType::kBreakAll) {
      property = UnicodeDecodeProperty::kInsertZeroWidthChar;
    } else if (word_break_ == starlight::WordBreakType::kKeepAll) {
      property = UnicodeDecodeProperty::kCjkInsertWordJoiner;
    }
    const auto converted_text = UnicodeDecodeUtils::Decode(text_, property);
    builder.AddText(converted_text.c_str());
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
