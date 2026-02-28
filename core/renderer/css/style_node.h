// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_STYLE_NODE_H_
#define CORE_RENDERER_CSS_STYLE_NODE_H_

#include <string>

#include "base/include/value/base_string.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {
class AttributeHolder;
}

namespace css {

using StyleNode = tasm::AttributeHolder;

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_STYLE_NODE_H_
