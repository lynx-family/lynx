// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_ANDROID_LYNX_TRAIL_HUB_IMPL_ANDROID_H_
#define CORE_RENDERER_UTILS_ANDROID_LYNX_TRAIL_HUB_IMPL_ANDROID_H_

#include <optional>
#include <string>

#include "core/renderer/utils/lynx_trail_hub.h"

namespace lynx {
namespace tasm {

class LynxTrailHubImplAndroid : public LynxTrailHub::TrailImpl {
 public:
  LynxTrailHubImplAndroid() = default;
  ~LynxTrailHubImplAndroid() override = default;

  std::optional<std::string> GetStringForTrailKey(
      const std::string& key) override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_ANDROID_LYNX_TRAIL_HUB_IMPL_ANDROID_H_
