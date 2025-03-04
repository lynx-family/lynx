// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/lynx_trail_hub.h"

#include <utility>

namespace lynx {
namespace tasm {

LynxTrailHub& LynxTrailHub::GetInstance() {
  static base::NoDestructor<LynxTrailHub> instance;
  return *instance;
}

LynxTrailHub::LynxTrailHub() { impl_ = TrailImpl::Create(); }

std::optional<std::string> LynxTrailHub::GetStringForTrailKey(
    const std::string& key) {
  return impl_ ? impl_->GetStringForTrailKey(key) : std::nullopt;
}

}  // namespace tasm
}  // namespace lynx
