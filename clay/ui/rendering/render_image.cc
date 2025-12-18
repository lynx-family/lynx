// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_image.h"

#include <cstdint>
#include <string>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/image/image.h"
#ifdef ENABLE_SKITY
#include "clay/gfx/image/base_image.h"
#endif

namespace clay {

RenderImage::~RenderImage() {
#ifndef ENABLE_SKITY
  if (image_resource_) {
    image_resource_->RemoveImageResourceClient(this);
  }

  if (pending_image_resource_) {
    pending_image_resource_->RemoveImageResourceClient(this);
  }

  if (placeholder_resource_) {
    placeholder_resource_->RemoveImageResourceClient(this);
  }
#endif  // ENABLE_SKITY
}

const char* RenderImage::GetName() const { return "RenderImage"; }

#ifndef ENABLE_SKITY
void RenderImage::SetImage(std::unique_ptr<ImageResource> resource) {
  bool resource_is_pending = false;

  if (resource) {
    resource->AddImageResourceClient(this);
    Image* image = resource->GetImage();

    // TODO(yudingqian): Set RepaintBoundary here would cause some overlap
    // problem. Need further research.
    // Set repaint boundary for animated image, e.g. GIF for performance.
    // if (!IsRepaintBoundary() && image && !image->IsSingleFrameImage()) {
    //   SetRepaintBoundary(true);
    // }

    if (image) {
      image->SetAutoPlay(auto_play_);
      image->SetLoopCount(loop_count_);
    }

    if (image && image->UseTextureBackend()) {
      // Check if the image has been decoded without triggerring decode
      auto graphics_image = image->TryGetGraphicsImage();
      if (graphics_image) {
        DecodeImageFinish(true, resource->GetUrl());
      } else if (!graphics_image && !image->UsePromise()) {
        resource_is_pending = true;
      }
    }
  }

  // Replacing the `image_resource_` immedidately will result in blank screen.
  // To prevent it, we memoize the new resource to `pending_image_resource_` and
  // check whether new image decoded success in `RequestRenderImage`
  if (image_resource_ && resource_is_pending) {
    pending_image_resource_ = std::move(resource);
  } else {
    image_resource_ = std::move(resource);
  }

  if (!ImageDecodeWithPriority()) {
    TryDecodeImmediately();
  }

  // For now there are 3 modes for rendering images:
  //   1. non-texture mode: Synchronously decode image data and do not upload
  //   texture to GPU.
  //   2. promised-texture mode: Asynchronously decode image data and upload
  //   texture to GPU. Before uploading, we will use promise to synchronously
  //   wait the decoded data.
  //   3. async-texture mode: Asynchronously decode image data and upload
  //   texture to GPU. This is the default mode in most cases for performance.
  //
  // In the first 2 modes, we mark dirty immediately, and the image will be
  // rendered in the next frame.
  //
  // In async-texture mode, we need to mark dirty to trigger image decoding
  // during paint
  //
  // For summary, we should always mark dirty when image changes.
  MarkNeedsPaint();

  AdjustSizeIfNeeded();

  if (image_resource_ && placeholder_resource_) {
    placeholder_resource_->RemoveImageResourceClient(this);
    placeholder_resource_.reset();
  }
}

void RenderImage::SetPlaceholderImage(
    std::unique_ptr<ImageResource> placeholder_resource) {
  if (image_resource_) {
    // Discard placeholder resource if the formal image is ready.
    return;
  }
  placeholder_resource_ = std::move(placeholder_resource);
  if (placeholder_resource_) {
    placeholder_resource_->AddImageResourceClient(this);
  }

  if (!ImageDecodeWithPriority()) {
    TryDecodeImmediately();
  }

  MarkNeedsPaint();
  AdjustSizeIfNeeded();
}
#else
void RenderImage::SetImage(std::shared_ptr<BaseImage> image) {
  if (image) {
    if (image->GetType() == ImageType::kAnimated) {
      auto animated_image = std::static_pointer_cast<AnimatedImage>(image);
      animated_image->SetAutoPlay(auto_play_);
      animated_image->SetLoopCount(loop_count_);
    }
  }

  image_resource_ = std::move(image);

  MarkNeedsPaint();

  AdjustSizeIfNeeded();

  if (image_resource_ && placeholder_resource_) {
    placeholder_resource_.reset();
  }
}

void RenderImage::SetPlaceholderImage(
    std::shared_ptr<BaseImage> placeholder_resource) {
  if (image_resource_) {
    // Discard placeholder resource if the formal image is ready.
    return;
  }
  placeholder_resource_ = std::move(placeholder_resource);
  MarkNeedsPaint();
  AdjustSizeIfNeeded();
}
#endif  // ENABLE_SKITY

void RenderImage::SetMode(FillMode mode) {
  if (mode_ == mode) {
    return;
  }

  MarkNeedsPaint();
  mode_ = mode;
}

void RenderImage::SetEffect(ImageEffect effect) {
  if (effect_ == effect) {
    return;
  }

  MarkNeedsPaint();
  effect_ = effect;
}

void RenderImage::SetRepeat(ImageRepeat repeat) {
  if (repeat_ == repeat) {
    return;
  }

  MarkNeedsPaint();
  repeat_ = repeat;
}

void RenderImage::SetDropShadow(float offset_x, float offset_y,
                                float blur_radius, const GrColor& color) {
  if (drop_shadow_offset_x_ == offset_x && drop_shadow_offset_y_ == offset_y &&
      drop_shadow_blur_radius_ == blur_radius && drop_shadow_color_ == color) {
    return;
  }

  MarkNeedsPaint();
  drop_shadow_offset_x_ = offset_x;
  drop_shadow_offset_y_ = offset_y;
  drop_shadow_blur_radius_ = blur_radius;
  drop_shadow_color_ = color;
}

void RenderImage::SetCapInsets(const std::array<Length, 4>& cap_insets) {
  if (cap_insets_ == cap_insets) {
    return;
  }

  MarkNeedsPaint();
  cap_insets_ = cap_insets;
  has_cap_insets_ = HasCapInsets();
}

void RenderImage::SetCapInsetsScale(float scale) {
  if (cap_insets_scale_ == scale) {
    return;
  }

  MarkNeedsPaint();
  cap_insets_scale_ = scale;
}

void RenderImage::SetAutoPlay(bool auto_play) {
  if (auto_play_ == auto_play) {
    return;
  }

  // No need to `MarkNeedsPaint`. Only has effect before resource loaded.
  auto_play_ = auto_play;
#ifndef ENABLE_SKITY
  if (image_resource_ && image_resource_->GetImage()) {
    image_resource_->GetImage()->SetAutoPlay(auto_play_);
  }
#else
  if (image_resource_ && image_resource_->GetType() == ImageType::kAnimated) {
    auto animated_image =
        std::static_pointer_cast<AnimatedImage>(image_resource_);
    animated_image->SetAutoPlay(auto_play_);
  }
#endif  // ENABLE_SKITY
}

void RenderImage::SetLoopCount(int loop_count) {
  if (loop_count_ == loop_count) {
    return;
  }

  MarkNeedsPaint();
  loop_count_ = loop_count;
#ifndef ENABLE_SKITY
  if (image_resource_ && image_resource_->GetImage()) {
    image_resource_->GetImage()->SetLoopCount(loop_count_);
  }
#else
  if (image_resource_ && image_resource_->GetType() == ImageType::kAnimated) {
    auto animated_image =
        std::static_pointer_cast<AnimatedImage>(image_resource_);
    animated_image->SetLoopCount(loop_count_);
  }
#endif
}

void RenderImage::SetAutoSize(bool auto_size) {
  if (auto_size_ == auto_size) {
    return;
  }

  auto_size_ = auto_size;
  AdjustSizeIfNeeded();
}

void RenderImage::SetTintColor(const Color& tint_color) {
  if (tint_color_ == tint_color) {
    return;
  }

  tint_color_ = tint_color;
  MarkNeedsPaint();
}

void RenderImage::OnNodeReady() { AdjustSizeIfNeeded(); }

void RenderImage::AdjustSizeIfNeeded() {
  auto image_resource = image_resource_.get();
  if (!image_resource) {
    image_resource = placeholder_resource_.get();
  }
  if (!image_resource) {
    return;
  }

  float bitmap_width = image_resource->GetWidth();
  float bitmap_height = image_resource->GetHeight();

  if (client_) {
    client_->AdjustSizeIfNeeded(auto_size_, bitmap_width, bitmap_height);
  }
}

void RenderImage::CreateImageData(ImageData& image_data) {
  image_data.image_resource = GetResourceForDisplay();
  image_data.mode = mode_;
  image_data.repeat = repeat_;
  image_data.has_cap_insets = has_cap_insets_;
  image_data.cap_insets = cap_insets_;
  image_data.cap_insets_scale = cap_insets_scale_;
  image_data.drop_shadow_offset_x = drop_shadow_offset_x_;
  image_data.drop_shadow_offset_y = drop_shadow_offset_y_;
  image_data.drop_shadow_color = drop_shadow_color_;
  image_data.drop_shadow_blur_radius = drop_shadow_blur_radius_;
  image_data.image_opacity = image_opacity_;
  image_data.tint_color = tint_color_;
}

bool RenderImage::IsRepaintBoundary() const {
  if (repeat_ == ImageRepeat::kNoRepeat &&
      (drop_shadow_blur_radius_ != 0.0f || drop_shadow_offset_x_ != 0.0f ||
       drop_shadow_offset_y_ != 0.0f)) {
    return true;
  }
  return RenderObject::IsRepaintBoundary();
}

void RenderImage::Paint(PaintingContext& context, const FloatPoint& offset) {
  if (!CanDisplay()) {
    return;
  }
  // Ask parent to paint background and borders first.
  RenderBox::Paint(context, offset);
  GraphicsContext* graphics_context = context.GetGraphicsContext();
  FML_DCHECK(graphics_context);

  {
    GraphicsContext::AutoRestore canvas_saver(graphics_context, true);
    graphics_context->Translate(offset.x(), offset.y());

    // radius clip
    bool has_border = HasBorder();
    if (has_border) {
      bool has_radius = Border().HasBorderRadius();
      if (has_radius) {
        const auto& borders_data = Border();
        skity::Vec2 radii[4];
        radii[skity::RRect::kUpperLeft] = {
            borders_data.radius_x_top_left_ - PaddingLeft() - BorderLeft(),
            borders_data.radius_y_top_left_ - PaddingTop() - BorderTop()};
        radii[skity::RRect::kUpperRight] = {
            borders_data.radius_x_top_right_ - PaddingRight() - BorderRight(),
            borders_data.radius_y_top_right_ - PaddingTop() - BorderTop()};
        radii[skity::RRect::kLowerRight] = {
            borders_data.radius_x_bottom_right_ - PaddingRight() -
                BorderRight(),
            borders_data.radius_y_bottom_right_ - PaddingBottom() -
                BorderBottom()};
        radii[skity::RRect::kLowerLeft] = {
            borders_data.radius_x_bottom_left_ - PaddingLeft() - BorderLeft(),
            borders_data.radius_y_bottom_left_ - PaddingBottom() -
                BorderBottom()};

        skity::Rect draw_rect = skity::Rect::MakeXYWH(
            PaddingLeft() + BorderLeft(), PaddingTop() + BorderTop(),
            ContentWidth(), ContentHeight());

        skity::RRect round_rect;
        round_rect.SetRectRadii(draw_rect, radii);
        graphics_context->ClipRRect(round_rect, GrClipOp::kIntersect, true);
      }
    }

    // Translate to content_rect where the ImagePainter is going to paint into.
    graphics_context->Translate(PaintOffset().x(), PaintOffset().y());

    // Paint image into content_rect.
    ImageData image_data;
    CreateImageData(image_data);
    ImagePainter(static_cast<RenderBox*>(this))
        .PaintImage(context.GetGraphicsContext(), image_data);

#ifndef ENABLE_SKITY
    ImageResource* resource = GetResourceForDisplay();
    // Disable raster cache for the animating image to save memory.
    if (resource && resource->GetImage() &&
        resource->GetImage()->MaybeAnimated()) {
      context.SetWillChangeHint();
    }

    // Trigger `pending_image_resource_` decode after `WillPaint`
    // which can set image size by it's UI layout
    if (pending_image_resource_) {
      auto* image = pending_image_resource_->GetImage();
      if (image && image->UseTextureBackend()) {
        image->GetGraphicsImage();
      }
    }
#endif  // ENABLE_SKITY
  }
}

void RenderImage::SetImageOpacity(float opacity) {
  if (image_opacity_ == opacity) {
    return;
  }
  MarkNeedsPaint();
  // This opacity is used to accomplish the `fadeIn` animation only.
  image_opacity_ = opacity;
}

PaintFunction RenderImage::FixupPainterIfNeeded(const PaintFunction& painter) {
  auto result = painter;
  if (effect_ == ImageEffect::kInverseColor) {
    result = [old_painter = result](PaintingContext& context,
                                    const FloatPoint& offset) {
#ifndef ENABLE_SKITY
      constexpr float mx[] = {-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  -1.0f,
                              0.0f,  1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f,
                              0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 0.0f};

      auto color_filter = ColorFilter::MakeMatrix(mx);
      context.PushColorFilter(color_filter, offset, old_painter);
#endif  // ENABLE_SKITY
    };
  }

  if (repeat_ == ImageRepeat::kNoRepeat &&
      (drop_shadow_blur_radius_ != 0.0f || drop_shadow_offset_x_ != 0.0f ||
       drop_shadow_offset_y_ != 0.0f)) {
    result = [this, old_painter = result](PaintingContext& context,
                                          const FloatPoint& offset) {
      auto filter = std::make_shared<DropShadowImageFilter>(
          drop_shadow_offset_x_, drop_shadow_offset_y_,
          drop_shadow_blur_radius_, drop_shadow_blur_radius_,
          drop_shadow_color_);
      context.PushImageFilter(filter, offset, old_painter);
    };
  }

  if (repeat_ == ImageRepeat::kNoRepeat && HasBlur()) {
    result = [this, old_painter = result](PaintingContext& ctx,
                                          const FloatPoint& offset) {
      // The ClipRect's offset is actually controlled by an `OffsetLayer`, so we
      // should just set clipRect's x and y to be 0.
      ctx.PushClipRect({0, 0, Width(), Height()}, offset, old_painter);
    };
  }
  return result;
}

void RenderImage::WillPaint() {
#ifndef ENABLE_SKITY
  // Check if any transform expansion would be applied to this image.
  // If there is an expansion operation, we will force the image to use the
  // original size for decoding to avoid blurring issues.
  // This is not very accurate, cause the size when enlarged may still be
  // smaller than the original size. And this will lead to some memory
  // waste. Maybe need further research.
  bool force_use_original_size = !down_sampling_ || HasTransformExpansion();
  for (auto* resource : {image_resource_.get(), pending_image_resource_.get(),
                         placeholder_resource_.get()}) {
    if (resource && resource->GetImage()) {
      FloatRect content_rect = ContentRect();
      skity::Vec2 output_size =
          skity::Vec2(content_rect.width(), content_rect.height());
      if (!resource->GetImage()->IsSVG()) {
        resource->GetImage()->SetExpectSizeCalculator(
            [has_cap_insets = has_cap_insets_, mode = mode_, output_size,
             pixel_ratio =
                 GetRenderer()
                     ->GetPixelRatio<kPixelTypeClay, kPixelTypePhysical>()](
                skity::Vec2 size) {
              if (has_cap_insets) {
                return size;
              }
              skity::Vec2 render_size = ImagePainter::CalculateSize(
                  mode, skity::Vec2(size.x, size.y), output_size);
              // We should use the size in physical pixels.
              render_size.y *= pixel_ratio;
              render_size.x *= pixel_ratio;
              return render_size;
            },
            force_use_original_size);
      } else {
        resource->GetImage()->SetExpectSizeCalculator(
            [output_size,
             pixel_ratio =
                 GetRenderer()
                     ->GetPixelRatio<kPixelTypeClay, kPixelTypePhysical>()](
                skity::Vec2 size) {
              // There is no `mode` or `cap_insets` provided for SVG image.
              // Just consider the output_size as render_size.
              skity::Vec2 render_size = output_size;
              // We should use the size in physical pixels.
              render_size.y *= pixel_ratio;
              render_size.x *= pixel_ratio;
              return render_size;
            },
            force_use_original_size);
      }
    }
  }
#endif  // ENABLE_SKITY
  RenderObject::WillPaint();
}

void RenderImage::RequestRenderImage(ImageResource* image_resource,
                                     bool success) {
#ifndef ENABLE_SKITY
  if (success) {
    MarkNeedsPaint();
    if (image_resource == pending_image_resource_.get()) {
      image_resource_ = std::move(pending_image_resource_);
    }
  }
#endif  // ENABLE_SKITY
}

void RenderImage::DecodeImageFinish(bool success, const std::string& url) {
  if (client_) {
    client_->OnDecodeFinished(success, url);
  }
}

void RenderImage::RegisterUploadTask(OneShotCallback<>&& task, int image_id) {
  if (client_) {
    client_->RegisterUploadTask(std::move(task), image_id);
  }
}

void RenderImage::OnImageChanged() {
  if (!IsActualVisible()) {
    return;
  }

  MarkNeedsPaint();
}

bool RenderImage::HasCapInsets() const {
  for (const auto& value : cap_insets_) {
    if (value != Length()) {
      return true;
    }
  }
  return false;
}

void RenderImage::StartAnimate() {
#ifndef ENABLE_SKITY
  if (image_resource_) {
    Image* image = image_resource_->GetImage();
    if (image) {
      image->StartAnimate();
      MarkNeedsPaint();
    }
  }
#else
  if (image_resource_ && image_resource_->GetType() == ImageType::kAnimated) {
    auto animated_image =
        std::static_pointer_cast<AnimatedImage>(image_resource_);
    animated_image->StartAnimate();
  }
#endif  // ENABLE_SKITY
}

void RenderImage::StopAnimation() {
#ifndef ENABLE_SKITY
  if (image_resource_) {
    Image* image = image_resource_->GetImage();
    if (image) {
      image->StopAnimation();
      MarkNeedsPaint();
    }
  }
#else
  if (image_resource_ && image_resource_->GetType() == ImageType::kAnimated) {
    auto animated_image =
        std::static_pointer_cast<AnimatedImage>(image_resource_);
    animated_image->StopAnimation();
  }
#endif
}

void RenderImage::PauseAnimation() {
#ifndef ENABLE_SKITY
  if (image_resource_) {
    Image* image = image_resource_->GetImage();
    if (image) {
      image->PauseAnimation();
      MarkNeedsPaint();
    }
  }
#else
  if (image_resource_ && image_resource_->GetType() == ImageType::kAnimated) {
    auto animated_image =
        std::static_pointer_cast<AnimatedImage>(image_resource_);
    animated_image->PauseAnimation();
  }
#endif  // ENABLE_SKITY
}

void RenderImage::ResumeAnimation() {
#ifndef ENABLE_SKITY
  if (image_resource_) {
    Image* image = image_resource_->GetImage();
    if (image) {
      image->ResumeAnimation();
      MarkNeedsPaint();
    }
  }
#else
  if (image_resource_ && image_resource_->GetType() == ImageType::kAnimated) {
    auto animated_image =
        std::static_pointer_cast<AnimatedImage>(image_resource_);
    animated_image->ResumeAnimation();
  }
#endif
}

void RenderImage::OnStartPlay() {
  if (client_) {
    client_->OnStartPlay();
  }
}

void RenderImage::OnCurrentLoopComplete() {
  if (client_) {
    client_->OnCurrentLoopComplete();
  }
}

void RenderImage::OnFinalLoopComplete() {
  if (client_) {
    client_->OnFinalLoopComplete();
  }
}

void RenderImage::TryDecodeImmediately() {
  if (!need_decode_immediately) {
    return;
  }
#ifndef ENABLE_SKITY
  DecodePriority priority = DecodeUtils::GetDecodePriority(this);
  if (image_resource_) {
    image_resource_->GetImage()->GetGraphicsImage(priority);
  }
  if (pending_image_resource_) {
    pending_image_resource_->GetImage()->GetGraphicsImage(priority);
  }
  if (placeholder_resource_) {
    placeholder_resource_->GetImage()->GetGraphicsImage(priority);
  }
#else
  if (image_resource_) {
    image_resource_->GetGraphicsImage();
  }
  if (pending_image_resource_) {
    pending_image_resource_->GetGraphicsImage();
  }
  if (placeholder_resource_) {
    placeholder_resource_->GetGraphicsImage();
  }
#endif  // ENABLE_SKITY
}

// This function is called before scroll actions to decode images
// for those which will be visible after scroll.
void RenderImage::DecodeImages() {
#ifndef ENABLE_SKITY
  RenderObject::DecodeImages();

  if (image_resource_ && image_resource_->GetImage()->NeedDecode()) {
    DecodePriority priority = DecodeUtils::GetDecodePriority(this);
    image_resource_->GetImage()->GetGraphicsImage(priority);
  }
  if (pending_image_resource_ &&
      pending_image_resource_->GetImage()->NeedDecode()) {
    DecodePriority priority = DecodeUtils::GetDecodePriority(this);
    pending_image_resource_->GetImage()->GetGraphicsImage(priority);
  }
  if (placeholder_resource_ &&
      placeholder_resource_->GetImage()->NeedDecode()) {
    DecodePriority priority = DecodeUtils::GetDecodePriority(this);
    placeholder_resource_->GetImage()->GetGraphicsImage(priority);
  }
#endif  // ENABLE_SKITY
}

#ifndef NDEBUG
std::string RenderImage::ToString() const {
  std::stringstream ss;
  ss << RenderBox::ToString();
  if (image_resource_) {
#ifndef ENABLE_SKITY
    ss << " image_src=" << image_resource_->GetUrl();
#endif  // ENABLE_SKITY
    ss << " image_size=(" << image_resource_->GetWidth() << ","
       << image_resource_->GetHeight() << ")";
  }
  if (placeholder_resource_) {
#ifndef ENABLE_SKITY
    ss << " placeholder_src=" << placeholder_resource_->GetUrl();
#endif  // ENABLE_SKITY
    ss << " placeholder_size=(" << placeholder_resource_->GetWidth() << ","
       << placeholder_resource_->GetHeight() << ")";
  }
  return ss.str();
}
#endif

}  // namespace clay
