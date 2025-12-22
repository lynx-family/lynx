// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_drawable.h"

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_path_effect.h>
#include <native_drawing/drawing_pen.h>
#include <native_drawing/drawing_rect.h>

#include <algorithm>
#include <cmath>
#include <iterator>

#include "base/include/float_comparison.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/renderer/utils/value_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/svg_path_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {

static constexpr const float kCenterMul = 0.5f;
static constexpr const float kInner2Mul = 0.75f;
static constexpr const float kOuter2Mul = 0.25f;
static constexpr const float kInner3Mul = 5.0f / 6.0f;
static constexpr const float kOuter3Mul = 1.0f / 6.0f;
static constexpr const float kOuterMul = 0.0f;
static constexpr const float kInnerMul = 1.0f;
static constexpr const float kAntiAliasClipLength = 2.0f;

BackgroundDrawable::BackgroundDrawable(const std::weak_ptr<UIBase>& ui_base,
                                       bool is_mask)
    : ui_base_(ui_base),
      is_mask_(is_mask),
      border_info_(std::make_unique<BorderInfo>()),
      border_radius_(std::make_unique<BorderRadius>()) {}

float BackgroundDrawable::GetLength(float length, int32_t unit,
                                    float reference) {
  if (unit == 0) {
    return scale_density_ * length;
  }
  return length * reference;
}

void BackgroundDrawable::SetBorderWidth(const std::array<float, 4>& value) {
  border_info_->border_left_origin_width = value[0];
  border_info_->border_right_origin_width = value[1];
  border_info_->border_top_origin_width = value[2];
  border_info_->border_bottom_origin_width = value[3];
}

void BackgroundDrawable::Render(OH_Drawing_Canvas* canvas) {
  if (base::FloatsLarger(view_width_, 0) &&
      base::FloatsLarger(view_height_, 0)) {
    if (blend_brush_) {  // draw mask
      OH_Drawing_CanvasSaveLayer(canvas, border_box_draw_rect_, blend_brush_);
      DrawBackground(canvas);
      OH_Drawing_CanvasRestore(canvas);
      return;
    }
    DrawBackground(canvas);
    DrawShadow(canvas);
    DrawBorder(canvas);
    if (!UIBase::CanDrawBehind()) {
      return;
    }
    // When DrawBehind is available, nodes without content can be clipped using
    // the clip-path, corresponding to the UIBase::ApplyOverflowClip
    auto ui = ui_base_.lock();
    if (ui && ui->NeedClip() && !ui->HasContent() && UseClipPath() &&
        inner_clip_path_) {
      OH_Drawing_CanvasClipPath(canvas, inner_clip_path_->path, INTERSECT,
                                true);
    }
  }
}

void BackgroundDrawable::DrawBorder(OH_Drawing_Canvas* canvas) {
  if (!border_info_) {
    return;
  }
  if (base::FloatsEqual(view_height_, 0) || base::FloatsEqual(view_width_, 0)) {
    return;
  }
  if (base::FloatsLarger(border_info_->border_top_width, 0) ||
      base::FloatsLarger(border_info_->border_right_width, 0) ||
      base::FloatsLarger(border_info_->border_left_width, 0) ||
      base::FloatsLarger(border_info_->border_bottom_width, 0)) {
    if (!pen_) {
      pen_ = OH_Drawing_PenCreate();
      OH_Drawing_PenSetAntiAlias(pen_, true);
    }
    if (!brush_) {
      brush_ = OH_Drawing_BrushCreate();
      OH_Drawing_BrushSetAntiAlias(brush_, true);
    }
    if (border_radius_->IsZero()) {
      DrawRectBorders(canvas, pen_);
    } else {
      DrawRoundedBorders(canvas, pen_);
    }
  }
}

void BackgroundDrawable::UpdateContentBox() {
  if (padding_box_draw_rect_) {
    OH_Drawing_RectDestroy(padding_box_draw_rect_);
    padding_box_draw_rect_ = nullptr;
  }
  if (border_box_draw_rect_) {
    OH_Drawing_RectDestroy(border_box_draw_rect_);
    border_box_draw_rect_ = nullptr;
  }
  if (content_box_draw_rect_) {
    OH_Drawing_RectDestroy(content_box_draw_rect_);
    content_box_draw_rect_ = nullptr;
  }
  if (!border_box_rect_) {
    border_box_rect_ = std::make_unique<LayerManager::Rect>();
  }
  border_box_rect_->left = left_;
  border_box_rect_->top = top_;
  border_box_rect_->right = view_width_ + left_;
  border_box_rect_->bottom = view_height_ + top_;
  if (!border_info_) {
    return;
  }
  if (!padding_box_rect_) {
    padding_box_rect_ = std::make_unique<LayerManager::Rect>();
  }
  padding_box_rect_->left = border_info_->border_left_width + left_;
  padding_box_rect_->top = border_info_->border_top_width + top_;
  padding_box_rect_->right =
      view_width_ - border_info_->border_right_width + left_;
  padding_box_rect_->bottom =
      view_height_ - border_info_->border_bottom_width + top_;
  if (!content_box_rect_) {
    content_box_rect_ = std::make_unique<LayerManager::Rect>();
  }
  content_box_rect_->left = padding_left_ + padding_box_rect_->left;
  content_box_rect_->top = padding_top_ + padding_box_rect_->top;
  content_box_rect_->right = padding_box_rect_->right - padding_right_;
  content_box_rect_->bottom = padding_box_rect_->bottom - padding_bottom_;
  border_box_draw_rect_ =
      OH_Drawing_RectCreate(border_box_rect_->left, border_box_rect_->top,
                            border_box_rect_->right, border_box_rect_->bottom);
  padding_box_draw_rect_ = OH_Drawing_RectCreate(
      padding_box_rect_->left, padding_box_rect_->top, padding_box_rect_->right,
      padding_box_rect_->bottom);
  content_box_draw_rect_ = OH_Drawing_RectCreate(
      content_box_rect_->left, content_box_rect_->top, content_box_rect_->right,
      content_box_rect_->bottom);
}

static inline uint32_t Alpha(uint32_t color) { return color >> 24; }

void BackgroundDrawable::DrawBackground(OH_Drawing_Canvas* canvas) {
  if (Alpha(background_color_) != 0) {
    if (!brush_) {
      brush_ = OH_Drawing_BrushCreate();
      OH_Drawing_BrushSetAntiAlias(brush_, true);
    }
    OH_Drawing_BrushSetColor(brush_, background_color_);
    OH_Drawing_CanvasAttachBrush(canvas, brush_);
    auto clip = starlight::BackgroundClipType::kBorderBox;
    if (layer_manager_) {
      clip = layer_manager_->GetLayerClip();
    }

    // Draw background-color respecting background-clip, including border-area.
    if (!border_radius_->IsZero()) {
      if (clip == starlight::BackgroundClipType::kBorderBox) {
        if (outer_clip_path_) {
          OH_Drawing_CanvasDrawPath(canvas, outer_clip_path_->path);
        } else if (inner_clip_path_) {
          OH_Drawing_CanvasDrawPath(canvas, inner_clip_path_->path);
        }
      } else if (clip == starlight::BackgroundClipType::kPaddingBox &&
                 inner_clip_path_) {
        OH_Drawing_CanvasDrawPath(canvas, inner_clip_path_->path);
      } else if (clip == starlight::BackgroundClipType::kBorderArea) {
        // Fill only the border ring area: outer minus inner.
        if (outer_clip_path_ && outer_clip_path_->path) {
          OH_Drawing_CanvasSave(canvas);
          OH_Drawing_CanvasClipPath(canvas, outer_clip_path_->path, INTERSECT,
                                    true);
          if (inner_clip_path_ && inner_clip_path_->path) {
            OH_Drawing_CanvasClipPath(canvas, inner_clip_path_->path,
                                      DIFFERENCE, true);
          }
          OH_Drawing_CanvasDrawRect(canvas, border_box_draw_rect_);
          OH_Drawing_CanvasRestore(canvas);
        }
      } else {
        OH_Drawing_CanvasDrawRect(canvas, content_box_draw_rect_);
      }
    } else {
      if (clip == starlight::BackgroundClipType::kBorderBox) {
        OH_Drawing_CanvasDrawRect(canvas, border_box_draw_rect_);
      } else if (clip == starlight::BackgroundClipType::kPaddingBox) {
        OH_Drawing_CanvasDrawRect(canvas, padding_box_draw_rect_);
      } else if (clip == starlight::BackgroundClipType::kBorderArea) {
        // Rectangular border-area: clip to outer rect then subtract padding
        // box.
        if (border_box_draw_rect_ && padding_box_draw_rect_) {
          OH_Drawing_CanvasSave(canvas);
          if (outer_clip_path_) {
            // Use the precomputed outer path if available.
            OH_Drawing_CanvasClipPath(canvas, outer_clip_path_->path, INTERSECT,
                                      true);
          } else {
            OH_Drawing_CanvasClipRect(canvas, border_box_draw_rect_, INTERSECT,
                                      true);
          }
          if (inner_clip_path_) {
            OH_Drawing_CanvasClipPath(canvas, inner_clip_path_->path,
                                      DIFFERENCE, true);
          } else {
            OH_Drawing_CanvasClipRect(canvas, padding_box_draw_rect_,
                                      DIFFERENCE, true);
          }
          OH_Drawing_CanvasDrawRect(canvas, border_box_draw_rect_);
          OH_Drawing_CanvasRestore(canvas);
        }
      } else {
        OH_Drawing_CanvasDrawRect(canvas, content_box_draw_rect_);
      }
    }
    OH_Drawing_CanvasDetachBrush(canvas);
  }

  if (layer_manager_ && layer_manager_->HasImageLayers()) {
    OH_Drawing_CanvasSave(canvas);
    OH_Drawing_Path* outer_path = nullptr;
    OH_Drawing_Path* inner_path = nullptr;
    if (!border_radius_->IsZero()) {
      if (outer_clip_path_) {
        outer_path = outer_clip_path_->path;
      }
      if (inner_clip_path_) {
        inner_path = inner_clip_path_->path;
      }
    }
    layer_manager_->Draw(canvas, *border_box_rect_.get(),
                         *padding_box_rect_.get(), *content_box_rect_.get(),
                         outer_path, inner_path,
                         has_border_ || !border_radius_->IsZero());
    OH_Drawing_CanvasRestore(canvas);
  }
}

void BackgroundDrawable::DrawRectBorders(OH_Drawing_Canvas* canvas,
                                         OH_Drawing_Pen* pen) {
  OH_Drawing_CanvasSave(canvas);
  auto width = view_width_;
  auto height = view_height_;
  float left = left_;
  float top = top_;
  if (base::FloatsLarger(border_info_->border_top_width, 0)) {
    float x1 = left;
    float y1 = top;
    float x2 = left + border_info_->border_left_width;
    float y2 = top + border_info_->border_top_width;
    float x3 = left + width - border_info_->border_right_width;
    float y3 = top + border_info_->border_top_width;
    float x4 = left + width;
    float y4 = top;
    float y5 = top + border_info_->border_top_width * 0.5f;
    OH_Drawing_CanvasSave(canvas);
    UpdateBorderClipPath(x1, y1, x2, y2, x3, y3, x4, y4, false);
    DrawRectBorderLines(canvas, pen, BorderPosition::kTop,
                        border_info_->border_top_width,
                        border_info_->border_top_color, x1, y5, x4, y5, width,
                        border_info_->border_top_style);
    OH_Drawing_CanvasRestore(canvas);
  }
  if (base::FloatsLarger(border_info_->border_right_width, 0)) {
    float x1 = left + width;
    float y1 = top;
    float x2 = left + width;
    float y2 = top + height;
    float x3 = left + width - border_info_->border_right_width;
    float y3 = top + height - border_info_->border_bottom_width;
    float x4 = left + width - border_info_->border_right_width;
    float y4 = top + border_info_->border_top_width;
    float x5 = left + width - border_info_->border_right_width * 0.5f;
    OH_Drawing_CanvasSave(canvas);
    UpdateBorderClipPath(x1, y1, x2, y2, x3, y3, x4, y4, false);
    DrawRectBorderLines(canvas, pen, BorderPosition::kRight,
                        border_info_->border_right_width,
                        border_info_->border_right_color, x5, y1, x5, y2,
                        height, border_info_->border_right_style);
    OH_Drawing_CanvasRestore(canvas);
  }

  if (base::FloatsLarger(border_info_->border_bottom_width, 0)) {
    float x1 = left;
    float y1 = top + height;
    float x2 = left + width;
    float y2 = top + height;
    float x3 = left + width - border_info_->border_right_width;
    float y3 = top + height - border_info_->border_bottom_width;
    float x4 = left + border_info_->border_left_width;
    float y4 = top + height - border_info_->border_bottom_width;
    float y5 = top + height - border_info_->border_bottom_width * 0.5f;
    OH_Drawing_CanvasSave(canvas);
    UpdateBorderClipPath(x1, y1, x2, y2, x3, y3, x4, y4, false);
    DrawRectBorderLines(canvas, pen, BorderPosition::kBottom,
                        border_info_->border_bottom_width,
                        border_info_->border_bottom_color, x2, y5, x1, y5,
                        width, border_info_->border_bottom_style);
    OH_Drawing_CanvasRestore(canvas);
  }
  if (base::FloatsLarger(border_info_->border_left_width, 0)) {
    float x1 = left;
    float y1 = top;
    float x2 = left + border_info_->border_left_width;
    float y2 = top + border_info_->border_top_width;
    float x3 = left + border_info_->border_left_width;
    float y3 = top + height - border_info_->border_bottom_width;
    float x4 = left;
    float y4 = top + height;
    float x5 = left + border_info_->border_left_width * 0.5f;
    OH_Drawing_CanvasSave(canvas);
    UpdateBorderClipPath(x1, y1, x2, y2, x3, y3, x4, y4, false);
    DrawRectBorderLines(canvas, pen, BorderPosition::kLeft,
                        border_info_->border_left_width,
                        border_info_->border_left_color, x5, y4, x5, y1, height,
                        border_info_->border_left_style);
    OH_Drawing_CanvasRestore(canvas);
  }
  OH_Drawing_CanvasRestore(canvas);
}

void BackgroundDrawable::UpdateBorderClipPath(float x1, float y1, float x2,
                                              float y2, float x3, float y3,
                                              float x4, float y4,
                                              bool clip_area) {
  if (!border_path_) {
    border_path_ = OH_Drawing_PathCreate();
  }
  OH_Drawing_PathReset(border_path_);
  OH_Drawing_PathMoveTo(border_path_, x1, y1);
  OH_Drawing_PathLineTo(border_path_, x2, y2);
  OH_Drawing_PathLineTo(border_path_, x3, y3);
  OH_Drawing_PathLineTo(border_path_, x4, y4);
  OH_Drawing_PathLineTo(border_path_, x1, y1);
  OH_Drawing_PathClose(border_path_);
  if (clip_area) {
    if (outer_clip_path_) {
      OH_Drawing_PathOp(border_path_, outer_clip_path_->path,
                        PATH_OP_MODE_INTERSECT);
    }
    if (inner_clip_path_) {
      OH_Drawing_PathOp(border_path_, inner_clip_path_->path,
                        PATH_OP_MODE_DIFFERENCE);
    }
  }
}

OH_Drawing_PathEffect* BackgroundDrawable::GetPathEffectAutoAdjust(
    float border_width, float border_length,
    const starlight::BorderStyleType& style) {
  if (style != starlight::BorderStyleType::kDashed &&
      style != starlight::BorderStyleType::kDotted) {
    return nullptr;
  }

  float section_len = (border_width >= 1 ? border_width : 1) *
                      (style == starlight::BorderStyleType::kDotted ? 2 : 6) *
                      0.5f;
  int new_section_count =
      (static_cast<int>((border_length / section_len - 0.5f) * 0.5f)) * 2 + 1;
  if (new_section_count <= 1) {
    return nullptr;
  }

  float intervals[2] = {border_length / new_section_count,
                        border_length / new_section_count};
  return OH_Drawing_CreateDashPathEffect(intervals, 2, 0);
}

static inline bool IsDarkColor(uint32_t color) {
  return (color & 0x00F0F0F0) == 0;
}

static inline uint32_t DarkenColor(uint32_t color) {
  return ((color & 0x00FEFEFE) >> 1) | (color & 0xFF000000);
}

static inline uint32_t BrightColor(uint32_t color) {
  return (color | 0x00808080);
}

static inline bool IsSimpleBorderStyle(
    starlight::BorderStyleType border_style) {
  return border_style == starlight::BorderStyleType::kDouble ||
         border_style == starlight::BorderStyleType::kDotted ||
         border_style == starlight::BorderStyleType::kDashed ||
         border_style == starlight::BorderStyleType::kSolid;
}

void BackgroundDrawable::DrawRectBorderLines(
    OH_Drawing_Canvas* canvas, OH_Drawing_Pen* pen,
    const BorderPosition& border_position, float border_width, uint32_t color,
    float start_x, float start_y, float stop_x, float stop_y,
    float border_length, const starlight::BorderStyleType& style) {
  if (path_effect_) {
    OH_Drawing_PathEffectDestroy(path_effect_);
    path_effect_ = nullptr;
  }
  switch (style) {
    case starlight::BorderStyleType::kUndefined:
    case starlight::BorderStyleType::kNone:
    case starlight::BorderStyleType::kHide:
      return;

    case starlight::BorderStyleType::kDashed:
    case starlight::BorderStyleType::kDotted:
      path_effect_ = BackgroundDrawable::GetPathEffectAutoAdjust(
          border_width, border_length, style);
      break;

    case starlight::BorderStyleType::kSolid:
      break;
    case starlight::BorderStyleType::kInset: {
      if (IsDarkColor(color)) {
        if (border_position == BorderPosition::kBottom ||
            border_position == BorderPosition::kRight) {
          color = BrightColor(color);
        }
      } else {
        if (border_position == BorderPosition::kTop ||
            border_position == BorderPosition::kLeft) {
          color = DarkenColor(color);
        }
      }
    } break;
    case starlight::BorderStyleType::kOutset:
      if (IsDarkColor(color)) {
        if (border_position == BorderPosition::kTop ||
            border_position == BorderPosition::kLeft) {
          color = BrightColor(color);
        }
      } else {
        if (border_position == BorderPosition::kBottom ||
            border_position == BorderPosition::kRight) {
          color = DarkenColor(color);
        }
      }
      break;
    case starlight::BorderStyleType::kDouble:
      BackgroundDrawable::DrawMultiColorBorderLine(
          canvas, pen, border_position, border_width / 3, border_width / 3,
          color, color, start_x, start_y, stop_x, stop_y);
      return;
    case starlight::BorderStyleType::kGroove:
      if (IsDarkColor(color)) {
        DrawMultiColorBorderLine(canvas, pen, border_position, border_width / 2,
                                 border_width / 4, BrightColor(color), color,
                                 start_x, start_y, stop_x, stop_y);
      } else {
        DrawMultiColorBorderLine(canvas, pen, border_position, border_width / 2,
                                 border_width / 4, color, DarkenColor(color),
                                 start_x, start_y, stop_x, stop_y);
      }
      return;
    case starlight::BorderStyleType::kRidge:
      if (IsDarkColor(color)) {
        DrawMultiColorBorderLine(canvas, pen, border_position, border_width / 2,
                                 border_width / 4, color, BrightColor(color),
                                 start_x, start_y, stop_x, stop_y);
      } else {
        DrawMultiColorBorderLine(canvas, pen, border_position, border_width / 2,
                                 border_width / 4, DarkenColor(color), color,
                                 start_x, start_y, stop_x, stop_y);
      }
      return;
  }
  OH_Drawing_PenSetPathEffect(pen, path_effect_);
  OH_Drawing_PenSetWidth(pen, border_width);
  DrawClipBorderLine(start_x, start_y, stop_x, stop_y, color, pen, canvas);
  OH_Drawing_PenSetPathEffect(pen, nullptr);
}

void BackgroundDrawable::DrawRoundedBorders(OH_Drawing_Canvas* canvas,
                                            OH_Drawing_Pen* pen) {
  OH_Drawing_CanvasSave(canvas);
  if (is_same_color_ && is_same_style_ && is_same_width_ && is_simple_style_) {
    BackgroundDrawable::DrawRoundBorderPath(
        canvas, pen, BorderPosition::kLeft, border_info_->border_left_style,
        border_info_->border_left_color, border_info_->border_left_width,
        border_info_->border_left_width, false);
  } else {
    float left = outer_clip_path_->left;
    float right = outer_clip_path_->right;
    float top = outer_clip_path_->top;
    float bottom = outer_clip_path_->bottom;
    float border_width = 0;
    bool need_clip = false;
    if (base::FloatsLarger(border_info_->border_top_width, 0)) {
      float x1 = left;
      float y1 = top;
      float x2 = inner_top_left_point_->x;
      float y2 = inner_top_left_point_->y;
      float x3 = inner_top_right_point_->x;
      float y3 = inner_top_right_point_->y;
      float x4 = right;
      float y4 = top;
      need_clip = false;
      border_width = border_info_->border_top_width;
      if (!is_same_width_) {
        border_width =
            std::max(border_width, std::max(border_info_->border_left_width,
                                            border_info_->border_right_width));
        need_clip = border_width - std::min(border_info_->border_left_width,
                                            border_info_->border_right_width) >=
                    kAntiAliasClipLength;
      }
      OH_Drawing_CanvasSave(canvas);
      UpdateBorderClipPath(x1, y1, x2, y2, x3, y3, x4, y4, need_clip);
      DrawRoundBorderPath(canvas, pen, BorderPosition::kTop,
                          border_info_->border_top_style,
                          border_info_->border_top_color,
                          border_info_->border_top_width, border_width, true);
      OH_Drawing_CanvasRestore(canvas);
    }
    if (base::FloatsLarger(border_info_->border_right_width, 0)) {
      float x1 = right;
      float y1 = top;
      float x2 = inner_top_right_point_->x;
      float y2 = inner_top_right_point_->y;
      float x3 = inner_bottom_right_point_->x;
      float y3 = inner_bottom_right_point_->y;
      float x4 = right;
      float y4 = bottom;
      need_clip = false;
      border_width = border_info_->border_right_width;
      if (!is_same_width_) {
        border_width =
            std::max(border_width, std::max(border_info_->border_top_width,
                                            border_info_->border_bottom_width));
        need_clip =
            border_width - std::min(border_info_->border_top_width,
                                    border_info_->border_bottom_width) >=
            kAntiAliasClipLength;
      }
      OH_Drawing_CanvasSave(canvas);
      UpdateBorderClipPath(x1, y1, x2, y2, x3, y3, x4, y4, need_clip);
      DrawRoundBorderPath(canvas, pen, BorderPosition::kRight,
                          border_info_->border_right_style,
                          border_info_->border_right_color,
                          border_info_->border_right_width, border_width, true);
      OH_Drawing_CanvasRestore(canvas);
    }
    if (base::FloatsLarger(border_info_->border_bottom_width, 0)) {
      float x1 = left;
      float y1 = bottom;
      float x2 = inner_bottom_left_point_->x;
      float y2 = inner_bottom_left_point_->y;
      float x3 = inner_bottom_right_point_->x;
      float y3 = inner_bottom_right_point_->y;
      float x4 = right;
      float y4 = bottom;
      need_clip = false;
      border_width = border_info_->border_bottom_width;
      if (!is_same_width_) {
        border_width =
            std::max(border_width, std::max(border_info_->border_left_width,
                                            border_info_->border_right_width));
        need_clip = border_width - std::min(border_info_->border_left_width,
                                            border_info_->border_right_width) >=
                    kAntiAliasClipLength;
      }
      OH_Drawing_CanvasSave(canvas);
      UpdateBorderClipPath(x1, y1, x2, y2, x3, y3, x4, y4, need_clip);
      DrawRoundBorderPath(
          canvas, pen, BorderPosition::kBottom,
          border_info_->border_bottom_style, border_info_->border_bottom_color,
          border_info_->border_bottom_width, border_width, true);
      OH_Drawing_CanvasRestore(canvas);
    }
    if (base::FloatsLarger(border_info_->border_left_width, 0)) {
      float x1 = left;
      float y1 = top;
      float x2 = inner_top_left_point_->x;
      float y2 = inner_top_left_point_->y;
      float x3 = inner_bottom_left_point_->x;
      float y3 = inner_bottom_left_point_->y;
      float x4 = left;
      float y4 = bottom;
      need_clip = false;
      border_width = border_info_->border_left_width;
      if (!is_same_width_) {
        border_width =
            std::max(border_width, std::max(border_info_->border_top_width,
                                            border_info_->border_bottom_width));
        need_clip =
            border_width - std::min(border_info_->border_top_width,
                                    border_info_->border_bottom_width) >=
            kAntiAliasClipLength;
      }
      OH_Drawing_CanvasSave(canvas);
      UpdateBorderClipPath(x1, y1, x2, y2, x3, y3, x4, y4, need_clip);
      DrawRoundBorderPath(canvas, pen, BorderPosition::kLeft,
                          border_info_->border_left_style,
                          border_info_->border_left_color,
                          border_info_->border_left_width, border_width, true);
      OH_Drawing_CanvasRestore(canvas);
    }
  }
  OH_Drawing_CanvasRestore(canvas);
}

static inline void GetEllipseIntersectionWithLine(
    float ellipse_bounds_left, float ellipse_bounds_top,
    float ellipse_bounds_right, float ellipse_bounds_bottom, float line_start_x,
    float line_start_y, float line_end_x, float line_end_y,
    const std::unique_ptr<BackgroundDrawable::Point>& result) {
  float ellipse_center_x = (ellipse_bounds_left + ellipse_bounds_right) / 2;
  float ellipse_center_y = (ellipse_bounds_top + ellipse_bounds_bottom) / 2;

  line_start_x -= ellipse_center_x;
  line_start_y -= ellipse_center_y;
  line_end_x -= ellipse_center_x;
  line_end_y -= ellipse_center_y;

  float semi_major_axis =
      std::abs(ellipse_bounds_right - ellipse_bounds_left) / 2;
  float semi_minor_axis =
      std::abs(ellipse_bounds_bottom - ellipse_bounds_top) / 2;
  float delta_x = line_end_x - line_start_x;
  float delta_y = line_end_y - line_start_y;
  if (base::FloatsEqual(delta_x, 0)) {
    delta_x = base::EPSILON;
  }
  float slope = delta_y / delta_x;
  float intercept = line_start_y - slope * line_start_x;
  float quadratic_a = semi_minor_axis * semi_minor_axis +
                      semi_major_axis * semi_major_axis * slope * slope;
  float quadratic_b = 2 * semi_major_axis * semi_major_axis * intercept * slope;
  float quadratic_c =
      semi_major_axis * semi_major_axis *
      (intercept * intercept - semi_minor_axis * semi_minor_axis);
  float discriminant =
      -quadratic_c / quadratic_a + std::pow(quadratic_b / (2 * quadratic_a), 2);
  if (base::FloatsEqual(0.0f, discriminant)) {
    discriminant = 0.0f;
  }
  float discriminant_sqrt = std::sqrt(discriminant);
  float x_intersection = -quadratic_b / (2 * quadratic_a) - discriminant_sqrt;
  float y_intersection = slope * x_intersection + intercept;
  if (!std::isnan(x_intersection) && !std::isnan(y_intersection)) {
    result->x = x_intersection + ellipse_center_x;
    result->y = y_intersection + ellipse_center_y;
  }
}

void BackgroundDrawable::DrawRoundBorderPath(
    OH_Drawing_Canvas* canvas, OH_Drawing_Pen* pen,
    const BorderPosition& border_position,
    const starlight::BorderStyleType& border_style, uint32_t color,
    float border_effect_width, float border_stoke_width, bool need_clip) {
  if (path_effect_) {
    OH_Drawing_PathEffectDestroy(path_effect_);
    path_effect_ = nullptr;
  }
  OH_Drawing_PenSetAntiAlias(pen, true);
  switch (border_style) {
    case starlight::BorderStyleType::kUndefined:
    case starlight::BorderStyleType::kNone:
    case starlight::BorderStyleType::kHide:
      return;

    case starlight::BorderStyleType::kDashed:
    case starlight::BorderStyleType::kDotted:
      path_effect_ = GetPathEffect(border_style, border_effect_width);
      break;

    case starlight::BorderStyleType::kSolid:
      break;
    case starlight::BorderStyleType::kInset:
      if (border_position == BorderPosition::kTop ||
          border_position == BorderPosition::kLeft) {
        color = DarkenColor(color);
      }
      break;
    case starlight::BorderStyleType::kOutset:
      if (border_position == BorderPosition::kBottom ||
          border_position == BorderPosition::kRight) {
        color = DarkenColor(color);
      }
      break;

    case starlight::BorderStyleType::kDouble:
      BackgroundDrawable::DrawMultiColorBorderPath(canvas, pen, border_position,
                                                   border_effect_width / 3.0f,
                                                   color, color, true);
      return;
    case starlight::BorderStyleType::kGroove:
      BackgroundDrawable::DrawMultiColorBorderPath(
          canvas, pen, border_position, border_effect_width / 2.0f, color,
          DarkenColor(color), false);
      return;
    case starlight::BorderStyleType::kRidge:
      BackgroundDrawable::DrawMultiColorBorderPath(
          canvas, pen, border_position, border_effect_width / 2.0f,
          DarkenColor(color), color, false);
      return;
  }
  UpdateRoundBorderPath(kCenterMul, center_draw_path_);
  OH_Drawing_PenSetWidth(pen, border_stoke_width);
  OH_Drawing_PenSetPathEffect(pen, path_effect_);
  if (!need_clip) {
    OH_Drawing_PenSetColor(pen, color);
    OH_Drawing_CanvasAttachPen(canvas, pen);
    OH_Drawing_CanvasDrawPath(canvas, center_draw_path_->path);
    OH_Drawing_CanvasDetachPen(canvas);
  } else {
    DrawClipBorderPath(center_draw_path_->path, color, pen, canvas);
  }
  OH_Drawing_PenSetPathEffect(pen, nullptr);
}

void BackgroundDrawable::DrawMultiColorBorderLine(
    OH_Drawing_Canvas* canvas, OH_Drawing_Pen* paint,
    const BorderPosition& border_position, float border_width,
    float border_offset, uint32_t color0, uint32_t color1, float start_x,
    float start_y, float stop_x, float stop_y) {
  OH_Drawing_PenSetPathEffect(paint, nullptr);
  OH_Drawing_PenSetWidth(paint, border_width);

  for (int i = -1; i <= 1; i += 2) {
    float x_offset = 0;
    float y_offset = 0;
    uint32_t color = color0;
    switch (border_position) {
      case BorderPosition::kTop:
        y_offset = border_offset * i;
        color = (i == 1 ? color0 : color1);
        break;
      case BorderPosition::kRight:
        x_offset = -border_offset * i;
        color = (i == 1 ? color1 : color0);
        break;
      case BorderPosition::kBottom:
        y_offset = -border_offset * i;
        color = (i == 1 ? color1 : color0);
        break;
      case BorderPosition::kLeft:
        x_offset = border_offset * i;
        color = (i == 1 ? color0 : color1);
        break;
    }
    DrawClipBorderLine(start_x + x_offset, start_y + y_offset,
                       stop_x + x_offset, stop_y + y_offset, color, paint,
                       canvas);
  }
}

void BackgroundDrawable::DrawClipBorderLine(float start_x, float start_y,
                                            float stop_x, float stop_y,
                                            uint32_t color, OH_Drawing_Pen* pen,
                                            OH_Drawing_Canvas* canvas) {
  OH_Drawing_PathReset(border_line_path_);
  OH_Drawing_PathMoveTo(border_line_path_, start_x, start_y);
  OH_Drawing_PathLineTo(border_line_path_, stop_x, stop_y);
  if (is_same_color_ && is_simple_style_ && is_same_style_ &&
      border_info_->border_left_style != starlight::BorderStyleType::kDouble) {
    OH_Drawing_PenSetColor(pen, color);
    OH_Drawing_CanvasAttachPen(canvas, pen);
    OH_Drawing_CanvasDrawPath(canvas, border_line_path_);
    OH_Drawing_CanvasDetachPen(canvas);
  } else {
    OH_Drawing_PathReset(border_fill_path_);
    OH_Drawing_BrushSetColor(brush_, color);
    OH_Drawing_CanvasAttachBrush(canvas, brush_);
    OH_Drawing_PenGetFillPath(pen, border_line_path_, border_fill_path_,
                              nullptr, nullptr);
    OH_Drawing_PathOp(border_fill_path_, border_path_, PATH_OP_MODE_INTERSECT);
    OH_Drawing_CanvasDrawPath(canvas, border_fill_path_);
    OH_Drawing_CanvasDetachBrush(canvas);
  }
}

void BackgroundDrawable::DrawClipBorderPath(OH_Drawing_Path* border_path,
                                            uint32_t color, OH_Drawing_Pen* pen,
                                            OH_Drawing_Canvas* canvas) {
  OH_Drawing_BrushSetColor(brush_, color);
  OH_Drawing_CanvasAttachBrush(canvas, brush_);
  OH_Drawing_PathReset(border_fill_path_);
  OH_Drawing_PenGetFillPath(pen, border_path, border_fill_path_, nullptr,
                            nullptr);
  OH_Drawing_PathOp(border_fill_path_, border_path_, PATH_OP_MODE_INTERSECT);
  OH_Drawing_CanvasDrawPath(canvas, border_fill_path_);
  OH_Drawing_CanvasDetachBrush(canvas);
}

BackgroundDrawable::~BackgroundDrawable() {
  if (border_path_) {
    OH_Drawing_PathDestroy(border_path_);
  }
  if (pen_) {
    OH_Drawing_PenDestroy(pen_);
  }
  if (brush_) {
    OH_Drawing_BrushDestroy(brush_);
  }
  if (path_effect_) {
    OH_Drawing_PathEffectDestroy(path_effect_);
  }
  if (border_line_path_) {
    OH_Drawing_PathDestroy(border_line_path_);
  }
  if (border_fill_path_) {
    OH_Drawing_PathDestroy(border_fill_path_);
  }
  if (blend_brush_) {
    OH_Drawing_BrushDestroy(blend_brush_);
  }
}

void BackgroundDrawable::DrawMultiColorBorderPath(
    OH_Drawing_Canvas* canvas, OH_Drawing_Pen* pen,
    const BackgroundDrawable::BorderPosition& border_position,
    float border_width, uint32_t color0, uint32_t color1,
    bool is_double_style) {
  OH_Drawing_PenSetPathEffect(pen, nullptr);
  OH_Drawing_PenSetWidth(pen, border_width);
  bool is_top_left = (border_position == BorderPosition::kLeft ||
                      border_position == BorderPosition::kTop);
  UpdateRoundBorderPath(is_double_style ? kOuter3Mul : kOuter2Mul,
                        outer_draw_path_);
  DrawClipBorderPath(outer_draw_path_->path, is_top_left ? color1 : color0, pen,
                     canvas);
  UpdateRoundBorderPath(is_double_style ? kInner3Mul : kInner2Mul,
                        inner_draw_path_);
  DrawClipBorderPath(inner_draw_path_->path, is_top_left ? color0 : color1, pen,
                     canvas);
}

static inline float GenCenterBorderRadius(float radius, float border_width,
                                          float mul) {
  return std::max(
      radius - border_width * mul,
      base::FloatsLarger(border_width, 0.0f)
          ? (radius / border_width)
          : 0.0f);  // Radius of the first ellipse in the X direction
}

void BackgroundDrawable::UpdateRoundBorderPath(
    float mul, std::unique_ptr<RoundRectPath>& round_path) {
  if (!round_path) {
    round_path = std::make_unique<RoundRectPath>();
  }
  float left =
      border_info_->left + border_info_->border_left_width * mul + left_;
  float top = border_info_->top + border_info_->border_top_width * mul + top_;
  float right = (border_info_->left + view_width_) -
                border_info_->border_right_width * mul + left_;
  float bottom = (border_info_->top + view_height_) -
                 border_info_->border_bottom_width * mul + top_;
  round_path->left = left;
  round_path->top = top;
  round_path->right = right;
  round_path->bottom = bottom;
  if (border_radius_->IsZero()) {
    OH_Drawing_PathAddRect(round_path->path, left, top, right, bottom,
                           PATH_DIRECTION_CW);
    return;
  }
  float rx1 =
      GenCenterBorderRadius(border_radius_->ComputedValue()[0] * scale_density_,
                            border_info_->border_left_width, mul);
  float ry1 =
      GenCenterBorderRadius(border_radius_->ComputedValue()[1] * scale_density_,
                            border_info_->border_top_width, mul);
  float rx2 =
      GenCenterBorderRadius(border_radius_->ComputedValue()[2] * scale_density_,
                            border_info_->border_right_width, mul);
  float ry2 =
      GenCenterBorderRadius(border_radius_->ComputedValue()[3] * scale_density_,
                            border_info_->border_top_width, mul);
  float rx3 =
      GenCenterBorderRadius(border_radius_->ComputedValue()[4] * scale_density_,
                            border_info_->border_right_width, mul);
  float ry3 =
      GenCenterBorderRadius(border_radius_->ComputedValue()[5] * scale_density_,
                            border_info_->border_bottom_width, mul);
  float rx4 =
      GenCenterBorderRadius(border_radius_->ComputedValue()[6] * scale_density_,
                            border_info_->border_left_width, mul);
  float ry4 =
      GenCenterBorderRadius(border_radius_->ComputedValue()[7] * scale_density_,
                            border_info_->border_bottom_width, mul);
  auto path = round_path->path;
  std::ostringstream oss;

  OH_Drawing_PathReset(path);
  OH_Drawing_PathMoveTo(path, left + rx1, top);
  SvgPathUtils::MoveTo(oss, left + rx1, top);
  OH_Drawing_PathLineTo(path, right - rx2, top);
  SvgPathUtils::LineTo(oss, right - rx2, top);
  OH_Drawing_PathArcTo(path, right - 2 * rx2, top, right, top + 2 * ry2, -90,
                       90);
  SvgPathUtils::ArcTo(oss, rx2, ry2, right - 2 * rx2, top, right, top + 2 * ry2,
                      -90, 90);
  OH_Drawing_PathLineTo(path, right, bottom - ry3);
  SvgPathUtils::LineTo(oss, right, bottom - ry3);
  OH_Drawing_PathArcTo(path, right - 2 * rx3, bottom - 2 * ry3, right, bottom,
                       0, 90);
  SvgPathUtils::ArcTo(oss, rx3, ry3, right - 2 * rx3, bottom - 2 * ry3, right,
                      bottom, 0, 90);
  OH_Drawing_PathLineTo(path, left + rx4, bottom);
  SvgPathUtils::LineTo(oss, left + rx4, bottom);
  OH_Drawing_PathArcTo(path, left, bottom - 2 * ry4, left + 2 * rx4, bottom, 90,
                       90);
  SvgPathUtils::ArcTo(oss, rx4, ry4, left, bottom - 2 * ry4, left + 2 * rx4,
                      bottom, 90, 90);
  OH_Drawing_PathLineTo(path, left, top + ry1);
  SvgPathUtils::LineTo(oss, left, top + ry1);
  OH_Drawing_PathArcTo(path, left, top, left + 2 * rx1, top + 2 * ry1, 180, 90);
  SvgPathUtils::ArcTo(oss, rx1, ry1, left + rx1, top);
  OH_Drawing_PathClose(path);
  SvgPathUtils::Close(oss);
  round_path->radius = {rx1, ry1, rx2, ry2, rx3, ry3, rx4, ry4};
  round_path->path_string = oss.str();
}

void BackgroundDrawable::UpdateRoundMultiColoredBorderPoint() {
  GetEllipseIntersectionWithLine(
      inner_clip_path_->left, inner_clip_path_->top,
      inner_clip_path_->left + 2 * inner_clip_path_->radius[0],
      inner_clip_path_->top + 2 * inner_clip_path_->radius[1],
      outer_clip_path_->left, outer_clip_path_->top, inner_clip_path_->left,
      inner_clip_path_->top, inner_top_left_point_);

  GetEllipseIntersectionWithLine(
      inner_clip_path_->left,
      inner_clip_path_->bottom - 2 * inner_clip_path_->radius[6],
      inner_clip_path_->left + 2 * inner_clip_path_->radius[7],
      inner_clip_path_->bottom, outer_clip_path_->left,
      outer_clip_path_->bottom, inner_clip_path_->left,
      inner_clip_path_->bottom, inner_bottom_left_point_);
  GetEllipseIntersectionWithLine(
      inner_clip_path_->right - 2 * inner_clip_path_->radius[2],
      inner_clip_path_->top, inner_clip_path_->right,
      inner_clip_path_->top + 2 * inner_clip_path_->radius[3],
      outer_clip_path_->right, outer_clip_path_->top, inner_clip_path_->right,
      inner_clip_path_->top, inner_top_right_point_);

  GetEllipseIntersectionWithLine(
      inner_clip_path_->right - 2 * inner_clip_path_->radius[4],
      inner_clip_path_->bottom - 2 * inner_clip_path_->radius[5],
      inner_clip_path_->right, inner_clip_path_->bottom,
      outer_clip_path_->right, outer_clip_path_->bottom,
      inner_clip_path_->right, inner_clip_path_->bottom,
      inner_bottom_right_point_);
}

void BackgroundDrawable::UpdateBorderPoint(
    std::unique_ptr<BackgroundDrawable::Point>& point, float x, float y) {
  if (!point) {
    point = std::make_unique<Point>();
  }
  point->x = x;
  point->y = y;
}

void BackgroundDrawable::UpdateBounds(float left, float top, float width,
                                      float height, float padding_left,
                                      float padding_top, float padding_right,
                                      float padding_bottom,
                                      float scale_density) {
  scale_density_ = scale_density;
  left_ = left * scale_density_;
  top_ = top * scale_density_;
  view_width_ = width * scale_density_;
  view_height_ = height * scale_density_;
  padding_left_ = padding_left * scale_density_;
  padding_top_ = padding_top * scale_density_;
  padding_right_ = padding_right * scale_density_;
  padding_bottom_ = padding_bottom * scale_density_;
}

void BackgroundDrawable::AdjustBorder() {
  if (base::FloatsEqual(view_width_, 0) || base::FloatsEqual(view_height_, 0)) {
    return;
  }
  has_border_ =
      base::FloatsLarger(border_info_->border_left_origin_width, 0) ||
      base::FloatsLarger(border_info_->border_top_origin_width, 0) ||
      base::FloatsLarger(border_info_->border_right_origin_width, 0) ||
      base::FloatsLarger(border_info_->border_bottom_origin_width, 0);
  if (has_border_) {
    if (!border_line_path_) {
      border_line_path_ = OH_Drawing_PathCreate();
    }
    if (!border_fill_path_) {
      border_fill_path_ = OH_Drawing_PathCreate();
    }
  }
  border_info_->border_left_width =
      border_info_->border_left_origin_width * scale_density_;
  border_info_->border_right_width =
      border_info_->border_right_origin_width * scale_density_;
  border_info_->border_bottom_width =
      border_info_->border_bottom_origin_width * scale_density_;
  border_info_->border_top_width =
      border_info_->border_top_origin_width * scale_density_;
  if (has_border_) {
    if (base::FloatsLarger(
            border_info_->border_left_width + border_info_->border_right_width,
            view_width_) &&
        base::FloatsLarger(view_width_, 1)) {
      auto tmp = view_width_ / (border_info_->border_left_width +
                                border_info_->border_right_width);
      border_info_->border_left_width *= tmp;
      border_info_->border_right_width *= tmp;
    }
    if (base::FloatsLarger(
            border_info_->border_top_width + border_info_->border_bottom_width,
            view_height_) &&
        base::FloatsLarger(view_height_, 1)) {
      auto tmp = view_height_ / (border_info_->border_top_width +
                                 border_info_->border_bottom_width);
      border_info_->border_top_width *= tmp;
      border_info_->border_bottom_width *= tmp;
    }
    is_same_color_ =
        border_info_->border_left_color == border_info_->border_right_color &&
        border_info_->border_left_color == border_info_->border_top_color &&
        border_info_->border_left_color == border_info_->border_bottom_color;
    is_same_width_ = base::FloatsEqual(border_info_->border_left_width,
                                       border_info_->border_right_width) &&
                     base::FloatsEqual(border_info_->border_left_width,
                                       border_info_->border_top_width) &&
                     base::FloatsEqual(border_info_->border_left_width,
                                       border_info_->border_bottom_width);
    is_same_style_ =
        border_info_->border_left_style == border_info_->border_right_style &&
        border_info_->border_left_style == border_info_->border_top_style &&
        border_info_->border_left_style == border_info_->border_bottom_style;
    is_simple_style_ = IsSimpleBorderStyle(border_info_->border_left_style);
  }
  UpdateContentBox();
  if (layer_manager_) {
    has_image_ = layer_manager_->HasImageLayers();
  }

  if (border_radius_) {
    border_radius_->Compute(view_width_ / scale_density_,
                            view_height_ / scale_density_);
    UpdatePath();
  }
  if (box_shadow_layer_) {
    box_shadow_layer_->UpdateShadowDrawer(left_, top_, view_width_,
                                          view_height_, border_radius_.get(),
                                          scale_density_);
  }
}

OH_Drawing_PathEffect* BackgroundDrawable::BackgroundDrawable::GetPathEffect(
    const starlight::BorderStyleType& style, float border_width) {
  if (style == starlight::BorderStyleType::kDashed) {
    float intervals[4] = {border_width * 3, border_width * 3, border_width * 3,
                          border_width * 3};
    return OH_Drawing_CreateDashPathEffect(intervals, 4, 0);
  } else if (style == starlight::BorderStyleType::kDotted) {
    float intervals[4] = {border_width, border_width, border_width,
                          border_width};
    return OH_Drawing_CreateDashPathEffect(intervals, 4, 0);
  }
  return nullptr;
}

void BackgroundDrawable::UpdatePath() {
  UpdateRoundBorderPath(kOuterMul, outer_clip_path_);
  UpdateRoundBorderPath(kInnerMul, inner_clip_path_);
  if (!border_radius_->IsZero()) {
    UpdateBorderPoint(inner_top_left_point_, inner_clip_path_->left,
                      inner_clip_path_->top);
    UpdateBorderPoint(inner_bottom_left_point_, inner_clip_path_->left,
                      inner_clip_path_->bottom);
    UpdateBorderPoint(inner_top_right_point_, inner_clip_path_->right,
                      inner_clip_path_->top);
    UpdateBorderPoint(inner_bottom_right_point_, inner_clip_path_->right,
                      inner_clip_path_->bottom);
    UpdateRoundMultiColoredBorderPoint();
  }
}

const std::string& BackgroundDrawable::GetClipPath() {
  return inner_clip_path_->path_string;
}

BorderRadius* BackgroundDrawable::GetBorderRadius() {
  return border_radius_.get();
}

float BackgroundDrawable::GetBorderLeftWidth() {
  if (border_info_) {
    return border_info_->border_left_origin_width;
  }
  return 0;
}

float BackgroundDrawable::GetBorderRightWidth() {
  if (border_info_) {
    return border_info_->border_right_origin_width;
  }
  return 0;
}

float BackgroundDrawable::GetBorderTopWidth() {
  if (border_info_) {
    return border_info_->border_top_origin_width;
  }
  return 0;
}

float BackgroundDrawable::GetBorderBottomWidth() {
  if (border_info_) {
    return border_info_->border_bottom_origin_width;
  }
  return 0;
}

bool BackgroundDrawable::HasImage() {
  return has_image_ || Alpha(background_color_) != 0;
}

bool BackgroundDrawable::HasShadow() {
  return box_shadow_layer_ && box_shadow_layer_->HasShadow();
}

void BackgroundDrawable::DrawShadow(OH_Drawing_Canvas* canvas) {
  if (HasShadow()) {
    OH_Drawing_CanvasSave(canvas);
    if (inner_clip_path_->path) {
      OH_Drawing_CanvasClipPath(canvas, inner_clip_path_->path, INTERSECT,
                                true);
    }
    box_shadow_layer_->DrawInsetLayer(canvas);
    OH_Drawing_CanvasRestore(canvas);

    OH_Drawing_CanvasSave(canvas);
    if (outer_clip_path_->path) {
      OH_Drawing_CanvasClipPath(canvas, outer_clip_path_->path, DIFFERENCE,
                                true);
    }
    box_shadow_layer_->DrawOutSetLayer(canvas);
    OH_Drawing_CanvasRestore(canvas);
  }
}

void BackgroundDrawable::SetBorderRadius(const lepus::Value& value) {
  border_radius_->SetRadius(BorderRadius::CornerPosition::kTopLeft, value, 0);
  border_radius_->SetRadius(BorderRadius::CornerPosition::kTopRight, value, 4);
  border_radius_->SetRadius(BorderRadius::CornerPosition::kBottomRight, value,
                            8);
  border_radius_->SetRadius(BorderRadius::CornerPosition::kBottomLeft, value,
                            12);
}

void BackgroundDrawable::SetBorderTopLeftRadius(const lepus::Value& value) {
  border_radius_->SetRadius(BorderRadius::CornerPosition::kTopLeft, value);
}

void BackgroundDrawable::SetBorderTopRightRadius(const lepus::Value& value) {
  border_radius_->SetRadius(BorderRadius::CornerPosition::kTopRight, value);
}

void BackgroundDrawable::SetBorderBottomRightRadius(const lepus::Value& value) {
  border_radius_->SetRadius(BorderRadius::CornerPosition::kBottomRight, value);
}

void BackgroundDrawable::SetBorderBottomLeftRadius(const lepus::Value& value) {
  border_radius_->SetRadius(BorderRadius::CornerPosition::kBottomLeft, value);
}

void BackgroundDrawable::SetBorderStyle(const lepus::Value& value) {
  border_info_->border_left_style =
      static_cast<starlight::BorderStyleType>(value.Number());
  border_info_->border_right_style =
      static_cast<starlight::BorderStyleType>(value.Number());
  border_info_->border_top_style =
      static_cast<starlight::BorderStyleType>(value.Number());
  border_info_->border_bottom_style =
      static_cast<starlight::BorderStyleType>(value.Number());
}

void BackgroundDrawable::SetBorderLeftStyle(const lepus::Value& value) {
  border_info_->border_left_style =
      static_cast<starlight::BorderStyleType>(value.Number());
}

void BackgroundDrawable::SetBorderRightStyle(const lepus::Value& value) {
  border_info_->border_right_style =
      static_cast<starlight::BorderStyleType>(value.Number());
}

void BackgroundDrawable::SetBorderTopStyle(const lepus::Value& value) {
  border_info_->border_top_style =
      static_cast<starlight::BorderStyleType>(value.Number());
}

void BackgroundDrawable::SetBorderBottomStyle(const lepus::Value& value) {
  border_info_->border_bottom_style =
      static_cast<starlight::BorderStyleType>(value.Number());
}

void BackgroundDrawable::SetBorderColor(const lepus::Value& value) {
  border_info_->border_left_color = static_cast<uint32_t>(value.Number());
  border_info_->border_right_color = static_cast<uint32_t>(value.Number());
  border_info_->border_top_color = static_cast<uint32_t>(value.Number());
  border_info_->border_bottom_color = static_cast<uint32_t>(value.Number());
}

void BackgroundDrawable::SetBorderLeftColor(const lepus::Value& value) {
  border_info_->border_left_color = static_cast<uint32_t>(value.Number());
}

void BackgroundDrawable::SetBorderRightColor(const lepus::Value& value) {
  border_info_->border_right_color = static_cast<uint32_t>(value.Number());
}

void BackgroundDrawable::SetBorderTopColor(const lepus::Value& value) {
  border_info_->border_top_color = static_cast<uint32_t>(value.Number());
}

void BackgroundDrawable::SetBorderBottomColor(const lepus::Value& value) {
  border_info_->border_bottom_color = static_cast<uint32_t>(value.Number());
}

void BackgroundDrawable::SetBorderWidth(const lepus::Value& value) {
  border_info_->border_left_origin_width = static_cast<float>(value.Number());
  border_info_->border_right_origin_width = static_cast<float>(value.Number());
  border_info_->border_top_origin_width = static_cast<float>(value.Number());
  border_info_->border_bottom_origin_width = static_cast<float>(value.Number());
}

void BackgroundDrawable::SetBorderLeftWidth(const lepus::Value& value) {
  border_info_->border_left_origin_width = static_cast<float>(value.Number());
}

void BackgroundDrawable::SetBorderRightWidth(const lepus::Value& value) {
  border_info_->border_right_origin_width = static_cast<float>(value.Number());
}

void BackgroundDrawable::SetBorderTopWidth(const lepus::Value& value) {
  border_info_->border_top_origin_width = static_cast<float>(value.Number());
}

void BackgroundDrawable::SetBorderBottomWidth(const lepus::Value& value) {
  border_info_->border_bottom_origin_width = static_cast<float>(value.Number());
}

void BackgroundDrawable::SetBackgroundClip(const lepus::Value& value) {
  InitLayerManager();
  layer_manager_->SetLayerClip(value);
}

void BackgroundDrawable::SetBackgroundOrigin(const lepus::Value& value) {
  InitLayerManager();
  layer_manager_->SetLayerOrigin(value);
}

void BackgroundDrawable::SetBackgroundImage(const lepus::Value& value) {
  InitLayerManager();
  layer_manager_->SetLayerImage(value);
}

void BackgroundDrawable::SetBackgroundColor(uint32_t background_color) {
  background_color_ = background_color;
}

void BackgroundDrawable::SetBackgroundSize(const lepus::Value& value) {
  InitLayerManager();
  layer_manager_->SetLayerSize(value);
}

void BackgroundDrawable::SetBackgroundPosition(const lepus::Value& value) {
  InitLayerManager();
  layer_manager_->SetLayerPosition(value);
}

void BackgroundDrawable::SetBackgroundRepeat(const lepus::Value& value) {
  InitLayerManager();
  layer_manager_->SetLayerRepeat(value);
}

void BackgroundDrawable::SetBoxShadow(const lepus::Value& value) {
  if (!box_shadow_layer_) {
    box_shadow_layer_ = std::make_unique<BoxShadowLayer>();
  }
  box_shadow_layer_->UpdateShadowData(value);
}

void BackgroundDrawable::InitLayerManager() {
  if (!layer_manager_) {
    layer_manager_ = std::make_unique<LayerManager>(ui_base_, is_mask_);
    if (is_mask_) {
      blend_brush_ = OH_Drawing_BrushCreate();
      OH_Drawing_BrushSetBlendMode(blend_brush_, BLEND_MODE_DST_IN);
    }
  }
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
