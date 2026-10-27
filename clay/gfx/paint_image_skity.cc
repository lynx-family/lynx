// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/paint_image_skity.h"

#include <utility>

#include "clay/fml/logging.h"

namespace clay {

fml::RefPtr<PaintImage> PaintImageSkity::Make(clay::GrImagePtr image) {
  return fml::MakeRefCounted<PaintImageSkity>(std::move(image));
}

PaintImageSkity::PaintImageSkity(clay::GrImagePtr image)
    : image_(std::move(image)) {}

// |PaintImage|
PaintImageSkity::~PaintImageSkity() = default;

// |PaintImage|
clay::GrImagePtr PaintImageSkity::gr_image() const { return image_; }

// |PaintImage|
bool PaintImageSkity::isOpaque() const {
  return image_ ? image_->GetAlphaType() == skity::kOpaque_AlphaType : false;
}

// |PaintImage|
bool PaintImageSkity::isTextureBacked() const {
  return image_ ? image_->IsTextureBackend() : false;
}

// |PaintImage|
skity::Vec2 PaintImageSkity::dimensions() const {
  return image_ ? skity::Vec2(image_->Width(), image_->Height())
                : skity::Vec2();
}

// |PaintImage|
size_t PaintImageSkity::GetApproximateByteSize() const {
  FML_UNIMPLEMENTED()
  return 0;
}

}  // namespace clay
