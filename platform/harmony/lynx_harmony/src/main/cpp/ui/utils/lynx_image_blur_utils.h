// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_IMAGE_BLUR_UTILS_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_IMAGE_BLUR_UTILS_H_

#include <multimedia/image_framework/image/pixelmap_native.h>

namespace lynx {
namespace tasm {
namespace harmony {

class LynxImageBlurUtils {
 public:
  static OH_PixelmapNative* ApplyBlurToBitmap(OH_PixelmapNative* pixel_map,
                                              float blur_radius);
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_IMAGE_BLUR_UTILS_H_
