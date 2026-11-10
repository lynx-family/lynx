// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/path_android.h"

#include <memory>
#include <utility>

namespace lynx::tasm {

PathAndroid::PathAndroid(gfx::PathSource source)
    : gfx::PathImpl(std::move(source)) {}
PathAndroid::~PathAndroid() = default;

std::unique_ptr<gfx::PathImpl> PathAndroidFactory::Create(
    gfx::PathSource source) const {
  return std::make_unique<PathAndroid>(std::move(source));
}

}  // namespace lynx::tasm
