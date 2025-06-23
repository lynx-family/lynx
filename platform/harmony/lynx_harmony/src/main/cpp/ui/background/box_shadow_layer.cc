// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/box_shadow_layer.h"

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_filter.h>
#include <native_drawing/drawing_mask_filter.h>
#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_path.h>

#include <algorithm>

#include "base/include/float_comparison.h"
#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {
static const float kBLUR_SIGMA_SCALE = 0.57735f;
void BoxShadowLayer::DrawInsetLayer(OH_Drawing_Canvas* canvas) {
  for (int i = shadow_list_.size() - 1; i >= 0; --i) {
    if (shadow_list_.at(i)->is_inset) {
      Draw(shadow_list_.at(i).get(), canvas);
    } else {
      continue;
    }
  }
}

static inline bool IsValidShadow(const std::unique_ptr<Shadow>& shadow) {
  if (base::FloatsEqual(shadow->offset_x, 0.f) &&
      base::FloatsEqual(shadow->offset_y, 0.f) &&
      base::FloatsLargerOrEqual(0.f, shadow->blur_radius) &&
      base::FloatsEqual(shadow->spread_radius, 0.f)) {
    return false;
  }
  if ((shadow->color >> 24) == 0) {
    return false;
  }
  return true;
}

void BoxShadowLayer::UpdateShadowData(const lepus::Value& data) {
  ClearShadowDrawStruct();
  if (!data.IsEmpty() && data.IsArray()) {
    auto items = data.Array();
    for (size_t i = 0; i < items->size(); ++i) {
      auto shadow = std::make_unique<Shadow>();
      auto shadow_data = items->get(i).Array();
      shadow->offset_x = shadow_data->get(0).Number();
      shadow->offset_y = shadow_data->get(1).Number();
      shadow->blur_radius = shadow_data->get(2).Number();
      shadow->spread_radius = shadow_data->get(3).Number();
      shadow->option =
          static_cast<starlight::ShadowOption>(shadow_data->get(4).Number());
      shadow->is_inset = shadow->option == starlight::ShadowOption::kInset;
      shadow->color = static_cast<uint32_t>(shadow_data->get(5).Number());
      if (IsValidShadow(shadow)) {
        shadow_list_.emplace_back(std::move(shadow));
      }
    }
  }
}

bool BoxShadowLayer::HasShadow() { return !shadow_list_.empty(); }

void BoxShadowLayer::UpdateShadowDrawer(float left, float top, float width,
                                        float height, BorderRadius* radius,
                                        float scale_density) {
  if (!filter_) {
    filter_ = OH_Drawing_FilterCreate();
  }
  if (!brush_) {
    brush_ = OH_Drawing_BrushCreate();
  }
  for (auto& shadow : shadow_list_) {
    if (!shadow->shadow_path) {
      shadow->shadow_path = OH_Drawing_PathCreate();
    }
    if (shadow->mask_filter) {
      OH_Drawing_MaskFilterDestroy(shadow->mask_filter);
      shadow->mask_filter = nullptr;
    }
    auto blur_radius =
        shadow->blur_radius > 0
            ? kBLUR_SIGMA_SCALE * shadow->blur_radius * scale_density + 0.5f
            : 0.0f;
    shadow->mask_filter =
        OH_Drawing_MaskFilterCreateBlur(NORMAL, blur_radius, true);
    float inset = shadow->spread_radius * scale_density;
    float inset_max = std::min(width, height) / 2;
    if (base::FloatsLarger(inset, inset_max)) {
      inset = inset_max;
    }
    inset = inset * (shadow->is_inset ? 1 : -1);

    auto path = shadow->shadow_path;
    OH_Drawing_PathReset(path);
    if (radius->IsZero()) {
      OH_Drawing_PathAddRect(path, 0, 0, width, height, PATH_DIRECTION_CW);
    } else {
      float rx1 = radius->ComputedValue()[0] * scale_density;
      float ry1 = radius->ComputedValue()[1] * scale_density;
      float rx2 = radius->ComputedValue()[2] * scale_density;
      float ry2 = radius->ComputedValue()[3] * scale_density;
      float rx3 = radius->ComputedValue()[4] * scale_density;
      float ry3 = radius->ComputedValue()[5] * scale_density;
      float rx4 = radius->ComputedValue()[6] * scale_density;
      float ry4 = radius->ComputedValue()[7] * scale_density;
      OH_Drawing_PathMoveTo(path, rx1, 0);
      OH_Drawing_PathLineTo(path, width - rx2, 0);
      OH_Drawing_PathArcTo(path, width - 2 * rx2, 0, width, 2 * ry2, -90, 90);
      OH_Drawing_PathLineTo(path, width, height - ry3);
      OH_Drawing_PathArcTo(path, width - 2 * rx3, height - 2 * ry3, width,
                           height, 0, 90);
      OH_Drawing_PathLineTo(path, rx4, height);
      OH_Drawing_PathArcTo(path, 0, height - 2 * ry4, 2 * rx4, height, 90, 90);
      OH_Drawing_PathLineTo(path, 0, ry1);
      OH_Drawing_PathArcTo(path, 0, 0, 2 * rx1, 2 * ry1, 180, 90);
      OH_Drawing_PathClose(path);
    }
    auto matrix = OH_Drawing_MatrixCreate();
    auto scale_x = (width - 2 * inset) / width;
    auto scale_y = (height - 2 * inset) / height;
    OH_Drawing_MatrixScale(matrix, scale_x, scale_y, width / 2, height / 2);
    // TODO(chengjunnan)
    //  To align with web implementation,
    //  maybe can use canvas.drawDRRect to draw a solid color background,
    //  then apply colorFilter and maskFilter on a new layer to achieve the blur
    //  effect, and use BlendMode.SrcIn for blending to achieve the desired
    //  result.
    if (shadow->is_inset) {
      auto odd_path = OH_Drawing_PathCreate();
      OH_Drawing_PathAddPath(odd_path, path, matrix);
      OH_Drawing_MatrixReset(matrix);
      OH_Drawing_MatrixTranslate(matrix, shadow->offset_x * scale_density,
                                 shadow->offset_y * scale_density);
      OH_Drawing_PathTransform(odd_path, matrix);
      OH_Drawing_PathOp(path, odd_path, PATH_OP_MODE_DIFFERENCE);
      OH_Drawing_PathDestroy(odd_path);
    } else {
      OH_Drawing_PathTransform(path, matrix);
      OH_Drawing_MatrixReset(matrix);
      OH_Drawing_MatrixTranslate(matrix, shadow->offset_x * scale_density,
                                 shadow->offset_y * scale_density);
      OH_Drawing_PathTransform(path, matrix);
    }
    OH_Drawing_MatrixReset(matrix);
    OH_Drawing_MatrixTranslate(matrix, left, top);
    OH_Drawing_PathTransform(path, matrix);
    OH_Drawing_MatrixDestroy(matrix);
  }
}

void BoxShadowLayer::Draw(Shadow* shadow, OH_Drawing_Canvas* canvas) {
  OH_Drawing_CanvasSave(canvas);
  OH_Drawing_BrushSetColor(brush_, shadow->color);
  OH_Drawing_BrushSetAntiAlias(brush_, true);
  OH_Drawing_FilterSetMaskFilter(filter_, shadow->mask_filter);
  OH_Drawing_BrushSetFilter(brush_, filter_);
  OH_Drawing_CanvasAttachBrush(canvas, brush_);
  OH_Drawing_CanvasDrawPath(canvas, shadow->shadow_path);
  OH_Drawing_CanvasDetachBrush(canvas);
  OH_Drawing_CanvasRestore(canvas);
}

void BoxShadowLayer::DrawOutSetLayer(OH_Drawing_Canvas* canvas) {
  for (int i = shadow_list_.size() - 1; i >= 0; --i) {
    if (!shadow_list_.at(i)->is_inset) {
      Draw(shadow_list_.at(i).get(), canvas);
    } else {
      continue;
    }
  }
}

BoxShadowLayer::~BoxShadowLayer() {
  if (brush_) {
    OH_Drawing_BrushDestroy(brush_);
  }
  if (filter_) {
    OH_Drawing_FilterDestroy(filter_);
  }
  ClearShadowDrawStruct();
}

void BoxShadowLayer::ClearShadowDrawStruct() {
  for (auto& shadow : shadow_list_) {
    if (shadow->shadow_path) {
      OH_Drawing_PathDestroy(shadow->shadow_path);
    }
    if (shadow->mask_filter) {
      OH_Drawing_MaskFilterDestroy(shadow->mask_filter);
    }
  }
  shadow_list_.clear();
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
