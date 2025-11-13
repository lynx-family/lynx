// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FRAGMENT_PAGE_FRAGMENT_BEHAVIOR_H_
#define CORE_RENDERER_DOM_FRAGMENT_PAGE_FRAGMENT_BEHAVIOR_H_

#include "core/renderer/dom/fragment/fragment_behavior.h"

namespace lynx::tasm {

class PageFragmentBehavior : public FragmentBehavior {
 public:
  explicit PageFragmentBehavior(Fragment* fragment)
      : FragmentBehavior(fragment) {}
  void CreatePlatformRenderer() override;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_PAGE_FRAGMENT_BEHAVIOR_H_
