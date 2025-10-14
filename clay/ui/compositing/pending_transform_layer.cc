// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_transform_layer.h"

#include <variant>

#include "clay/gfx/geometry/transform_operations.h"

namespace clay {

PendingTransformLayer::PendingTransformLayer(const skity::Matrix& transform,
                                             const FloatPoint& offset)
    : PendingOffsetLayer(offset), transform_(transform) {}

PendingTransformLayer::PendingTransformLayer(
    const TransformOperations& transform, const FloatPoint& origin,
    const FloatPoint& offset)
    : PendingOffsetLayer(offset),
      transform_(transform),
      transform_origin_(origin) {}

PendingTransformLayer::~PendingTransformLayer() = default;

void PendingTransformLayer::AddToFrame(FrameBuilder* builder,
                                       const FloatPoint& offset) {
  FloatPoint total_offset = offset + Offset();

  if (std::holds_alternative<skity::Matrix>(transform_)) {
    skity::Matrix matrix = std::get<skity::Matrix>(transform_);
    if (total_offset != FloatPoint()) {
      matrix.PreTranslate(total_offset.x(), total_offset.y());
    }
    builder->PushStaticTransform(matrix, this);
  } else if (std::holds_alternative<TransformOperations>(transform_)) {
    builder->PushTransformOperations(
        std::get<TransformOperations>(transform_), transform_origin_.x(),
        transform_origin_.y(), total_offset.x(), total_offset.y(), this);
  } else {
    return;
  }
  AddChildrenToFrame(builder, FloatPoint());
  builder->Pop();
}

void PendingTransformLayer::SetTransform(const skity::Matrix& transform) {
  transform_.emplace<0>(transform);
}

#ifndef NDEBUG
std::string PendingTransformLayer::ToString() const {
  std::stringstream ss;
  skity::Matrix transform;
  if (std::holds_alternative<skity::Matrix>(transform_)) {
    transform = std::get<skity::Matrix>(transform_);
  } else if (std::holds_alternative<TransformOperations>(transform_)) {
    transform = std::get<TransformOperations>(transform_).Apply().matrix();
  }
  ss << PendingOffsetLayer::ToString();
  ss << " translate=(" << transform.Get(0, 3) << "," << transform.Get(1, 3)
     << ")";
  ss << " scale=(" << transform.Get(0, 0) << "," << transform.Get(1, 1) << ")";
  return ss.str();
}
#endif

}  // namespace clay
