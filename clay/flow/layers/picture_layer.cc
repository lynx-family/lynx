// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/picture_layer.h"

#include <functional>
#include <memory>
#include <utility>

#include "clay/flow/flow_rendering_backend.h"
#include "clay/flow/layers/cacheable_layer.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

#ifndef ENABLE_SKITY
PictureLayer::PictureLayer(const skity::Vec2& offset,
                           clay::GPUObject<clay::PictureSkia> picture,
                           bool is_complex, bool will_change,
                           CacheStrategy strategy, bool has_lazy_image)
    : offset_(offset),
      picture_(std::move(picture)),
      strategy_(strategy),
      has_lazy_image_(has_lazy_image),
      weak_factory_(this) {
  if (picture_.object() != nullptr) {
    bounds_ =
        clay::ConvertSkRectToSkityRect(picture_.object()->raw()->cullRect());
    bounds_.Offset(offset_.x, offset_.y);
    picture_raster_cache_item_ = PictureRasterCacheItem::Make(
        picture_.object()->raw().get(), offset_, is_complex, will_change);
  }
}
#else
PictureLayer::PictureLayer(const skity::Vec2& offset,
                           clay::GPUObject<clay::PictureSkity> picture,
                           bool is_complex, bool will_change,
                           CacheStrategy strategy, bool has_lazy_image)
    : offset_(offset),
      picture_(std::move(picture)),
      strategy_(strategy),
      has_lazy_image_(has_lazy_image),
      weak_factory_(this) {
  if (picture_.object() != nullptr) {
    skity::Rect rect = picture_.object()->raw()->GetBounds();
    rect.Offset(offset_.x, offset_.y);
    bounds_ = skity::Rect::MakeLTRB(rect.Left(), rect.Top(), rect.Right(),
                                    rect.Bottom());
    picture_raster_cache_item_ = PictureRasterCacheItem::Make(
        picture_.object()->raw().get(), picture_.object()->unique_id(), offset_,
        is_complex, will_change);
  }
}
#endif  // ENABLE_SKITY

bool PictureLayer::IsReplacing(DiffContext* context, const Layer* layer) const {
  // Only return true for identical picture; This way
  // ContainerLayer::DiffChildren can detect when a picture layer
  // got inserted between other picture layers
  auto old_layer = layer->as_picture_layer();
  return old_layer != nullptr && offset_ == old_layer->offset_ &&
         Compare(context->statistics(), this, old_layer);
}

void PictureLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  if (!context->IsSubtreeDirty()) {
#ifndef NDEBUG
    FML_DCHECK(old_layer);
    auto prev = old_layer->as_picture_layer();
    DiffContext::Statistics dummy_statistics;
    // IsReplacing has already determined that the display list is same
    FML_DCHECK(prev->offset_ == offset_ &&
               Compare(dummy_statistics, this, prev));
#endif
  }
#ifndef ENABLE_SKITY
  if (HasAnimationRunning()) {
    context->MarkSubtreeHasRasterAnimation();
    if (!context->IsSubtreeDirty()) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }
#endif  //   ENABLE_SKITY
  if (has_lazy_image_) {
    context->MarkSubtreeHasDeferredImage();
    if (!context->IsSubtreeDirty()) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }
  context->PushTransform(skity::Matrix::Translate(offset_.x, offset_.y));
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  if (context->has_raster_cache()) {
    context->SetTransform(
        RasterCacheUtil::GetIntegralTransCTM(context->GetTransform()));
  }
#endif
#ifndef ENABLE_SKITY
  auto bounds = clay::ConvertSkRectToSkityRect(picture()->cullRect());
#else
  auto bounds = picture()->GetBounds();
#endif  // ENABLE_SKITY
  context->AddLayerBounds(bounds);
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

bool PictureLayer::Compare(DiffContext::Statistics& statistics,
                           const PictureLayer* l1, const PictureLayer* l2) {
#ifndef ENABLE_SKITY
  // If text color or background color is running, just returns false.
  if (l1->HasAnimationRunning()) {
    return false;
  }
  const auto& dl1 = l1->picture_.object();
  const auto& dl2 = l2->picture_.object();
  if (dl1.get() == dl2.get() || dl1->raw() == dl2->raw() ||
      dl1->unique_id() == dl2->unique_id()) {
    statistics.AddSameInstancePicture();
    return true;
  }
  statistics.AddNewPicture();
  return false;

#else
  // TODO(feiyue:1998): Wait skity to provide more information to determine if
  // two pictures are same.
  return l1->picture() == l2->picture();
#endif  // ENABLE_SKITY
}

void PictureLayer::Preroll(PrerollContext* context) {
  context->has_running_picture_animation = HasAnimationRunning();

  std::function<void(bool)> decode_callback =
      [self = weak_factory_.GetWeakPtr(),
       request_new_frame = context->request_new_frame](bool success) {
        if (self && !self->is_empty()) {
          request_new_frame();
        }
      };

#ifndef ENABLE_SKITY
// TODO(feiyue.1998): Support lazy image logic in skia.
#else
  if (has_lazy_image_) {
    has_lazy_image_ = clay::SkityLazyImageTraveller::Traversal(picture().get(),
                                                               decode_callback);
  }
#endif  // ENABLE_SKITY

  context->has_deferred_image = has_lazy_image_;

  AutoCache cache = AutoCache(picture_raster_cache_item_.get(), context,
                              context->state_stack.transform_4x4());

  if (context->has_running_picture_animation || has_lazy_image_) {
    cache.ShouldNotBeCached();
  }

  set_paint_bounds(bounds_);
}

void PictureLayer::Paint(PaintContext& context) const {
  FML_DCHECK(picture());
  FML_DCHECK(needs_painting(context));

  auto mutator = context.state_stack.save();
  mutator.translate(offset_.x, offset_.y);

  if (strategy_ != CacheStrategy::NotCache && context.raster_cache) {
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
    // Always apply the integral transform in the presence of a raster cache
    // whether or not we successfully draw from the cache
    mutator.integralTransform();
#endif
    if (picture_raster_cache_item_) {
      clay::GrPaint paint;
      if (picture_raster_cache_item_->Draw(context,
                                           context.state_stack.fill(paint))) {
        TRACE_EVENT_INSTANT("clay", "raster cache hit");
        return;
      }
    }
  }

#ifndef ENABLE_SKITY
  if (context.enable_leaf_layer_tracing) {
    const auto canvas_size = context.canvas->getBaseLayerSize();
    auto offscreen_surface = std::make_unique<OffscreenSurface>(
        context.gr_context,
        skity::Vec2(canvas_size.fWidth, canvas_size.fHeight));

    const auto& ctm = context.canvas->getTotalMatrix();

    const auto start_time = fml::TimePoint::Now();
    {
      // render picture to offscreen surface.
      auto* canvas = offscreen_surface->GetCanvas();
      SkAutoCanvasRestore save(canvas, true);
      canvas->clear(SK_ColorTRANSPARENT);
      canvas->setMatrix(ctm);
      picture()->playback(canvas);
      canvas->flush();
    }
    const fml::TimeDelta offscreen_render_time =
        fml::TimePoint::Now() - start_time;

    const skity::Rect device_bounds = RasterCacheUtil::GetDeviceBounds(
        paint_bounds(), clay::ConvertSkMatrixToSkityMatrix(ctm));
    sk_sp<SkData> raster_data = offscreen_surface->GetRasterData(true);
    LayerSnapshotData snapshot_data(unique_id(), offscreen_render_time,
                                    raster_data, device_bounds);
    context.layer_snapshot_store->Add(snapshot_data);
  }

  picture()->playback(context.canvas);
#else
  picture()->Draw(context.canvas);
#endif  // ENABLE_SKITY

#ifndef NDEBUG
  if (context.enable_debug_borders) {
    DrawDebugBorders(context.canvas, bounds_);
  }

  if (context.enable_raster_cache_tag && picture_raster_cache_item_ &&
      picture_raster_cache_item_->has_been_cached()) {
    // Generated raster cache, but not used.
    DrawRasterCacheTag(context.canvas, paint_bounds().Width() / 2,
                       paint_bounds().Height() / 2, 0);
  }
#endif  // NDEBUG
}

}  // namespace clay
