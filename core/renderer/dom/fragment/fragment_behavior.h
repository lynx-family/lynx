// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_BEHAVIOR_H_
#define CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_BEHAVIOR_H_

#include "base/include/fml/memory/ref_ptr.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"

namespace lynx::tasm {
class PaintingContext;
class Fragment;
class FragmentBehavior {
 public:
  explicit FragmentBehavior(Fragment* fragment);
  virtual ~FragmentBehavior() = default;
  virtual void CreatePlatformRenderer();
  // TODO(zhongyr): TO be implemented for basic <view>.
  virtual void OnAttributeUpdate(const fml::RefPtr<PropBundle>& attributes){};

  virtual void OnDraw(DisplayListBuilder& display_list_builder){};

  virtual void OnUpdateLayout(
      const starlight::LayoutResultForRendering& layout_result) {}

 protected:
  // Used for other painting related operations.
  Fragment* fragment_;
  NativePaintingContext* painting_context_;
};
}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_BEHAVIOR_H_
