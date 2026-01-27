// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_ANIMATED_IMAGE_H_
#define CLAY_GFX_IMAGE_ANIMATED_IMAGE_H_

#include <memory>
#include <string>

#include "base/include/fml/time/timer.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/base_image.h"

namespace clay {

class AnimatedImage : public BaseImage {
 public:
  static std::shared_ptr<AnimatedImage> Make(
      fml::WeakPtr<ImageFetcher> image_fetcher, std::string url,
      fml::RefPtr<fml::TaskRunner> task_runner,
      std::shared_ptr<PlatformImage> image);

  void Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) override;

  void SetAutoPlay(bool auto_play);
  void SetLoopCount(int loop_count);
  void StartAnimate();
  void StopAnimation();
  void PauseAnimation();
  void ResumeAnimation();

 private:
  AnimatedImage() = default;

  void NextFrame();
  void OnNotifyAnimationFrame();

 private:
  std::unique_ptr<fml::OneshotTimer> frame_timer_;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_ANIMATED_IMAGE_H_
