// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/background_data.h"

#include <algorithm>
#include <utility>

#include "clay/gfx/geometry/rect.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/painter/gradient_factory.h"

namespace clay {

BackgroundImage::BackgroundImage(const BackgroundImage& background_image) {
  if (background_image.image_resource_) {
#ifndef ENABLE_SKITY
    image_resource_ = std::make_unique<ImageResource>(
        *background_image.image_resource_.get());

#else
    image_resource_ = background_image.image_resource_;
#endif  // ENABLE_SKITY
  }
  if (background_image.gradient_.has_value()) {
    gradient_ = background_image.gradient_;
  }
}

#ifndef ENABLE_SKITY
void BackgroundImage::SetImageResource(
    std::unique_ptr<ImageResource> image_resource) {
  image_resource_ = std::move(image_resource);
  gradient_ = std::nullopt;
}
#else
void BackgroundImage::SetImageResource(std::shared_ptr<BaseImage> image) {
  image_resource_ = image;
  gradient_ = std::nullopt;
}
#endif  // ENABLE_SKITY

void BackgroundImage::SetGradient(const Gradient& gradient) {
  gradient_ = std::make_optional<Gradient>(gradient);
  image_resource_.reset();
}

bool BackgroundImage::IsEmpty() const {
  if (!image_resource_ && !gradient_.has_value()) {
    return true;
  }

  if (image_resource_ &&
      (image_resource_->GetWidth() <= 0 || image_resource_->GetHeight() <= 0)) {
    return true;
  }

  return false;
}

bool BackgroundImage::IsSkImage() const {
  if (image_resource_) {
    return true;
  }
  return false;
}

int BackgroundImage::Width() const {
  if (!image_resource_) {
    return 0;
  }

  return image_resource_->GetWidth();
}

int BackgroundImage::Height() const {
  if (!image_resource_) {
    return 0;
  }

  return image_resource_->GetHeight();
}

}  // namespace clay
