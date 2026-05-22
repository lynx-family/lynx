// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_drawable_image_layer.h"

#include "clay/ui/compositing/frame_builder.h"

namespace clay {

PendingDrawableImageLayer::PendingDrawableImageLayer(
    float dx, float dy, float width, float height, int64_t image_id,
    clay::DrawableImage::FitMode fit_mode)
    : dx_(dx),
      dy_(dy),
      width_(width),
      height_(height),
      image_id_(image_id),
      fit_mode_(fit_mode) {}

PendingDrawableImageLayer::~PendingDrawableImageLayer() = default;

void PendingDrawableImageLayer::AddToFrame(FrameBuilder* builder,
                                           const FloatPoint& offset) {
  builder->AddDrawableImage(offset.x() + dx_, offset.y() + dy_, width_, height_,
                            image_id_, fit_mode_);
}

#ifndef NDEBUG
std::string PendingDrawableImageLayer::ToString() const {
  std::stringstream ss;
  ss << PendingLayer::ToString();
  ss << " image_id_=" << image_id_;
  ss << " texture_offset=(" << dx_ << "," << dy_ << ")";
  ss << " texture_size=(" << width_ << "," << height_ << ")";
  return ss.str();
}
#endif

}  // namespace clay
