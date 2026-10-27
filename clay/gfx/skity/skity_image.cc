// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skity/skity_image.h"

namespace clay {

SkityImage::SkityImage(std::shared_ptr<skity::Image> image,
                       ClayVoidCallback destruction_callback, void* user_data)
    : image_(image),
      destruction_callback_(destruction_callback),
      user_data_(user_data) {}

SkityImage::~SkityImage() {
  if (destruction_callback_) {
    destruction_callback_(user_data_);
  }
}

}  // namespace clay
