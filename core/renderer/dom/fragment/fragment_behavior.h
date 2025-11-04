// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_BEHAVIOR_H_
#define CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_BEHAVIOR_H_

#include "base/include/fml/memory/ref_ptr.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/dom/fragment/display_list_builder.h"

namespace lynx::tasm {
class PaintingContext;
class Fragment;
class FragmentBehavior {
 public:
  explicit FragmentBehavior(Fragment* fragment) : fragment_(fragment) {}
  virtual ~FragmentBehavior() = default;
  virtual void CreatePlatformRenderer() = 0;
  virtual void OnAttributeUpdate(const fml::RefPtr<PropBundle>& attributes) = 0;
  virtual void OnDraw(DisplayListBuilder& display_list_builder) = 0;

 protected:
  // Used for other painting related operations.
  Fragment* fragment_;
};
}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_BEHAVIOR_H_
