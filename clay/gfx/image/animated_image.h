// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_ANIMATED_IMAGE_H_
#define CLAY_GFX_IMAGE_ANIMATED_IMAGE_H_

#include <memory>
#include <string>

#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/animated_image_player.h"
#include "clay/gfx/image/base_image.h"

namespace clay {

class AnimatedImageInstance;

// A cacheable animated image resource. Playback state is owned by
// AnimatedImageInstance so multiple users of the same PlatformImage can play
// independently.
class AnimatedImage : public BaseImage {
 public:
  static std::shared_ptr<AnimatedImage> Make(
      fml::WeakPtr<ImageFetcher> image_fetcher, std::string url,
      fml::RefPtr<fml::TaskRunner> task_runner,
      std::shared_ptr<PlatformImage> image);

  std::unique_ptr<BaseImageInstance> NewInstance() override;

  size_t GetGraphicsImageAllocSize() const override;

  // Animated images are uploaded by AnimatedImageInstance because each
  // instance can display a different frame.
  void Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) override;

 private:
  AnimatedImage() = default;

  fml::RefPtr<fml::TaskRunner> task_runner_;

  friend class AnimatedImageInstance;
};

class AnimatedImageInstance : public BaseImageInstance {
 public:
  explicit AnimatedImageInstance(std::shared_ptr<AnimatedImage> image);
  AnimatedImageInstance(const AnimatedImageInstance& other);
  ~AnimatedImageInstance() override;

  std::unique_ptr<BaseImageInstance> Clone() const override;

  size_t GetGraphicsImageAllocSize() const override;
  fml::RefPtr<GraphicsImage> GetGraphicsImage() const override;
  void Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) const override;

  void SetAutoPlay(bool auto_play) override;
  void SetLoopCount(int loop_count) override;
  void StartAnimate() override;
  void StopAnimation() override;
  void PauseAnimation() override;
  void ResumeAnimation() override;

 private:
  void OnFrameChanged();

  mutable std::unique_ptr<AnimatedImagePlayer> player_;
  mutable GPUObject<GraphicsImage> gpu_image_;
  std::shared_ptr<int> lifetime_ = std::make_shared<int>(0);
  mutable ImageInfo uploaded_info_;
  bool auto_play_ = true;
  int loop_count_ = 0;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_ANIMATED_IMAGE_H_
