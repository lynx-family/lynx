// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/paint_image_skia_lazy.h"

#include <utility>

#include "clay/gfx/paint_decoding_image.h"

namespace clay {

PaintImageSkiaLazy::PaintImageSkiaLazy(fml::RefPtr<PaintDecodingImage> image)
    : PaintImageSkia(nullptr), decoding_image_(std::move(image)) {}

// |PaintImage|
PaintImageSkiaLazy::~PaintImageSkiaLazy() = default;

// |PaintImage|
sk_sp<SkImage> PaintImageSkiaLazy::gr_image() const {
  return decoding_image_ ? decoding_image_->gr_image() : nullptr;
};

}  // namespace clay
