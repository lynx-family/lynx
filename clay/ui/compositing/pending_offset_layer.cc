// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_offset_layer.h"

namespace clay {

PendingOffsetLayer::PendingOffsetLayer(const FloatPoint& offset)
    : offset_(offset) {}

PendingOffsetLayer::~PendingOffsetLayer() = default;

void PendingOffsetLayer::AddToFrame(FrameBuilder* builder,
                                    const FloatPoint& offset) {
  builder->PushOffset(offset.x() + offset_.x(), offset.y() + offset_.y(), this);
  AddChildrenToFrame(builder);
  builder->Pop();
}

#ifndef NDEBUG
std::string PendingOffsetLayer::ToString() const {
  std::stringstream ss;
  ss << PendingContainerLayer::ToString() << " " << offset_.ToString();
  return ss.str();
}
#endif

}  // namespace clay
