// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/layer.h"

#include "clay/flow/animation/animation_host.h"
#include "clay/flow/animation/animation_mutator.h"
#include "clay/flow/paint_utils.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

Layer::Layer()
    : paint_bounds_(skity::Rect::MakeEmpty()),
      unique_id_(NextUniqueID()),
      original_layer_id_(unique_id_),
      subtree_has_platform_view_(false),
      subtree_has_punch_hole_(false) {}

Layer::~Layer() = default;

uint64_t Layer::NextUniqueID() {
  static std::atomic<uint64_t> next_id(1);
  uint64_t id;
  do {
    id = next_id.fetch_add(1);
  } while (id == 0);  // 0 is reserved for an invalid id.
  return id;
}

void Layer::PreservePaintRegion(DiffContext* context) {
  // retained layer means same instance so 'this' is used to index into both
  // current and old region
  context->SetLayerPaintRegion(this, context->GetOldLayerPaintRegion(this));
}

Layer::AutoPrerollSaveLayerState::AutoPrerollSaveLayerState(
    PrerollContext* preroll_context, bool save_layer_is_active,
    bool layer_itself_performs_readback)
    : preroll_context_(preroll_context),
      save_layer_is_active_(save_layer_is_active),
      layer_itself_performs_readback_(layer_itself_performs_readback) {
  if (save_layer_is_active_) {
    prev_surface_needs_readback_ = preroll_context_->surface_needs_readback;
    preroll_context_->surface_needs_readback = false;
  }
}

Layer::AutoPrerollSaveLayerState Layer::AutoPrerollSaveLayerState::Create(
    PrerollContext* preroll_context, bool save_layer_is_active,
    bool layer_itself_performs_readback) {
  return Layer::AutoPrerollSaveLayerState(preroll_context, save_layer_is_active,
                                          layer_itself_performs_readback);
}

Layer::AutoPrerollSaveLayerState::~AutoPrerollSaveLayerState() {
  if (save_layer_is_active_) {
    preroll_context_->surface_needs_readback =
        (prev_surface_needs_readback_ || layer_itself_performs_readback_);
  }
}

bool Layer::HasAnimation() const {
  return animation_host_ && animation_host_->HasAnimation(unique_id());
}

bool Layer::HasAnimationRunning() const {
  return animation_host_ && animation_host_->HasAnimationRunning(unique_id());
}

const std::shared_ptr<AnimationMutator>& Layer::GetAnimationMutator() const {
  return animation_host_->GetAnimationMutator(unique_id());
}

void Layer::SetAnimationHost(
    const std::shared_ptr<AnimationHost>& animation_host) {
  animation_host_ = animation_host;
}

#ifndef NDEBUG
std::string Layer::ToString() const {
  std::stringstream ss;
  ss << " unique_id_=" << unique_id_;
  if (unique_id_ != original_layer_id_) {
    ss << " original_layer_id_=" << original_layer_id_;
  }
  // if (needs_system_composite_) {
  //   ss << " needs_system_composite_=" << needs_system_composite_;
  // }
  ss << " paint_bounds_=(" << paint_bounds_.Left() << "," << paint_bounds_.Top()
     << "," << paint_bounds_.Width() << "," << paint_bounds_.Height() << ")";
  return ss.str();
}

void Layer::DebugDumpTree(int depth) const {
  std::string indent;
  indent.append(2 * depth, ' ');
  FML_LOG(ERROR) << indent << "[" << DebugName() << "] -" << ToString();
}
#endif
}  // namespace clay
