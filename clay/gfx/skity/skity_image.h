// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_SKITY_IMAGE_H_
#define CLAY_GFX_SKITY_SKITY_IMAGE_H_

#include <memory>

#include "clay/public/clay.h"
#include "skity/graphic/image.hpp"

namespace clay {

class SkityImage {
 public:
  explicit SkityImage(std::shared_ptr<skity::Image> image,
                      ClayVoidCallback destruction_callback = nullptr,
                      void* user_data = nullptr);
  ~SkityImage();

  std::shared_ptr<skity::Image> gr_image() const { return image_; }

 private:
  SkityImage() = default;

  std::shared_ptr<skity::Image> image_;
  ClayVoidCallback destruction_callback_;
  void* user_data_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_GFX_SKITY_SKITY_IMAGE_H_
