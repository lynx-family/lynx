// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_picture_layer.h"

#include "clay/fml/logging.h"
#include "clay/gfx/image/image_resource.h"

namespace clay {

PendingPictureLayer::PendingPictureLayer() = default;

PendingPictureLayer::~PendingPictureLayer() = default;

void PendingPictureLayer::AddToFrame(FrameBuilder* builder,
                                     const FloatPoint& offset) {
  if (!picture()) {
    return;
  }
  builder->AddPicture(offset.x(), offset.y(), this, IsComplexHint(),
                      WillChangeHint(), strategy_);
  if (strategy_ == CacheStrategy::NotCache) {
    // CacheStrategy::NotCache only used for one frame.
    strategy_ = CacheStrategy::None;
  }
}

}  // namespace clay
