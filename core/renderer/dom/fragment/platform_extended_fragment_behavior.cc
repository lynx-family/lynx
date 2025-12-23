// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/platform_extended_fragment_behavior.h"

#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx::tasm {

PlatformExtendedFragmentBehavior::PlatformExtendedFragmentBehavior(
    Fragment* fragment, const base::String& tag_name)
    : FragmentBehavior(fragment), tag_name_(tag_name) {}

void PlatformExtendedFragmentBehavior::CreatePlatformRenderer(
    const fml::RefPtr<PropBundle>& attributes) {
  if (painting_context_ && fragment_) {
    // Create platform renderer with extended type for custom components
    painting_context_->CreatePlatformExtendedRenderer(fragment_->id(),
                                                      tag_name_, attributes);
  }
}

}  // namespace lynx::tasm
