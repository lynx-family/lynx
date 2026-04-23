// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/animated_image.h"

#include <utility>

#include "clay/gfx/graphics_context.h"

namespace clay {

std::shared_ptr<AnimatedImage> AnimatedImage::Make(
    fml::WeakPtr<ImageFetcher> image_fetcher, std::string url,
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<PlatformImage> platform_image) {
  auto image = std::shared_ptr<AnimatedImage>(new AnimatedImage);
  image->type_ = ImageType::kAnimated;
  image->image_fetcher_ = image_fetcher;
  image->url_ = std::move(url);
  image->image_ = platform_image;
  image->frame_timer_ = std::make_unique<fml::OneshotTimer>(task_runner);
  image->orig_info_ = ImageInfo::makeWH(platform_image->GetWidth(),
                                        platform_image->GetHeight());
  return image;
}

void AnimatedImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
  if (!unref_queue || !unref_queue->GetContext()) {
    FML_LOG(ERROR) << "AnimatedImage::Upload: unref_queue or context is null";
    return;
  }
  if (!is_running_) {
    OnNotifyAnimationFrame();
  }
  if (!gpu_image_.object()) {
    auto pixmap = image_->ToBitmap();
    if (!pixmap) {
      return;
    }
    auto image = skity::Image::MakeDeferredTextureImage(
        skity::Texture::FormatFromColorType(pixmap->GetColorType()),
        pixmap->Width(), pixmap->Height(), pixmap->GetAlphaType());
    gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
    unref_queue->GetTaskRunner()->PostTask(
        [context = unref_queue->GetContext(), image, pixmap,
         mipmapped = mipmapped_, weak = weak_from_this()]() {
          if (auto self = weak.lock()) {
            skity::TextureDescriptor desc{};
            desc.format =
                skity::Texture::FormatFromColorType(pixmap->GetColorType());
            desc.width = pixmap->Width();
            desc.height = pixmap->Height();
            desc.alpha_type = pixmap->GetAlphaType();
            desc.mipmapped = mipmapped;
            auto texture = context->CreateTextureWithDesc(&desc);
            if (texture) {
              texture->DeferredUploadImage(std::move(pixmap));
              image->SetTexture(texture);
            }
          }
        });
  }
}

void AnimatedImage::NextFrame() {
  if (image_->GetDuration() <= 0) {
    return;
  }
  frame_timer_->Start(fml::TimeDelta::FromMilliseconds(image_->GetDuration()),
                      [weak = weak_from_this()] {
                        if (auto self = weak.lock()) {
                          auto animated_image =
                              static_cast<AnimatedImage*>(self.get());
                          animated_image->image_->DrawFrame([animated_image] {
                            animated_image->OnNotifyAnimationFrame();
                          });
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
  OnNotifyAnimationFrame();
}
void AnimatedImage::StopAnimation() { image_->StopAnimation(); }
void AnimatedImage::PauseAnimation() { image_->PauseAnimation(); }
void AnimatedImage::ResumeAnimation() {
  image_->ResumeAnimation();
  OnNotifyAnimationFrame();
}

void AnimatedImage::OnNotifyAnimationFrame() {
  if (!IsAnyInstanceVisible()) {
    is_running_ = false;
    return;
  }
  for (auto& instance : instances_) {
    instance->OnNotifyAnimationFrame();
  }
  NextFrame();
  gpu_image_.reset();
  is_running_ = true;
}

void AnimatedImage::OnInstanceCreated(BaseImageInstance* instance) {
  BaseImage::OnInstanceCreated(instance);
  if (instances_.size() <= 1) {
    StartAnimate();
  }
}

void AnimatedImage::OnInstanceDestroyed(BaseImageInstance* instance) {
  BaseImage::OnInstanceDestroyed(instance);
  if (instances_.empty()) {
    StopAnimation();
    frame_timer_->Stop();
  }
}

bool AnimatedImage::IsAnyInstanceVisible() const {
  if (instances_.empty()) {
    return false;
  }
  for (auto& instance : instances_) {
    if (instance->IsVisible()) {
      return true;
    }
  }
  return false;
}

}  // namespace clay
