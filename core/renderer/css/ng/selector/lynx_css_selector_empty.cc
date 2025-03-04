// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/renderer/css/ng/selector/lynx_css_selector.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"

namespace lynx {
namespace css {

void LynxCSSSelector::UpdatePseudoType(PseudoType pseudo_type) {}

unsigned LynxCSSSelector::CalcSpecificity() const { return 0; }

unsigned LynxCSSSelector::CalcSpecificityForSimple() const { return 0; }

}  // namespace css
}  // namespace lynx
