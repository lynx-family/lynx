// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_UTILS_H_
#define CORE_RENDERER_CSS_CSS_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

using RadialGradientSizeType = starlight::RadialGradientSizeType;
using RadialGradientShapeType = starlight::RadialGradientShapeType;
using DeclarationListConsumeFunction =
    base::MoveOnlyClosure<void, const char* /*key_start*/,
                          uint32_t /* key_length*/, const char* /*value_start*/,
                          uint32_t /*value_length*/>;

std::pair<float, float> GetRadialGradientRadius(
    RadialGradientShapeType shape, RadialGradientSizeType shape_size, float cx,
    float cy, float sx, float sy);

bool ParseStyleDeclarationList(const char* content, uint32_t content_length,
                               DeclarationListConsumeFunction consume_func);

ClassList SplitClasses(const char* content, size_t length);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_UTILS_H_
