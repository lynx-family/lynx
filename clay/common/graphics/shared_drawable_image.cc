// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/shared_drawable_image.h"

#include <algorithm>

#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink_accessor.h"

namespace clay {

SharedDrawableImage::SharedDrawableImage(
    fml::RefPtr<SharedImageSink> image_sink)
    : image_sink_(image_sink) {}

SharedDrawableImage::~SharedDrawableImage() = default;

// |clay::DrawableImage|
// Called on platform thread.
void SharedDrawableImage::SetFrameAvailableCallback(
    const fml::closure& callback) {
  image_sink_->SetFrameAvailableCallback(callback);
}

// |clay::raster|
// Called on raster thread.
void SharedDrawableImage::MarkNewFrameAvailable() { frame_produce_cnt_++; }

// |clay::raster|
// Called on raster thread.
void SharedDrawableImage::OnDrawableImageUnregistered() {
  image_sink_->SetFrameAvailableCallback(nullptr);
}

// |clay::ContextListener|
// Called from raster thread.
void SharedDrawableImage::OnGrContextCreated() {
  state_ = AttachmentState::uninitialized;
}

// |clay::ContextListener|
// Called from raster thread.
void SharedDrawableImage::OnGrContextDestroyed() {
  state_ = AttachmentState::detached;
  accessor_ = nullptr;
}

// Called from raster thread.
bool SharedDrawableImage::EnsureAttached() {
  if (state_ == AttachmentState::detached) {
    return false;
  }

  if (state_ == AttachmentState::uninitialized) {
    accessor_ = std::make_unique<SharedImageSinkAccessor>(
        image_sink_,
        [this](fml::RefPtr<SharedImageBacking> backing)
            -> fml::RefPtr<SharedImageRepresentation> {
#ifndef ENABLE_SKITY
          return backing->CreateSkiaRepresentation(GetGrContext());
#else
          return backing->CreateSkityRepresentation(GetGrContext());
#endif  // ENABLE_SKITY
        });
    state_ = AttachmentState::attached;
  }
  return true;
}

// Called from raster thread.
void SharedDrawableImage::AdvanceFrameConsumption(bool freeze) {
  if (image_sink_->Capacity() == 1) {
    [[maybe_unused]] bool update_success = Update();
  } else if (!freeze) {
    // TODO(youfeng) request a raster frame instead drain all images
    while (frame_consume_cnt_ < frame_produce_cnt_) {
      if (!Update()) {
        break;
      }
      frame_consume_cnt_++;
    }
  }
}

// Called from raster thread.
bool SharedDrawableImage::Update() {
  fml::RefPtr<SharedImageRepresentation> repr = accessor_->UpdateFront();
  if (!repr) {
    return false;
  }

#ifndef ENABLE_SKITY
  FML_DCHECK(repr->GetType() == ImageRepresentationType::kSkia);
  sk_image_ = static_cast<SkiaImageRepresentation*>(repr.get())->GetSkImage();
  if (!sk_image_) {
    return false;
  }
#else
  FML_DCHECK(repr->GetType() == ImageRepresentationType::kSkity);
  skity_image_ =
      static_cast<SkityImageRepresentation*>(repr.get())->GetSkityImage();
  if (!skity_image_) {
    return false;
  }
#endif  // ENABLE_SKITY
  transform_ = repr->GetBacking()->GetTransformation();

  return true;
}

#ifndef ENABLE_SKITY
// Called from raster thread.
void SharedDrawableImage::DrawSkiaImage(sk_sp<SkImage> sk_image,
                                        PaintContext& context,
                                        const skity::Rect& bounds,
                                        const GrSamplingOptions& sampling,
                                        FitMode fit_mode) {
  SkAutoCanvasRestore autoRestore(context.canvas, true);
  context.canvas->translate(bounds.X(), bounds.Y());
  SkRect dst_rect;
  switch (fit_mode) {
    case clay::DrawableImage::FitMode::kClipToBounds:
      dst_rect = SkRect::MakeWH(
          std::min(static_cast<int>(bounds.Width()), sk_image->width()),
          std::min(static_cast<int>(bounds.Height()), sk_image->height()));
      break;
    case clay::DrawableImage::FitMode::kScaleToFill:
      dst_rect = SkRect::MakeWH(bounds.Width(), bounds.Height());
      break;
  }

  if (!transform_.IsIdentity()) {
    SkMatrix scaled_transform;
    switch (fit_mode) {
      case clay::DrawableImage::FitMode::kScaleToFill:
        scaled_transform =
            SkMatrix::Scale(dst_rect.width(), dst_rect.height()) *
            ConvertSkityMatrixToSkMatrix(transform_) *
            SkMatrix::Scale(1.0 / sk_image->width(), 1.0 / sk_image->height());
        break;
      case clay::DrawableImage::FitMode::kClipToBounds:
        scaled_transform =
            SkMatrix::Scale(sk_image->width(), sk_image->height()) *
            ConvertSkityMatrixToSkMatrix(transform_) *
            SkMatrix::Scale(1.0 / sk_image->width(), 1.0 / sk_image->height());
        break;
    }
    sk_sp<SkShader> shader = sk_image->makeShader(
        SkTileMode::kRepeat, SkTileMode::kRepeat, sampling, scaled_transform);

    SkPaint paint_with_shader;
    if (context.sk_paint) {
      paint_with_shader = *context.sk_paint;
    }
    paint_with_shader.setShader(shader);
    context.canvas->drawRect(dst_rect, paint_with_shader);
  } else {
    SkRect image_src_rect;
    switch (fit_mode) {
      case clay::DrawableImage::FitMode::kClipToBounds:
        image_src_rect = dst_rect;
        break;
      case clay::DrawableImage::FitMode::kScaleToFill:
        image_src_rect = SkRect::MakeWH(sk_image->width(), sk_image->height());
        break;
    }
    context.canvas->drawImageRect(
        sk_image_, image_src_rect, dst_rect, sampling, context.sk_paint,
        SkCanvas::SrcRectConstraint::kStrict_SrcRectConstraint);
  }
}
#else
// Called from raster thread.
void SharedDrawableImage::DrawSkityImage(
    std::shared_ptr<skity::Image> skity_image, PaintContext& context,
    const skity::Rect& bounds, const GrSamplingOptions& sampling,
    FitMode fit_mode) {
  context.canvas->Save();
  context.canvas->Translate(bounds.X(), bounds.Y());
  skity::Rect dst_rect;
  switch (fit_mode) {
    case clay::DrawableImage::FitMode::kClipToBounds:
      dst_rect = skity::Rect::MakeWH(
          std::min(static_cast<size_t>(bounds.Width()), skity_image->Width()),
          std::min(static_cast<size_t>(bounds.Height()),
                   skity_image->Height()));
      break;
    case clay::DrawableImage::FitMode::kScaleToFill:
      dst_rect = skity::Rect::MakeWH(bounds.Width(), bounds.Height());
      break;
  }

  // We have specified the BottomLeft as the origin of the surface when create
  // SkImage. But Skity doesn't provide this parameter. So flip the y-axis
  // again.
  skity::Matrix transform = skity::Matrix::Translate(0.f, 1.f) *
                            skity::Matrix::Scale(1.f, -1.f) * transform_;

  skity::Matrix scaled_transform;
  switch (fit_mode) {
    case clay::DrawableImage::FitMode::kScaleToFill:
      scaled_transform =
          skity::Matrix::Scale(dst_rect.Width(), dst_rect.Height()) *
          transform *
          skity::Matrix::Scale(1.0 / skity_image->Width(),
                               1.0 / skity_image->Height());
      break;
    case clay::DrawableImage::FitMode::kClipToBounds:
      scaled_transform =
          skity::Matrix::Scale(skity_image->Width(), skity_image->Height()) *
          transform *
          skity::Matrix::Scale(1.0 / skity_image->Width(),
                               1.0 / skity_image->Height());
      break;
  }
  std::shared_ptr<skity::Shader> shader =
      skity::Shader::MakeShader(skity_image, sampling, skity::TileMode::kRepeat,
                                skity::TileMode::kRepeat, scaled_transform);

  skity::Paint paint_with_shader;
  if (context.sk_paint) {
    paint_with_shader = *context.sk_paint;
  }
  paint_with_shader.SetShader(shader);
  context.canvas->DrawRect(dst_rect, paint_with_shader);
  context.canvas->Restore();
}
#endif  // ENABLE_SKITY

// Called from raster thread.
void SharedDrawableImage::FlushAndReleaseFrontForSingleBuffer() {
  if (image_sink_->Capacity() == 1) {
#ifndef ENABLE_SKITY
    // Flush sk_image to make sure the shared image can be transferred to the
    // producer.
    // This is not required in multi-buffer mode.
    // Since the previous sk_image_ is not paint in this frame and already
    // released.
    if (GetGrContext()) {
      GetGrContext()->flush(sk_image_);
    }
    accessor_->ReleaseFront();
    sk_image_ = nullptr;
#else
    // Flush sk_image to make sure the shared image can be transferred to the
    // producer.
    // This is not required in multi-buffer mode.
    // Since the previous skity_image_ is not paint in this frame and already
    // released.
    // TODO(yudingqian): Skity doesn't support this API.
    // if (GetGrContext()) {
    //   GetGrContext()->flush(skity_image_);
    // }
    accessor_->ReleaseFront();
    skity_image_ = nullptr;
#endif
  }
}

}  // namespace clay
