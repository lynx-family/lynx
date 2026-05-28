// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_image_blur_utils.h"

#include <native_effect/effect_filter.h>
#include <native_effect/effect_types.h>

namespace lynx {
namespace tasm {
namespace harmony {
OH_PixelmapNative* LynxImageBlurUtils::ApplyBlurToBitmap(
    OH_PixelmapNative* pixel_map, float blur_radius) {
  OH_Filter* filter = nullptr;
  EffectErrorCode errCode = OH_Filter_CreateEffect(pixel_map, &filter);
  if (errCode != 0 || filter == nullptr) {
    return nullptr;
  }
  OH_Filter_Blur(filter, blur_radius);
  OH_PixelmapNative* new_pixelmap = nullptr;
  OH_Filter_GetEffectPixelMap(filter, &new_pixelmap);
  OH_Filter_Release(filter);
  return new_pixelmap;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
