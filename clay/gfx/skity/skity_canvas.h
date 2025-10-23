// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_SKITY_CANVAS_H_
#define CLAY_GFX_SKITY_SKITY_CANVAS_H_

#include <memory>

#include "clay/gfx/animation/picture_animation_type.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/graphics_canvas.h"
#include "skity/recorder/picture_recorder.hpp"
#include "skity/recorder/recording_canvas.hpp"
#include "skity/render/canvas.hpp"

namespace clay {

class SkityCanvas : public GraphicsCanvas {
 public:
  SkityCanvas() = default;
  ~SkityCanvas() override = default;

  int Save() override;
  void Restore() override;
  int SaveLayer(const skity::Rect& bounds, const Paint* paint) override;
  int GetSaveCount() override;
  void RestoreToCount(int save_count) override;

  void Translate(float dx, float dy) override;
  void Scale(float sx, float sy) override;
  void Rotate(float degrees) override;
  void Rotate(float degrees, float px, float py) override;
  void Skew(float sx, float sy) override;
  void Concat(const skity::Matrix& matrix) override;
  void SetMatrix(const skity::Matrix& matrix) override;

  void ClipRect(const skity::Rect& rect, skity::Canvas::ClipOp op,
                bool do_anti_alias) override;

  void ClipRRect(const skity::RRect& rrect, skity::Canvas::ClipOp op,
                 bool do_anti_alias) override;

  void ClipPath(const skity::Path& path, skity::Canvas::ClipOp op,
                bool do_anti_alias) override;

  bool QuickReject(const skity::Rect& rect) override;

  bool GetDeviceClipBounds(skity::Rect* bounds) override;

  void Clear(uint32_t color) override;

  void DrawPaint(const Paint& paint) override;
  void DrawLine(float x0, float y0, float x1, float y1,
                const Paint& paint) override;
  void DrawRect(const skity::Rect& rect, const Paint& paint) override;
  void DrawRRect(const skity::RRect& rrect, const Paint& paint) override;
  void DrawDRRect(const skity::RRect& outer, const skity::RRect& inner,
                  const Paint& paint) override;
  void DrawCircle(float cx, float cy, float radius,
                  const Paint& paint) override;
  void DrawArc(const skity::Rect& oval, float start_angle, float sweep_angle,
               bool use_center, const Paint& paint) override;
  void DrawPath(const skity::Path& path, const Paint& paint) override;
  void DrawImage(const GraphicsImage* image, float x, float y,
                 const GrSamplingOptions& sampling,
                 const Paint* paint) override;
  void DrawImageRect(const GraphicsImage* image, const skity::Rect& src,
                     const skity::Rect& dst, const GrSamplingOptions& sampling,
                     const Paint* paint) override;
  void DrawImageNine(const GraphicsImage* image, const skity::Rect& center,
                     const skity::Rect& dst, FilterMode filter,
                     const Paint* paint) override;
  void DrawTextBlob(const fml::RefPtr<TextBlob>& blob, float x, float y,
                    const Paint& paint) override;

  void DrawPicture(const Picture* picture) override;

  skity::Matrix GetTotalMatrix() override;

  skity::Canvas* GetGrCanvas() override { return canvas_; }

  void OnDrawDynamicTextBlobsStart() override;
  void OnDrawDynamicTextBlobsEnd() override;

  using GraphicsCanvas::ClipPath;
  using GraphicsCanvas::ClipRect;
  using GraphicsCanvas::ClipRRect;
  using GraphicsCanvas::DrawImage;
  using GraphicsCanvas::ResetMatrix;

 private:
  void RecordDynamicOpOffset(const Paint& paint);

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(SkityCanvas);

 protected:
  skity::RecordingCanvas* canvas_ = nullptr;
  DynamicOps dynamic_ops_;
  bool start_draw_dynamic_text_blobs_ = false;
};

class SkityRecorderCanvas : public SkityCanvas {
 public:
  SkityRecorderCanvas(const skity::Rect& bounds,
                      fml::RefPtr<GPUUnrefQueue> unref_queue);
  ~SkityRecorderCanvas() override = default;

  std::unique_ptr<Picture> FinishRecordingAsPicture() override;

 private:
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  std::unique_ptr<skity::PictureRecorder> picture_recorder_;
};

}  // namespace clay

#endif  // CLAY_GFX_SKITY_SKITY_CANVAS_H_
