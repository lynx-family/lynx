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

class PlatformImage : public std::enable_shared_from_this<PlatformImage> {
 public:
  virtual ~PlatformImage() = default;

  virtual int GetWidth() = 0;
  virtual int GetHeight() = 0;

  virtual std::tuple<std::shared_ptr<skity::Data>, ImageInfo> ToBitmap() = 0;
  virtual fml::RefPtr<SharedImageSink> ToSharedImage() = 0;

  virtual void DrawFrame(int64_t frame_time,
                         std::function<void()> on_frame_changed) = 0;
  virtual bool IsAnimated() = 0;
  virtual void SetAutoPlay(bool auto_play) = 0;
  virtual void SetLoopCount(int loop_count) = 0;
  virtual void StartAnimation() = 0;
  virtual void StopAnimation() = 0;
  virtual void PauseAnimation() = 0;
  virtual void ResumeAnimation() = 0;
};
}  // namespace clay

#endif  // CLAY_GFX_IMAGE_PLATFORM_IMAGE_H_
