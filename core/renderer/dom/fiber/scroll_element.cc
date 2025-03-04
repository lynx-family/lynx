// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/scroll_element.h"

#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace tasm {

void ScrollElement::OnNodeAdded(FiberElement* child) {
  // Scroll's child should not be layout only.
  child->MarkCanBeLayoutOnly(false);

  UpdateRenderRootElementIfNecessary(child);
}

void ScrollElement::SetAttributeInternal(const base::String& key,
                                         const lepus::Value& value) {
  FiberElement::SetAttributeInternal(key, value);

  const auto& value_str = value.StdString();
  if (key.IsEquals(kScrollX) && value_str == kTrue) {
    CacheStyleFromAttributes(
        kPropertyIDLinearOrientation,
        CSSValue::MakeEnum(
            static_cast<int>(starlight::LinearOrientationType::kHorizontal)));
    HandleLayoutNodeAttributeUpdate();
  } else if (key.IsEquals(kScrollY) && value_str == kTrue) {
    CacheStyleFromAttributes(kPropertyIDLinearOrientation,
                             CSSValue::MakeEnum(static_cast<int>(
                                 starlight::LinearOrientationType::kVertical)));
    HandleLayoutNodeAttributeUpdate();
  } else if (key.IsEquals(kScrollOrientation)) {
    if (value_str == kHorizontal) {
      CacheStyleFromAttributes(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(
              static_cast<int>(starlight::LinearOrientationType::kHorizontal)));
      HandleLayoutNodeAttributeUpdate();
    } else if (value_str == kVertical) {
      CacheStyleFromAttributes(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(
              static_cast<int>(starlight::LinearOrientationType::kVertical)));
      HandleLayoutNodeAttributeUpdate();
    }
    //(TODO)fangzhou.fz: If it becomes necessary in the future, extend the
    //'both' mode.
  } else if (key.IsEquals(kScrollXReverse) && value_str == kTrue) {
    CacheStyleFromAttributes(
        kPropertyIDLinearOrientation,
        CSSValue::MakeEnum(static_cast<int>(
            starlight::LinearOrientationType::kHorizontalReverse)));
    HandleLayoutNodeAttributeUpdate();
  } else if (key.IsEquals(kScrollYReverse) && value_str == kTrue) {
    CacheStyleFromAttributes(
        kPropertyIDLinearOrientation,
        CSSValue::MakeEnum(static_cast<int>(
            starlight::LinearOrientationType::kVerticalReverse)));
    HandleLayoutNodeAttributeUpdate();
  }
}

void ScrollElement::HandleLayoutNodeAttributeUpdate() {
  UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                            lepus::Value(true));
}

}  // namespace tasm
}  // namespace lynx
