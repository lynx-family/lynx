// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/decoding_image.h"

#include <utility>

namespace clay {

DecodingImage::DecodingImage(std::shared_ptr<Image> image)
    : GraphicsImageSkia(nullptr), raw_image_(image) {}

DecodingImage::~DecodingImage() {
  if (image_resource_) {
    image_resource_->RemoveImageResourceClient(this);
  }
}

int DecodingImage::GetWidth() const { return raw_image_->GetWidth(); }

int DecodingImage::GetHeight() const { return raw_image_->GetHeight(); }

skity::Vec2 DecodingImage::dimensions() const {
  return skity::Vec2(GetWidth(), GetHeight());
}

void DecodingImage::ScheduleDecodeAndUpload(
    const LazyImageDecodeCallback& callback) {
  callback_ = callback;
  if (!image_resource_) {
    image_resource_ = raw_image_->GetAccessor(true);
    image_resource_->AddImageResourceClient(this);
  }
  if (auto g_image = image_resource_->GetImage()->GetGraphicsImage()) {
    if (auto sk_image = g_image->gr_image()) {
      image_ = g_image->gr_image();
    }
  }
}

bool DecodingImage::MaybeAnimated() const {
  return raw_image_->MaybeAnimated();
}

void DecodingImage::DecodeImageFinish(bool success) {
  // Save the latest decoded image, if decoding fails, continue to use the
  // previous frame or empty.
  if (auto g_image = image_resource_->GetImage()->GetGraphicsImage()) {
    if (auto sk_image = g_image->gr_image()) {
      image_ = g_image->gr_image();
    }
  }
  // Decoding is finished, notify Rasterizer to redraw LayerTree.
  if (callback_) {
    callback_(success);
  }
}

}  // namespace clay
