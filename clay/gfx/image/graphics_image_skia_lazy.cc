// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/graphics_image_skia_lazy.h"

namespace clay {

GraphicsImageSkiaLazy::GraphicsImageSkiaLazy(std::shared_ptr<Image> image)
    : GraphicsImageSkia(nullptr),
      decoding_image_(fml::MakeRefCounted<DecodingImage>(image)) {}

GraphicsImageSkiaLazy::~GraphicsImageSkiaLazy() {}

skity::Vec2 GraphicsImageSkiaLazy::dimensions() const {
  return skity::Vec2(decoding_image_->GetWidth(), decoding_image_->GetHeight());
}

}  // namespace clay
