// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_TYPES_H_
#define CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_TYPES_H_

#include "base/include/float_comparison.h"
#include "base/include/vector.h"
#include "core/renderer/starlight/types/layout_attribute.h"
#include "core/renderer/starlight/types/layout_configs.h"
#include "core/renderer/starlight/types/layout_constraints.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/starlight/types/layout_unit.h"

namespace lynx {
namespace starlight {

class LayoutObject;
class LayoutComputedStyle;

// 0 to 6 children account for more than 99%.
constexpr const static unsigned kChildrenInlineVectorSize = 6;

using LayoutItems =
    base::InlineVector<LayoutObject*, kChildrenInlineVectorSize>;
using InlineFloatArray = base::InlineVector<float, kChildrenInlineVectorSize>;
using InlineBoolArray = base::InlineVector<bool, kChildrenInlineVectorSize>;

}  // namespace starlight
}  // namespace lynx
#endif  // CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_TYPES_H_
