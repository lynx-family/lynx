// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/graphics_image_skity_lazy.h"

#include "skity/include/skity/geometry/vector.hpp"

namespace clay {

GraphicsImageSkityLazy::GraphicsImageSkityLazy(std::shared_ptr<Image> image)
    : GraphicsImageSkity(nullptr),
      decoding_image_(std::make_shared<SkityDecodingImage>(image)) {}

GraphicsImageSkityLazy::~GraphicsImageSkityLazy() = default;

skity::Vec2 GraphicsImageSkityLazy::dimensions() const {
  return skity::Vec2(decoding_image_->GetWidth(), decoding_image_->GetHeight());
}

}  // namespace clay
