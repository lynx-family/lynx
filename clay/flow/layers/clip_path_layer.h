// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_CLIP_PATH_LAYER_H_
#define CLAY_FLOW_LAYERS_CLIP_PATH_LAYER_H_

#include <string>

#include "clay/flow/layers/clip_shape_layer.h"

namespace clay {

class ClipPathLayer : public ClipShapeLayer<clay::GrPath> {
 public:
  explicit ClipPathLayer(const clay::GrPath& clip_path,
                         Clip clip_behavior = Clip::antiAlias);

 protected:
  const skity::Rect& clip_shape_bounds() const override;

  void ApplyClip(LayerStateStack::MutatorContext& mutator) const override;

#ifndef NDEBUG
  std::string DebugName() const override { return "ClipPathLayer"; }
#endif

 private:
  skity::Rect clip_shape_bounds_;

  BASE_DISALLOW_COPY_AND_ASSIGN(ClipPathLayer);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_CLIP_PATH_LAYER_H_
