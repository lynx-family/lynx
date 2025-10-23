// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKIA_LAZY_H_
#define CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKIA_LAZY_H_

#include <functional>
#include <memory>

#include "clay/gfx/image/decoding_image.h"
#include "clay/gfx/image/graphics_image_skia.h"
#include "clay/gfx/image/image.h"

namespace clay {

class GraphicsImageSkiaLazy final : public GraphicsImageSkia {
 public:
  explicit GraphicsImageSkiaLazy(std::shared_ptr<Image> image);
  ~GraphicsImageSkiaLazy() override;

  fml::RefPtr<DecodingImage> decoding_image() const { return decoding_image_; }

  bool IsLazyImage() const override { return true; }

 private:
  skity::Vec2 dimensions() const override;

  fml::RefPtr<DecodingImage> decoding_image_;

  BASE_DISALLOW_COPY_AND_ASSIGN(GraphicsImageSkiaLazy);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKIA_LAZY_H_
