// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/list_fragment_behavior.h"

#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx {
namespace tasm {

ListFragmentBehavior::ListFragmentBehavior(Fragment* fragment)
    : FragmentBehavior(fragment) {}

void ListFragmentBehavior::CreatePlatformRenderer() {
  if (painting_context() && fragment()) {
    painting_context()->CreatePlatformRenderer(fragment()->id(),
                                               PlatformRendererType::kList);
  }
}

}  // namespace tasm
}  // namespace lynx
