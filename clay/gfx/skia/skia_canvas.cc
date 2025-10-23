// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skia/skia_canvas.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "clay/gfx/animation/picture_animation_type.h"
#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/image/graphics_image_skia_lazy.h"
#include "clay/gfx/skia/picture_skia.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/gfx/style/sampling_options.h"
#include "third_party/skia/src/core/SkRecorder.h"

namespace clay {

SkCanvas* SkiaCanvas::GetGrCanvas() { return canvas_; }

GrRecordingContext* SkiaCanvas::RecordingContext() { return nullptr; }

int SkiaCanvas::Save() { return canvas_->save(); }

void SkiaCanvas::Restore() { canvas_->restore(); }

int SkiaCanvas::SaveLayer(const skity::Rect& bounds, const Paint* paint) {
  SkPaint skia_paint;
  if (paint) {
    skia_paint = paint->gr_object();
  }
  return canvas_->saveLayer(ConvertSkityRectToSkRect(bounds), &skia_paint);
}

int SkiaCanvas::GetSaveCount() { return canvas_->getSaveCount(); }

void SkiaCanvas::RestoreToCount(int save_count) {
  canvas_->restoreToCount(save_count);
}

void SkiaCanvas::Translate(float dx, float dy) { canvas_->translate(dx, dy); }

void SkiaCanvas::Scale(float sx, float sy) { canvas_->scale(sx, sy); }

void SkiaCanvas::Rotate(float degrees) { canvas_->rotate(degrees); }

void SkiaCanvas::Rotate(float degrees, float px, float py) {
  canvas_->rotate(degrees, px, py);
}

void SkiaCanvas::Skew(float sx, float sy) { canvas_->skew(sx, sy); }

void SkiaCanvas::Concat(const skity::Matrix& matrix) {
  canvas_->concat(ConvertSkityMatrixToSkMatrix(matrix));
}
void SkiaCanvas::SetMatrix(const skity::Matrix& matrix) {
  canvas_->resetMatrix();
  canvas_->concat(ConvertSkityMatrixToSkMatrix(matrix));
}

void SkiaCanvas::ClipRect(const skity::Rect& rect, SkClipOp op,
                          bool do_anti_alias) {
  canvas_->clipRect(ConvertSkityRectToSkRect(rect), op, do_anti_alias);
}

void SkiaCanvas::ClipRRect(const skity::RRect& rrect, SkClipOp op,
                           bool do_anti_alias) {
  canvas_->clipRRect(ConvertSkityRRectToSkia(rrect), op, do_anti_alias);
}

void SkiaCanvas::ClipPath(const SkPath& path, SkClipOp op, bool do_anti_alias) {
  canvas_->clipPath(path, op, do_anti_alias);
}

bool SkiaCanvas::QuickReject(const skity::Rect& rect) {
  return canvas_->quickReject(ConvertSkityRectToSkRect(rect));
}

bool SkiaCanvas::GetDeviceClipBounds(skity::Rect* bounds) {
  if (!bounds) {
    return false;
  }
  SkIRect clip_bounds;
  bool result = canvas_->getDeviceClipBounds(&clip_bounds);
  *bounds = ConvertSkIRectToSkityRect(clip_bounds);
  return result;
}

void SkiaCanvas::Clear(uint32_t color) {
  canvas_->clear(static_cast<SkColor>(color));
}

void SkiaCanvas::DrawPaint(const Paint& paint) {
  SkPaint skia_paint = paint.gr_object();
  canvas_->drawPaint(skia_paint);
}

void SkiaCanvas::DrawLine(float x0, float y0, float x1, float y1,
                          const Paint& paint) {
  canvas_->drawLine(x0, y0, x1, y1, paint.gr_object());
}

void SkiaCanvas::DrawRect(const skity::Rect& rect, const Paint& paint) {
  canvas_->drawRect(ConvertSkityRectToSkRect(rect), paint.gr_object());
  RecordDynamicOpOffset(paint);
}

void SkiaCanvas::DrawRRect(const skity::RRect& rrect, const Paint& paint) {
  canvas_->drawRRect(ConvertSkityRRectToSkia(rrect), paint.gr_object());
}

void SkiaCanvas::DrawDRRect(const skity::RRect& outer,
                            const skity::RRect& inner, const Paint& paint) {
  canvas_->drawDRRect(ConvertSkityRRectToSkia(outer),
                      ConvertSkityRRectToSkia(inner), paint.gr_object());
}

void SkiaCanvas::DrawCircle(float cx, float cy, float radius,
                            const Paint& paint) {
  canvas_->drawCircle(cx, cy, radius, paint.gr_object());
}

void SkiaCanvas::DrawArc(const skity::Rect& oval, float start_angle,
                         float sweep_angle, bool use_center,
                         const Paint& paint) {
  canvas_->drawArc(ConvertSkityRectToSkRect(oval), start_angle, sweep_angle,
                   use_center, paint.gr_object());
}

void SkiaCanvas::DrawPath(const SkPath& path, const Paint& paint) {
  canvas_->drawPath(path, paint.gr_object());
}

void SkiaCanvas::DrawImage(const GraphicsImage* image, float x, float y,
                           const SkSamplingOptions& sampling,
                           const Paint* paint) {
  FML_DCHECK(image);
  FML_DCHECK(image->isTextureBacked());

  SkPaint skia_paint;
  if (paint) {
    skia_paint = paint->gr_object();
  }
  auto skia_image = image->gr_image();
  // TODO(feiyue.1998): Support lazy image.
  canvas_->drawImage(skia_image, x, y, sampling, &skia_paint);
}

void SkiaCanvas::DrawImageRect(const GraphicsImage* image,
                               const skity::Rect& src, const skity::Rect& dst,
                               const SkSamplingOptions& sampling,
                               const Paint* paint) {
  FML_DCHECK(image);

  SkPaint skia_paint;
  if (paint) {
    skia_paint = paint->gr_object();
  }

  auto skia_image = image->gr_image();
  // TODO(feiyue.1998): Support lazy image.
  canvas_->drawImageRect(skia_image, ConvertSkityRectToSkRect(src),
                         ConvertSkityRectToSkRect(dst), sampling, &skia_paint,
                         SkCanvas::kFast_SrcRectConstraint);
}

void SkiaCanvas::DrawImageNine(const GraphicsImage* image,
                               const skity::Rect& center,
                               const skity::Rect& dst, FilterMode filter,
                               const Paint* paint) {
  SkPaint skia_paint;
  if (paint) {
    skia_paint = paint->gr_object();
  }
  auto skia_image = image->gr_image();
  SkFilterMode sk_filter;
  switch (filter) {
    case clay::FilterMode::kNearest: {
      sk_filter = SkFilterMode::kNearest;
      break;
    }
    case clay::FilterMode::kLinear: {
      sk_filter = SkFilterMode::kLinear;
      break;
    }
    default: {
      break;
    }
  }
  canvas_->drawImageNine(skia_image.get(), ConvertSkityRectToSkIRect(center),
                         ConvertSkityRectToSkRect(dst), sk_filter, &skia_paint);
}
void SkiaCanvas::DrawTextBlob(const fml::RefPtr<TextBlob>& blob, float x,
                              float y, const Paint& paint) {
  auto skia_text_blob = blob->gr_text_blob();
  if (!skia_text_blob) {
    return;
  }
  canvas_->drawTextBlob(skia_text_blob, x, y, paint.gr_object());
}

void SkiaCanvas::DrawPicture(const Picture* picture) {
  canvas_->drawPicture(picture->picture()->raw());
}

skity::Matrix SkiaCanvas::GetTotalMatrix() {
  return ConvertSkMatrixToSkityMatrix(canvas_->getTotalMatrix());
}

void SkiaCanvas::RecordDynamicOpOffset(const Paint& paint) {
  switch (paint.getDynamicOpType()) {
    case clay::DynamicOpType::kSetBackgroundColor: {
      SkRecorder* recorder = static_cast<SkRecorder*>(canvas_);
      int op_offset = recorder->getCurrentOpCount() - 1;
      FML_DCHECK(op_offset >= 0);
      dynamic_ops_.emplace_back(
          std::make_pair(clay::DynamicOpType::kSetBackgroundColor, op_offset));
    } break;
    case clay::DynamicOpType::kSetTextColor: {
      if (start_draw_dynamic_text_blobs_) {
        SkRecorder* recorder = static_cast<SkRecorder*>(canvas_);
        int op_offset = recorder->getCurrentOpCount() - 1;
        FML_DCHECK(op_offset >= 0);
        dynamic_ops_.emplace_back(
            std::make_pair(clay::DynamicOpType::kSetTextColor, op_offset));
      }
    } break;
    default:
      break;
  }
}

void SkiaCanvas::OnDrawDynamicTextBlobsStart() {
  start_draw_dynamic_text_blobs_ = true;
}

void SkiaCanvas::OnDrawDynamicTextBlobsEnd() {
  start_draw_dynamic_text_blobs_ = false;
}

SkiaRecorderCanvas::SkiaRecorderCanvas(const skity::Rect& bounds,
                                       fml::RefPtr<GPUUnrefQueue> unref_queue)
    : picture_recorder_(std::make_unique<SkPictureRecorder>()),
      rtree_factory_(std::make_unique<SkRTreeFactory>()),
      unref_queue_(unref_queue) {
  picture_recorder_->beginRecording(ConvertSkityRectToSkRect(bounds),
                                    rtree_factory_.get());
  canvas_ = picture_recorder_->getRecordingCanvas();
}

std::unique_ptr<Picture> SkiaRecorderCanvas::FinishRecordingAsPicture() {
  auto skia_picture = picture_recorder_->finishRecordingAsPicture();
  rtree_factory_.reset();
  auto picture_skia =
      fml::MakeRefCounted<PictureSkia>(skia_picture, std::move(dynamic_ops_));
  return std::make_unique<Picture>(
      GPUObject(std::move(picture_skia), std::move(unref_queue_)),
      has_lazy_image_);
}

SkiaBitmapCanvas::SkiaBitmapCanvas(const SkBitmap& bitmap)
    : owned_(std::make_unique<SkCanvas>(bitmap)) {
  canvas_ = owned_.get();
}

std::unique_ptr<Picture> SkiaBitmapCanvas::FinishRecordingAsPicture() {
  return nullptr;
}

}  // namespace clay
