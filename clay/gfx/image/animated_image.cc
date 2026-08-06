// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/animated_image.h"

#include <utility>

#include "clay/fml/logging.h"

namespace clay {

std::shared_ptr<AnimatedImage> AnimatedImage::Make(
    fml::WeakPtr<ImageFetcher> image_fetcher, std::string url,
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<PlatformImage> platform_image) {
  auto image = std::shared_ptr<AnimatedImage>(new AnimatedImage);
  image->type_ = ImageType::kAnimated;
  image->image_fetcher_ = image_fetcher;
  image->url_ = std::move(url);
  image->image_ = std::move(platform_image);
  image->task_runner_ = std::move(task_runner);
  image->orig_info_ =
      ImageInfo::makeWH(image->image_->GetWidth(), image->image_->GetHeight());
  return image;
}

std::unique_ptr<BaseImageInstance> AnimatedImage::NewInstance() {
  return std::make_unique<AnimatedImageInstance>(
      std::static_pointer_cast<AnimatedImage>(shared_from_this()));
}

size_t AnimatedImage::GetGraphicsImageAllocSize() const {
  if (orig_info_.width() <= 0 || orig_info_.height() <= 0) {
    return 0;
  }
  // The current-frame texture belongs to AnimatedImageInstance. Use a
  // first-frame estimate here so the shared PlatformImage remains cacheable
  // after its last instance is released.
  return static_cast<size_t>(orig_info_.width()) *
         static_cast<size_t>(orig_info_.height()) * 4;
}

void AnimatedImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
  (void)unref_queue;
  (void)size;
  FML_DLOG(ERROR)
      << "AnimatedImage::Upload must be called through AnimatedImageInstance";
}

AnimatedImageInstance::AnimatedImageInstance(
    std::shared_ptr<AnimatedImage> image)
    : BaseImageInstance(image) {
  player_ = std::make_unique<AnimatedImagePlayer>(
      image->image_->CreateAnimation(), image->task_runner_,
      [this] { OnFrameChanged(); }, [this] { return IsVisible(); });
  if (!player_->IsValid()) {
    FML_LOG(ERROR) << "AnimatedImageInstance: failed to create animation";
    return;
  }
  player_->SetLoopCount(loop_count_);
  player_->SetAutoPlay(auto_play_);
}

AnimatedImageInstance::AnimatedImageInstance(const AnimatedImageInstance& other)
    : BaseImageInstance(other),
      auto_play_(other.auto_play_),
      loop_count_(other.loop_count_) {
  auto image = std::static_pointer_cast<AnimatedImage>(image_);
  player_ = std::make_unique<AnimatedImagePlayer>(
      image->image_->CreateAnimation(), image->task_runner_,
      [this] { OnFrameChanged(); }, [this] { return IsVisible(); });
  if (!player_->IsValid()) {
    FML_LOG(ERROR) << "AnimatedImageInstance: failed to clone animation";
    return;
  }
  player_->SetLoopCount(loop_count_);
  player_->SetAutoPlay(auto_play_);
}

AnimatedImageInstance::~AnimatedImageInstance() { player_.reset(); }

std::unique_ptr<BaseImageInstance> AnimatedImageInstance::Clone() const {
  return std::make_unique<AnimatedImageInstance>(*this);
}

size_t AnimatedImageInstance::GetGraphicsImageAllocSize() const {
  return gpu_image_.object()
             ? gpu_image_.object()->width() * gpu_image_.object()->height() * 4
             : 0;
}

fml::RefPtr<GraphicsImage> AnimatedImageInstance::GetGraphicsImage() const {
  return gpu_image_.object();
}

void AnimatedImageInstance::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue,
                                   Size size) const {
  (void)size;
  if (!unref_queue || !unref_queue->GetContext()) {
    FML_LOG(ERROR)
        << "AnimatedImageInstance::Upload: unref_queue or context is null";
    return;
  }
  if (!player_ || !player_->IsValid()) {
    return;
  }
  player_->EnsureAnimationScheduled();
  auto image_resource = std::static_pointer_cast<AnimatedImage>(image_);
  ImageInfo render_info = image_resource->render_info_;
  if (render_info.isEmpty()) {
    render_info = image_resource->orig_info_;
  }
  if (gpu_image_.object() && uploaded_info_ == render_info) {
    return;
  }
  gpu_image_.reset();
  bool need_mipmapped =
      image_resource->IsMipmapped() && image_resource->HasResized();
  auto image = skity::Image::MakeDeferredTextureImage(
      skity::Texture::FormatFromColorType(
          image_resource->image_->GetColorType()),
      render_info.width(), render_info.height(),
      image_resource->image_->GetAlphaType());
  if (!image) {
    return;
  }
  gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
  uploaded_info_ = render_info;
  std::weak_ptr<int> weak_lifetime = lifetime_;
  auto animation = player_->GetAnimation();
  unref_queue->GetTaskRunner()->PostTask(
      [context = unref_queue->GetContext(), image, render_info,
       mipmapped = need_mipmapped, weak_lifetime,
       animation = std::move(animation)]() {
        if (weak_lifetime.expired()) {
          return;
        }
        auto pixmap = animation->ToBitmap(render_info);
        if (!pixmap) {
          FML_LOG(ERROR) << "AnimatedImageInstance::Upload: Bitmap is null";
          return;
        }
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
      });
}

void AnimatedImageInstance::SetAutoPlay(bool auto_play) {
  auto_play_ = auto_play;
  if (player_) {
    player_->SetAutoPlay(auto_play_);
  }
}

void AnimatedImageInstance::SetLoopCount(int loop_count) {
  loop_count_ = loop_count;
  if (player_) {
    player_->SetLoopCount(loop_count_);
  }
}

void AnimatedImageInstance::StartAnimate() {
  if (player_) {
    player_->StartAnimation();
  }
}

void AnimatedImageInstance::StopAnimation() {
  if (player_) {
    player_->StopAnimation();
  }
}

void AnimatedImageInstance::PauseAnimation() {
  if (player_) {
    player_->PauseAnimation();
  }
}

void AnimatedImageInstance::ResumeAnimation() {
  if (player_) {
    player_->ResumeAnimation();
  }
}

void AnimatedImageInstance::OnFrameChanged() {
  gpu_image_.reset();
  uploaded_info_ = ImageInfo();
  BaseImageInstance::OnNotifyAnimationFrame();
}

}  // namespace clay
