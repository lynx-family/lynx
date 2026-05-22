// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKITY_LAZY_H_
#define CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKITY_LAZY_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/gfx/image/graphics_image_skity.h"
#include "clay/gfx/image/image.h"
#include "clay/gfx/skity/skity_decoding_image.h"

namespace clay {

class GraphicsImageSkityLazy final : public GraphicsImageSkity {
 public:
  explicit GraphicsImageSkityLazy(std::shared_ptr<clay::Image> image);
  ~GraphicsImageSkityLazy() override;

  std::shared_ptr<SkityDecodingImage> decoding_image() const {
    return decoding_image_;
  }

  bool IsLazyImage() const override { return true; }

 private:
  skity::Vec2 dimensions() const override;

  std::shared_ptr<SkityDecodingImage> decoding_image_;

  BASE_DISALLOW_COPY_AND_ASSIGN(GraphicsImageSkityLazy);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_GRAPHICS_IMAGE_SKITY_LAZY_H_
