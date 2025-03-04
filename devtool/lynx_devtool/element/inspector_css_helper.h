// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_ELEMENT_INSPECTOR_CSS_HELPER_H_
#define DEVTOOL_LYNX_DEVTOOL_ELEMENT_INSPECTOR_CSS_HELPER_H_

#include <string>

#include "core/renderer/css/css_property.h"

namespace lynx {
namespace devtool {

class InspectorCSSHelper {
 public:
  static bool IsColor(lynx::tasm::CSSPropertyID id);
  static bool IsDimension(lynx::tasm::CSSPropertyID id);
  static bool IsAutoDimension(lynx::tasm::CSSPropertyID id);
  static bool IsStringProp(lynx::tasm::CSSPropertyID id);
  static bool IsIntProp(lynx::tasm::CSSPropertyID id);
  static bool IsFloatProp(lynx::tasm::CSSPropertyID id);
  static bool IsBorderProp(lynx::tasm::CSSPropertyID id);
  static bool IsSupportedProp(lynx::tasm::CSSPropertyID id);

  static bool IsLegal(const std::string& name, const std::string& value);
  static bool IsAnimationLegal(const std::string& name,
                               const std::string& value);
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_ELEMENT_INSPECTOR_CSS_HELPER_H_
