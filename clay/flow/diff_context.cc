// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/diff_context.h"

#include <inttypes.h>

#include <algorithm>

#include "clay/flow/layers/layer.h"

namespace clay {

DiffContext::DiffContext(skity::Vec2 frame_size,
                         double frame_device_pixel_ratio,
                         PaintRegionMap& this_frame_paint_region_map,
                         const PaintRegionMap& last_frame_paint_region_map,
                         bool has_raster_cache)
    : rects_(std::make_shared<std::vector<skity::Rect>>()),
      frame_size_(frame_size),
      frame_device_pixel_ratio_(frame_device_pixel_ratio),
      this_frame_paint_region_map_(this_frame_paint_region_map),
      last_frame_paint_region_map_(last_frame_paint_region_map),
      has_raster_cache_(has_raster_cache) {}

void DiffContext::BeginSubtree() {
  state_stack_.push_back(state_);
  state_.rect_index_ = rects_->size();
  state_.has_filter_bounds_adjustment = false;
  state_.has_drawable_image = false;
  state_.has_animation = false;
  state_.has_deferred_image = false;
  if (state_.transform_override) {
    state_.transform = *state_.transform_override;
    state_.transform_override = std::nullopt;
  }
}

void DiffContext::EndSubtree() {
  FML_DCHECK(!state_stack_.empty());
  if (state_.has_filter_bounds_adjustment) {
    filter_bounds_adjustment_stack_.pop_back();
  }
  state_ = state_stack_.back();
  state_stack_.pop_back();
}

DiffContext::State::State()
    : dirty(false),
      cull_rect(kGiantRect),
      rect_index_(0),
      has_filter_bounds_adjustment(false),
      has_drawable_image(false),
      has_animation(false) {}

void DiffContext::PushTransform(const skity::Matrix& transform) {
  state_.transform.PreConcat(transform);
}

void DiffContext::SetTransform(const skity::Matrix& transform) {
  state_.transform_override = transform;
}

void DiffContext::PushFilterBoundsAdjustment(
    const FilterBoundsAdjustment& filter) {
  FML_DCHECK(state_.has_filter_bounds_adjustment == false);
  state_.has_filter_bounds_adjustment = true;
  filter_bounds_adjustment_stack_.push_back(filter);
}

skity::Rect DiffContext::ApplyFilterBoundsAdjustment(skity::Rect rect) const {
  // Apply filter bounds adjustment in reverse order
  for (auto i = filter_bounds_adjustment_stack_.rbegin();
       i != filter_bounds_adjustment_stack_.rend(); ++i) {
    rect = (*i)(rect);
  }
  return rect;
}

void DiffContext::AlignRect(skity::Rect& rect, int horizontal_alignment,
                            int vertical_alignment) const {
  int top = rect.Top();
  int left = rect.Left();
  int right = rect.Right();
  int bottom = rect.Bottom();
  if (top % vertical_alignment != 0) {
    top -= top % vertical_alignment;
  }
  if (left % horizontal_alignment != 0) {
    left -= left % horizontal_alignment;
  }
  if (right % horizontal_alignment != 0) {
    right += horizontal_alignment - right % horizontal_alignment;
  }
  if (bottom % vertical_alignment != 0) {
    bottom += vertical_alignment - bottom % vertical_alignment;
  }
  right = std::min(right, static_cast<int>(frame_size_.x));
  bottom = std::min(bottom, static_cast<int>(frame_size_.y));
  rect = skity::Rect::MakeLTRB(left, top, right, bottom);
}

Damage DiffContext::ComputeDamage(const skity::Rect& accumulated_buffer_damage,
                                  int horizontal_clip_alignment,
                                  int vertical_clip_alignment) const {
  Damage res;
  skity::Rect& buffer_damage = res.buffer_damage;
  buffer_damage = accumulated_buffer_damage;
  buffer_damage.Join(damage_);
  skity::Rect& frame_damage = res.frame_damage;
  frame_damage = damage_;

  for (const auto& r : readbacks_) {
    skity::Rect rect = r.rect;
    if (skity::Rect::Intersect(rect, frame_damage)) {
      frame_damage.Join(rect);
    }
    if (skity::Rect::Intersect(rect, buffer_damage)) {
      buffer_damage.Join(rect);
    }
  }

  buffer_damage.RoundOut();
  frame_damage.RoundOut();

  skity::Rect frame_clip = skity::Rect::MakeSize(frame_size_);
  buffer_damage.Intersect(frame_clip);
  frame_damage.Intersect(frame_clip);

  if (horizontal_clip_alignment > 1 || vertical_clip_alignment > 1) {
    AlignRect(buffer_damage, horizontal_clip_alignment,
              vertical_clip_alignment);
    AlignRect(frame_damage, horizontal_clip_alignment, vertical_clip_alignment);
  }
  return res;
}

bool DiffContext::PushCullRect(const skity::Rect& clip) {
  skity::Rect cull_rect = state_.transform.MapRect(clip);
  return state_.cull_rect.Intersect(cull_rect);
}

skity::Rect DiffContext::GetCullRect() const {
  skity::Matrix inverse_transform;
  // Perspective projections don't produce rectangles that are useful for
  // culling for some reason.
  if (!state_.transform.HasPersp() &&
      state_.transform.Invert(&inverse_transform)) {
    return inverse_transform.MapRect(state_.cull_rect);
  } else {
    return kGiantRect;
  }
}

void DiffContext::MarkSubtreeDirty(const PaintRegion& previous_paint_region) {
  FML_DCHECK(!IsSubtreeDirty());
  if (previous_paint_region.is_valid()) {
    AddDamage(previous_paint_region);
  }
  state_.dirty = true;
}

void DiffContext::MarkSubtreeDirty(const skity::Rect& previous_paint_region) {
  FML_DCHECK(!IsSubtreeDirty());
  AddDamage(previous_paint_region);
  state_.dirty = true;
}

void DiffContext::AddLayerBounds(const skity::Rect& rect) {
  // During painting we cull based on non-overriden transform and then
  // override the transform right before paint. Do the same thing here to get
  // identical paint rect.
  auto transformed_rect =
      ApplyFilterBoundsAdjustment(state_.transform.MapRect(rect));
  if (skity::Rect::Intersect(transformed_rect, state_.cull_rect)) {
    auto paint_rect = state_.transform_override
                          ? ApplyFilterBoundsAdjustment(
                                state_.transform_override->MapRect(rect))
                          : transformed_rect;
    paint_rect.Intersect(state_.cull_rect);
    rects_->push_back(paint_rect);
    if (IsSubtreeDirty()) {
      AddDamage(paint_rect);
    }
  }
}

void DiffContext::MarkSubtreeHasDrawableImageLayer() {
  // Set the has_drawable_image flag on current state and all parent states.
  // That way we'll know that we can't skip diff for retained layers because
  // they contain a DrawableImageLayer.
  for (auto& state : state_stack_) {
    state.has_drawable_image = true;
  }
  state_.has_drawable_image = true;
}

void DiffContext::MarkSubtreeHasDeferredImage() {
  for (auto& state : state_stack_) {
    state.has_deferred_image = true;
  }
  state_.has_deferred_image = true;
}

void DiffContext::MarkSubtreeHasRasterAnimation() {
  for (auto& state : state_stack_) {
    state.has_animation = true;
  }
  state_.has_animation = true;
}

void DiffContext::AddExistingPaintRegion(const PaintRegion& region) {
  // Adding paint region for retained layer implies that current subtree is not
  // dirty, so we know, for example, that the inherited transforms must match
  FML_DCHECK(!IsSubtreeDirty());
  if (region.is_valid()) {
    rects_->insert(rects_->end(), region.begin(), region.end());
  }
}

void DiffContext::AddReadbackRegion(const skity::Rect& rect) {
  Readback readback;
  readback.rect = rect;
  readback.position = rects_->size();
  // Push empty rect as a placeholder for position in current subtree
  rects_->push_back(skity::Rect::MakeEmpty());
  readbacks_.push_back(readback);
}

PaintRegion DiffContext::CurrentSubtreeRegion() const {
  bool has_readback = std::any_of(
      readbacks_.begin(), readbacks_.end(),
      [&](const Readback& r) { return r.position >= state_.rect_index_; });
  return PaintRegion(rects_, state_.rect_index_, rects_->size(), has_readback,
                     state_.has_drawable_image, state_.has_animation,
                     state_.has_deferred_image);
}

void DiffContext::AddDamage(const PaintRegion& damage) {
  FML_DCHECK(damage.is_valid());
  for (const auto& r : damage) {
    damage_.Join(r);
  }
}

void DiffContext::AddDamage(const skity::Rect& rect) { damage_.Join(rect); }

void DiffContext::SetLayerPaintRegion(const Layer* layer,
                                      const PaintRegion& region) {
  this_frame_paint_region_map_[layer->unique_id()] = region;
}

PaintRegion DiffContext::GetOldLayerPaintRegion(const Layer* layer) const {
  auto i = last_frame_paint_region_map_.find(layer->unique_id());
  if (i != last_frame_paint_region_map_.end()) {
    return i->second;
  } else {
    // This is valid when Layer::PreservePaintRegion is called for retained
    // layer with zero sized parent clip (these layers are not diffed)
    return PaintRegion();
  }
}

void DiffContext::Statistics::LogStatistics() {
#if ENABLE_TRACE_PERFETTO
  auto trace_func = [&](const char* name, size_t count) {
    char buf[128] = {0};
    std::sprintf(buf, "DiffContext.%s_%" PRIxPTR, name,
                 reinterpret_cast<uintptr_t>(this));
    TRACE_COUNTER("clay", lynx::perfetto::CounterTrack(buf), count);
  };
  trace_func("NewPictures", new_pictures_);
  trace_func("PicturesTooComplexToCompare", pictures_too_complex_to_compare_);
  trace_func("DeepComparePictures", deep_compare_pictures_);
  trace_func("SameInstancePictures", same_instance_pictures_);
  trace_func("DifferentInstanceButEqualPictures",
             different_instance_but_equal_pictures_);
#endif  // !FLUTTER_RELEASE
}

}  // namespace clay
