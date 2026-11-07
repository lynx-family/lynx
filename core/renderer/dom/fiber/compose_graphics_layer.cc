// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_graphics_layer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

#include "base/include/log/logging.h"
#include "base/include/value/base_string.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/runtime/mts_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "gfx/geometry/matrix44.h"

namespace lynx {
namespace tasm {

namespace {

BASE_STATIC_STRING_DECL(kPivotFractionX, "pivotFractionX");
BASE_STATIC_STRING_DECL(kPivotFractionY, "pivotFractionY");
BASE_STATIC_STRING_DECL(kRotationX, "rotationX");
BASE_STATIC_STRING_DECL(kRotationY, "rotationY");
BASE_STATIC_STRING_DECL(kRotationZ, "rotationZ");
BASE_STATIC_STRING_DECL(kTranslationX, "translationX");
BASE_STATIC_STRING_DECL(kTranslationY, "translationY");
BASE_STATIC_STRING_DECL(kScaleX, "scaleX");
BASE_STATIC_STRING_DECL(kScaleY, "scaleY");
BASE_STATIC_STRING_DECL(kCameraDistance, "cameraDistance");

gfx::Matrix44 LayerTransform(const lepus::Value& properties, float width,
                             float height, double pixels) {
  const auto read = [&](const base::String& key, float fallback) -> float {
    const auto field = properties.GetProperty(key).ToLepusValue();
    if (!field.IsNumber()) return fallback;
    const double n = field.Number();
    return std::isfinite(n) && std::abs(n) <= std::numeric_limits<float>::max()
               ? static_cast<float>(n)
               : fallback;
  };
  const float pivot_x = width * read(kPivotFractionX, 0.5f);
  const float pivot_y = height * read(kPivotFractionY, 0.5f);
  const float rotation_x = read(kRotationX, 0.f);
  const float rotation_y = read(kRotationY, 0.f);
  gfx::Matrix44 matrix, rotation;
  matrix.preTranslate(pivot_x + read(kTranslationX, 0.f) / pixels,
                      pivot_y + read(kTranslationY, 0.f) / pixels, 0);

  // Match Compose's projected layer plane: scale, rotate Z/Y/X, project,
  // then move the pivot back. The camera convention is 72 physical pixels
  // per unit; the DisplayList is expressed in layout units.
  constexpr float kMinPerspectiveAngleDegrees = 0.001f;
  if (std::abs(rotation_x) > kMinPerspectiveAngleDegrees ||
      std::abs(rotation_y) > kMinPerspectiveAngleDegrees) {
    constexpr float kDefaultCameraDistance = 8.f;
    const float distance = read(kCameraDistance, kDefaultCameraDistance);
    const float camera = distance > 0.f ? distance : kDefaultCameraDistance;
    gfx::Matrix44 perspective;
    perspective.setRC(3, 2, -pixels / (72.0 * camera));
    matrix.preConcat(perspective);
  }
  if (rotation_x != 0.f) {
    rotation.setRotateAboutXAxis(rotation_x);
    matrix.preConcat(rotation);
  }
  if (rotation_y != 0.f) {
    rotation.setRotateAboutYAxis(rotation_y);
    matrix.preConcat(rotation);
  }
  rotation.setRotateAboutZAxis(read(kRotationZ, 0.f));
  matrix.preConcat(rotation);
  matrix.preScale(read(kScaleX, 1.f), read(kScaleY, 1.f), 1);
  matrix.preTranslate(-pivot_x, -pivot_y, 0);
  if (rotation_x != 0.f || rotation_y != 0.f) {
    // The output is a flat layer, not a shared 3D scene. Preserve its planar
    // homography (including w) without feeding depth into an ancestor layer.
    for (int axis : {0, 1, 3}) {
      matrix.setRC(axis, 2, 0.f);
      matrix.setRC(2, axis, 0.f);
    }
    matrix.setRC(2, 2, 1.f);
  }
  // Finite inputs can still overflow (e.g. scale times pivot). Reset only
  // the transform; the layer's opacity is independent.
  for (size_t i = 0; i < 16; ++i) {
    if (!std::isfinite(matrix.Data()[i])) return gfx::Matrix44();
  }
  return matrix;
}

}  // namespace

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
        // Alpha is optional so previously built transform-only templates keep
        // their ordinary opacity. An absent key reads like null, so test the
        // key itself. Validate before emitting any custom output.
        static constexpr char kAlpha[] = "alpha";
        const auto alpha_value =
            value.GetProperty(BASE_STATIC_STRING(kAlpha)).ToLepusValue();
        float alpha = 1.f;
        if (value.Contains(BASE_STATIC_STRING(kAlpha)) &&
            !alpha_value.IsUndefined()) {
          if (!alpha_value.IsNumber() || !std::isfinite(alpha_value.Number())) {
            LOGE("graphicsLayer resolver returned invalid alpha");
            return;
          }
          alpha = static_cast<float>(
              std::max(0., std::min(1., alpha_value.Number())));
        }
        const auto transform =
            LayerTransform(value, target->width(), target->height(), pixels);
        target->SetCustomLayerTransform(transform);
        builder.Transform(transform);
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
