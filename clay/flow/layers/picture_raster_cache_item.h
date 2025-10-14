// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_PICTURE_RASTER_CACHE_ITEM_H_
#define CLAY_FLOW_LAYERS_PICTURE_RASTER_CACHE_ITEM_H_

#include <memory>
#include <optional>

#include "clay/flow/embedded_views.h"
#include "clay/flow/raster_cache_item.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class PictureRasterCacheItem : public RasterCacheItem {
 public:
  void PrerollSetup(PrerollContext* context,
                    const skity::Matrix& matrix) override;

  void PrerollFinalize(PrerollContext* context,
                       const skity::Matrix& matrix) override;

#ifndef ENABLE_SKITY
  PictureRasterCacheItem(SkPicture* picture, const skity::Vec2& offset,
                         bool is_complex = true, bool will_change = false);

  static std::unique_ptr<PictureRasterCacheItem> Make(SkPicture*,
                                                      const skity::Vec2& offset,
                                                      bool is_complex,
                                                      bool will_change);
#else
  PictureRasterCacheItem(skity::DisplayList* display_list,
                         uint32_t cache_key_id, const skity::Vec2& offset,
                         bool is_complex = true, bool will_change = false);

  static std::unique_ptr<PictureRasterCacheItem> Make(skity::DisplayList*,
                                                      uint32_t cache_key_id,
                                                      const skity::Vec2& offset,
                                                      bool is_complex,
                                                      bool will_change);
#endif  // ENABLE_SKITY

  bool Draw(const PaintContext& context,
            const clay::GrPaint* paint) const override;

  bool Draw(const PaintContext& context, clay::GrCanvas* canvas,
            const clay::GrPaint* paint) const override;

  bool TryToPrepareRasterCache(const PaintContext& context,
                               bool parent_cached = false) const override;

  void ModifyMatrix(skity::Vec2 offset) const {
    matrix_ = matrix_.PreTranslate(offset.x, offset.y);
  }

#ifndef ENABLE_SKITY
  const SkPicture* picture() const { return picture_; }
#else
  const skity::DisplayList* picture() const { return picture_; }
#endif

 private:
  skity::Matrix transformation_matrix_;
#ifndef ENABLE_SKITY
  SkPicture* picture_;
#else
  skity::DisplayList* picture_;
#endif
  skity::Vec2 offset_;
  bool is_complex_;
  bool will_change_;
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_PICTURE_RASTER_CACHE_ITEM_H_
