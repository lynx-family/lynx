// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/path_harmony.h"

#include <memory>
#include <utility>

namespace lynx::tasm {

PathHarmony::PathHarmony(gfx::PathSource source)
    : gfx::PathImpl(std::move(source)) {}
PathHarmony::~PathHarmony() = default;

std::unique_ptr<gfx::PathImpl> PathHarmonyFactory::Create(
    gfx::PathSource source) const {
  return std::make_unique<PathHarmony>(std::move(source));
}

}  // namespace lynx::tasm
