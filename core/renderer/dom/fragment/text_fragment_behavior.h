// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FRAGMENT_TEXT_FRAGMENT_BEHAVIOR_H_
#define CORE_RENDERER_DOM_FRAGMENT_TEXT_FRAGMENT_BEHAVIOR_H_

#include "core/renderer/dom/fragment/fragment_behavior.h"

namespace lynx::tasm {

class DisplayListBuilder;

class TextFragmentBehavior : public FragmentBehavior {
 public:
  explicit TextFragmentBehavior(Fragment* fragment);
  void CreatePlatformRenderer(
      const fml::RefPtr<PropBundle>& attributes) override;
  void OnUpdateLayout(const LayoutInfoForDraw& layout_result) override;
  void OnDraw(DisplayListBuilder& builder) override;
  void SetTextBundle(intptr_t bundle) override { text_bundle_ = bundle; }
  void OnElementDestroying() override;

 private:
  intptr_t text_bundle_{0};
  void DispatchLayoutEvent(const LayoutInfoForDraw& layout_result);
};
}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_TEXT_FRAGMENT_BEHAVIOR_H_
