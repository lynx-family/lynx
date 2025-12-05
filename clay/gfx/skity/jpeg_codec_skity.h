// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_JPEG_CODEC_SKITY_H_
#define CLAY_GFX_SKITY_JPEG_CODEC_SKITY_H_

#include <memory>

// Forward declarations to avoid requiring Skity headers for all includers.
namespace skity {
class Data;
class Pixmap;
}  // namespace skity

namespace clay {

class JPEGCodecSkity {
 public:
  JPEGCodecSkity() = default;
  ~JPEGCodecSkity() = default;

  std::shared_ptr<skity::Data> Encode(const skity::Pixmap* pixmap);
};

}  // namespace clay

#endif  // CLAY_GFX_SKITY_JPEG_CODEC_SKITY_H_
