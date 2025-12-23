// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/page_fragment_behavior.h"

#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx::tasm {

void PageFragmentBehavior::CreatePlatformRenderer(
    const fml::RefPtr<PropBundle>& attributes) {
  if (painting_context_ && fragment_) {
    painting_context_->CreatePlatformRenderer(
        fragment_->id(), PlatformRendererType::kPage, attributes);
  }
}

}  // namespace lynx::tasm
