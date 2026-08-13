// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_TRANSFORMS_TRANSFORM_OPERATIONS_HELPER_H_
#define CORE_RENDERER_CSS_TRANSFORMS_TRANSFORM_OPERATIONS_HELPER_H_

#include <optional>

#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/renderer/css/css_value.h"
#include "core/style/transform_raw_data.h"
#include "gfx/geometry/transform_operations.h"

namespace lynx {
namespace tasm {
class CssMeasureContext;
struct CSSParserConfigs;
}  // namespace tasm

namespace transforms {

// CSS-facing conversions only. Transform storage and math live in gfx/geometry.
gfx::TransformOperations ConvertToGfxTransformOperations(
    const base::Vector<starlight::TransformRawData>& transform_raw_data,
    float reference_width, float reference_height);

std::optional<gfx::TransformOperations> ResolveTransformOperations(
    const tasm::CSSValue& raw_data,
    const tasm::CssMeasureContext& measure_context,
    const tasm::CSSParserConfigs& parser_configs, float reference_width,
    float reference_height);

tasm::CSSValue ConvertToCSSValue(const gfx::TransformOperations& operations,
                                 float layouts_unit_per_px);

base::String ConvertToCSSText(const gfx::TransformOperations& operations,
                              float reference_width, float reference_height,
                              float layouts_unit_per_px);

}  // namespace transforms
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_TRANSFORMS_TRANSFORM_OPERATIONS_HELPER_H_
