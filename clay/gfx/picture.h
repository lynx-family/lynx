// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_PICTURE_H_
#define CLAY_GFX_PICTURE_H_

#include <memory>

#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/gpu_object.h"

namespace clay {

class Picture {
 public:
#ifndef ENABLE_SKITY
  explicit Picture(GPUObject<PictureSkia> picture, bool has_lazy_image = false);

  fml::RefPtr<PictureSkia> picture() const { return picture_.object(); }
#else
  explicit Picture(GPUObject<PictureSkity> picture,
                   bool has_lazy_image = false);
  fml::RefPtr<PictureSkity> picture() const { return picture_.object(); }

#endif  // ENABLE_SKITY
  bool HasLazyImage() const { return has_lazy_image_; }

  bool IsEmpty() const {
    return picture_.object() == nullptr ||
#ifndef ENABLE_SKITY
           picture_.object()->raw()->approximateOpCount() == 0;
#else
           picture_.object()->raw()->OpCount() == 0;
#endif  // ENABLE_SKITY
  }

 private:
#ifndef ENABLE_SKITY
  GPUObject<PictureSkia> picture_;
#else
  GPUObject<PictureSkity> picture_;
#endif  // ENABLE_SKITY
  bool has_lazy_image_ = false;
};

}  // namespace clay

#endif  // CLAY_GFX_PICTURE_H_
