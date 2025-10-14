// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_TRANSFORM_LAYER_H_
#define CLAY_FLOW_LAYERS_TRANSFORM_LAYER_H_

#include <memory>
#include <string>
#include <variant>

#include "clay/flow/layers/container_layer.h"
#include "clay/gfx/geometry/transform_operations.h"

namespace clay {

// Be careful that skity::Matrix's default constructor doesn't initialize the
// matrix at all. Hence |set_transform| must be called with an initialized
// skity::Matrix.
class TransformLayer : public ContainerLayer {
 public:
  explicit TransformLayer(const skity::Matrix& transform = skity::Matrix());
  TransformLayer(const clay::TransformOperations& transform, skity::Vec2 origin,
                 skity::Vec2 offset);

  void Diff(DiffContext* context, const Layer* old_layer) override;

  void Preroll(PrerollContext* context) override;

  void Paint(PaintContext& context) const override;

  skity::Matrix GetMatrix() const;
  clay::TransformOperations GetTransform() const;

#ifndef NDEBUG
  std::string DebugName() const override { return "TransformLayer"; }
  std::string ToString() const override;
#endif

 private:
  std::variant<skity::Matrix, clay::TransformOperations> transform_;
  skity::Vec2 origin_;
  skity::Vec2 offset_;

  BASE_DISALLOW_COPY_AND_ASSIGN(TransformLayer);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_TRANSFORM_LAYER_H_
