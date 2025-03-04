// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/android/lynx_trail_hub_impl_android.h"

#include <memory>
#include <optional>
#include <string>

#include "core/renderer/utils/android/lynx_env_android.h"

namespace lynx {
namespace tasm {

std::optional<std::string> LynxTrailHubImplAndroid::GetStringForTrailKey(
    const std::string& key) {
  return tasm::LynxEnvAndroid::GetStringFromExternalEnv(key);
}

std::unique_ptr<LynxTrailHub::TrailImpl> LynxTrailHub::TrailImpl::Create() {
  return std::make_unique<LynxTrailHubImplAndroid>();
}

}  // namespace tasm
}  // namespace lynx
