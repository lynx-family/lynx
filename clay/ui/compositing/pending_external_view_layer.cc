// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_external_view_layer.h"

namespace clay {

PendingExternalViewLayer::PendingExternalViewLayer(const ElementId &element_id,
                                                   const skity::Vec2 &size)
    : element_id_(element_id), size_(size) {}
void PendingExternalViewLayer::AddToFrame(FrameBuilder *builder,
                                          const FloatPoint &offset) {
  builder->PushExternalViewLayer(element_id_, size_);
  AddChildrenToFrame(builder, offset);
  builder->Pop();
}

}  // namespace clay
