// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKIA_H_
#define CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKIA_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/gfx/image/graphics_image.h"

namespace clay {

class GraphicsImageSkia : public GraphicsImage {
 public:
  explicit GraphicsImageSkia(sk_sp<SkImage> image);

  // |GraphicsImage|
  ~GraphicsImageSkia() override;

  // |GraphicsImage|
  sk_sp<SkImage> gr_image() const override;

  // |GraphicsImage|
  bool isOpaque() const override;

  // |GraphicsImage|
  bool isTextureBacked() const override;

  // |GraphicsImage|
  skity::Vec2 dimensions() const override;

  // |GraphicsImage|
  size_t GetApproximateByteSize() const override;

  fml::RefPtr<GraphicsImage> makeWithFilter(GrRecordingContext* context,
                                            const ImageFilter* filter,
                                            const SkIRect& subset,
                                            const SkIRect& clipBounds,
                                            SkIRect* outSubset,
                                            SkIPoint* offset) const override;

  bool peekPixels(SkPixmap* pixmap) const override;

  fml::RefPtr<GraphicsImage> makeTextureImage(
      GrDirectContext*, GrMipmapped = GrMipmapped::kNo,
      skgpu::Budgeted = skgpu::Budgeted::kYes) const override;

  fml::RefPtr<GraphicsImage> makeRasterImage(
      SkImage::CachingHint cachingHint =
          SkImage::CachingHint::kDisallow_CachingHint) const override;

  const SkImageInfo& imageInfo() const override;

  void flushAndSubmit(GrDirectContext*) override;

  bool scalePixels(const SkPixmap& dst, const SkSamplingOptions&,
                   SkImage::CachingHint cachingHint =
                       SkImage::kAllow_CachingHint) const override;

 protected:
  sk_sp<SkImage> image_;

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(GraphicsImageSkia);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKIA_H_
