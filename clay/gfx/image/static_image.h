// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_STATIC_IMAGE_H_
#define CLAY_GFX_IMAGE_STATIC_IMAGE_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/base_image.h"

namespace clay {

class StaticImage : public BaseImage {
 public:
  static std::shared_ptr<StaticImage> Make(
      std::shared_ptr<PlatformImage> image);

  fml::RefPtr<GraphicsImage> GetGraphicsImage() const override {
    return gpu_image_.object();
  }
  void Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) override;

 private:
  StaticImage() = default;

 private:
  GPUObject<GraphicsImage> gpu_image_;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_STATIC_IMAGE_H_
