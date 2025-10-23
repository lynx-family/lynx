// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/picture.h"

#include <memory>
#include <utility>

namespace clay {

#ifndef ENABLE_SKITY
Picture::Picture(GPUObject<PictureSkia> picture, bool has_lazy_image)
    : picture_(std::move(picture)), has_lazy_image_(has_lazy_image) {}
#else
Picture::Picture(GPUObject<PictureSkity> picture, bool has_lazy_image)
    : picture_(std::move(picture)), has_lazy_image_(has_lazy_image) {}
#endif  // ENABLE_SKITY

}  // namespace clay
