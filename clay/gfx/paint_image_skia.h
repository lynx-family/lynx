// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_PAINT_IMAGE_SKIA_H_
#define CLAY_GFX_PAINT_IMAGE_SKIA_H_

#include "base/include/fml/macros.h"
#include "clay/gfx/paint_image.h"

namespace clay {

class PaintImageSkia : public PaintImage {
 public:
  PaintImageSkia(sk_sp<SkImage> image);

  // |PaintImage|
  ~PaintImageSkia() override;

  // |PaintImage|
  clay::GrImagePtr gr_image() const override;

  // |PaintImage|
  bool isOpaque() const override;

  // |PaintImage|
  bool isTextureBacked() const override;

  // |PaintImage|
  skity::Vec2 dimensions() const override;

  // |PaintImage|
  size_t GetApproximateByteSize() const override;

 private:
  clay::GrImagePtr image_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PaintImageSkia);
};

}  // namespace clay

#endif  // CLAY_GFX_PAINT_IMAGE_SKIA_H_
