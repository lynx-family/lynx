// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_TRANSFORM_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_TRANSFORM_LAYER_H_

#include <optional>
#include <string>
#include <variant>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/transform_operations.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/compositing/pending_offset_layer.h"

namespace clay {

class PendingTransformLayer : public PendingOffsetLayer {
 public:
  explicit PendingTransformLayer(const skity::Matrix& transform,
                                 const FloatPoint& offset = FloatPoint());
  PendingTransformLayer(const TransformOperations& transform,
                        const FloatPoint& transform_origin,
                        const FloatPoint& offset = FloatPoint());
  ~PendingTransformLayer() override;

  std::string GetName() const override { return "PendingTransformLayer"; }

  void SetTransform(const skity::Matrix& transform);

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override;

  std::variant<skity::Matrix, TransformOperations> transform_;
  FloatPoint transform_origin_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_TRANSFORM_LAYER_H_
