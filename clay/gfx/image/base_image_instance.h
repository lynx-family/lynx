// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_BASE_IMAGE_INSTANCE_H_
#define CLAY_GFX_IMAGE_BASE_IMAGE_INSTANCE_H_

#include <memory>
#include <utility>

#include "clay/gfx/image/graphics_image.h"

namespace clay {
class BaseImage;

class BaseImageInstance {
 public:
  explicit BaseImageInstance(std::shared_ptr<BaseImage> image);
  BaseImageInstance(const BaseImageInstance& other);
  BaseImageInstance& operator=(const BaseImageInstance& other) = delete;
  ~BaseImageInstance();

  std::shared_ptr<BaseImage> GetImage() const { return image_; }
  int GetWidth() const;
  int GetHeight() const;
  size_t GetGraphicsImageAllocSize() const;
  fml::RefPtr<GraphicsImage> GetGraphicsImage() const;

  void SetAnimationFrameCallback(std::function<void()> func);

  void OnNotifyAnimationFrame();

 protected:
  std::shared_ptr<BaseImage> image_;
  std::function<void()> animation_frame_callback_;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_BASE_IMAGE_INSTANCE_H_
