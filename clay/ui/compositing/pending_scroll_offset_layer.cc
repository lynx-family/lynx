// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_scroll_offset_layer.h"

#include <utility>

namespace clay {

PendingScrollOffsetLayer::PendingScrollOffsetLayer(
    const FloatPoint& offset, const FloatPoint& scroll_offset,
    const FloatRect& visible_offset_range, const FloatRect& max_offset_range,
    std::shared_ptr<clay::ScrollOffsetAnimation> animation)
    : PendingOffsetLayer(offset),
      scroll_offset_(scroll_offset),
      visible_offset_range_(visible_offset_range),
      max_offset_range_(max_offset_range),
      animation_(std::move(animation)) {}

void PendingScrollOffsetLayer::AddToFrame(FrameBuilder* builder,
                                          const FloatPoint& offset) {
  builder->PushScrollOffset(offset.x() + Offset().x(),
                            offset.y() + Offset().y(), scroll_offset_.x(),
                            scroll_offset_.y(), visible_offset_range_,
                            max_offset_range_, animation_, this);
  AddChildrenToFrame(builder);
  builder->Pop();
}

}  // namespace clay
