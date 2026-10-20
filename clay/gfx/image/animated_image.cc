// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/animated_image.h"

#include <utility>
#include <vector>

#include "clay/gfx/graphics_context.h"
#include "clay/gfx/image/image_resource_client.h"

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
  if (is_playing_ && !is_running_) {
    OnNotifyAnimationFrame();
  }
  if (!gpu_image_.object()) {
    auto pixmap = image_->ToBitmap();
    if (!pixmap) {
      return;
    }
    bool need_mipmapped = IsMipmapped() && HasResized();
    auto image = skity::Image::MakeDeferredTextureImage(
        skity::Texture::FormatFromColorType(pixmap->GetColorType()),
        pixmap->Width(), pixmap->Height(), pixmap->GetAlphaType());
    gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
    unref_queue->GetTaskRunner()->PostTask(
        [context = unref_queue->GetContext(), image, pixmap,
         mipmapped = need_mipmapped, weak = weak_from_this()]() {
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
  if (!is_playing_ || image_->GetDuration() <= 0) {
    return;
  }
  frame_timer_->Start(
      fml::TimeDelta::FromMilliseconds(image_->GetDuration()),
      [weak = weak_from_this()] {
        if (auto self = weak.lock()) {
          auto* image = static_cast<AnimatedImage*>(self.get());
          auto result = image->image_->DrawFrame();
          if (result == PlatformImage::FrameResult::kNoFrame) {
            image->is_running_ = false;
            image->is_playing_ = false;
            return;
          }
          if (result == PlatformImage::FrameResult::kFinalLoopComplete) {
            image->is_playing_ = false;
          }
          // Display the final frame before reporting completion.
          image->OnNotifyAnimationFrame();
          if (result == PlatformImage::FrameResult::kLoopComplete ||
              result == PlatformImage::FrameResult::kFinalLoopComplete) {
            image->NotifyAnimationEvent(
                &ImageResourceClient::OnCurrentLoopComplete);
          }
          if (result == PlatformImage::FrameResult::kFinalLoopComplete) {
            image->NotifyAnimationEvent(
                &ImageResourceClient::OnFinalLoopComplete);
          }
        }
      });
}

void AnimatedImage::SetAutoPlay(bool auto_play) {
  if (auto_play == is_playing_) {
    return;
  }
  if (auto_play) {
    StartAnimate();
  } else {
    StopAnimation();
  }
}
void AnimatedImage::SetLoopCount(int loop_count) {
  image_->SetLoopCount(loop_count);
}
void AnimatedImage::StartAnimate() {
  auto keep_alive = shared_from_this();
  StopAnimation();
  image_->StartAnimation();
  is_playing_ = true;
  NotifyAnimationEvent(&ImageResourceClient::OnStartPlay);
  if (is_playing_) {
    OnNotifyAnimationFrame();
  }
}
void AnimatedImage::StopAnimation() {
  image_->StopAnimation();
  frame_timer_->Stop();
  is_running_ = false;
  is_playing_ = false;
}
void AnimatedImage::PauseAnimation() {
  image_->PauseAnimation();
  frame_timer_->Stop();
  is_running_ = false;
  is_playing_ = false;
}
void AnimatedImage::ResumeAnimation() {
  if (is_playing_) {
    return;
  }
  auto keep_alive = shared_from_this();
  image_->ResumeAnimation();
  is_playing_ = true;
  NotifyAnimationEvent(&ImageResourceClient::OnStartPlay);
  if (is_playing_) {
    OnNotifyAnimationFrame();
  }
}

void AnimatedImage::NotifyAnimationEvent(
    void (ImageResourceClient::*callback)()) {
  std::vector<fml::WeakPtr<BaseImageInstance>> instances;
  for (auto* instance : instances_) {
    instances.push_back(instance->GetWeakPtr());
  }
  for (auto& instance : instances) {
    // An earlier callback may remove another view or replace its listener.
    if (instance && instance->GetAnimationListener()) {
      (instance->GetAnimationListener()->*callback)();
    }
  }
}

void AnimatedImage::OnNotifyAnimationFrame() {
  gpu_image_.reset();
  if (!IsAnyInstanceVisible()) {
    is_running_ = false;
    return;
  }
  for (auto& instance : instances_) {
    instance->OnNotifyAnimationFrame();
  }
  NextFrame();
  is_running_ = is_playing_;
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
