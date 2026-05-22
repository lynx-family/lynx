// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/paint_image.h"

#include <utility>

#include "clay/gfx/gfx_rendering_backend.h"

namespace clay {

fml::RefPtr<PaintImage> PaintImage::Make(PaintDecodingImage* image) {
  return Make(fml::RefPtr<PaintDecodingImage>(image));
}

#ifndef ENABLE_SKITY
fml::RefPtr<PaintImage> PaintImage::Make(const SkImage* image) {
  return Make(sk_ref_sp(image));
}

fml::RefPtr<PaintImage> PaintImage::Make(sk_sp<SkImage> image) {
  return fml::MakeRefCounted<PaintImageSkia>(std::move(image));
}

fml::RefPtr<PaintImage> PaintImage::Make(
    fml::RefPtr<PaintDecodingImage> image) {
  return fml::MakeRefCounted<PaintImageSkiaLazy>(std::move(image));
}
#else
fml::RefPtr<PaintImage> PaintImage::Make(
    fml::RefPtr<PaintDecodingImage> image) {
  return fml::MakeRefCounted<PaintImageSkity>(image->gr_image());
}
#endif  // ENABLE_SKITY

PaintImage::PaintImage() = default;

PaintImage::~PaintImage() = default;

int PaintImage::width() const { return dimensions().x; };

int PaintImage::height() const { return dimensions().y; };

skity::Rect PaintImage::bounds() const {
  return skity::Rect::MakeSize(dimensions());
}

std::optional<std::string> PaintImage::get_error() const {
  return std::nullopt;
}

}  // namespace clay
