// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/path_darwin.h"

#include <memory>
#include <utility>

namespace lynx::tasm {

PathDarwin::PathDarwin(gfx::PathSource source) : gfx::PathImpl(std::move(source)) {}
PathDarwin::~PathDarwin() = default;

std::unique_ptr<gfx::PathImpl> PathDarwinFactory::Create(gfx::PathSource source) const {
  return std::make_unique<PathDarwin>(std::move(source));
}

}  // namespace lynx::tasm
