// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_BASE_IMAGE_H_
#define CLAY_GFX_IMAGE_BASE_IMAGE_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/rect.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/image/platform_image.h"

namespace clay {
enum class ImageType {
  kStatic,
  kAnimated,
  kSVG,
};

class BaseImage : public std::enable_shared_from_this<BaseImage> {
 public:
  virtual ~BaseImage() = default;

  virtual int GetWidth() const { return image_->GetWidth(); }
  virtual int GetHeight() const { return image_->GetHeight(); }
  virtual void Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) = 0;
  virtual fml::RefPtr<GraphicsImage> GetGraphicsImage() const {
    return nullptr;
  }
  ImageType GetType() const { return type_; }
  size_t GetGraphicsImageAllocSize() const {
    return GetWidth() * GetHeight() * 4;
  }

 protected:
  ImageType type_ = ImageType::kStatic;
  std::shared_ptr<PlatformImage> image_;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_BASE_IMAGE_H_
