// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_PICTURE_LAYER_H_
#define CLAY_FLOW_LAYERS_PICTURE_LAYER_H_

#include <memory>
#include <string>

#include "clay/flow/layers/layer.h"
#include "clay/flow/layers/picture_raster_cache_item.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/rendering_backend.h"
#ifdef ENABLE_SKITY
#include "clay/gfx/skity/picture_skity.h"
#endif  // ENABLE_SKITY

namespace clay {

using clay::GPUObject;
class Layer;

class PictureLayer : public Layer {
 public:
  static constexpr size_t kMaxBytesToCompare = 10000;

#ifndef ENABLE_SKITY
  PictureLayer(const skity::Vec2& offset,
               clay::GPUObject<clay::PictureSkia> picture, bool is_complex,
               bool will_change, CacheStrategy strategy = CacheStrategy::None,
               bool has_lazy_image = false);

  sk_sp<SkPicture> picture() const { return picture_.object()->raw(); }

  clay::PictureSkia* picture_skia() const { return picture_.object().get(); }

  RasterCacheKeyID caching_key_id() const override {
    return RasterCacheKeyID(picture_.object()->unique_id(),
                            RasterCacheKeyType::kPicture);
  }
#else
  PictureLayer(const skity::Vec2& offset,
               clay::GPUObject<clay::PictureSkity> picture, bool is_complex,
               bool will_change, CacheStrategy strategy = CacheStrategy::None,
               bool has_lazy_image = false);

  std::shared_ptr<skity::DisplayList> picture() const {
    return picture_.object()->raw();
  }

  clay::PictureSkity* picture_skity() const { return picture_.object().get(); }

  RasterCacheKeyID caching_key_id() const override {
    return RasterCacheKeyID(picture_.object()->unique_id(),
                            RasterCacheKeyType::kPicture);
  }
#endif  // ENABLE_SKITY

  bool IsReplacing(DiffContext* context, const Layer* layer) const override;

  void Diff(DiffContext* context, const Layer* old_layer) override;

  const PictureLayer* as_picture_layer() const override { return this; }

  void Preroll(PrerollContext* frame) override;

  void Paint(PaintContext& context) const override;

  const PictureRasterCacheItem* raster_cache_item() const {
    return picture_raster_cache_item_.get();
  }

#ifndef NDEBUG
  virtual std::string DebugName() const override { return "PictureLayer"; }
#endif

 private:
  std::unique_ptr<PictureRasterCacheItem> picture_raster_cache_item_;

  skity::Vec2 offset_;
  skity::Rect bounds_;

#ifndef ENABLE_SKITY
  clay::GPUObject<clay::PictureSkia> picture_;
#else
  clay::GPUObject<clay::PictureSkity> picture_;
#endif  // ENABLE_SKITY

  static bool Compare(DiffContext::Statistics& statistics,
                      const PictureLayer* l1, const PictureLayer* l2);

  [[maybe_unused]] CacheStrategy strategy_;
  bool has_lazy_image_ = false;

  fml::WeakPtrFactory<PictureLayer> weak_factory_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PictureLayer);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_PICTURE_LAYER_H_
