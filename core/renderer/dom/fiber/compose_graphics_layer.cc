// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_graphics_layer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

#include "base/include/log/logging.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/runtime/mts_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "gfx/geometry/matrix44.h"

namespace lynx {
namespace tasm {

// Fragment owns the hook and property invalidation. No layer state is retained
// here: each property update evaluates one closure and records its result.
CustomDrawFunctions GraphicsLayerDrawFunctions(runtime::MTSRuntime* runtime,
                                               lepus::Value callback) {
  CustomDrawFunctions functions;
  functions.draw_layer_properties =
      [runtime, callback](Fragment& fragment, DisplayListBuilder& builder) {
        auto* target = fragment.element();
        // Fragment retains its last binding until Flush installs a replacement.
        // A pending replacement or removal must not invoke this closure.
        if (!target->IsAttached() || !target->HasCustomDrawFunctions() ||
            (target->dirty() & Element::kDirtyCustomDraw)) {
          return;
        }
        const double pixels = target->element_manager()
                                  ->GetLynxEnvConfig()
                                  .PhysicalPixelsPerLayoutUnit();
        const lepus::Value width(target->width() * pixels);
        const lepus::Value height(target->height() * pixels);
        const lepus::Value element{fml::RefPtr<Element>(target)};
        const lepus::Value* args[] = {&element, &width, &height};
        // Already inside the rendering pipeline. Invoke the owning context
        // directly. Supply this layer's Element so the closure can observe
        // State and invalidate its target without rewriting the Modifier IR.
        const auto value = runtime->GetMTSContext()
                               ->CallClosureArgs(callback, args, 3)
                               .ToLepusValue();
        if (value.IsEmpty()) return;
        if (!value.IsObject()) {
          LOGE("graphicsLayer resolver returned invalid properties");
          return;
        }
        constexpr const char* keys[] = {"translationX", "translationY",
                                        "rotationZ", "scaleX", "scaleY"};
        float properties[5];
        for (size_t i = 0; i < 5; ++i) {
          const auto number = value.GetProperty(keys[i]).ToLepusValue();
          if (!number.IsNumber() || !std::isfinite(number.Number()) ||
              std::abs(number.Number()) > std::numeric_limits<float>::max()) {
            LOGE("graphicsLayer resolver returned invalid numeric properties");
            return;
          }
          properties[i] = static_cast<float>(number.Number());
        }
        // Alpha is optional so previously built transform-only templates keep
        // their ordinary opacity. Validate before emitting any custom output.
        static constexpr char kAlpha[] = "alpha";
        const auto alpha_value =
            value.GetProperty(BASE_STATIC_STRING(kAlpha)).ToLepusValue();
        float alpha = 1.f;
        if (!alpha_value.IsUndefined()) {
          if (!alpha_value.IsNumber() || !std::isfinite(alpha_value.Number())) {
            LOGE("graphicsLayer resolver returned invalid alpha");
            return;
          }
          alpha = static_cast<float>(
              std::max(0., std::min(1., alpha_value.Number())));
        }
        const float center_x = target->width() * 0.5f;
        const float center_y = target->height() * 0.5f;
        gfx::Matrix44 matrix;
        matrix.preTranslate(center_x + properties[0] / pixels,
                            center_y + properties[1] / pixels, 0);
        gfx::Matrix44 rotation;
        rotation.setRotateAboutZAxis(properties[2]);
        matrix.preConcat(rotation);
        matrix.preScale(properties[3], properties[4], 1);
        matrix.preTranslate(-center_x, -center_y, 0);
        for (size_t i = 0; i < 16; ++i) {
          if (!std::isfinite(matrix.Data()[i])) {
            LOGE("graphicsLayer transform matrix overflowed");
            return;
          }
        }
        builder.Transform(matrix);
        // The innermost layer can bind to the content Element itself. Preserve
        // opacity from the content (for example an image's own alpha) and apply
        // the layer's opacity to that output. Removal restores the ordinary
        // value.
        builder.Opacity(alpha * target->computed_css_style()->GetOpacity());
      };
  return functions;
}

}  // namespace tasm
}  // namespace lynx
