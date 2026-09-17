// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/desktop/codec/desktop_image_codec_registry.h"

#include <algorithm>

namespace clay {

DesktopImageCodecRegistry& DesktopImageCodecRegistry::GetInstance() {
  static DesktopImageCodecRegistry registry;
  return registry;
}

void DesktopImageCodecRegistry::RegisterGeneratorFactory(
    ImageCodecGeneratorFactory factory) {
  if (!factory) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  if (std::find(factories_.begin(), factories_.end(), factory) !=
      factories_.end()) {
    return;
  }
  factories_.push_back(factory);
}

std::vector<ImageCodecGeneratorFactory>
DesktopImageCodecRegistry::GetFactories() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return factories_;
}

}  // namespace clay
