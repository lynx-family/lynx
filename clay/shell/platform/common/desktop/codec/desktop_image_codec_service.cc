// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/desktop/codec/desktop_image_codec_service.h"

#include <algorithm>

#include "clay/shell/platform/common/desktop/codec/skity_image_codec.h"

namespace clay {

DesktopImageCodecService::DesktopImageCodecService() {
  // Skity is the built-in codec and always has the highest priority.
  RegisterGeneratorFactory(&SkityImageCodecGenerator::Create);
  for (auto factory : DesktopImageCodecRegistry::GetInstance().GetFactories()) {
    RegisterGeneratorFactory(factory);
  }
}

std::shared_ptr<DesktopImageCodecService> DesktopImageCodecService::Create() {
  return std::make_shared<DesktopImageCodecService>();
}

void DesktopImageCodecService::RegisterGeneratorFactory(
    ImageCodecGeneratorFactory factory) {
  if (!factory || std::find(factories_.begin(), factories_.end(), factory) !=
                      factories_.end()) {
    return;
  }
  factories_.push_back(factory);
}

}  // namespace clay
