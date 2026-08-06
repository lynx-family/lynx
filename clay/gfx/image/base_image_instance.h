// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_BASE_IMAGE_INSTANCE_H_
#define CLAY_GFX_IMAGE_BASE_IMAGE_INSTANCE_H_

#include <memory>
#include <utility>

#include "clay/gfx/geometry/size.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/graphics_image.h"

namespace clay {
class BaseImage;

class BaseImageInstance {
 public:
  explicit BaseImageInstance(std::shared_ptr<BaseImage> image);
  BaseImageInstance(const BaseImageInstance& other);
  BaseImageInstance& operator=(const BaseImageInstance& other) = delete;
  virtual ~BaseImageInstance();

  virtual std::unique_ptr<BaseImageInstance> Clone() const;

  std::shared_ptr<BaseImage> GetImage() const { return image_; }
  int GetWidth() const;
  int GetHeight() const;
  virtual size_t GetGraphicsImageAllocSize() const;
  virtual fml::RefPtr<GraphicsImage> GetGraphicsImage() const;
  virtual void Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) const;

  virtual void SetAutoPlay(bool auto_play) {}
  virtual void SetLoopCount(int loop_count) {}
  virtual void StartAnimate() {}
  virtual void StopAnimation() {}
  virtual void PauseAnimation() {}
  virtual void ResumeAnimation() {}

  void SetAnimationFrameCallback(std::function<void()> func);
  void SetVisibleCallback(std::function<bool()> func);

  void OnNotifyAnimationFrame();

  bool IsVisible() const;

 protected:
  std::shared_ptr<BaseImage> image_;
  std::function<void()> animation_frame_callback_;
  std::function<bool()> visible_callback_;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_BASE_IMAGE_INSTANCE_H_
