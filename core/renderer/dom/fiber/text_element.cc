// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/text_element.h"

#include <memory>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"

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
    HandleLayoutTask([this, value]() {
      ResolveAndFlushFontFaces(value.GetValue().String());
    });
  }
}

void TextElement::OnNodeAdded(FiberElement* child) {
  child->ConvertToInlineElement();
  UpdateRenderRootElementIfNecessary(child);
}

void TextElement::SetAttributeInternal(const base::String& key,
                                       const lepus::Value& value) {
  // sometimes, text-overflow is used as attribute, so we need to parse the
  // value as CSS style here. it's better to mark such kind of attribute as
  // internal attributes, which may be processed as const IDs
  if (key.IsEqual("text-overflow")) {
    CacheStyleFromAttributes(kPropertyIDTextOverflow, value);
    has_layout_only_props_ = false;
  } else if (key.IsEqual("text") && !children().empty()) {
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

}  // namespace tasm
}  // namespace lynx
