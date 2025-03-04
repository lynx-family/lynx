// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/darwin/lynx_trail_hub_impl_darwin.h"

#include <memory>
#include <optional>
#include <string>

#include "core/base/darwin/config_darwin.h"  // nogncheck

namespace lynx {
namespace tasm {

std::optional<std::string> LynxTrailHubImplDarwin::GetStringForTrailKey(const std::string& key) {
  return tasm::LynxConfigDarwin::stringFromExternalEnv(key);
}

std::unique_ptr<LynxTrailHub::TrailImpl> LynxTrailHub::TrailImpl::Create() {
  return std::make_unique<LynxTrailHubImplDarwin>();
}

}  // namespace tasm
}  // namespace lynx
