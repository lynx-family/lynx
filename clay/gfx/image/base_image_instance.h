// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_BASE_IMAGE_INSTANCE_H_
#define CLAY_GFX_IMAGE_BASE_IMAGE_INSTANCE_H_

#include <memory>
#include <utility>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/gfx/image/graphics_image.h"

namespace clay {
class BaseImage;
class ImageResourceClient;

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
  void SetVisibleCallback(std::function<bool()> func);
  void SetAnimationListener(ImageResourceClient* listener);
  ImageResourceClient* GetAnimationListener() const {
    return animation_listener_;
  }
  fml::WeakPtr<BaseImageInstance> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void OnNotifyAnimationFrame();

  bool IsVisible() const;

 protected:
  std::shared_ptr<BaseImage> image_;
  std::function<void()> animation_frame_callback_;
  std::function<bool()> visible_callback_;

 private:
  // Each view binds its own listener, including when the instance is cloned.
  ImageResourceClient* animation_listener_ = nullptr;
  fml::WeakPtrFactory<BaseImageInstance> weak_factory_{this};
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_BASE_IMAGE_INSTANCE_H_
