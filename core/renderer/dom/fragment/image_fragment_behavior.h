// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FRAGMENT_IMAGE_FRAGMENT_BEHAVIOR_H_
#define CORE_RENDERER_DOM_FRAGMENT_IMAGE_FRAGMENT_BEHAVIOR_H_

#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"

namespace lynx::tasm {

class Fragment;

// Event flags for image elements (must match LynxImageManager.java)
constexpr int32_t kFlagImageLoadEvent = 1;
constexpr int32_t kFlagImageErrorEvent = 1 << 1;

class ImageFragmentBehavior : public FragmentBehavior {
 public:
  explicit ImageFragmentBehavior(Fragment* fragment)
      : FragmentBehavior(fragment) {}
  void OnUpdateLayout(const LayoutInfoForDraw& layout_result) override;
  void OnDraw(DisplayListBuilder& display_list_builder) override;

 private:
  // Computes event mask based on element's event map.
  // Returns a bitmask of kFlagImageLoadEvent and kFlagImageErrorEvent.
  int32_t ComputeEventMask() const;

  base::String image_url_;
  // Cached event mask - computed lazily on first use, then never changes.
  mutable int32_t event_mask_{-1};  // -1 means not yet computed
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_IMAGE_FRAGMENT_BEHAVIOR_H_
