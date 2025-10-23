// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skity/skity_decoding_image.h"

#include "clay/fml/logging.h"

namespace clay {

SkityDecodingImage::SkityDecodingImage(std::shared_ptr<clay::Image> image)
    : raw_image_(image) {
  FML_DCHECK(raw_image_);
}

SkityDecodingImage::~SkityDecodingImage() {
  if (image_resource_) {
    image_resource_->RemoveImageResourceClient(this);
  }
}

int SkityDecodingImage::GetWidth() const { return raw_image_->GetWidth(); }
int SkityDecodingImage::GetHeight() const { return raw_image_->GetHeight(); }

bool SkityDecodingImage::IsTextureBackend() const {
  return raw_image_->UseTextureBackend();
}

const std::shared_ptr<skity::Texture>* SkityDecodingImage::GetTexture() const {
  return decoded_image_ ? decoded_image_->GetTexture() : nullptr;
}

const std::shared_ptr<skity::Pixmap>* SkityDecodingImage::GetPixmap() const {
  return decoded_image_ ? decoded_image_->GetPixmap() : nullptr;
}

size_t SkityDecodingImage::Width() const { return raw_image_->GetWidth(); }

size_t SkityDecodingImage::Height() const { return raw_image_->GetHeight(); }

skity::AlphaType SkityDecodingImage::GetAlphaType() const {
  return decoded_image_ ? decoded_image_->GetAlphaType()
                        : skity::AlphaType::kUnknown_AlphaType;
}

bool SkityDecodingImage::ScalePixels(
    std::shared_ptr<skity::Pixmap> dst, skity::GPUContext* context,
    const GrSamplingOptions& sampling_options) const {
  return decoded_image_ &&
         decoded_image_->ScalePixels(dst, context, sampling_options);
}

void SkityDecodingImage::ScheduleDecodeAndUpload(
    const LazyImageDecodeCallback& callback) {
  callback_ = callback;
  if (!image_resource_) {
    image_resource_ = raw_image_->GetAccessor(true);
    image_resource_->AddImageResourceClient(this);
  }
  if (auto g_image = image_resource_->GetImage()->GetGraphicsImage()) {
    if (auto sk_image = g_image->gr_image()) {
      decoded_image_ = g_image->gr_image();
    }
  }
}

bool SkityDecodingImage::MaybeAnimated() const {
  return raw_image_ && raw_image_->MaybeAnimated();
}

void SkityDecodingImage::DecodeImageFinish(bool success) {
  // Save the latest decoded image, if decoding fails, continue to use the
  // previous frame or empty.
  if (auto g_image = image_resource_->GetImage()->GetGraphicsImage()) {
    if (auto sk_image = g_image->gr_image()) {
      decoded_image_ = g_image->gr_image();
    }
  }
  // Decoding is finished, notify Rasterizer to redraw LayerTree.
  if (callback_) {
    callback_(success);
  }
}

}  // namespace clay
