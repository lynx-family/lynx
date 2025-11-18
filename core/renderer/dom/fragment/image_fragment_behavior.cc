// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/image_fragment_behavior.h"

#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/starlight/types/layout_result.h"

namespace lynx::tasm {

void ImageFragmentBehavior::OnUpdateLayout(
    const starlight::LayoutResultForRendering& layout_result) {
  // TODO(zhongyr): Try to fetch image here. Use NativePaintingContext to create
  // image request bounded with id.

  // The src can be accessed with fragment_->element()->Attritbutes().
}

void ImageFragmentBehavior::OnDraw(DisplayListBuilder& display_list_builder) {
  display_list_builder.DrawImage(fragment_->id());
}

}  // namespace lynx::tasm
