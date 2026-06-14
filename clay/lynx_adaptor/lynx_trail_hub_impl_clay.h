// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_LYNX_ADAPTOR_LYNX_TRAIL_HUB_IMPL_CLAY_H_
#define CLAY_LYNX_ADAPTOR_LYNX_TRAIL_HUB_IMPL_CLAY_H_

#include <optional>
#include <string>

#include "core/renderer/utils/lynx_trail_hub.h"

namespace lynx {
namespace tasm {

class LynxTrailHubImplClay : public LynxTrailHub::TrailImpl {
 public:
  LynxTrailHubImplClay() = default;
  ~LynxTrailHubImplClay() override = default;

  std::optional<std::string> GetStringForTrailKey(
      const std::string& key) override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_LYNX_TRAIL_HUB_IMPL_CLAY_H_
