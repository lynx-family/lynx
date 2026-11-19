// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_graphics_layer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

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

BASE_STATIC_STRING_DECL(kAlpha, "alpha");
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

gfx::Matrix44 BuildLayerTransform(const lepus::Value& properties, float width,
                                  float height, double pixels) {
  const auto read = [&](const base::String& key, float fallback) -> float {
    const auto field = properties.GetProperty(key).ToLepusValue();
    if (!field.IsNumber()) return fallback;
    const double n = field.Number();
    return std::isfinite(n) && std::abs(n) <= std::numeric_limits<float>::max()
               ? static_cast<float>(n)
               : fallback;
  };
  const float tx = read(kTranslationX, 0.f);
  const float ty = read(kTranslationY, 0.f);
  const float rotation_x = read(kRotationX, 0.f);
  const float rotation_y = read(kRotationY, 0.f);
  const float rotation_z = read(kRotationZ, 0.f);
  const float scale_x = read(kScaleX, 1.f);
  const float scale_y = read(kScaleY, 1.f);

  // Fast path: identity or pure 2D translation avoids pivot lookups,
  // trigonometry, and 4x4 matrix multiplications.
  if (rotation_x == 0.f && rotation_y == 0.f && rotation_z == 0.f &&
      scale_x == 1.f && scale_y == 1.f) {
    gfx::Matrix44 matrix;
    if (tx != 0.f || ty != 0.f) {
      const double dx = tx / pixels;
      const double dy = ty / pixels;
      if (std::isfinite(dx) && std::isfinite(dy) &&
          std::abs(dx) <= std::numeric_limits<float>::max() &&
          std::abs(dy) <= std::numeric_limits<float>::max()) {
        matrix.preTranslate(static_cast<float>(dx), static_cast<float>(dy), 0);
      }
    }
    return matrix;
  }

  const float pivot_x = width * read(kPivotFractionX, 0.5f);
  const float pivot_y = height * read(kPivotFractionY, 0.5f);
  gfx::Matrix44 matrix;
  matrix.preTranslate(pivot_x + tx / pixels, pivot_y + ty / pixels, 0);

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
  gfx::Matrix44 rotation;
  if (rotation_x != 0.f) {
    rotation.setRotateAboutXAxis(rotation_x);
    matrix.preConcat(rotation);
  }
  if (rotation_y != 0.f) {
    rotation.setRotateAboutYAxis(rotation_y);
    matrix.preConcat(rotation);
  }
  if (rotation_z != 0.f) {
    rotation.setRotateAboutZAxis(rotation_z);
    matrix.preConcat(rotation);
  }
  if (scale_x != 1.f || scale_y != 1.f) {
    matrix.preScale(scale_x, scale_y, 1);
  }
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

class GraphicsLayerDrawHandler final : public CustomDrawHandler {
 public:
  GraphicsLayerDrawHandler(runtime::MTSRuntime* runtime, lepus::Value callback)
      : CustomDrawHandler(kLayerProperties | kRequiresLayer | kMayReenter),
        runtime_(runtime),
        callback_(std::move(callback)) {}

  const gfx::Matrix44* LayerTransform() const override {
    return has_layer_transform_ ? &layer_transform_ : nullptr;
  }

  void Draw(Fragment& fragment, DisplayListBuilder& builder,
            Phase /*phase*/) override {
    auto* target = fragment.element();
    // Fragment retains its last binding until Flush installs a replacement.
    // A pending replacement or removal must not invoke this closure.
    if (!target->IsAttached() || !target->HasCustomDrawHandler() ||
        (target->dirty() & Element::kDirtyCustomDraw)) {
      has_layer_transform_ = false;
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
    // Keep the previous layer_transform_ visible for reentrant geometry
    // queries while the closure runs.
    const auto value = runtime_->GetMTSContext()
                           ->CallClosureArgs(callback_, args, 3)
                           .ToLepusValue();
    if (value.IsEmpty()) {
      has_layer_transform_ = false;
      return;
    }
    if (!value.IsObject()) {
      has_layer_transform_ = false;
      LOGE("graphicsLayer resolver returned invalid properties");
      return;
    }
    // Alpha is optional so previously built transform-only templates keep
    // their ordinary opacity. An absent key reads like null, so test the
    // key itself. Validate before emitting any custom output.
    float alpha = 1.f;
    if (value.Contains(kAlpha)) {
      const auto alpha_value = value.GetProperty(kAlpha).ToLepusValue();
      if (!alpha_value.IsUndefined()) {
        if (!alpha_value.IsNumber() || !std::isfinite(alpha_value.Number())) {
          has_layer_transform_ = false;
          LOGE("graphicsLayer resolver returned invalid alpha");
          return;
        }
        alpha = static_cast<float>(
            std::max(0., std::min(1., alpha_value.Number())));
      }
    }
    layer_transform_ =
        BuildLayerTransform(value, target->width(), target->height(), pixels);
    has_layer_transform_ = true;
    builder.Transform(layer_transform_);
    // The innermost layer can bind to the content Element itself. Preserve
    // opacity from the content (for example an image's own alpha) and apply
    // the layer's opacity to that output. Removal restores the ordinary
    // value.
    builder.Opacity(alpha * target->computed_css_style()->GetOpacity());
  }

 private:
  runtime::MTSRuntime* runtime_;
  lepus::Value callback_;
  gfx::Matrix44 layer_transform_;
  bool has_layer_transform_{false};
};

}  // namespace

// Fragment owns the hook and property invalidation. No layer state is retained
// here: each property update evaluates one closure and records its result.
fml::RefPtr<CustomDrawHandler> GraphicsLayerDrawFunctions(
    runtime::MTSRuntime* runtime, lepus::Value callback) {
  return fml::MakeRefCounted<GraphicsLayerDrawHandler>(runtime,
                                                       std::move(callback));
}

}  // namespace tasm
}  // namespace lynx
