// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/paint_image_skia.h"

#include <utility>

namespace clay {

PaintImageSkia::PaintImageSkia(sk_sp<SkImage> image)
    : image_(std::move(image)) {}

// |PaintImage|
PaintImageSkia::~PaintImageSkia() = default;

// |PaintImage|
clay::GrImagePtr PaintImageSkia::gr_image() const { return image_; };

// |PaintImage|
bool PaintImageSkia::isOpaque() const {
  if (auto sk_img = gr_image()) {
    return sk_img->isOpaque();
  }
  return false;
}

// |PaintImage|
bool PaintImageSkia::isTextureBacked() const {
  if (auto sk_img = gr_image()) {
    return sk_img->isTextureBacked();
  }
  return false;
}

// |PaintImage|
skity::Vec2 PaintImageSkia::dimensions() const {
  if (auto sk_img = gr_image()) {
    return skity::Vec2(sk_img->dimensions().width(),
                       sk_img->dimensions().height());
  }
  return skity::Vec2{};
}

// |PaintImage|
size_t PaintImageSkia::GetApproximateByteSize() const {
  auto size = sizeof(this);
  if (auto sk_img = gr_image()) {
    const auto& info = sk_img->imageInfo();
    const auto kMipmapOverhead = 4.0 / 3.0;
    const size_t image_byte_size = info.computeMinByteSize() * kMipmapOverhead;
    size += image_byte_size;
  }
  return size;
}

}  // namespace clay
