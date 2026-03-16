// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/fragment_behavior.h"

#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"

namespace lynx::tasm {
FragmentBehavior::FragmentBehavior(Fragment* fragment)
    : fragment_(fragment),
      painting_context_(
          fragment->painting_context()->impl()->CastToNativeCtx()) {}

void FragmentBehavior::Destroy() {
  OnElementDestroying();
  fragment_ = nullptr;
  painting_context_ = nullptr;
}

void FragmentBehavior::CreatePlatformRenderer(
    const fml::RefPtr<PropBundle>& attributes) {
  if (!painting_context_) {
    return;
  }
  painting_context_->CreatePlatformRenderer(
      fragment_->id(), PlatformRendererType::kView, attributes);
}

}  // namespace lynx::tasm
