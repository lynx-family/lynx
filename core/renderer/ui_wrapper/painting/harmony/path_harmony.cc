// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/path_harmony.h"

#include <native_drawing/drawing_error_code.h>
#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_rect.h>

#include <cmath>
#include <memory>
#include <utility>

#include "core/renderer/ui_wrapper/painting/harmony/drawing_error_scope.h"
#include "core/renderer/ui_wrapper/painting/harmony/drawing_round_rect.h"
#include "gfx/public/path.h"

namespace lynx::tasm {
namespace {
template <typename T, void (*Destroy)(T*)>
using NativeOwner = std::unique_ptr<T, decltype(Destroy)>;

bool BuildGeometry(OH_Drawing_Path* path, const gfx::PathView& view,
                   float density) {
  const auto& h = view.header;
  if (h.kind != gfx::PathKind::kCommands) {
    const float left = h.x * density;
    const float top = h.y * density;
    const float right = (h.x + h.width) * density;
    const float bottom = (h.y + h.height) * density;
    if (!std::isfinite(left) || !std::isfinite(top) || !std::isfinite(right) ||
        !std::isfinite(bottom)) {
      return false;
    }
    if (h.width == 0 || h.height == 0) return true;
    if (h.kind == gfx::PathKind::kRect) {
      OH_Drawing_PathAddRect(path, left, top, right, bottom, PATH_DIRECTION_CW);
      return true;
    }
    NativeOwner<OH_Drawing_Rect, OH_Drawing_RectDestroy> rect(
        OH_Drawing_RectCreate(left, top, right, bottom),
        OH_Drawing_RectDestroy);
    auto round_rect = CreateDrawingRoundRect(rect.get(), h.radii, density);
    if (!round_rect) return false;
    OH_Drawing_PathAddRoundRect(path, round_rect.get(), PATH_DIRECTION_CW);
    return true;
  }
  for (uint32_t i = 0; i < h.point_count; ++i) {
    if (!std::isfinite(view.points[i].x * density) ||
        !std::isfinite(view.points[i].y * density))
      return false;
  }
  uint32_t point = 0;
  for (uint32_t i = 0; i < h.command_count; ++i) {
    const auto* p = view.points + point;
    switch (view.commands[i]) {
      case gfx::PathVerb::kBegin:
        OH_Drawing_PathMoveTo(path, p[0].x * density, p[0].y * density);
        ++point;
        break;
      case gfx::PathVerb::kLine:
        OH_Drawing_PathLineTo(path, p[0].x * density, p[0].y * density);
        ++point;
        break;
      case gfx::PathVerb::kQuadratic:
        OH_Drawing_PathQuadTo(path, p[0].x * density, p[0].y * density,
                              p[1].x * density, p[1].y * density);
        point += 2;
        break;
      case gfx::PathVerb::kCubic:
        OH_Drawing_PathCubicTo(path, p[0].x * density, p[0].y * density,
                               p[1].x * density, p[1].y * density,
                               p[2].x * density, p[2].y * density);
        point += 3;
        break;
      case gfx::PathVerb::kClose:
        OH_Drawing_PathClose(path);
        break;
      case gfx::PathVerb::kEnd:
        break;
    }
  }
  return true;
}
}  // namespace

PathHarmony::PathHarmony(gfx::PathSource source)
    : gfx::PathImpl(std::move(source)) {}
PathHarmony::~PathHarmony() = default;

void PathHarmony::PathDeleter::operator()(OH_Drawing_Path* path) const {
  OH_Drawing_PathDestroy(path);
}

DrawingPathResult PathHarmony::GetDrawingPath(const gfx::Path& path,
                                              gfx::PathFillRule rule,
                                              float density) {
  const auto& impl = path.GetImpl();
  if (impl.GetBackendKind() != BackendKind::kHarmony) {
    return {PathHarmonyStatus::kUnsupported, nullptr};
  }
  return static_cast<const PathHarmony&>(impl).GetDrawingPath(rule, density);
}

DrawingPathResult PathHarmony::GetDrawingPath(gfx::PathFillRule rule,
                                              float density) const {
  if (!std::isfinite(density) || density <= 0) {
    return {PathHarmonyStatus::kInvalid, nullptr};
  }
  if (rule != gfx::PathFillRule::kNonzero &&
      rule != gfx::PathFillRule::kEvenOdd) {
    return {PathHarmonyStatus::kUnsupported, nullptr};
  }
  for (const auto& variant : drawing_variants_) {
    if (variant.density == density && variant.rule == rule) {
      return {variant.status, variant.path.get()};
    }
  }
  drawing_variants_.push_back(BuildDrawingPath(rule, density));
  const auto& variant = drawing_variants_.back();
  return {variant.status, variant.path.get()};
}

NodePathResult PathHarmony::GetNodeClip(const gfx::Path& path,
                                        gfx::PathFillRule rule, float density) {
  const auto& impl = path.GetImpl();
  if (impl.GetBackendKind() != BackendKind::kHarmony) {
    return {HarmonyNodePathStatus::kUnsupported, nullptr};
  }
  return static_cast<const PathHarmony&>(impl).GetNodeClip(rule, density);
}

NodePathResult PathHarmony::GetNodeClip(gfx::PathFillRule rule,
                                        float density) const {
  if (!std::isfinite(density) || density <= 0) {
    return {HarmonyNodePathStatus::kInvalid, nullptr};
  }
  for (const auto& variant : node_variants_) {
    if (variant.density == density && variant.rule == rule) {
      return {variant.descriptor->status(), variant.descriptor};
    }
  }
  auto descriptor = std::make_shared<const HarmonyNodePathDescriptor>(
      HarmonyNodePathDescriptor::Build(GetSourceView(), rule, density));
  node_variants_.push_back({density, rule, descriptor});
  return {descriptor->status(), std::move(descriptor)};
}

PathHarmony::DrawingVariant PathHarmony::BuildDrawingPath(
    gfx::PathFillRule rule, float density) const {
  DrawingVariant result{density, rule};
  const HarmonyDrawingErrorScope error_scope;
  std::unique_ptr<OH_Drawing_Path, PathDeleter> path(OH_Drawing_PathCreate());
  if (!path) return result;
  const auto source = GetSourceView();
  if (const auto* svg = std::get_if<gfx::SvgPathView>(&source)) {
    const float scale = svg->source_to_layout_scale * density;
    if (!std::isfinite(scale) || scale <= 0 ||
        !OH_Drawing_PathBuildFromSvgString(path.get(), svg->data.c_str())) {
      return result;
    }
    NativeOwner<OH_Drawing_Matrix, OH_Drawing_MatrixDestroy> matrix(
        OH_Drawing_MatrixCreateScale(scale, scale, 0, 0),
        OH_Drawing_MatrixDestroy);
    if (!matrix) return result;
    OH_Drawing_PathTransform(path.get(), matrix.get());
  } else if (!BuildGeometry(path.get(), std::get<gfx::PathView>(source),
                            density)) {
    return result;
  }
  OH_Drawing_PathSetFillType(path.get(), rule == gfx::PathFillRule::kEvenOdd
                                             ? PATH_FILL_TYPE_EVEN_ODD
                                             : PATH_FILL_TYPE_WINDING);
  NativeOwner<OH_Drawing_Rect, OH_Drawing_RectDestroy> bounds(
      OH_Drawing_RectCreate(0, 0, 0, 0), OH_Drawing_RectDestroy);
  if (!bounds) return result;
  OH_Drawing_PathGetBounds(path.get(), bounds.get());
  const float left = OH_Drawing_RectGetLeft(bounds.get());
  const float top = OH_Drawing_RectGetTop(bounds.get());
  const float right = OH_Drawing_RectGetRight(bounds.get());
  const float bottom = OH_Drawing_RectGetBottom(bounds.get());
  if (!error_scope.Succeeded() || !std::isfinite(left) || !std::isfinite(top) ||
      !std::isfinite(right) || !std::isfinite(bottom))
    return result;
  const bool empty = right <= left || bottom <= top;
  result.status = empty ? PathHarmonyStatus::kEmpty : PathHarmonyStatus::kReady;
  if (!empty) result.path = std::move(path);
  return result;
}

std::unique_ptr<gfx::PathImpl> PathHarmonyFactory::Create(
    gfx::PathSource source) const {
  return std::make_unique<PathHarmony>(std::move(source));
}

}  // namespace lynx::tasm
