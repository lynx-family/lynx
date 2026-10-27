// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_external_view.h"

#include <memory>

#include "clay/ui/compositing/pending_external_view_layer.h"

namespace clay {

void RenderExternalView::Paint(clay::PaintingContext& context,
                               const clay::FloatPoint& offset) {
  RenderContainer::Paint(context, offset);
}

void RenderExternalView::SetBackingSize(skity::Vec2 size) {
  if (size.x == size_.x && size.y == size_.y) {
    return;
  }
  size_ = skity::Vec2(size.x, size.y);
  SetContainerLayer(
      std::make_unique<PendingExternalViewLayer>(element_id(), size_));
  MarkNeedsPaint(true);
}

}  // namespace clay
