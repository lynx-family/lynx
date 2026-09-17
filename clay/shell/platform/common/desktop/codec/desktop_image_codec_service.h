// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_CODEC_SERVICE_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_CODEC_SERVICE_H_

#include <memory>
#include <vector>

#include "clay/common/service/service.h"
#include "clay/gfx/geometry/size.h"
#include "clay/gfx/image/codec.h"
#include "skity/io/data.hpp"

namespace clay {

using DesktopImageCodecFactory = fml::RefPtr<Codec> (*)(
    std::shared_ptr<skity::Data> encoded_data, const Size& decode_size);

// A per-engine registry of desktop image codecs. Factories are tried in
// registration order.
class DesktopImageCodecService
    : public Service<DesktopImageCodecService, Owner::kUI,
                     ServiceFlags::kMultiThread> {
 public:
  DesktopImageCodecService();

  static std::shared_ptr<DesktopImageCodecService> Create();

  // Registers a codec factory before the service is published.
  void RegisterCodecFactory(DesktopImageCodecFactory factory) {
    if (!factory) {
      return;
    }
    factories_.push_back(factory);
  }

  fml::RefPtr<Codec> CreateCodec(
      const std::shared_ptr<skity::Data>& encoded_data,
      const Size& decode_size) const {
    for (auto factory : factories_) {
      if (auto codec = factory(encoded_data, decode_size)) {
        return codec;
      }
    }
    return nullptr;
  }

 private:
  std::vector<DesktopImageCodecFactory> factories_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_CODEC_SERVICE_H_
