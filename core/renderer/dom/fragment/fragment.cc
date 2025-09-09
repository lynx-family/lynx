// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/fragment.h"

#include <utility>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"

namespace lynx {
namespace tasm {

Fragment::Fragment(const starlight::ComputedCSSStyle& style, int sign,
                   PaintingContext& painting_context, base::String tag)
    : style_{style},
      sign_{sign},
      painting_context_{painting_context},
      tag_(std::move(tag)) {}

void Fragment::CreateLayerIfNeeded() {
  // TODO(zhongyr): Create HostPlatformRenderer here.
  painting_context_.CreatePaintingNode(sign_, Tag().str(), nullptr, false,
                                       false, 0);
}

void Fragment::UpdateLayout(
    LayoutResultForRendering layout_result_for_rendering) {
  layout_result_for_rendering_ = std::move(layout_result_for_rendering);
}

}  // namespace tasm
}  // namespace lynx
