// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKITY_H_
#define CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKITY_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/image/image_info.h"
#include "skity/geometry/rect.hpp"

namespace clay {

class GraphicsImageSkity : public GraphicsImage {
 public:
  explicit GraphicsImageSkity(std::shared_ptr<skity::Image> image);

  // |GraphicsImage|
  ~GraphicsImageSkity() override;

  std::shared_ptr<skity::Image> gr_image() const override;

  // |GraphicsImage|
  bool isOpaque() const override;

  // |GraphicsImage|
  bool isTextureBacked() const override;

  // |GraphicsImage|
  skity::Vec2 dimensions() const override;

  // |GraphicsImage|
  size_t GetApproximateByteSize() const override;

  // |GraphicsImage|
  const ImageInfo& imageInfo() const override;

  // |GraphicsImage|
  fml::RefPtr<GraphicsImage> makeWithFilter(skity::GPUContext* context,
                                            const ImageFilter* filter,
                                            const skity::Rect& subset,
                                            const skity::Rect& clipBounds,
                                            skity::Rect* outSubset,
                                            GrPoint* offset) const override;

  std::shared_ptr<skity::Pixmap> peekPixels() const override;

  // |GraphicsImage|
  fml::RefPtr<GraphicsImage> makeRasterImage() const override;

  fml::RefPtr<GraphicsImage> makeTextureImage(
      skity::GPUContext* context) const override;

  bool scalePixels(std::shared_ptr<skity::Pixmap> dst,
                   skity::GPUContext* context,
                   const GrSamplingOptions& sampling_options) const override;

 protected:
  std::shared_ptr<skity::Image> image_;

 private:
  ImageInfo image_info_;

  BASE_DISALLOW_COPY_AND_ASSIGN(GraphicsImageSkity);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKITY_H_
