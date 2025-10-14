// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/platform_view_layer.h"

#include <memory>
#include <utility>

namespace clay {

PlatformViewLayer::PlatformViewLayer(const skity::Vec2& offset,
                                     const skity::Vec2& size, int64_t view_id)
    : offset_(offset), size_(size), view_id_(view_id) {}

void PlatformViewLayer::Preroll(PrerollContext* context) {
  set_paint_bounds(
      skity::Rect::MakeXYWH(offset_.x, offset_.y, size_.x, size_.y));

  if (context->compositor_state == nullptr) {
    FML_DLOG(ERROR) << "Trying to embed a platform view but the PrerollContext "
                       "does not support embedding";
    return;
  }
  context->has_platform_view = true;
  set_subtree_has_platform_view(true);
  MutatorsStack mutators;
  context->state_stack.fill(&mutators);
  std::unique_ptr<EmbeddedViewParams> params =
      std::make_unique<EmbeddedViewParams>(context->state_stack.transform_4x4(),
                                           size_, mutators);
  context->compositor_state->PrerollCompositeEmbeddedView(view_id_,
                                                          std::move(params));
}

void PlatformViewLayer::Paint(PaintContext& context) const {
  if (context.compositor_state == nullptr) {
    FML_DLOG(ERROR) << "Trying to embed a platform view but the PaintContext "
                       "does not support embedding";
    return;
  }
  clay::GrCanvas* canvas =
      context.compositor_state->CompositeEmbeddedView(view_id_);
  context.canvas = canvas;
  context.state_stack.set_delegate(context.canvas);
}

}  // namespace clay
