// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_IMAGE_FRAGMENT_BEHAVIOR_H_
#define CORE_RENDERER_DOM_FRAGMENT_IMAGE_FRAGMENT_BEHAVIOR_H_

#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"

namespace lynx::tasm {

class Fragment;

class ImageFragmentBehavior : public FragmentBehavior {
 public:
  explicit ImageFragmentBehavior(Fragment* fragment)
      : FragmentBehavior(fragment) {}
  void OnUpdateLayout(
      const starlight::LayoutResultForRendering& layout_result) override;
  void OnDraw(DisplayListBuilder& display_list_builder) override;

 private:
  base::String image_url_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_IMAGE_FRAGMENT_BEHAVIOR_H_
