// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/desktop/codec/desktop_image_codec_service.h"

#include "clay/shell/platform/common/desktop/codec/skity_image_codec.h"

namespace clay {

DesktopImageCodecService::DesktopImageCodecService() {
  // Skity is the built-in codec and always has the highest priority.
  RegisterGeneratorFactory(&SkityImageCodecGenerator::Create);
}

std::shared_ptr<DesktopImageCodecService> DesktopImageCodecService::Create() {
  return std::make_shared<DesktopImageCodecService>();
}

}  // namespace clay
