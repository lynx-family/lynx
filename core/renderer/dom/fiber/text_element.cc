// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/text_element.h"

#include <memory>
#include <utility>

#include "base/include/value/base_string.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace tasm {

TextElement::TextElement(ElementManager* manager, const base::String& tag)
    : FiberElement(manager, tag) {
  is_text_ = true;
  if (element_manager_ == nullptr) {
    return;
  }
  SetDefaultOverflow(element_manager_->GetDefaultTextOverflow());
}

void TextElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  FiberElement::AttachToElementManager(manager, style_manager, keep_element_id);
  SetDefaultOverflow(manager->GetDefaultTextOverflow());
}

void TextElement::SetStyleInternal(CSSPropertyID id,
                                   const tasm::CSSValue& value,
                                   bool force_update) {
  FiberElement::SetStyleInternal(id, value, force_update);

  if (id == kPropertyIDFontFamily) {
    if (!EnableLayoutInElementMode()) {
      EnqueueLayoutTask([this, value]() {
        ResolveAndFlushFontFaces(value.GetValue().String());
      });
    } else {
      ResolveAndFlushFontFaces(value.GetValue().String());
    }
  }
}

void TextElement::OnNodeAdded(FiberElement* child) {
  child->ConvertToInlineElement();
  UpdateRenderRootElementIfNecessary(child);
}

base::String TextElement::ConvertContent(const lepus::Value value) {
  auto result = value.String();
  if (result.empty()) {
    if (value.IsInt32()) {
      result = base::String(std::to_string(value.Int32()));
    } else if (value.IsInt64()) {
      result = base::String(std::to_string(value.Int64()));
    } else if (value.IsNumber()) {
      std::stringstream stream;
      stream << value.Number();
      result = stream.str();
    } else if (value.IsNaN()) {
      BASE_STATIC_STRING_DECL(kNaN, "NaN");
      result = kNaN;
    } else if (value.IsNil()) {
      BASE_STATIC_STRING_DECL(kNull, "null");
      result = kNull;
    } else if (value.IsUndefined()) {
      BASE_STATIC_STRING_DECL(kUndefined, "undefined");
      result = kUndefined;
    }
  }
  return result;
}

void TextElement::SetAttributeInternal(const base::String& key,
                                       const lepus::Value& value) {
  // sometimes, text-overflow is used as attribute, so we need to parse the
  // value as CSS style here. it's better to mark such kind of attribute as
  // internal attributes, which may be processed as const IDs

  BASE_STATIC_STRING_DECL(kTextAttr, "text");
  BASE_STATIC_STRING_DECL(kTextMaxlineAttr, "text-maxline");
  BASE_STATIC_STRING_DECL(kTextOverflowAttr, "text-overflow");

  if (EnableLayoutInElementMode()) {
    if (key.IsEqual(kTextAttr)) {
      content_ = ConvertContent(value);
    } else if (key.IsEqual(kTextMaxlineAttr)) {
      EnsureTextProps();
      text_props_->text_max_line =
          value.IsNumber() ? value.Number() : std::stoi(value.StdString());
    } else {
      FiberElement::SetAttributeInternal(key, value);
    }
    return;
  }

  if (key.IsEqual(kTextOverflowAttr)) {
    CacheStyleFromAttributes(kPropertyIDTextOverflow, value);
    has_layout_only_props_ = false;
  } else if (key.IsEqual(kTextAttr) && !children().empty()) {
    // if setNativeProps with key "text" on TextElement, we need to update it's
    // children.
    if (children().begin()->get()->is_raw_text()) {
      RawTextElement* raw_text =
          static_cast<RawTextElement*>(children().begin()->get());
      raw_text->SetText(value);
    }
  } else {
    FiberElement::SetAttributeInternal(key, value);
  }
}

void TextElement::ConvertToInlineElement() {
  if (tag_.IsEqual(kElementXTextTag)) {
    tag_ = BASE_STATIC_STRING(kElementXInlineTextTag);
  } else {
    tag_ = BASE_STATIC_STRING(kElementInlineTextTag);
  }
  data_model()->set_tag(tag_);
  UpdateTagToLayoutBundle();
  FiberElement::ConvertToInlineElement();
}

void TextElement::ResolveAndFlushFontFaces(const base::String& font_family) {
  auto* fragment = GetRelatedCSSFragment();
  if (fragment && !fragment->GetFontFaceRuleMap().empty() &&
      !fragment->HasFontFacesResolved()) {
    // FIXME(linxs): parse the font face according to font_family, instead of
    // flushing all font faces
    SetFontFaces(fragment->GetFontFaceRuleMap());
    fragment->MarkFontFacesResolved(true);
  }
}

bool TextElement::ResolveStyleValue(CSSPropertyID id,
                                    const tasm::CSSValue& value,
                                    bool force_update) {
  bool has_processed = false;
  if (EnableLayoutInElementMode()) {
    has_processed = ProcessTextStyles(id, value);
  }

  if (!has_processed) {
    has_processed = FiberElement::ResolveStyleValue(id, value, force_update);
    ;
  }

  return has_processed;
}

bool TextElement::ProcessTextStyles(CSSPropertyID id,
                                    const tasm::CSSValue& value) {
  bool processed = true;
  EnsureTextProps();
  switch (id) {
    case kPropertyIDFontSize:
      text_props_->font_size = value.AsNumber();
      break;
    case kPropertyIDColor:
      text_props_->color = value.AsNumber();
      break;
    case kPropertyIDLineHeight:
      text_props_->line_height = value.AsNumber();
      break;
    case kPropertyIDFontWeight:
      text_props_->font_weight = value.AsNumber();
      break;
    case kPropertyIDFontStyle:
      text_props_->font_style = value.AsNumber();
      break;
    case kPropertyIDTextOverflow:
      text_props_->text_overflow = static_cast<TextProps::TextOverflow>(
          static_cast<int>(value.AsNumber()));
      break;

    case kPropertyIDVerticalAlign: {
      auto arr = value.GetValue().Array();
      text_props_->vertical_align_type =
          static_cast<starlight::VerticalAlignType>(arr->get(0).Number());

      if (text_props_->vertical_align_type == VerticalAlignType::kLength) {
        auto pattern = static_cast<tasm::CSSValuePattern>(arr->get(3).Number());
        std::pair<starlight::NLength, bool> result =
            starlight::CSSStyleUtils::ToLength(
                tasm::CSSValue(arr->get(2), pattern),
                computed_css_style()->GetMeasureContext(),
                element_manager_->GetCSSParserConfigs());

        text_props_->vertical_align_length = result.first.GetRawValue();
      } else if (text_props_->vertical_align_type ==
                 VerticalAlignType::kPercent) {
        text_props_->vertical_align_length = arr->get(2).Number();
      } else {
        text_props_->vertical_align_length = 0.0f;
      }
    } break;
      //...
    default:
      processed = false;
      break;
  }
  return processed;
}

void TextElement::BuildTextPropsBuffer(std::string& output, PropArray* props) {
  auto start = output.length();
  output += content_.str();

  auto* child = first_render_child();
  while (child) {
    if (static_cast<FiberElement*>(child)->is_raw_text()) {
      output += static_cast<RawTextElement*>(child)->content().str();
    } else if (child->is_text()) {
      // inline text
      static_cast<TextElement*>(child)->BuildTextPropsBuffer(output, props);
    } else if (child->is_image() || child->is_view()) {
      // inline image
      output += kInlinePlaceHolder;
      static_cast<FiberElement*>(child)->BuildAttributedStringProps(
          output.length() - 1, output.length(), props);
    }
    child = child->next_render_sibling();
  }

  auto end = output.length();
  if (end > start) {
    BuildAttributedStringProps(start, end, props);
  }
}

LayoutResult TextElement::Measure(float width, int32_t width_mode, float height,
                                  int32_t height_mode, bool final_measure) {
  if (is_inline_element()) {
    return LayoutResult(0, 0, 0);
  }
  auto props = element_manager_->GetPropBundleCreator()->CreatePropArray();
  if (!props) {
    LOGE("TextElement: no PropArray defined!")
    return LayoutResult(0, 0, 0);
  }
  std::string output_str;
  BuildTextPropsBuffer(output_str, props.get());

  props->AddProp(kPropTextString);
  props->AddProp(output_str.c_str());

  return element_manager_->MeasureText(id_, props.get(), width, width_mode,
                                       height, height_mode);
}

void TextElement::OnLayoutObjectCreated() {
  if (!is_inline_element()) {
    SetMeasureFunc(
        this, [](void* context, const starlight::Constraints& constraints,
                 bool final_measure) {
          TextElement* element = static_cast<TextElement*>(context);
          DCHECK(element);
          SLMeasureMode width_mode = constraints[starlight::kHorizontal].Mode();
          SLMeasureMode height_mode = constraints[starlight::kVertical].Mode();
          float width = IsSLIndefiniteMode(width_mode)
                            ? 0.f
                            : constraints[starlight::kHorizontal].Size();
          float height = IsSLIndefiniteMode(height_mode)
                             ? 0.f
                             : constraints[starlight::kVertical].Size();

          LayoutResult result = element->Measure(width, width_mode, height,
                                                 height_mode, final_measure);

          return FloatSize(result.width_, result.height_, result.baseline_);
        });
  }
}

void TextElement::UpdateLayoutNodeFontSize(double cur_node_font_size,
                                           double root_node_font_size) {
  if (EnableLayoutInElementMode()) {
    EnsureTextProps();
    text_props_->font_size = cur_node_font_size;
  } else {
    FiberElement::UpdateLayoutNodeFontSize(cur_node_font_size,
                                           root_node_font_size);
  }
}

// static
void TextElement::BuildAttributedStringProps(size_t pos_start, size_t pos_end,
                                             PropArray* props) {
  if (text_props_) {
    // only inline text need the pass the range，   kPropRangeStart should be
    // the first key
    if (is_inline_element()) {
      props->AddProp(kPropInlineStart);
      props->AddProp(static_cast<int>(pos_start));
    }

    // FontSize
    if (text_props_->font_size) {
      props->AddProp(kTextPropFontSize);
      props->AddProp(*(text_props_->font_size));
    }
    // Color
    if (text_props_->color) {
      props->AddProp(kTextPropColor);
      props->AddProp(*text_props_->color);
    }
    // WhiteSpace
    if (text_props_->white_space) {
      props->AddProp(kTextPropWhiteSpace);
      props->AddProp(static_cast<int>(*text_props_->white_space));
    }
    // TextOverflow
    if (text_props_->text_overflow) {
      props->AddProp(kTextPropTextOverflow);
      props->AddProp(static_cast<int>(*text_props_->text_overflow));
    }
    // FontWeight
    if (text_props_->font_weight) {
      props->AddProp(kTextPropFontWeight);
      props->AddProp(static_cast<int>(*text_props_->font_weight));
    }
    // FontStyle
    if (text_props_->font_style) {
      props->AddProp(kTextPropFontStyle);
      props->AddProp(static_cast<int>(*text_props_->font_style));
    }
    // FontFamily
    if (text_props_->font_family) {
      props->AddProp(kTextPropFontFamily);
      props->AddProp(text_props_->font_family->c_str());
    }
    // LineHeight
    if (text_props_->line_height) {
      props->AddProp(kTextPropLineHeight);
      props->AddProp(*text_props_->line_height);
    }
    // letterSpacing
    if (text_props_->letter_spacing) {
      props->AddProp(kTextPropLetterSpacing);
      props->AddProp(*text_props_->letter_spacing);
    }
    // lineSpacing
    if (text_props_->line_spacing) {
      props->AddProp(kTextPropLineSpacing);
      props->AddProp(*text_props_->line_spacing);
    }
    // text_shadow
    if (text_props_->text_shadow) {
      props->AddProp(kTextPropTextShadow);
      props->AddProp(text_props_->text_shadow->c_str());
    }
    // text_decoration
    if (text_props_->text_decoration) {
      props->AddProp(kTextPropTextDecoration);
      props->AddProp(text_props_->text_decoration->c_str());
    }
    // text_align
    if (text_props_->text_align) {
      props->AddProp(kTextPropTextAlign);
      props->AddProp(static_cast<int>(*text_props_->text_align));
    }

    // vertical-align
    if (text_props_->vertical_align_type &&
        text_props_->vertical_align_length) {
      props->AddProp(kTextPropVerticalAlign);
      props->AddProp(static_cast<int>(*text_props_->vertical_align_type));
      props->AddProp(*text_props_->vertical_align_length);
    }

    // attributes
    // background_color
    if (text_props_->background_color) {
      props->AddProp(kTextPropBackGroundColor);
      props->AddProp(*text_props_->background_color);
    }
    // text_maxline
    if (text_props_->text_max_line) {
      props->AddProp(kTextPropTextMaxLine);
      props->AddProp(*text_props_->text_max_line);
    }

    // only inline text need the pass the range, kPropRangeEnd should be the
    // first key
    if (is_inline_element()) {
      props->AddProp(kPropInlineEnd);
      props->AddProp(static_cast<int>(pos_end));
    }
  }
}

}  // namespace tasm
}  // namespace lynx
