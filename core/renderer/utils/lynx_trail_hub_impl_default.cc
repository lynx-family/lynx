// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/renderer/utils/lynx_trail_hub.h"

namespace lynx {
namespace tasm {

std::unique_ptr<LynxTrailHub::TrailImpl> LynxTrailHub::TrailImpl::Create() {
  return nullptr;
}

}  // namespace tasm
}  // namespace lynx
