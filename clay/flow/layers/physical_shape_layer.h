// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_PHYSICAL_SHAPE_LAYER_H_
#define CLAY_FLOW_LAYERS_PHYSICAL_SHAPE_LAYER_H_

#include <string>

#include "clay/flow/layers/container_layer.h"

namespace clay {

class PhysicalShapeLayer : public ContainerLayer {
 public:
  PhysicalShapeLayer(clay::GrColor color, clay::GrColor shadow_color,
                     float elevation, const clay::GrPath& path,
                     Clip clip_behavior);

  void Diff(DiffContext* context, const Layer* old_layer) override;

  void Preroll(PrerollContext* context) override;

  void Paint(PaintContext& context) const override;

  bool UsesSaveLayer() const {
    return clip_behavior_ == Clip::antiAliasWithSaveLayer;
  }

  float elevation() const { return elevation_; }

#ifndef NDEBUG
  std::string DebugName() const override { return "PhysicalShapeLayer"; }
#endif

 private:
  clay::GrColor color_;
  clay::GrColor shadow_color_;
  float elevation_ = 0.0f;
  clay::GrPath path_;
  Clip clip_behavior_;
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_PHYSICAL_SHAPE_LAYER_H_
