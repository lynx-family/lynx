// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/frame_surface_layer.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/flow/layers/layer_tree.h"

namespace clay {

FrameSurfaceLayer::FrameSurfaceLayer(std::shared_ptr<FrameSurface> surface,
                                     const skity::Rect& rect)
    : surface_(std::move(surface)), rect_(rect) {
  FML_DCHECK(surface_);
}

FrameSurfaceLayer::~FrameSurfaceLayer() = default;

bool FrameSurfaceLayer::IsReplacing(DiffContext* context,
                                    const Layer* old_layer) const {
  return old_layer && old_layer->as_frame_surface_layer() != nullptr;
}

void FrameSurfaceLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(old_layer);
    auto* prev = old_layer->as_frame_surface_layer();
    const auto& current_surface_id = surface_id();
    const bool can_use_child_damage =
        prev && rect_ == prev->rect_ &&
        current_surface_id.element_id() == prev->surface_id().element_id() &&
        current_surface_id.incarnation() == prev->surface_id().incarnation() &&
        current_surface_id.size_generation() ==
            prev->surface_id().size_generation();
    if (!can_use_child_damage) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    } else if (current_surface_id != prev->surface_id() &&
               !AddMappedSurfaceDamage(context)) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }

  context->AddLayerBounds(rect_);
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void FrameSurfaceLayer::Preroll(PrerollContext* context) {
  auto child_root =
      surface_->layer_tree() ? surface_->layer_tree()->root_layer() : nullptr;
  if (child_root) {
    const skity::Vec2 child_frame_size = surface_->layer_tree()->frame_size();
    const bool has_child_frame_size =
        child_frame_size.x > 0.f && child_frame_size.y > 0.f;
    auto mutator = context->state_stack.save();
    mutator.translate(rect_.Left(), rect_.Top());
    mutator.clipRect(LocalSurfaceRect(), false);
    if (has_child_frame_size) {
      mutator.transform(
          skity::Matrix::Scale(rect_.Width() / child_frame_size.x,
                               rect_.Height() / child_frame_size.y));
    }
    child_root->Preroll(context);
    child_root->set_paint_bounds(has_child_frame_size
                                     ? skity::Rect::MakeSize(child_frame_size)
                                     : LocalSurfaceRect());
  }
  set_paint_bounds(rect_);
  context->renderable_state_flags = LayerStateStack::kCallerCanApplyOpacity;
}

void FrameSurfaceLayer::Paint(PaintContext& context) const {
  TRACE_EVENT("clay", "FrameSurfaceLayer::Paint");
  FML_DCHECK(needs_painting(context));
  auto child_root =
      surface_->layer_tree() ? surface_->layer_tree()->root_layer() : nullptr;
  if (!child_root) {
    return;
  }

  const skity::Vec2 child_frame_size = surface_->layer_tree()->frame_size();
  const bool has_child_frame_size =
      child_frame_size.x > 0.f && child_frame_size.y > 0.f;
  auto mutator = context.state_stack.save();
  mutator.translate(rect_.Left(), rect_.Top());
  mutator.clipRect(LocalSurfaceRect(), false);
  if (has_child_frame_size) {
    mutator.transform(
        skity::Matrix::Scale(rect_.Width() / child_frame_size.x,
                             rect_.Height() / child_frame_size.y));
  }
  child_root->set_paint_bounds(has_child_frame_size
                                   ? skity::Rect::MakeSize(child_frame_size)
                                   : LocalSurfaceRect());
  child_root->Paint(context);
}

skity::Rect FrameSurfaceLayer::LocalSurfaceRect() const {
  return skity::Rect::MakeWH(rect_.Width(), rect_.Height());
}

bool FrameSurfaceLayer::AddMappedSurfaceDamage(DiffContext* context) const {
  if (!surface_->layer_tree()) {
    return false;
  }

  const skity::Vec2 child_frame_size = surface_->layer_tree()->frame_size();
  if (child_frame_size.x <= 0.f || child_frame_size.y <= 0.f ||
      rect_.Width() <= 0.f || rect_.Height() <= 0.f) {
    return false;
  }

  skity::Rect child_damage =
      surface_->damage_rect().value_or(skity::Rect::MakeSize(child_frame_size));
  child_damage.Intersect(skity::Rect::MakeSize(child_frame_size));
  if (child_damage.IsEmpty()) {
    return true;
  }

  const float scale_x = rect_.Width() / child_frame_size.x;
  const float scale_y = rect_.Height() / child_frame_size.y;
  context->AddLocalDamageRect(
      skity::Rect::MakeLTRB(rect_.Left() + child_damage.Left() * scale_x,
                            rect_.Top() + child_damage.Top() * scale_y,
                            rect_.Left() + child_damage.Right() * scale_x,
                            rect_.Top() + child_damage.Bottom() * scale_y));
  return true;
}

}  // namespace clay
