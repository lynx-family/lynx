// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/scroll_element.h"

#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace tasm {

void ScrollElement::OnNodeAdded(FiberElement* child) {
  FiberElement::OnNodeAdded(child);

  // Scroll's child should not be layout only.
  child->MarkCanBeLayoutOnly(false);
}

const StyleMap* ScrollElement::PeekCommittedStylesFromAttributes() const {
  if (!committed_styles_from_attributes_.has_value()) {
    return nullptr;
  }
  return &*committed_styles_from_attributes_;
}

void ScrollElement::CacheCommittedStyleFromAttributes(CSSPropertyID id,
                                                      const CSSValue& value) {
  committed_styles_from_attributes_->insert_or_assign(id, value);
}

void ScrollElement::CacheCommittedStyleFromAttributes(
    CSSPropertyID id, const lepus::Value& value) {
  UnitHandler::Process(id, value, *committed_styles_from_attributes_,
                       element_manager()->GetCSSParserConfigs());
}

void ScrollElement::RemoveCommittedStyleFromAttributes(CSSPropertyID id) {
  if (!committed_styles_from_attributes_.has_value()) {
    return;
  }
  committed_styles_from_attributes_->erase(id);
  if (committed_styles_from_attributes_->empty()) {
    committed_styles_from_attributes_.reset();
  }
}

void ScrollElement::SetAttributeInternal(const base::String& key,
                                         const lepus::Value& value) {
  FiberElement::SetAttributeInternal(key, value);

  const auto& value_str = value.StdString();
  if (key.IsEquals(kScrollX) && value_str == kTrue) {
    CacheStyleFromAttributes(
        kPropertyIDLinearOrientation,
        CSSValue(starlight::LinearOrientationType::kHorizontal));
    HandleLayoutNodeAttributeUpdate();
  } else if (key.IsEquals(kScrollY) && value_str == kTrue) {
    CacheStyleFromAttributes(
        kPropertyIDLinearOrientation,
        CSSValue(starlight::LinearOrientationType::kVertical));
    HandleLayoutNodeAttributeUpdate();
  } else if (key.IsEquals(kScrollOrientation)) {
    if (value_str == kHorizontal) {
      CacheStyleFromAttributes(
          kPropertyIDLinearOrientation,
          CSSValue(starlight::LinearOrientationType::kHorizontal));
      HandleLayoutNodeAttributeUpdate();
    } else if (value_str == kVertical) {
      CacheStyleFromAttributes(
          kPropertyIDLinearOrientation,
          CSSValue(starlight::LinearOrientationType::kVertical));
      HandleLayoutNodeAttributeUpdate();
    }
    //(TODO)fangzhou.fz: If it becomes necessary in the future, extend the
    //'both' mode.
  } else if (key.IsEquals(kScrollXReverse) && value_str == kTrue) {
    CacheStyleFromAttributes(
        kPropertyIDLinearOrientation,
        CSSValue(starlight::LinearOrientationType::kHorizontalReverse));
    HandleLayoutNodeAttributeUpdate();
  } else if (key.IsEquals(kScrollYReverse) && value_str == kTrue) {
    CacheStyleFromAttributes(
        kPropertyIDLinearOrientation,
        CSSValue(starlight::LinearOrientationType::kVerticalReverse));
    HandleLayoutNodeAttributeUpdate();
  } else if (key.IsEquals(kScrollNewArch) && value_str == kTrue) {
    platform_node_tag_ = BASE_STATIC_STRING(kScrollNewArch);
  }
}

void ScrollElement::ResetAttribute(const base::String& key) {
  FiberElement::ResetAttribute(key);

  if (key.IsEquals(kScrollX) || key.IsEquals(kScrollY) ||
      key.IsEquals(kScrollOrientation) || key.IsEquals(kScrollXReverse) ||
      key.IsEquals(kScrollYReverse)) {
    RemoveStyleFromAttributes(kPropertyIDLinearOrientation);
    MarkStyleDirty(false);
  }
}

void ScrollElement::HandleLayoutNodeAttributeUpdate() {
  UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                            lepus::Value(true));
}

}  // namespace tasm
}  // namespace lynx
