// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_PUNCH_HOLE_LAYER_H_
#define CLAY_FLOW_LAYERS_PUNCH_HOLE_LAYER_H_

#include <string>

#include "clay/flow/layers/layer.h"

namespace clay {
class PunchHoleLayer : public Layer {
 public:
  explicit PunchHoleLayer(const skity::Rect& punch_hole);

  bool IsReplacing(DiffContext* context, const Layer* layer) const override {
    return layer->as_punch_hole_layer() != nullptr;
  }

  const PunchHoleLayer* as_punch_hole_layer() const override { return this; }

  void Diff(DiffContext* context, const Layer* old_layer) override;

  void Preroll(PrerollContext* context) override;
  void Paint(PaintContext& context) const override;

  const skity::Rect& PunchHoleRect() const { return punch_hole_; }

#ifndef NDEBUG
  std::string DebugName() const override { return "PunchHoleLayer"; }
#endif

 private:
  skity::Rect punch_hole_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PunchHoleLayer);
};
}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_PUNCH_HOLE_LAYER_H_
