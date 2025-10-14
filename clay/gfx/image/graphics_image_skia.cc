// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/graphics_image_skia.h"

#include <utility>

#include "third_party/skia/include/gpu/GrDirectContext.h"

namespace clay {

GraphicsImageSkia::GraphicsImageSkia(sk_sp<SkImage> image)
    : image_(std::move(image)) {}

// |GraphicsImage|
GraphicsImageSkia::~GraphicsImageSkia() = default;

// |GraphicsImage|
sk_sp<SkImage> GraphicsImageSkia::gr_image() const { return image_; }

// |GraphicsImage|
bool GraphicsImageSkia::isOpaque() const {
  return image_ ? image_->isOpaque() : false;
}

// |GraphicsImage|
bool GraphicsImageSkia::isTextureBacked() const {
  return image_ ? image_->isTextureBacked() : false;
}

// |GraphicsImage|
skity::Vec2 GraphicsImageSkia::dimensions() const {
  return image_ ? skity::Vec2{image_->dimensions().width(),
                              image_->dimensions().height()}
                : skity::Vec2{0, 0};
}

// |GraphicsImage|
size_t GraphicsImageSkia::GetApproximateByteSize() const {
  auto size = sizeof(this);
  if (image_) {
    const auto& info = image_->imageInfo();
    const auto kMipmapOverhead = 4.0 / 3.0;
    const size_t image_byte_size = info.computeMinByteSize() * kMipmapOverhead;
    size += image_byte_size;
  }
  return size;
}

fml::RefPtr<GraphicsImage> GraphicsImageSkia::makeWithFilter(
    GrRecordingContext* context, const ImageFilter* filter,
    const SkIRect& subset, const SkIRect& clipBounds, SkIRect* outSubset,
    SkIPoint* offset) const {
  return image_ ? GraphicsImage::Make(image_->makeWithFilter(
                      context, filter->gr_object().get(), subset, clipBounds,
                      outSubset, offset))
                : nullptr;
}

bool GraphicsImageSkia::peekPixels(SkPixmap* pixmap) const {
  return image_ ? image_->peekPixels(pixmap) : false;
}

fml::RefPtr<GraphicsImage> GraphicsImageSkia::makeTextureImage(
    GrDirectContext* dContext, GrMipmapped mipmapped,
    skgpu::Budgeted budgeted) const {
  return image_ ? GraphicsImage::Make(SkImages::TextureFromImage(
                      dContext, image_, mipmapped, budgeted))
                : nullptr;
}

fml::RefPtr<GraphicsImage> GraphicsImageSkia::makeRasterImage(
    SkImage::CachingHint cachingHint) const {
  return image_ ? GraphicsImage::Make(image_->makeRasterImage(cachingHint))
                : nullptr;
}

const SkImageInfo& GraphicsImageSkia::imageInfo() const {
  static const SkImageInfo default_image_info = SkImageInfo();
  return image_ ? image_->imageInfo() : default_image_info;
}

void GraphicsImageSkia::flushAndSubmit(GrDirectContext* dContext) {
  if (image_) {
    dContext->flushAndSubmit(image_);
  }
}

bool GraphicsImageSkia::scalePixels(const SkPixmap& dst,
                                    const SkSamplingOptions& sampling,
                                    SkImage::CachingHint cachingHint) const {
  return image_ ? image_->scalePixels(dst, sampling, cachingHint) : false;
}

}  // namespace clay
