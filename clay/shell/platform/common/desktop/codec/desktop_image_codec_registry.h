// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_CODEC_REGISTRY_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_CODEC_REGISTRY_H_

#include <memory>
#include <mutex>
#include <vector>

#include "clay/gfx/geometry/size.h"
#include "clay/shell/platform/common/desktop/codec/image_codec_generator.h"
#include "skity/io/data.hpp"

namespace clay {

using ImageCodecGeneratorFactory = std::shared_ptr<ImageCodecGenerator> (*)(
    std::shared_ptr<skity::Data> encoded_data, const Size& decode_size);

// Stores downstream codec factories copied by each codec service at creation.
class DesktopImageCodecRegistry {
 public:
  static DesktopImageCodecRegistry& GetInstance();

  void RegisterGeneratorFactory(ImageCodecGeneratorFactory factory);

  std::vector<ImageCodecGeneratorFactory> GetFactories() const;

 private:
  DesktopImageCodecRegistry() = default;

  mutable std::mutex mutex_;
  std::vector<ImageCodecGeneratorFactory> factories_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_CODEC_REGISTRY_H_
