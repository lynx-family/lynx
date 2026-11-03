// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_PLATFORM_IMAGE_H_
#define CLAY_GFX_IMAGE_PLATFORM_IMAGE_H_

#include <memory>
#include <tuple>

#include "clay/gfx/image/image_info.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/shared_image/shared_image_sink.h"

namespace clay {

class PlatformImageAnimation {
 public:
  virtual ~PlatformImageAnimation() = default;

  // Raw frame durations are in milliseconds. Playback timing and loop state
  // belong exclusively to AnimatedImagePlayer.
  virtual int GetFrameCount() const = 0;
  virtual int64_t GetFrameDuration(int frame_index) const = 0;

  // Returns a complete frame snapshot, including any required composition.
  // Calls may skip frames or seek backwards. Mutable decoder caches must be
  // synchronized because decoding runs independently of playback updates.
  virtual std::shared_ptr<skity::Pixmap> DecodeFrame(
      int frame_index, const ImageInfo& render_info) = 0;
};

class PlatformImage : public std::enable_shared_from_this<PlatformImage> {
 public:
  virtual ~PlatformImage() = default;

  virtual int GetWidth() = 0;
  virtual int GetHeight() = 0;
  virtual skity::ColorType GetColorType() = 0;
  virtual skity::AlphaType GetAlphaType() = 0;

  virtual std::shared_ptr<skity::Pixmap> ToBitmap(
      const ImageInfo& render_info) = 0;

  virtual bool IsAnimated() = 0;
  virtual std::unique_ptr<PlatformImageAnimation> CreateAnimation() = 0;
};
}  // namespace clay

#endif  // CLAY_GFX_IMAGE_PLATFORM_IMAGE_H_
