// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/base_image_instance.h"

#include "clay/gfx/image/base_image.h"

namespace clay {

BaseImageInstance::BaseImageInstance(std::shared_ptr<BaseImage> image)
    : image_(image) {
  image_->OnInstanceCreated(this);
}

BaseImageInstance::BaseImageInstance(const BaseImageInstance& other)
    : image_(other.image_),
      animation_frame_callback_(other.animation_frame_callback_) {
  if (image_) {
    image_->OnInstanceCreated(this);
  }
}

BaseImageInstance::~BaseImageInstance() { image_->OnInstanceDestroyed(this); }

int BaseImageInstance::GetWidth() const { return image_->GetWidth(); }

int BaseImageInstance::GetHeight() const { return image_->GetHeight(); }

size_t BaseImageInstance::GetGraphicsImageAllocSize() const {
  return image_->GetGraphicsImageAllocSize();
}

fml::RefPtr<GraphicsImage> BaseImageInstance::GetGraphicsImage() const {
  return image_->GetGraphicsImage();
}

void BaseImageInstance::SetAnimationFrameCallback(std::function<void()> func) {
  animation_frame_callback_ = std::move(func);
}

void BaseImageInstance::OnNotifyAnimationFrame() {
  if (animation_frame_callback_) {
    animation_frame_callback_();
  }
}
}  // namespace clay
