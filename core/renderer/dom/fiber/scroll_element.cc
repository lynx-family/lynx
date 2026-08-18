// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/scroll_element.h"

#include <memory>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/scroll_fragment_behavior.h"

namespace lynx {
namespace tasm {

void ScrollElement::OnNodeAdded(Element* child) {
  Element::OnNodeAdded(child);
  if (enable_platform_renderer_.value_or(false)) {
    child->MarkAsDirectChildOfCompatibleComponent(false);
  }
  // Scroll's child should not be layout only.
  child->MarkCanBeLayoutOnly(false);
}

void ScrollElement::SetupFragmentBehavior(Fragment* fragment) {
  if (enable_platform_renderer_.value_or(false)) {
    fragment->SetBehavior(std::make_unique<ScrollFragmentBehavior>(fragment));
  } else {
    Element::SetupFragmentBehavior(fragment);
  }
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

ParallelFlushReturn ScrollElement::PrepareForCreateOrUpdate() {
  if (!initial_resolved_) {
    ResolvePlatformTagName();
    ResolveEnablePlatformRenderer();
    initial_resolved_ = true;
  }
  return Element::PrepareForCreateOrUpdate();
}

void ScrollElement::SetAttributeInternal(const base::String& key,
                                         const lepus::Value& value) {
  Element::SetAttributeInternal(key, value);

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
  }
}

void ScrollElement::ResetAttribute(const base::String& key) {
  Element::ResetAttribute(key);
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

void ScrollElement::ResolvePlatformTagName() {
  const auto& attr_map = updated_attr_map();
  auto it = attr_map.find(BASE_STATIC_STRING(kScrollNewArch));
  if (it != attr_map.end() && it->second.StdString() == kTrue) {
    platform_node_tag_ = BASE_STATIC_STRING(kScrollNewArch);
  }
}

void ScrollElement::ResolveEnablePlatformRenderer() {
  if (enable_platform_renderer_.has_value()) {
    return;
  }
  enable_platform_renderer_ =
      GetPlatformNodeTag().IsEqual(kElementScrollViewTag) &&
      LynxEnv::GetInstance().EnablePlatformRendererScroll();
  if (*enable_platform_renderer_) {
    for (const auto& child : scoped_children_) {
      child->MarkAsDirectChildOfCompatibleComponent(false);
    }
  }
}

}  // namespace tasm
}  // namespace lynx
