// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/fragment.h"

#include <memory>
#include <utility>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"

namespace lynx {
namespace tasm {

Fragment::Fragment(Element* element)
    : ElementContainer(element),
      sign_(element->impl_id()),
      style_(element->computed_css_style()),
      tag_(element->GetTag()) {}

void Fragment::CreateLayerIfNeeded() const {
  // TODO(zhongyr): CreatePlatformRenderer according to result of layerize.
  if (behavior_) {
    behavior_->CreatePlatformRenderer();
  }
}

void Fragment::CreatePaintingNode(
    bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) {
  element()->SetupFragmentBehavior(this);
  CreateLayerIfNeeded();
}

// TODO(zhongyr): Finish Update attributes Fragment tree related operations.
// Should create displayList and update it to platform renderer.

void Fragment::UpdateLayout(
    LayoutResultForRendering layout_result_for_rendering) {
  layout_result_for_rendering_ = std::move(layout_result_for_rendering);
}

void Fragment::SetBehavior(std::unique_ptr<FragmentBehavior> behavior) {
  behavior_ = std::move(behavior);
}

}  // namespace tasm
}  // namespace lynx
