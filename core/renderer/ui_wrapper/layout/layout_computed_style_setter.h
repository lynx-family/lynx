// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_COMPUTED_STYLE_SETTER_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_COMPUTED_STYLE_SETTER_H_

#include "core/renderer/css/css_property_id.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/css/parser/css_parser_configs.h"

namespace lynx {
namespace starlight {
class LayoutComputedStyle;
}
namespace tasm {
class LayoutNode;
struct LayoutComputedStyleSetter {
  LayoutComputedStyleSetter(const CssMeasureContext&, const CSSParserConfigs&,
                            bool css_align_with_legacy_w3c,
                            starlight::LayoutComputedStyle&);

  const CssMeasureContext& measure_context;
  const CSSParserConfigs& parser_configs;
  bool css_align_with_legacy_w3c{false};
  starlight::LayoutComputedStyle& computed_style;
  // style setter by CSSValue
#define LAYOUT_PROPERTY_SETTER(name, consumption_status) \
  bool Set##name(const CSSValue& value, const bool reset) const;
  FOREACH_LAYOUT_PROPERTY(LAYOUT_PROPERTY_SETTER)
#undef LAYOUT_PROPERTY_SETTER

  bool SetValue(CSSPropertyID id, const CSSValue& value, bool reset) const;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_COMPUTED_STYLE_SETTER_H_
