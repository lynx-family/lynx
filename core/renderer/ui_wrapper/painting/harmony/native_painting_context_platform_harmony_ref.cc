// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"

#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {

NativePaintingCtxPlatformHarmonyRef::NativePaintingCtxPlatformHarmonyRef(
    std::unique_ptr<PlatformRendererFactory> view_factory,
    harmony::UIOwner* ui_owner)
    : NativePaintingCtxPlatformRef(std::move(view_factory)),
      ui_owner_(ui_owner) {}

void NativePaintingCtxPlatformHarmonyRef::SetNeedMarkPaintEndTiming(
    const tasm::PipelineID& pipeline_id) {
  if (ui_owner_ != nullptr) {
    ui_owner_->PostDrawEndTimingFrameCallback(pipeline_id);
  }
}

}  // namespace tasm
}  // namespace lynx
