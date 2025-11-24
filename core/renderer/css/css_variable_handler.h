// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_VARIABLE_HANDLER_H_
#define CORE_RENDERER_CSS_CSS_VARIABLE_HANDLER_H_

#include <string>

#include "base/include/closure.h"
#include "core/base/lynx_export.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/attribute_holder.h"

namespace lynx {
namespace tasm {

class CSSVariableHandler {
 public:
  explicit CSSVariableHandler(bool enable_fiber_arch = false)
      : enable_fiber_arch_(enable_fiber_arch) {}

  // method to handle css variables in style map.
  // If no CSS variables are present in the style map, return false; otherwise,
  // return true.
  bool HandleCSSVariables(StyleMap& map, AttributeHolder* holder,
                          const CSSParserConfigs& configs);

  // method to get variable value by DOM structure.
  // if value not found, return default_props.
  LYNX_EXPORT_FOR_DEVTOOL base::String GetCSSVariableByRule(
      const std::string& format, AttributeHolder* holder,
      const base::String& default_props, const lepus::Value& default_value_map);

  bool HasCSSVariableInStyleMap(const StyleMap& map);

  bool HasCSSVariableInHolder(const AttributeHolder* holder);

 private:
  void ResolveCSSVariables(CSSPropertyID id, const CSSValue& value,
                           StyleMap& style_map, AttributeHolder* holder,
                           const CSSParserConfigs& configs);
  static base::String GetCSSVariableByRule(
      const std::string& format,
      base::MoveOnlyClosure<base::String, const std::string&> rule_matcher);

  bool enable_fiber_arch_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_VARIABLE_HANDLER_H_
