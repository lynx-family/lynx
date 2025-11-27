// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/animated_image.h"

#include "clay/gfx/graphics_context.h"
#if (defined(OS_MAC) || defined(OS_WIN))
#include "clay/shell/platform/common/codec/desktop_image.h"
#endif

namespace clay {

std::shared_ptr<AnimatedImage> AnimatedImage::Make(
    std::shared_ptr<PlatformImage> platform_image) {
  auto image = std::shared_ptr<AnimatedImage>(new AnimatedImage);
  image->type_ = ImageType::kAnimated;
  image->image_ = platform_image;
  return image;
}

void AnimatedImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
#if (defined(OS_MAC) || defined(OS_WIN))
  auto skity_pixmap =
      static_cast<DesktopImage*>(image_.get())->GetCurrentPixmap();
  if (!skity_pixmap) {
    return;
  }
  auto image = skity::Image::MakeDeferredTextureImage(
      skity::Texture::FormatFromColorType(skity_pixmap->GetColorType()),
      skity_pixmap->Width(), skity_pixmap->Height(),
      skity_pixmap->GetAlphaType());
  gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
  unref_queue->GetTaskRunner()->PostTask(
      [context = unref_queue->GetContext(), image, skity_pixmap]() {
        auto texture = context->CreateTexture(
            skity::Texture::FormatFromColorType(skity_pixmap->GetColorType()),
            skity_pixmap->Width(), skity_pixmap->Height(),
            skity_pixmap->GetAlphaType());
        texture->DeferredUploadImage(std::move(skity_pixmap));
        image->SetTexture(texture);
      });
#endif
}

fml::RefPtr<SharedImageSink> AnimatedImage::GetSharedImageSink() {
  if (!shared_image_) {
    shared_image_ = image_->ToSharedImage();
  }
  return shared_image_;
}

bool AnimatedImage::DoAnimationFrame(int64_t frame_time,
                                     std::function<void()> on_frame_changed) {
  image_->DrawFrame(frame_time, std::move(on_frame_changed));
  return true;
}

void AnimatedImage::SetAutoPlay(bool auto_play) {
  image_->SetAutoPlay(auto_play);
}
void AnimatedImage::SetLoopCount(int loop_count) {
  image_->SetLoopCount(loop_count);
}
void AnimatedImage::StartAnimate() { image_->StartAnimation(); }
void AnimatedImage::StopAnimation() { image_->StopAnimation(); }
void AnimatedImage::PauseAnimation() { image_->PauseAnimation(); }
void AnimatedImage::ResumeAnimation() { image_->ResumeAnimation(); }

}  // namespace clay
