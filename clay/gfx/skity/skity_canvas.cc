// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skity/skity_canvas.h"

#include <utility>

#include "clay/gfx/image/graphics_image_skity_lazy.h"
#include "clay/gfx/skity_to_skia_utils.h"

namespace clay {

int SkityCanvas::Save() { return canvas_->Save(); }

void SkityCanvas::Restore() { canvas_->Restore(); }

int SkityCanvas::SaveLayer(const skity::Rect& bounds, const Paint* paint) {
  skity::Paint skity_paint;
  if (paint) {
    skity_paint = paint->gr_object();
  }
  return canvas_->SaveLayer(bounds, skity_paint);
}

int SkityCanvas::GetSaveCount() { return canvas_->GetSaveCount(); }

void SkityCanvas::RestoreToCount(int save_count) {
  canvas_->RestoreToCount(save_count);
}

void SkityCanvas::Translate(float dx, float dy) { canvas_->Translate(dx, dy); }

void SkityCanvas::Scale(float sx, float sy) { canvas_->Scale(sx, sy); }

void SkityCanvas::Rotate(float degrees) { canvas_->Rotate(degrees); }

void SkityCanvas::Rotate(float degrees, float px, float py) {
  canvas_->Rotate(degrees, px, py);
}

void SkityCanvas::Skew(float sx, float sy) { canvas_->Skew(sx, sy); }

void SkityCanvas::Concat(const skity::Matrix& matrix) {
  canvas_->Concat(matrix);
}
void SkityCanvas::SetMatrix(const skity::Matrix& matrix) {
  canvas_->ResetMatrix();
  canvas_->Concat(matrix);
}

void SkityCanvas::ClipRect(const skity::Rect& rect, skity::Canvas::ClipOp op,
                           bool do_anti_alias) {
  canvas_->ClipRect(rect, op);
}

void SkityCanvas::ClipRRect(const skity::RRect& rrect, skity::Canvas::ClipOp op,
                            bool do_anti_alias) {
  skity::Path skity_path;
  skity_path.AddRRect(rrect);
  canvas_->ClipPath(skity_path, op);
}

void SkityCanvas::ClipPath(const skity::Path& path, skity::Canvas::ClipOp op,
                           bool do_anti_alias) {
  canvas_->ClipPath(path, op);
}

bool SkityCanvas::QuickReject(const skity::Rect& rect) {
  return canvas_->QuickReject(rect);
}

bool SkityCanvas::GetDeviceClipBounds(skity::Rect* bounds) {
  if (!bounds) {
    return false;
  }
  *bounds = canvas_->GetGlobalClipBounds();
  return true;
}

void SkityCanvas::Clear(uint32_t color) {
  canvas_->Clear(static_cast<skity::Color>(color));
}

void SkityCanvas::DrawPaint(const Paint& paint) {
  skity::Paint skity_paint = paint.gr_object();
  canvas_->DrawPaint(skity_paint);
  RecordDynamicOpOffset(paint);
}

void SkityCanvas::DrawLine(float x0, float y0, float x1, float y1,
                           const Paint& paint) {
  canvas_->DrawLine(x0, y0, x1, y1, paint.gr_object());
  RecordDynamicOpOffset(paint);
}

void SkityCanvas::DrawRect(const skity::Rect& rect, const Paint& paint) {
  canvas_->DrawRect(rect, paint.gr_object());
  RecordDynamicOpOffset(paint);
}

void SkityCanvas::DrawRRect(const skity::RRect& rrect, const Paint& paint) {
  canvas_->DrawRRect(rrect, paint.gr_object());
  RecordDynamicOpOffset(paint);
}

void SkityCanvas::DrawDRRect(const skity::RRect& outer,
                             const skity::RRect& inner, const Paint& paint) {
  if (outer.IsEmpty()) {
    return;
  }
  if (inner.IsEmpty()) {
    this->DrawRRect(outer, paint);
    return;
  }

  if (!outer.GetBounds().Contains(inner.GetBounds())) {
    return;
  }

  skity::Path skity_path;
  skity_path.AddRRect(outer);
  skity_path.AddRRect(inner);
  skity_path.SetFillType(skity::Path::PathFillType::kEvenOdd);

  canvas_->DrawPath(skity_path, paint.gr_object());
  RecordDynamicOpOffset(paint);
}

void SkityCanvas::DrawCircle(float cx, float cy, float radius,
                             const Paint& paint) {
  canvas_->DrawCircle(cx, cy, radius, paint.gr_object());
  RecordDynamicOpOffset(paint);
}

void SkityCanvas::DrawArc(const skity::Rect& oval, float start_angle,
                          float sweep_angle, bool use_center,
                          const Paint& paint) {
  canvas_->DrawArc(oval, start_angle, sweep_angle, use_center,
                   paint.gr_object());
  RecordDynamicOpOffset(paint);
}

void SkityCanvas::DrawPath(const skity::Path& path, const Paint& paint) {
  canvas_->DrawPath(path, paint.gr_object());
  RecordDynamicOpOffset(paint);
}

void SkityCanvas::DrawImage(const GraphicsImage* image, float x, float y,
                            const GrSamplingOptions& sampling,
                            const Paint* paint) {
  FML_DCHECK(image);
  FML_DCHECK(image->isTextureBacked());

  skity::Rect rect =
      skity::Rect::MakeLTRB(x, y, x + image->width(), y + image->height());
  skity::Paint skity_paint;
  if (paint) {
    skity_paint = paint->gr_object();
  }
  auto skity_image = image->gr_image();
  has_lazy_image_ |= image->IsLazyImage();
  canvas_->DrawImage(
      image->IsLazyImage()
          ? static_cast<const GraphicsImageSkityLazy*>(image)->decoding_image()
          : skity_image,
      rect, sampling, &skity_paint);
  if (paint) {
    RecordDynamicOpOffset(*paint);
  }
}

void SkityCanvas::DrawImageRect(const GraphicsImage* image,
                                const skity::Rect& src, const skity::Rect& dst,
                                const GrSamplingOptions& sampling,
                                const Paint* paint) {
  FML_DCHECK(image);
  FML_DCHECK(image->IsLazyImage() || image->isTextureBacked());

  skity::Paint skity_paint;
  if (paint) {
    skity_paint = paint->gr_object();
  }
  skity::SamplingOptions skity_sampling_options(
      static_cast<skity::FilterMode>(sampling.filter),
      static_cast<skity::MipmapMode>(sampling.mipmap));

  auto skity_image = image->gr_image();
  has_lazy_image_ |= image->IsLazyImage();
  canvas_->DrawImageRect(
      image->IsLazyImage()
          ? static_cast<const GraphicsImageSkityLazy*>(image)->decoding_image()
          : skity_image,
      src, dst, sampling, &skity_paint);
  if (paint) {
    RecordDynamicOpOffset(*paint);
  }
}

void SkityCanvas::DrawImageNine(const GraphicsImage* image,
                                const skity::Rect& center,
                                const skity::Rect& dst, FilterMode filter,
                                const Paint* paint) {
  // TODO(zhangxiao.ninja) implement later
  FML_DCHECK(false);
}
void SkityCanvas::DrawTextBlob(const fml::RefPtr<TextBlob>& blob, float x,
                               float y, const Paint& paint) {
  auto skity_text_blob = blob->gr_text_blob();
  if (!skity_text_blob) {
    return;
  }
  canvas_->DrawTextBlob(skity_text_blob, x, y, paint.gr_object());
  if (start_draw_dynamic_text_blobs_) {
    skity::RecordedOpOffset offset = canvas_->GetLastOpOffset();
    FML_DCHECK(offset.IsValid());
    dynamic_ops_.emplace_back(
        std::make_pair(clay::DynamicOpType::kSetTextColor, offset));
  }
}

void SkityCanvas::DrawPicture(const Picture* picture) {
  // TODO(zhangxiao.ninja) implement later
  FML_DCHECK(false);
}

skity::Matrix SkityCanvas::GetTotalMatrix() {
  return canvas_->GetTotalMatrix();
}

void SkityCanvas::OnDrawDynamicTextBlobsStart() {
  start_draw_dynamic_text_blobs_ = true;
}

void SkityCanvas::OnDrawDynamicTextBlobsEnd() {
  start_draw_dynamic_text_blobs_ = false;
}

void SkityCanvas::RecordDynamicOpOffset(const Paint& paint) {
  switch (paint.getDynamicOpType()) {
    case clay::DynamicOpType::kSetTextColor:
      if (start_draw_dynamic_text_blobs_) {
        skity::RecordedOpOffset offset = canvas_->GetLastOpOffset();
        FML_DCHECK(offset.IsValid());
        dynamic_ops_.emplace_back(
            std::make_pair(clay::DynamicOpType::kSetTextColor, offset));
      }
      break;
    case clay::DynamicOpType::kSetBackgroundColor: {
      skity::RecordedOpOffset offset = canvas_->GetLastOpOffset();
      FML_DCHECK(offset.IsValid());
      dynamic_ops_.emplace_back(
          std::make_pair(clay::DynamicOpType::kSetBackgroundColor, offset));
    } break;
    default:
      break;
  }
}

SkityRecorderCanvas::SkityRecorderCanvas(const skity::Rect& bounds,
                                         fml::RefPtr<GPUUnrefQueue> unref_queue)
    : unref_queue_(unref_queue),
      picture_recorder_(std::make_unique<skity::PictureRecorder>()) {
  picture_recorder_->BeginRecording(bounds);
  canvas_ = picture_recorder_->GetRecordingCanvas();
}

std::unique_ptr<Picture> SkityRecorderCanvas::FinishRecordingAsPicture() {
  auto display_list = fml::MakeRefCounted<PictureSkity>(
      std::move(dynamic_ops_), picture_recorder_->FinishRecording());
  return std::make_unique<Picture>(
      GPUObject(std::move(display_list), std::move(unref_queue_)),
      has_lazy_image_);
}

}  // namespace clay
