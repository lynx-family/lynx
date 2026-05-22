// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GRAPHICS_CONTEXT_H_
#define CLAY_GFX_GRAPHICS_CONTEXT_H_

#include <array>
#include <memory>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/graphics_canvas.h"
#include "clay/gfx/image/image_data.h"
#include "clay/gfx/image/image_resource.h"
#include "clay/gfx/paint_recorder.h"
#include "clay/gfx/picture.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/gfx/style/color.h"
#include "clay/gfx/style/length.h"

namespace clay {

class GraphicsContext final {
 public:
  class AutoRestore {
   public:
    AutoRestore(GraphicsContext* context, bool do_save);
    ~AutoRestore();
    void Restore();

   private:
    GraphicsCanvas* canvas_ = nullptr;
    int save_count_ = 0;
  };

  explicit GraphicsContext(fml::RefPtr<GPUUnrefQueue> unref_queue)
      : unref_queue_(unref_queue), paint_recorder_(unref_queue) {}

  bool BeginRecording(const skity::Rect& bounds);
#ifndef ENABLE_SKITY
  bool BeginRecording(const SkBitmap& bitmap);
#endif  // ENABLE_SKITY
  bool IsRecording() const;
  std::unique_ptr<Picture> FinishRecording();

  void DrawNinePatch(const GraphicsImage* image,
                     const std::array<float, 4>& cap_insets,
                     float cap_insets_scale, float pixel_ratio,
                     const skity::Rect& dst_rect);

  void ApplyBlurEffect(float blur, Paint* paint) const;
  void ApplyDropShadowEffect(Paint* paint, const ImageData& image_data);

  bool DrawImageWithDropShadow(fml::RefPtr<GraphicsImage> image,
                               const ImageData& image_data,
                               const skity::Rect& src_rect,
                               const skity::Rect& dst_rect,
                               const skity::Rect& padding_rect,
                               float padding_left, float padding_top);

  void DrawImageRect(const fml::RefPtr<GraphicsImage>& image,
                     const skity::Rect& src, const skity::Rect& dst,
                     const GrSamplingOptions& sampling, const Paint* paint);

  void DrawImageRect(const fml::RefPtr<GraphicsImage>& image,
                     const skity::Rect& dst, const GrSamplingOptions& sampling,
                     const Paint* paint);

  void DrawLine(Paint stroke_paint, const FloatPoint& point1,
                const FloatPoint& point2, float thickness,
                BorderStyleType style);

  void DrawRect(Paint paint, const skity::Rect& rect);

  void FillPolygon(size_t num_points, const FloatPoint* points,
                   const Color& color, bool should_antialias);

  static void SetupPaintDashPathEffect(Paint* paint, int length,
                                       float thickness, BorderStyleType style);
  static float GetPathLength(const GrPath& path);
  static void SetPathFromPoints(GrPath* path, size_t num_points,
                                const FloatPoint* points);
  static void AdjustLineToPixelBoundaries(FloatPoint* p1, FloatPoint* p2,
                                          float stroke_width,
                                          BorderStyleType style);
  static float ConvertRadiusToSigma(float radius);

  // GraphicsCanvas wrappers
  GraphicsCanvas* Canvas() { return paint_recorder_.Canvas(); }

  clay::GrCanvas* GetGrCanvas() {
    return paint_recorder_.Canvas()->GetGrCanvas();
  }

  fml::RefPtr<GPUUnrefQueue> GetUnrefQueue() { return unref_queue_; }
  int Save() { return Canvas()->Save(); }
  int SaveLayer(const skity::Rect& bounds, const Paint* paint) {
    return Canvas()->SaveLayer(bounds, paint);
  }
  void Restore() { Canvas()->Restore(); }
  void Translate(float dx, float dy) { Canvas()->Translate(dx, dy); }
  void Scale(float sx, float sy) { Canvas()->Scale(sx, sy); }
  void Rotate(float degrees) { Canvas()->Rotate(degrees); }
  void Concat(const skity::Matrix& matrix) { Canvas()->Concat(matrix); }
  void SetMatrix(const skity::Matrix& matrix) { Canvas()->SetMatrix(matrix); }
  void ResetMatrix() { Canvas()->ResetMatrix(); }
  void ClipRect(const skity::Rect& rect, GrClipOp op, bool do_anti_alias) {
    Canvas()->ClipRect(rect, op, do_anti_alias);
  }
  void DrawLine(float x0, float y0, float x1, float y1, const Paint& paint) {
    Canvas()->DrawLine(x0, y0, x1, y1, paint);
  }
  void ClipRRect(const skity::RRect& rrect, GrClipOp op, bool do_anti_alias) {
    Canvas()->ClipRRect(rrect, op, do_anti_alias);
  }
  void DrawRect(const skity::Rect& rect, const Paint& paint) {
    Canvas()->DrawRect(rect, paint);
  }
  void DrawRRect(const skity::RRect& rrect, const Paint& paint) {
    Canvas()->DrawRRect(rrect, paint);
  }
  void ClipPath(const GrPath& path, GrClipOp op, bool do_anti_alias) {
    Canvas()->ClipPath(path, op, do_anti_alias);
  }
  bool GetDeviceClipBounds(skity::Rect* bounds) {
    return Canvas()->GetDeviceClipBounds(bounds);
  }
  void DrawDRRect(const skity::RRect& outer, const skity::RRect& inner,
                  const Paint& paint) {
    Canvas()->DrawDRRect(outer, inner, paint);
  }
  void DrawPath(const GrPath& path, const Paint& paint) {
    Canvas()->DrawPath(path, paint);
  }
  void DrawPicture(const Picture* picture) { Canvas()->DrawPicture(picture); }

 private:
  void DrawNinePatchIndividually(const GraphicsImage* image, float pixel_ratio,
                                 const std::array<float, 4>& cap_insets,
                                 const skity::Rect& center,
                                 const skity::Rect& dst_rect);

  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  PaintRecorder paint_recorder_;
};

}  // namespace clay

#endif  // CLAY_GFX_GRAPHICS_CONTEXT_H_
