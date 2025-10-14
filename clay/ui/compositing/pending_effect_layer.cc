// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_effect_layer.h"

#include "clay/gfx/geometry/float_size.h"

namespace clay {

PendingEffectLayer::PendingEffectLayer() = default;

PendingEffectLayer::~PendingEffectLayer() = default;

void PendingEffectLayer::AddToFrame(FrameBuilder* builder,
                                    const FloatPoint& offset) {
  // Apply effects to the subtree.
  if (opacity_.has_value()) {
    builder->PushOpacity(opacity_.value(), offset_.x() + offset.x(),
                         offset_.y() + offset.y(), this);
    AddChildrenToFrame(builder);
    builder->Pop();
  } else if (color_filter_) {
    builder->PushColorFilter(color_filter_, this);
    AddChildrenToFrame(builder);
    builder->Pop();
  } else if (image_filter_) {
    builder->PushImageFilter(image_filter_, this);
    AddChildrenToFrame(builder);
    builder->Pop();
  } else if (backdrop_filter_) {
    builder->PushBackdropFilter(backdrop_filter_, this);
    AddChildrenToFrame(builder);
    builder->Pop();
  } else if (clip_rect_.has_value()) {
    FloatRect clip_rect = clip_rect_.value();
    clip_rect.Move(offset.x(), offset.y());
    int hard_edge = 1;  // Hard edge as default.
    builder->PushClipRect(clip_rect, hard_edge, this);
    AddChildrenToFrame(builder);
    builder->Pop();
  } else if (clip_rrect_.has_value()) {
    FloatRoundedRect clip_rrect = clip_rrect_.value();
    clip_rrect.Move(FloatSize(offset.x(), offset.y()));
    int anti_alias = 2;  // Anti Alias as default.
    builder->PushClipRRect(clip_rrect, anti_alias, this);
    AddChildrenToFrame(builder);
    builder->Pop();
  } else if (clip_path_.has_value()) {
    GrPath clip_path = clip_path_.value();
    int anti_alias = 2;
    builder->PushClipPath(clip_path, anti_alias, this);
    AddChildrenToFrame(builder);
    builder->Pop();
  } else if (color_source_) {
    FloatRect mask_rect = mask_rect_.value();
    mask_rect.Move(offset.x(), offset.y());
    builder->PushShaderMask(color_source_, mask_rect, blend_mode_, this);
    AddChildrenToFrame(builder);
    builder->Pop();
  }
}

#ifndef NDEBUG
std::string PendingEffectLayer::ToString() const {
  std::stringstream ss;
  ss << PendingLayer::ToString();
  ss << " effect_offset_=" << offset_.ToString();
  return ss.str();
}
#endif

}  // namespace clay
