// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_DARWIN_LYNX_TRAIL_HUB_IMPL_DARWIN_H_
#define CORE_BASE_DARWIN_LYNX_TRAIL_HUB_IMPL_DARWIN_H_

#include <optional>
#include <string>

#include "core/renderer/utils/lynx_trail_hub.h"  // nogncheck

namespace lynx {
namespace tasm {

class LynxTrailHubImplDarwin : public LynxTrailHub::TrailImpl {
 public:
  LynxTrailHubImplDarwin() = default;
  ~LynxTrailHubImplDarwin() override = default;

  std::optional<std::string> GetStringForTrailKey(
      const std::string& key) override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_BASE_DARWIN_LYNX_TRAIL_HUB_IMPL_DARWIN_H_
