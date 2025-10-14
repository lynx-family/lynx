// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_SCROLL_OFFSET_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_SCROLL_OFFSET_LAYER_H_

#include <memory>
#include <string>

#include "clay/flow/animation/scroll_offset_animation.h"
#include "clay/ui/compositing/pending_offset_layer.h"

namespace clay {

class PendingScrollOffsetLayer : public PendingOffsetLayer {
 public:
  PendingScrollOffsetLayer(
      const FloatPoint& offset, const FloatPoint& scroll_offset,
      const FloatRect& visible_offset_range, const FloatRect& max_offset_range,
      std::shared_ptr<clay::ScrollOffsetAnimation> animation);

  std::string GetName() const override { return "PendingScrollOffsetLayer"; }

 private:
  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override;

  // `scroll_offset_` is the current scroll offset of the scroll view.
  FloatPoint scroll_offset_;
  // `visible_offset_range_` is the visible scroll offset range of the scroll
  // (maybe has more invisible items out of this range, but should not stop the
  // animation). If the scroll animation runs beyond this range, but still is
  // not out of the `max_offset_range_`, it means the animation should be
  // blocked at the edge and waiting for more visible items.
  FloatRect visible_offset_range_;
  // `max_offset_range_` is the max scroll offset range of the scroll view. If
  // the scroll animation runs beyond this range, it means the animation needs
  // to end.
  FloatRect max_offset_range_;
  // The `ScrollOffsetAnimation` is constructed by RasterFlingManager when
  // starting a new fling animation, and will be passed to the
  // `ScrollOffsetMutator` with FrameBuilder during the animation.
  std::shared_ptr<clay::ScrollOffsetAnimation> animation_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_SCROLL_OFFSET_LAYER_H_
