// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/animated_image.h"

#include <utility>

#include "clay/gfx/graphics_context.h"

namespace clay {

std::shared_ptr<AnimatedImage> AnimatedImage::Make(
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<PlatformImage> platform_image) {
  auto image = std::shared_ptr<AnimatedImage>(new AnimatedImage);
  image->type_ = ImageType::kAnimated;
  image->image_ = platform_image;
  image->frame_timer_ = std::make_unique<fml::OneshotTimer>(task_runner);
  return image;
}

void AnimatedImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
  auto pixmap = image_->ToBitmap();
  if (!pixmap) {
    return;
  }
  auto image = skity::Image::MakeDeferredTextureImage(
      skity::Texture::FormatFromColorType(pixmap->GetColorType()),
      pixmap->Width(), pixmap->Height(), pixmap->GetAlphaType());
  gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
  unref_queue->GetTaskRunner()->PostTask([context = unref_queue->GetContext(),
                                          image, pixmap,
                                          weak = weak_from_this()]() {
    if (auto self = weak.lock()) {
      auto texture = context->CreateTexture(
          skity::Texture::FormatFromColorType(pixmap->GetColorType()),
          pixmap->Width(), pixmap->Height(), pixmap->GetAlphaType());
      if (texture) {
        texture->DeferredUploadImage(std::move(pixmap));
        image->SetTexture(texture);
      }
    }
  });
  NextFrame();
}

void AnimatedImage::SetAnimationFrameCallback(std::function<void()> func) {
  animation_frame_callback_ = std::move(func);
}

void AnimatedImage::NextFrame() {
  frame_timer_->Start(
      fml::TimeDelta::FromMilliseconds(image_->GetDuration()),
      [weak = weak_from_this()] {
        if (auto self = weak.lock()) {
          auto animated_image = static_cast<AnimatedImage*>(self.get());
          animated_image->image_->DrawFrame(
              fml::TimePoint::Now().ToEpochDelta().ToMilliseconds(),
              animated_image->animation_frame_callback_);
        }
      });
}

void AnimatedImage::SetAutoPlay(bool auto_play) {
  image_->SetAutoPlay(auto_play);
}
void AnimatedImage::SetLoopCount(int loop_count) {
  image_->SetLoopCount(loop_count);
}
void AnimatedImage::StartAnimate() {
  StopAnimation();
  image_->StartAnimation();
  if (animation_frame_callback_) {
    animation_frame_callback_();
  }
}
void AnimatedImage::StopAnimation() { image_->StopAnimation(); }
void AnimatedImage::PauseAnimation() { image_->PauseAnimation(); }
void AnimatedImage::ResumeAnimation() {
  image_->ResumeAnimation();
  if (animation_frame_callback_) {
    animation_frame_callback_();
  }
}

}  // namespace clay
