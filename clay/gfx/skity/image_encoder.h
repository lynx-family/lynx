// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_IMAGE_ENCODER_H_
#define CLAY_GFX_SKITY_IMAGE_ENCODER_H_

#include <memory>

#include "skity/codec/codec.hpp"

namespace clay {

using ImageEncoderFactory = std::shared_ptr<skity::Codec> (*)();

// Returns null when image encoding support is not linked into this build.
inline ImageEncoderFactory GetImageEncoderFactory(bool compress_jpeg) {
#ifdef ENABLE_SKITY_CODEC
  return compress_jpeg ? &skity::Codec::MakeJPEGCodec
                       : &skity::Codec::MakePngCodec;
#else
  return nullptr;
#endif
}

}  // namespace clay

#endif  // CLAY_GFX_SKITY_IMAGE_ENCODER_H_
