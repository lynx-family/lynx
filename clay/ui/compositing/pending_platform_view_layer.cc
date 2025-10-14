// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_platform_view_layer.h"

#include "clay/ui/compositing/frame_builder.h"

namespace clay {

PendingPlatformViewLayer::PendingPlatformViewLayer(float dx, float dy,
                                                   float width, float height,
                                                   int64_t view_id)
    : dx_(dx), dy_(dy), width_(width), height_(height), view_id_(view_id) {}

PendingPlatformViewLayer::~PendingPlatformViewLayer() = default;

void PendingPlatformViewLayer::AddToFrame(FrameBuilder* builder,
                                          const FloatPoint& offset) {
  builder->AddPlatformView(offset.x() + dx_, offset.y() + dy_, width_, height_,
                           view_id_);
}

#ifndef NDEBUG
std::string PendingPlatformViewLayer::ToString() const {
  std::stringstream ss;
  ss << PendingLayer::ToString();
  ss << " platform_view_id=" << view_id_;
  ss << " platform_view_offset=(" << dx_ << "," << dy_ << ")";
  ss << " platform_view_size=(" << width_ << "," << height_ << ")";
  return ss.str();
}
#endif

}  // namespace clay
