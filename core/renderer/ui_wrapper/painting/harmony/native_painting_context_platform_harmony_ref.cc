// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"

#include <utility>

namespace lynx::tasm {

NativePaintingCtxPlatformHarmonyRef::NativePaintingCtxPlatformHarmonyRef(
    std::unique_ptr<PlatformRendererFactory> renderer_factory)
    : NativePaintingCtxPlatformRef(std::move(renderer_factory)) {}

NativePaintingCtxPlatformHarmonyRef::~NativePaintingCtxPlatformHarmonyRef() {
  Destroy();
}

}  // namespace lynx::tasm
