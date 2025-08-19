// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/text_element.h"

#include <memory>
#include <utility>

#include "base/include/value/base_string.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/view_element.h"

namespace lynx {
namespace tasm {

TextElement::TextElement(ElementManager* manager, const base::String& tag)
    : FiberElement(manager, tag) {
  is_text_ = true;
  if (element_manager_ == nullptr) {
    return;
  }
  SetDefaultOverflow(element_manager_->GetDefaultTextOverflow() &&
                     !EnableLayoutInElementMode());
  element_manager_->IncreaseTextElementCount();
}

void TextElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  FiberElement::AttachToElementManager(manager, style_manager, keep_element_id);
  SetDefaultOverflow(manager->GetDefaultTextOverflow() &&
                     !EnableLayoutInElementMode());
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
  if (!child->is_raw_text()) {
    has_inline_child_ = true;
  }
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
  bool processed = EnableLayoutInElementMode()
                       ? ProcessAttributeForLayoutInElement(key, value)
                       : ProcessAttributeForNormalLayoutMode(key, value);
  if (!processed) {
    FiberElement::SetAttributeInternal(key, value);
  }
}

void TextElement::ResetAttribute(const base::String& key) {
  if (!EnableLayoutInElementMode() ||
      !ProcessAttributeForLayoutInElement(key, lepus::Value(), true)) {
    FiberElement::ResetAttribute(key);
  }
}

bool TextElement::ProcessAttributeForLayoutInElement(const base::String& key,
                                                     const lepus::Value& value,
                                                     bool is_reset) {
  if (key.IsEqual(kTextAttr)) {
    content_ = !is_reset ? ConvertContent(value) : base::String();
    content_utf16_length_ =
        GetUtf16SizeFromUtf8(content_.c_str(), content_.length());
    MarkLayoutDirty();
    return true;
  }

  if (key.IsEqual(kTextMaxlineAttr)) {
    EnsureTextProps();
    text_props_->text_max_line =
        !is_reset
            ? (value.IsNumber() ? value.Number() : std::stoi(value.StdString()))
            : 1;
    MarkLayoutDirty();
    return true;
  }
  return false;
}

bool TextElement::ProcessAttributeForNormalLayoutMode(
    const base::String& key, const lepus::Value& value) {
  if (key.IsEqual(kTextOverflowAttr)) {
    CacheStyleFromAttributes(kPropertyIDTextOverflow, value);
    has_layout_only_props_ = false;
    return true;
  }

  if (key.IsEqual(kTextAttr) && !children().empty()) {
    // if setNativeProps with key "text" on TextElement, we need to update it's
    // children.
    if (children().front()->is_raw_text()) {
      auto* raw_text = static_cast<RawTextElement*>(children().front().get());
      raw_text->SetText(value);
    }
    return true;
  }
  return false;
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

  if (EnableLayoutInElementMode() && IsTextMeasurerWanted(id)) {
    if (computed_css_style()->SetValue(id, value)) {
      property_bits_.Set(id);
      has_processed = true;
    }
  } else {
    has_processed = FiberElement::ResolveStyleValue(id, value, force_update);
  }

  return has_processed;
}

bool TextElement::ResetCSSValue(CSSPropertyID id) {
  bool has_processed = false;
  if (EnableLayoutInElementMode()) {
    if (id == kPropertyIDFontSize) {
      // font-size has been reset to default value in WillResetCSSValue
      return false;
    }
    if (computed_css_style()->ResetValue(id)) {
      property_bits_.Set(id);
      has_processed = true;
    }
  } else {
    has_processed = FiberElement::ResetCSSValue(id);
  }

  return has_processed;
}

void TextElement::DispatchLayoutBefore() {
  if (is_inline_element()) {
    return;
  }

  element_manager_->DispatchLayoutBefore(this);
}

LayoutResult TextElement::Measure(float width, int32_t width_mode, float height,
                                  int32_t height_mode, bool final_measure) {
  if (is_inline_element()) {
    return LayoutResult(0, 0, 0);
  }

  return element_manager_->MeasureText(this, width, width_mode, height,
                                       height_mode);
}

void TextElement::Align() {
  if (is_inline_element() || !need_layout_children_) {
    return;
  }

  element_manager_->AlignText(this);
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

    SetAlignmentFunc(this, [](void* context) {
      TextElement* element = static_cast<TextElement*>(context);
      DCHECK(element);
      element->Align();
    });
  }
}

void TextElement::UpdateLayoutNodeFontSize(double cur_node_font_size,
                                           double root_node_font_size) {
  if (EnableLayoutInElementMode()) {
    property_bits_.Set(kPropertyIDFontSize);
  } else {
    FiberElement::UpdateLayoutNodeFontSize(cur_node_font_size,
                                           root_node_font_size);
  }
}

}  // namespace tasm
}  // namespace lynx
