// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_PUNCH_HOLE_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_PUNCH_HOLE_LAYER_H_

#include <string>

#include "clay/ui/compositing/pending_layer.h"

namespace clay {

class PendingPunchHoleLayer : public PendingLayer {
 public:
  explicit PendingPunchHoleLayer(const skity::Rect& rect);
  ~PendingPunchHoleLayer() override;

  std::string GetName() const override { return "PendingPunchHoleLayer"; }

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  void AddToFrame(FrameBuilder* builder,
                  const FloatPoint& offset = FloatPoint()) override;

  skity::Rect punch_hole_rect_;
};
}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_PUNCH_HOLE_LAYER_H_
