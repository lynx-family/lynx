// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/picture_raster_cache_item.h"

#include <optional>
#include <utility>

#include "clay/flow/layers/layer.h"
#include "clay/flow/raster_cache.h"
#include "clay/flow/raster_cache_item.h"
#include "clay/flow/raster_cache_key.h"
#include "clay/flow/raster_cache_util.h"

namespace clay {

static bool IsPictureWorthRasterizing(
#ifndef ENABLE_SKITY
    SkPicture* picture,
#else
    skity::DisplayList* picture,
#endif
    bool will_change, bool is_complex,
    PictureComplexityCalculator* complexity_calculator,
    skity::Vec2 frame_size) {
  if (will_change) {
    // If the display list is going to change in the future, there is no point
    // in doing to extra work to rasterize.
    return false;
  }

#ifndef ENABLE_SKITY
  auto bounds = clay::ConvertSkRectToSkityRect(picture->cullRect());
#else
  auto bounds = picture->GetBounds();
#endif  // ENABLE_SKITY
  auto width = bounds.Width();
  auto height = bounds.Height();

  if (picture == nullptr || !RasterCacheUtil::CanRasterizeRect(bounds)) {
    // No point in deciding whether the display list is worth rasterizing if it
    // cannot be rasterized at all.
    return false;
  }

  if (width > frame_size.x || height > frame_size.y) {
    // Don't cache if the display list is larger than current frame.
    return false;
  }

  if (is_complex) {
    // The caller seems to have extra information about the display list and
    // thinks the display list is always worth rasterizing.
    return true;
  }

  unsigned int complexity_score = complexity_calculator->Compute(picture);

  return complexity_calculator->ShouldBeCached(complexity_score);
}

#ifndef ENABLE_SKITY
PictureRasterCacheItem::PictureRasterCacheItem(SkPicture* picture,
                                               const skity::Vec2& offset,
                                               bool is_complex,
                                               bool will_change)
    : RasterCacheItem(
          RasterCacheKeyID(picture->uniqueID(), RasterCacheKeyType::kPicture),
          CacheState::kCurrent),
      picture_(picture),
      offset_(offset),
      is_complex_(is_complex),
      will_change_(will_change) {}

std::unique_ptr<PictureRasterCacheItem> PictureRasterCacheItem::Make(
    SkPicture* picture, const skity::Vec2& offset, bool is_complex,
    bool will_change) {
  return std::make_unique<PictureRasterCacheItem>(picture, offset, is_complex,
                                                  will_change);
}
#else
PictureRasterCacheItem::PictureRasterCacheItem(skity::DisplayList* picture,
                                               uint32_t cache_key_id,
                                               const skity::Vec2& offset,
                                               bool is_complex,
                                               bool will_change)
    : RasterCacheItem(
          RasterCacheKeyID(cache_key_id, RasterCacheKeyType::kPicture),
          CacheState::kCurrent),
      picture_(picture),
      offset_(offset),
      is_complex_(is_complex),
      will_change_(will_change) {}

std::unique_ptr<PictureRasterCacheItem> PictureRasterCacheItem::Make(
    skity::DisplayList* picture, uint32_t cache_key_id,
    const skity::Vec2& offset, bool is_complex, bool will_change) {
  return std::make_unique<PictureRasterCacheItem>(picture, cache_key_id, offset,
                                                  is_complex, will_change);
}
#endif  // ENABLE_SKITY

void PictureRasterCacheItem::PrerollSetup(PrerollContext* context,
                                          const skity::Matrix& matrix) {
  cache_state_ = CacheState::kNone;
#ifndef ENABLE_SKITY
  PictureComplexityCalculator* complexity_calculator =
      context->gr_context ? PictureComplexityCalculator::GetForBackend(
                                context->gr_context->backend())
                          : PictureComplexityCalculator::GetForSoftware();
#else
  PictureComplexityCalculator* complexity_calculator =
      PictureComplexityCalculator::GetForSoftware();
#endif  // ENABLE_SKITY

  auto frame_size = context->compositor_state
                        ? context->compositor_state->GetFrameSize()
                        : skity::Vec2{1e9, 1e9};
  // If there is a deferred decoding image, skip raster cache.
  if (!IsPictureWorthRasterizing(
          picture_,
          will_change_ || context->has_deferred_image ||
              context->has_running_picture_animation,
          is_complex_ || context->parent_has_running_transform_animation,
          complexity_calculator, frame_size)) {
    // We only deal with display lists that are worthy of rasterization.
    return;
  }

  transformation_matrix_ = matrix;
  transformation_matrix_.PreTranslate(offset_.x, offset_.y);

  if (!transformation_matrix_.Invert(nullptr)) {
    // The matrix was singular. No point in going further.
    return;
  }

  if (context->raster_cached_entries && context->raster_cache) {
    context->raster_cached_entries->push_back(this);
    cache_state_ = CacheState::kCurrent;
  }
  return;
}

void PictureRasterCacheItem::PrerollFinalize(PrerollContext* context,
                                             const skity::Matrix& matrix) {
  if (cache_state_ == CacheState::kNone || !context->raster_cache ||
      !context->raster_cached_entries) {
    return;
  }
  auto* raster_cache = context->raster_cache;
#ifndef ENABLE_SKITY
  skity::Rect bounds = clay::ConvertSkRectToSkityRect(picture_->cullRect());
#else
  skity::Rect bounds = picture_->GetBounds();
#endif
  bounds.Offset(offset_.x, offset_.y);
  bool visible = !context->state_stack.content_culled(bounds);
  auto cache_info = raster_cache->MarkSeen(key_id_, matrix, visible);
  if (!visible ||
      cache_info.accesses_since_visible <= raster_cache->access_threshold()) {
    cache_state_ = kNone;
  } else {
    // kCallerCanApplyOpacity can only be enabled when display list
    // has cached.
    // See issue: https://github.com/flutter/flutter/issues/120455
    if (cache_info.has_image) {
      context->renderable_state_flags |=
          LayerStateStack::kCallerCanApplyOpacity;
    }
    cache_state_ = kCurrent;
  }
  return;
}

static const auto* flow_type = "RasterCacheFlow::DisplayList";

bool PictureRasterCacheItem::Draw(const PaintContext& context,
                                  const clay::GrPaint* paint) const {
  return Draw(context, context.canvas, paint);
}

bool PictureRasterCacheItem::Draw(const PaintContext& context,
                                  clay::GrCanvas* canvas,
                                  const clay::GrPaint* paint) const {
  if (!context.raster_cache || !canvas) {
    return false;
  }
  if (cache_state_ == CacheState::kCurrent) {
    return context.raster_cache->Draw(key_id_, *canvas, paint);
  }
  return false;
}

bool PictureRasterCacheItem::TryToPrepareRasterCache(
    const PaintContext& context, bool parent_cached) const {
  has_been_cached_ = false;
  // If we don't have raster_cache we should not cache the current display_list.
  // If the current node's ancestor has been cached we also should not cache the
  // current node. In the current frame, the raster_cache will collect all
  // display_list or picture_list to calculate the memory they used, we
  // shouldn't cache the current node if the memory is more significant than the
  // limit.
  if (cache_state_ == kNone || !context.raster_cache || parent_cached ||
      !context.raster_cache->GenerateNewCacheInThisFrame()) {
    return false;
  }
#ifndef ENABLE_SKITY
  skity::Rect bounds = clay::ConvertSkRectToSkityRect(picture_->cullRect());
#else
  skity::Rect bounds = picture_->GetBounds();
#endif  // ENABLE_SKITY

  bounds.Offset(offset_.x, offset_.y);
  RasterCache::Context r_context = {
      // clang-format off
      .gr_context         = context.gr_context,
#ifndef ENABLE_SKITY
      .dst_color_space    = context.dst_color_space,
#endif // ENABLE_SKITY
      .matrix             = transformation_matrix_,
      .logical_rect       = bounds,
      .flow_type          = flow_type,
      // clang-format on
  };
  has_been_cached_ = context.raster_cache->UpdateCacheEntry(
      GetId().value(), r_context, [picture = picture_](clay::GrCanvas* canvas) {
#ifndef ENABLE_SKITY
        picture->playback(canvas);
#else
        picture->Draw(canvas);
#endif  // ENABLE_SKITY
      });
  return has_been_cached_;
}

}  // namespace clay
