// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_IMAGE_CODEC_GENERATOR_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_IMAGE_CODEC_GENERATOR_H_

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/gfx/image/codec.h"
#include "clay/gfx/image/image_info.h"

namespace clay {

// Owns the underlying desktop image decoder and creates per-playback codecs.
class ImageCodecGenerator {
 public:
  virtual ~ImageCodecGenerator() = default;

  virtual const ImageInfo& GetImageInfo() const = 0;
  virtual int FrameCount() const = 0;
  virtual fml::RefPtr<GraphicsImage> GetImage() const = 0;
  virtual fml::RefPtr<Codec> CreateCodec() = 0;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_IMAGE_CODEC_GENERATOR_H_
