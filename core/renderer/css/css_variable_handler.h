// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_VARIABLE_HANDLER_H_
#define CORE_RENDERER_CSS_CSS_VARIABLE_HANDLER_H_

#include <string>

#include "base/include/base_export.h"
#include "base/include/closure.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/attribute_holder.h"

namespace lynx {
namespace tasm {

class CSSVariableHandler {
 public:
  void SetEnableFiberArch(bool fiberArch) { enable_fiber_arch_ = fiberArch; }

  bool HandleCSSVariables(StyleMap& map, AttributeHolder* holder,
                          const CSSParserConfigs& configs);

  // method to get variable value by DOM structure.
  // if value not found, return default_props.
  BASE_EXPORT_FOR_DEVTOOL base::String GetCSSVariableByRule(
      const std::string& format, AttributeHolder* holder,
      const base::String& default_props, const lepus::Value& default_value_map);

  bool HasCSSVariableInStyleMap(const StyleMap& map);

 private:
  static base::String GetCSSVariableByRule(
      const std::string& format,
      base::MoveOnlyClosure<base::String, const std::string&> rule_matcher);

  bool enable_fiber_arch_{false};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_VARIABLE_HANDLER_H_
