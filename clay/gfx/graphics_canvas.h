// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GRAPHICS_CANVAS_H_
#define CLAY_GFX_GRAPHICS_CANVAS_H_

#include <memory>
#include <vector>

#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/paint.h"
#include "clay/gfx/picture.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/text_blob.h"
#include "skity/geometry/rect.hpp"

class SkCanvas;
class GrRecordingContext;

namespace clay {

class GraphicsCanvas {
 public:
  GraphicsCanvas() = default;
  virtual ~GraphicsCanvas() = default;

  virtual clay::GrCanvas* GetGrCanvas() = 0;

#ifndef ENABLE_SKITY
  virtual GrRecordingContext* RecordingContext() = 0;
#endif  // ENABLE_SKITY

  virtual int Save() = 0;
  virtual void Restore() = 0;
  virtual int SaveLayer(const skity::Rect& bounds, const Paint* paint) = 0;
  virtual int GetSaveCount() = 0;
  virtual void RestoreToCount(int save_count) = 0;

  virtual void Translate(float dx, float dy) = 0;
  virtual void Scale(float sx, float sy) = 0;
  virtual void Rotate(float degrees) = 0;
  virtual void Rotate(float degrees, float px, float py) = 0;
  virtual void Skew(float sx, float sy) = 0;
  virtual void Concat(const skity::Matrix& matrix) = 0;
  virtual void SetMatrix(const skity::Matrix& matrix) = 0;
  void ResetMatrix() { SetMatrix(skity::Matrix()); }

  virtual void ClipRect(const skity::Rect& rect, GrClipOp op,
                        bool do_anti_alias) = 0;
  void ClipRect(const skity::Rect& rect, GrClipOp op) {
    ClipRect(rect, op, false);
  }
  void ClipRect(const skity::Rect& rect, bool do_anti_alias) {
    ClipRect(rect, GrClipOp::kIntersect, false);
  }

  virtual void ClipRRect(const skity::RRect& rrect, GrClipOp op,
                         bool do_anti_alias) = 0;
  void ClipRRect(const skity::RRect& rrect, bool do_anti_alias) {
    ClipRRect(rrect, GrClipOp::kIntersect, do_anti_alias);
  }
  void ClipRRect(const skity::RRect& rrect, GrClipOp op) {
    ClipRRect(rrect, op, false);
  }
  void ClipRRect(const skity::RRect& rrect) {
    ClipRRect(rrect, GrClipOp::kIntersect, false);
  }

  virtual void ClipPath(const GrPath& path, GrClipOp op,
                        bool do_anti_alias) = 0;
  void ClipPath(const GrPath& path, GrClipOp op) { ClipPath(path, op, false); }
  void ClipPath(const GrPath& path, bool do_anti_alias) {
    ClipPath(path, GrClipOp::kIntersect, do_anti_alias);
  }

  virtual void OnDrawDynamicTextBlobsStart() {}
  virtual void OnDrawDynamicTextBlobsEnd() {}

  virtual bool QuickReject(const skity::Rect& rect) = 0;

  virtual bool GetDeviceClipBounds(skity::Rect* bounds) = 0;

  virtual void Clear(uint32_t color) = 0;

  virtual void DrawPaint(const Paint& paint) = 0;

  virtual void DrawLine(float x0, float y0, float x1, float y1,
                        const Paint& paint) = 0;
  void DrawLine(GrPoint p0, GrPoint p1, const Paint& paint) {
#if defined(ENABLE_SKITY)
    DrawLine(p0.x, p0.y, p1.x, p1.y, paint);
#else
    DrawLine(p0.x(), p0.y(), p1.x(), p1.y(), paint);
#endif  // ENABLE_SKITY
  }

  virtual void DrawRect(const skity::Rect& rect, const Paint& paint) = 0;

  virtual void DrawRRect(const skity::RRect& rrect, const Paint& paint) = 0;
  virtual void DrawDRRect(const skity::RRect& outer, const skity::RRect& inner,
                          const Paint& paint) = 0;
  virtual void DrawCircle(float cx, float cy, float radius,
                          const Paint& paint) = 0;
  virtual void DrawArc(const skity::Rect& oval, float start_angle,
                       float sweep_angle, bool use_center,
                       const Paint& paint) = 0;
  virtual void DrawPath(const GrPath& path, const Paint& paint) = 0;

  virtual void DrawImage(const GraphicsImage* image, float x, float y,
                         const GrSamplingOptions& sampling,
                         const Paint* paint) = 0;
  void DrawImage(const fml::RefPtr<GraphicsImage>& image, float x, float y,
                 const GrSamplingOptions& sampling, const Paint* paint) {
    DrawImage(image.get(), x, y, sampling, paint);
  }
  void DrawImage(GraphicsImage* image, double left, double top) {
    DrawImage(image, left, top, GrSamplingOptions(), nullptr);
  }
  void DrawImage(const fml::RefPtr<GraphicsImage>& image, float left,
                 float top) {
    DrawImage(image.get(), left, top, GrSamplingOptions(), nullptr);
  }

  virtual void DrawImageRect(const GraphicsImage* image, const skity::Rect& src,
                             const skity::Rect& dst,
                             const GrSamplingOptions& sampling,
                             const Paint* paint) = 0;
  void DrawImageRect(const fml::RefPtr<GraphicsImage>& image,
                     const skity::Rect& src, const skity::Rect& dst,
                     const GrSamplingOptions& sampling, const Paint* paint) {
    DrawImageRect(image.get(), src, dst, sampling, paint);
  }
  void DrawImageRect(const GraphicsImage* image, const skity::Rect& dst,
                     const GrSamplingOptions& sampling, const Paint* paint) {
    if (image == nullptr) {
      return;
    }
    DrawImageRect(image, skity::Rect::MakeWH(image->width(), image->height()),
                  dst, sampling, paint);
  }
  void DrawImageRect(const fml::RefPtr<GraphicsImage>& image,
                     const skity::Rect& dst, const GrSamplingOptions& sampling,
                     const Paint* paint) {
    DrawImageRect(image.get(), dst, sampling, paint);
  }

  virtual void DrawImageNine(const GraphicsImage* image,
                             const skity::Rect& center, const skity::Rect& dst,
                             FilterMode filter, const Paint* paint) = 0;
  virtual void DrawTextBlob(const fml::RefPtr<TextBlob>& blob, float x, float y,
                            const Paint& paint) = 0;

  virtual void DrawPicture(const Picture* picture) = 0;

  virtual skity::Matrix GetTotalMatrix() = 0;

  virtual std::unique_ptr<Picture> FinishRecordingAsPicture() = 0;

 protected:
  bool has_lazy_image_ = false;

 private:
  GraphicsCanvas(const GraphicsCanvas&) = delete;
  GraphicsCanvas(GraphicsCanvas&&) = delete;
  GraphicsCanvas& operator=(GraphicsCanvas&&) = delete;
  GraphicsCanvas& operator=(const GraphicsCanvas&) = delete;
};

}  // namespace clay

#endif  // CLAY_GFX_GRAPHICS_CANVAS_H_
