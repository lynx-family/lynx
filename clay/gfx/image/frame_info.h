// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_FRAME_INFO_H_
#define CLAY_GFX_IMAGE_FRAME_INFO_H_

#include "clay/gfx/image/graphics_image.h"

namespace clay {

// Information for a single frame of an animation.
struct FrameInfo {
  // The [GraphicsImage] object for this frame.
  fml::RefPtr<GraphicsImage> image;

  // The duration this frame should be shown.
  int duration = 0;
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_FRAME_INFO_H_
