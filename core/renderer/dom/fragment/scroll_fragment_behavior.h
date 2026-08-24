// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_SCROLL_FRAGMENT_BEHAVIOR_H_
#define CORE_RENDERER_DOM_FRAGMENT_SCROLL_FRAGMENT_BEHAVIOR_H_

#include "core/renderer/dom/fragment/fragment_behavior.h"

namespace lynx::tasm {

class ScrollFragmentBehavior : public FragmentBehavior {
 public:
  explicit ScrollFragmentBehavior(Fragment* fragment);

  PlatformRendererType GetType() const override {
    return PlatformRendererType::kScroll;
  }

  void BeforeDrawChildren(DisplayListBuilder& display_list_builder) override;

  void AfterDrawChildren(DisplayListBuilder& display_list_builder) override;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_SCROLL_FRAGMENT_BEHAVIOR_H_
