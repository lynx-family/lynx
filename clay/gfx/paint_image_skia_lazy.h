// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_PAINT_IMAGE_SKIA_LAZY_H_
#define CLAY_GFX_PAINT_IMAGE_SKIA_LAZY_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/gfx/paint_decoding_image.h"
#include "clay/gfx/paint_image_skia.h"

namespace clay {

class PaintImageSkiaLazy final : public PaintImageSkia {
 public:
  explicit PaintImageSkiaLazy(fml::RefPtr<PaintDecodingImage> image);

  // |PaintImage|
  ~PaintImageSkiaLazy() override;

  // |PaintImage|
  sk_sp<SkImage> gr_image() const override;

  fml::RefPtr<PaintDecodingImage> decoding_image() const override {
    return decoding_image_;
  }

 private:
  fml::RefPtr<PaintDecodingImage> decoding_image_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PaintImageSkiaLazy);
};

}  // namespace clay

#endif  // CLAY_GFX_PAINT_IMAGE_SKIA_LAZY_H_
