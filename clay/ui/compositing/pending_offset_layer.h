// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_OFFSET_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_OFFSET_LAYER_H_

#include <string>

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/compositing/pending_container_layer.h"

namespace clay {

class PendingOffsetLayer : public PendingContainerLayer {
 public:
  explicit PendingOffsetLayer(const FloatPoint& offset = FloatPoint());
  ~PendingOffsetLayer() override;

  std::string GetName() const override { return "PendingOffsetLayer"; }

  const FloatPoint& Offset() const { return offset_; }
  void SetOffset(const FloatPoint& offset) {
    if (offset_ != offset) {
      offset_ = offset;
      MarkNeedsAddToFrame();
    }
  }

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override;

  FloatPoint offset_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_OFFSET_LAYER_H_
