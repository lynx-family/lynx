// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"

#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"

namespace lynx::tasm {

NativePaintingCtxPlatformHarmonyRef::NativePaintingCtxPlatformHarmonyRef(
    std::unique_ptr<PlatformRendererFactory> renderer_factory,
    std::weak_ptr<harmony::LynxRendererContext> renderer_context)
    : NativePaintingCtxPlatformRef(std::move(renderer_factory)),
      renderer_context_(std::move(renderer_context)) {}

NativePaintingCtxPlatformHarmonyRef::~NativePaintingCtxPlatformHarmonyRef() {
  Destroy();
}

void NativePaintingCtxPlatformHarmonyRef::DestroyImageOnPlatformThread(
    int32_t image_key) {
  if (auto renderer_context = renderer_context_.lock()) {
    renderer_context->DestroyImageManager(image_key);
  }
}

}  // namespace lynx::tasm
