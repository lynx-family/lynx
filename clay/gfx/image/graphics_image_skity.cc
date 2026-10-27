// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/graphics_image_skity.h"

#include <utility>

#include "skity/geometry/rect.hpp"
#include "skity/graphic/alpha_type.hpp"
#include "skity/graphic/image.hpp"
#include "skity/include/skity/geometry/vector.hpp"

namespace clay {

GraphicsImageSkity::GraphicsImageSkity(std::shared_ptr<skity::Image> image)
    : image_(std::move(image)) {
  if (image_) {
    image_info_ = ImageInfo::makeWH(image_->Width(), image_->Height());
  }
}

// |GraphicsImage|
GraphicsImageSkity::~GraphicsImageSkity() = default;

std::shared_ptr<skity::Image> GraphicsImageSkity::gr_image() const {
  return image_;
}

// |GraphicsImage|
bool GraphicsImageSkity::isOpaque() const {
  return image_ ? image_->GetAlphaType() == skity::AlphaType::kOpaque_AlphaType
                : false;
}

// |GraphicsImage|
bool GraphicsImageSkity::isTextureBacked() const {
  return image_ ? image_->IsTextureBackend() : false;
}

// |GraphicsImage|
skity::Vec2 GraphicsImageSkity::dimensions() const {
  return image_ ? skity::Vec2(image_->Width(), image_->Height())
                : skity::Vec2{0, 0};
}

// |GraphicsImage|
size_t GraphicsImageSkity::GetApproximateByteSize() const {
  auto size = sizeof(this);
  if (image_) {
    const size_t image_byte_size =
        width() * height() * imageInfo().bytesPerPixel();
    size += image_byte_size;
  }
  return size;
}

const ImageInfo& GraphicsImageSkity::imageInfo() const {
  static const ImageInfo default_image_info = ImageInfo();
  return image_ ? image_info_ : default_image_info;
}

fml::RefPtr<GraphicsImage> GraphicsImageSkity::makeWithFilter(
    skity::GPUContext* context, const ImageFilter* filter,
    const skity::Rect& subset, const skity::Rect& clipBounds,
    skity::Rect* outSubset, GrPoint* offset) const {
  // TODO(zhangxiao.ninja) implement later
  FML_UNIMPLEMENTED()
  return nullptr;
}

std::shared_ptr<skity::Pixmap> GraphicsImageSkity::peekPixels() const {
  return image_ ? *image_->GetPixmap() : nullptr;
}

fml::RefPtr<GraphicsImage> GraphicsImageSkity::makeRasterImage() const {
  // TODO(zhangxiao.ninja) implement later
  FML_UNIMPLEMENTED()
  return nullptr;
}

fml::RefPtr<GraphicsImage> GraphicsImageSkity::makeTextureImage(
    skity::GPUContext* context) const {
  return image_ ? GraphicsImage::Make(
                      skity::Image::MakeImage(*image_->GetPixmap(), context))
                : nullptr;
}

bool GraphicsImageSkity::scalePixels(
    std::shared_ptr<skity::Pixmap> dst, skity::GPUContext* context,
    const GrSamplingOptions& sampling_options) const {
  return image_ ? image_->ScalePixels(dst, context, sampling_options) : false;
}

}  // namespace clay
