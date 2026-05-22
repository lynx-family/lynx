// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/animated_image.h"

#include "clay/gfx/graphics_context.h"

namespace clay {

std::shared_ptr<AnimatedImage> AnimatedImage::Make(
    std::shared_ptr<PlatformImage> platform_image) {
  auto image = std::shared_ptr<AnimatedImage>(new AnimatedImage);
  image->type_ = ImageType::kAnimated;
  image->image_ = platform_image;
  return image;
}

void AnimatedImage::Upload(GraphicsContext* context, Size size) {}

fml::RefPtr<SharedImageSink> AnimatedImage::GetSharedImageSink() {
  if (!shared_image_) {
    shared_image_ = image_->ToSharedImage();
  }
  return shared_image_;
}

bool AnimatedImage::DoAnimationFrame(int64_t frame_time) {
  image_->DrawFrame();
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
