// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_DRAWABLE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_DRAWABLE_H_

#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_types.h>

#include <memory>
#include <string>

#include "base/include/value/base_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/box_shadow_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/layer_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/border_radius.h"

namespace lynx {
namespace tasm {
namespace harmony {
class BackgroundDrawable {
  enum class BorderPosition : unsigned {
    kTop,
    kLeft,
    kRight,
    kBottom,
  };

  struct RoundRectPath {
    RoundRectPath() { path = OH_Drawing_PathCreate(); }
    ~RoundRectPath() {
      OH_Drawing_PathDestroy(path);
      path = nullptr;
    }
    OH_Drawing_Path* path{nullptr};
    std::array<float, 8> radius{};
    std::string path_string;
    float left{0};
    float top{0};
    float right{0};
    float bottom{0};
  };

  struct BorderInfo {
    starlight::BorderStyleType border_left_style{
        starlight::BorderStyleType::kSolid};
    starlight::BorderStyleType border_right_style{
        starlight::BorderStyleType::kSolid};
    starlight::BorderStyleType border_top_style{
        starlight::BorderStyleType::kSolid};
    starlight::BorderStyleType border_bottom_style{
        starlight::BorderStyleType::kSolid};
    float left{0};
    float top{0};
    float border_left_width{0};
    float border_right_width{0};
    float border_top_width{0};
    float border_bottom_width{0};
    float border_left_origin_width{0};
    float border_right_origin_width{0};
    float border_top_origin_width{0};
    float border_bottom_origin_width{0};
    uint32_t border_left_color{0xFF000000};
    uint32_t border_right_color{0xFF000000};
    uint32_t border_top_color{0xFF000000};
    uint32_t border_bottom_color{0xFF000000};
  };

 public:
  BackgroundDrawable(const std::weak_ptr<UIBase>& ui_base, bool is_mask);
  void UpdateBounds(float left, float top, float width, float height,
                    float padding_left, float padding_top, float padding_right,
                    float padding_bottom, float scale_density);
  void SetBorderRadius(const lepus::Value& value);
  void SetBorderTopLeftRadius(const lepus::Value& value);
  void SetBorderTopRightRadius(const lepus::Value& value);
  void SetBorderBottomRightRadius(const lepus::Value& value);
  void SetBorderBottomLeftRadius(const lepus::Value& value);
  void SetBorderStyle(const lepus::Value& value);
  void SetBorderLeftStyle(const lepus::Value& value);
  void SetBorderRightStyle(const lepus::Value& value);
  void SetBorderTopStyle(const lepus::Value& value);
  void SetBorderBottomStyle(const lepus::Value& value);
  void SetBorderColor(const lepus::Value& value);
  void SetBorderLeftColor(const lepus::Value& value);
  void SetBorderRightColor(const lepus::Value& value);
  void SetBorderTopColor(const lepus::Value& value);
  void SetBorderBottomColor(const lepus::Value& value);
  void SetBorderWidth(const lepus::Value& value);
  void SetBorderLeftWidth(const lepus::Value& value);
  void SetBorderRightWidth(const lepus::Value& value);
  void SetBorderTopWidth(const lepus::Value& value);
  void SetBorderBottomWidth(const lepus::Value& value);
  void SetBackgroundClip(const lepus::Value& value);
  void SetBackgroundOrigin(const lepus::Value& value);
  void SetBackgroundImage(const lepus::Value& value);
  void SetBackgroundColor(uint32_t background_color);
  void SetBackgroundSize(const lepus::Value& value);
  void SetBackgroundPosition(const lepus::Value& value);
  void SetBackgroundRepeat(const lepus::Value& value);
  void SetBoxShadow(const lepus::Value& value);
  void SetBorderWidth(const std::array<float, 4>& value);
  void Render(OH_Drawing_Canvas* canvas);
  bool HasShadow();
  bool HasBorder() { return has_border_; }
  bool HasImage();
  bool UseClipPath() {
    return has_border_ || has_image_ || !border_radius_->IsZero();
  }
  const std::string& GetClipPath();
  BorderRadius* GetBorderRadius();
  float GetBorderLeftWidth();
  float GetBorderRightWidth();
  float GetBorderTopWidth();
  float GetBorderBottomWidth();

  void AdjustBorder();

  ~BackgroundDrawable();

  struct Point {
    float x;
    float y;
  };

 private:
  std::weak_ptr<UIBase> ui_base_;
  bool is_mask_;
  float scale_density_{1.0};
  std::unique_ptr<BorderInfo> border_info_{nullptr};
  std::unique_ptr<RoundRectPath> inner_clip_path_{nullptr};
  std::unique_ptr<RoundRectPath> outer_clip_path_{nullptr};
  std::unique_ptr<RoundRectPath> inner_draw_path_{nullptr};
  std::unique_ptr<RoundRectPath> outer_draw_path_{nullptr};
  std::unique_ptr<RoundRectPath> center_draw_path_{nullptr};
  std::unique_ptr<Point> inner_top_left_point_{nullptr};
  std::unique_ptr<Point> inner_top_right_point_{nullptr};
  std::unique_ptr<Point> inner_bottom_right_point_{nullptr};
  std::unique_ptr<Point> inner_bottom_left_point_{nullptr};
  std::unique_ptr<BorderRadius> border_radius_{nullptr};
  std::unique_ptr<BoxShadowLayer> box_shadow_layer_{nullptr};
  float view_width_{0};
  float view_height_{0};
  float left_{0};
  float top_{0};
  bool has_border_{false};
  bool has_image_{false};
  std::unique_ptr<LayerManager> layer_manager_{nullptr};
  uint32_t background_color_{};
  float padding_left_{0};
  float padding_right_{0};
  float padding_top_{0};
  float padding_bottom_{0};
  std::unique_ptr<LayerManager::Rect> padding_box_rect_{nullptr};
  std::unique_ptr<LayerManager::Rect> content_box_rect_{nullptr};
  std::unique_ptr<LayerManager::Rect> border_box_rect_{nullptr};
  OH_Drawing_Rect* padding_box_draw_rect_{nullptr};
  OH_Drawing_Rect* content_box_draw_rect_{nullptr};
  OH_Drawing_Rect* border_box_draw_rect_{nullptr};
  OH_Drawing_Brush* brush_{nullptr};
  OH_Drawing_Brush* blend_brush_{nullptr};
  OH_Drawing_Path* border_path_{nullptr};
  OH_Drawing_Pen* pen_{nullptr};
  OH_Drawing_Path* border_line_path_{nullptr};
  OH_Drawing_Path* border_fill_path_{nullptr};
  OH_Drawing_PathEffect* path_effect_{nullptr};
  bool is_same_color_{false};
  bool is_same_width_{false};
  bool is_same_style_{false};
  bool is_simple_style_{false};

  void DrawBorder(OH_Drawing_Canvas* canvas);

  void DrawBackground(OH_Drawing_Canvas* canvas);

  void DrawShadow(OH_Drawing_Canvas* canvas);

  void DrawRectBorders(OH_Drawing_Canvas* canvas, OH_Drawing_Pen* pen);
  void DrawRoundedBorders(OH_Drawing_Canvas* canvas, OH_Drawing_Pen* pen);

  void UpdateContentBox();

  void UpdateBorderClipPath(float x1, float y1, float x2, float y2, float x3,
                            float y3, float x4, float y4, bool clip_area);

  void DrawRectBorderLines(OH_Drawing_Canvas* canvas, OH_Drawing_Pen* pen,
                           const BorderPosition& border_position,
                           float border_width, uint32_t color, float start_x,
                           float start_y, float stop_x, float stop_y,
                           float border_length,
                           const starlight::BorderStyleType& style);

  void DrawRoundBorderPath(OH_Drawing_Canvas* canvas, OH_Drawing_Pen* paint,
                           const BorderPosition& border_position,
                           const starlight::BorderStyleType& border_style,
                           uint32_t color, float border_effect_width,
                           float border_stoke_width, bool need_clip);

  void UpdateRoundBorderPath(float mul,
                             std::unique_ptr<RoundRectPath>& round_path);

  OH_Drawing_PathEffect* GetPathEffectAutoAdjust(
      float border_width, float border_length,
      const starlight::BorderStyleType& style);
  OH_Drawing_PathEffect* GetPathEffect(const starlight::BorderStyleType& style,
                                       float border_width);

  void DrawMultiColorBorderLine(OH_Drawing_Canvas* canvas,
                                OH_Drawing_Pen* paint,
                                const BorderPosition& border_position,
                                float border_width, float border_offset,
                                uint32_t color0, uint32_t color1, float start_x,
                                float start_y, float stop_x, float stop_y);

  void DrawMultiColorBorderPath(OH_Drawing_Canvas* canvas, OH_Drawing_Pen* pen,
                                const BorderPosition& border_position,
                                float border_width, uint32_t color0,
                                uint32_t color1, bool is_double_style);
  void DestroyPath(
      const std::unique_ptr<BackgroundDrawable::RoundRectPath>& rect_path);
  void UpdateBorderPoint(std::unique_ptr<BackgroundDrawable::Point>& point,
                         float x, float y);
  void UpdateRoundMultiColoredBorderPoint();
  float GetLength(float length, int32_t unit, float reference);
  void UpdatePath();
  void DrawClipBorderLine(float start_x, float start_y, float stop_x,
                          float stop_y, uint32_t color, OH_Drawing_Pen* pen,
                          OH_Drawing_Canvas* canvas);
  void DrawClipBorderPath(OH_Drawing_Path* path, uint32_t color,
                          OH_Drawing_Pen* pen, OH_Drawing_Canvas* canvas);
  void InitLayerManager();
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_DRAWABLE_H_
