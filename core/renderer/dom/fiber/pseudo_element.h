// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_PSEUDO_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_PSEUDO_ELEMENT_H_

#include <memory>
#include <unordered_map>

#include "core/renderer/css/computed_css_style.h"

namespace lynx {
namespace tasm {
class FiberElement;

class PseudoElement {
 public:
  PseudoElement(PseudoState state, FiberElement* holder_element);

  void UpdateStyleMap(StyleMap& new_style_map);

  starlight::ComputedCSSStyle* ComputedCSSStyle() {
    return platform_css_style_.get();
  }
  void SetFontSize(double cur_node_font_size, double root_node_font_size);

 private:
  void SetHolderElementProperty(CSSPropertyID id);
  void SetPseudoStylesInternal(
      CSSPropertyID id,
      const std::unordered_map<CSSPropertyID, const char*>& map);
  void UpdatePropertyFromStyleMap(StyleMap& style_map);

  PseudoState state_;
  FiberElement* holder_element_;
  StyleMap style_map_;
  std::unique_ptr<starlight::ComputedCSSStyle> platform_css_style_;
};
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FIBER_PSEUDO_ELEMENT_H_
