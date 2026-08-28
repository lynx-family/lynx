// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_KNIFE_OPTION_COMPAT_H_
#define PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_KNIFE_OPTION_COMPAT_H_

#include "imageknife.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"

namespace lynx {
namespace service {

class ImageKnifeOptionCompat {
 public:
  static void Apply(ImageKnifePro::ImageKnifeOption* option,
                    const tasm::harmony::ImageRequestInfo& info);
  static void SetEnableVisibleAreaControl(
      ImageKnifePro::ImageKnifeOption* option, bool enabled);
};

}  // namespace service
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_KNIFE_OPTION_COMPAT_H_
