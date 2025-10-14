// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_ANIMATED_IMAGE_H_
#define CLAY_GFX_IMAGE_ANIMATED_IMAGE_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/image/base_image.h"
#include "clay/gfx/shared_image/shared_image_sink.h"

namespace clay {

class AnimatedImage : public BaseImage {
 public:
  static std::shared_ptr<AnimatedImage> Make(
      std::shared_ptr<PlatformImage> image);

  fml::RefPtr<GraphicsImage> GetGraphicsImage() const override {
    return gpu_image_.object();
  }
  fml::RefPtr<SharedImageSink> GetSharedImageSink();
  void SetTextureId(int64_t texture_id) { texture_id_ = texture_id; }
  int64_t GetTextureId() { return texture_id_; }
  void Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) override;

  bool DoAnimationFrame(int64_t frame_time,
                        std::function<void()> on_frame_changed);
  void SetAutoPlay(bool auto_play);
  void SetLoopCount(int loop_count);
  void StartAnimate();
  void StopAnimation();
  void PauseAnimation();
  void ResumeAnimation();

 private:
  AnimatedImage() = default;

 private:
  GPUObject<GraphicsImage> gpu_image_;
  fml::RefPtr<SharedImageSink> shared_image_;
  int64_t texture_id_ = -1;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_ANIMATED_IMAGE_H_
