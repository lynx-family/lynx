// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/pseudo_element.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

const std::unordered_map<CSSPropertyID, const char*>&
SelectionPseudoElementStyleNames() {
  static base::NoDestructor<std::unordered_map<CSSPropertyID, const char*>>
      kSelectionPseudoElementStyleName{{
          {kPropertyIDBackgroundColor, "selection-background-color"},
          {kPropertyIDXHandleColor, "selection-handle-color"},
          {kPropertyIDXHandleSize, "selection-handle-size"},
      }};

  return *kSelectionPseudoElementStyleName;
}

const std::unordered_map<CSSPropertyID, const char*>&
PlaceHolderPseudoElementStyleNames() {
  static base::NoDestructor<std::unordered_map<CSSPropertyID, const char*>>
      kPlaceHolderPseudoElementStyleName{{
          {kPropertyIDColor, "placeholder-color"},
          {kPropertyIDFontSize, "placeholder-font-size"},
          {kPropertyIDFontFamily, "placeholder-font-family"},
          {kPropertyIDFontWeight, "placeholder-font-weight"},
      }};

  return *kPlaceHolderPseudoElementStyleName;
}

PseudoElement::PseudoElement(PseudoState state, FiberElement* holder_element)
    : state_(state), holder_element_(holder_element) {
  platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
      *holder_element->computed_css_style());
}

void PseudoElement::UpdateStyleMap(StyleMap& new_style_map) {
  StyleMap update_map;
  for (const auto& [key, value] : new_style_map) {
    auto iter_old_map = style_map_.find(key);
    if (iter_old_map == style_map_.end() || value != iter_old_map->second) {
      update_map.insert_or_assign(key, value);
    }
    if (iter_old_map != style_map_.end()) {
      style_map_.erase(iter_old_map);
    }
  }

  // reset value
  for (const auto& [key, value] : style_map_) {
    platform_css_style_->ResetValue(key);
    SetHolderElementProperty(key);
  }

  // update value
  UpdatePropertyFromStyleMap(update_map);

  style_map_ = new_style_map;
}

void PseudoElement::UpdatePropertyFromStyleMap(StyleMap& style_map) {
  for (const auto& [key, value] : style_map) {
    if (key == kPropertyIDFontSize) {
      // Font size need to be resolved independently.
      auto font_size = starlight::CSSStyleUtils::ResolveFontSize(
          value, holder_element_->element_manager()->GetLynxEnvConfig(),
          holder_element_->element_manager()
              ->GetLynxEnvConfig()
              .ViewportWidth(),
          holder_element_->element_manager()
              ->GetLynxEnvConfig()
              .ViewportHeight(),
          platform_css_style_->GetMeasureContext().cur_node_font_size_,
          platform_css_style_->GetMeasureContext().root_node_font_size_,
          holder_element_->element_manager()->GetCSSParserConfigs());
      if (font_size.has_value()) {
        platform_css_style_->SetFontSize(
            *font_size,
            platform_css_style_->GetMeasureContext().root_node_font_size_);
      }
    }
    platform_css_style_->SetValue(key, value);

    SetHolderElementProperty(key);
  }
}

void PseudoElement::SetHolderElementProperty(CSSPropertyID id) {
  switch (state_) {
    case kPseudoStateSelection:
      SetPseudoStylesInternal(id, SelectionPseudoElementStyleNames());
      break;
    case kPseudoStatePlaceHolder:
      SetPseudoStylesInternal(id, PlaceHolderPseudoElementStyleNames());
      break;
    default:
      break;
  }
}

void PseudoElement::SetPseudoStylesInternal(
    CSSPropertyID id,
    const std::unordered_map<CSSPropertyID, const char*>& map) {
  auto style_name = map.find(id);
  if (style_name != map.end()) {
    holder_element_->UpdateAttrMap(style_name->second,
                                   platform_css_style_->GetValue(id));
    holder_element_->MarkAttrDirtyForPseudoElement();
  }
}

void PseudoElement::SetFontSize(double cur_node_font_size,
                                double root_node_font_size) {
  platform_css_style_->SetFontSize(cur_node_font_size, root_node_font_size);

  // update em rem unit
  UpdatePropertyFromStyleMap(style_map_);
}

}  // namespace tasm
}  // namespace lynx
