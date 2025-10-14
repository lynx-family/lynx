// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_RASTER_CACHE_UTIL_H_
#define CLAY_FLOW_RASTER_CACHE_UTIL_H_

#include <cmath>

#include "clay/fml/logging.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/matrix.hpp"
#include "skity/geometry/rect.hpp"

namespace clay {

struct RasterCacheUtil {
  // The default max number of picture and display list raster caches to be
  // generated per frame. Generating too many caches in one frame may cause jank
  // on that frame. This limit allows us to throttle the cache and distribute
  // the work across multiple frames.
  static constexpr int kDefaultPictureAndDispLayListCacheLimitPerFrame = 4;

  // The ImageFilterLayer might cache the filtered output of this layer
  // if the layer remains stable (if it is not animating for instance).
  // If the ImageFilterLayer is not the same between rendered frames,
  // though, it will cache its children instead and filter their cached
  // output on the fly.
  // Caching just the children saves the time to render them and also
  // avoids a rendering surface switch to draw them.
  // Caching the layer itself avoids all of that and additionally avoids
  // the cost of applying the filter, but can be worse than caching the
  // children if the filter itself is not stable from frame to frame.
  // This constant controls how many times we will Preroll and Paint this
  // same ImageFilterLayer before we consider the layer and filter to be
  // stable enough to switch from caching the children to caching the
  // filtered output of this layer.
  static constexpr int kMinimumRendersBeforeCachingFilterLayer = 3;

  static bool CanRasterizeRect(const skity::Rect& cull_rect) {
    if (cull_rect.IsEmpty()) {
      // No point in ever rasterizing an empty display list.
      return false;
    }

    if (!cull_rect.IsFinite()) {
      // Cannot attempt to rasterize into an infinitely large surface.
      FML_DLOG(INFO) << "Attempted to raster cache non-finite display list";
      return false;
    }

    return true;
  }

  static skity::Rect GetDeviceBounds(const skity::Rect& rect,
                                     const skity::Matrix& ctm) {
    skity::Rect device_rect;
    ctm.MapRect(&device_rect, rect);
    return device_rect;
  }

  static skity::Rect GetRoundedOutDeviceBounds(const skity::Rect& rect,
                                               const skity::Matrix& ctm) {
    skity::Rect device_rect;
    ctm.MapRect(&device_rect, rect);
    device_rect.RoundOut();
    return device_rect;
  }

  /**
   * @brief Snap the translation components of the matrix to integers.
   *
   * The snapping will only happen if the matrix only has scale and translation
   * transformations. This is used, along with GetRoundedOutDeviceBounds, to
   * ensure that the textures drawn by the raster cache are exactly aligned to
   * physical pixels. Any layers that participate in raster caching must align
   * themselves to physical pixels even when not cached to prevent a change in
   * apparent location if caching is later applied.
   *
   * @param ctm the current transformation matrix.
   * @return skity::Matrix the snapped transformation matrix.
   */
  static skity::Matrix GetIntegralTransCTM(const skity::Matrix& ctm) {
    // Avoid integral snapping if the matrix has complex transformation to avoid
    // the artifact observed in https://github.com/flutter/flutter/issues/41654.
    if (ctm.Get(0, 1) != 0 || ctm.Get(0, 2) != 0) {
      // X multiplied by either Y or Z
      return ctm;
    }
    if (ctm.Get(1, 0) != 0 || ctm.Get(1, 2) != 0) {
      // Y multiplied by either X or Z
      return ctm;
    }
    // We do not need to worry about the Z row unless the W row
    // has perspective entries...
    if (ctm.Get(3, 0) != 0 || ctm.Get(3, 1) != 0 || ctm.Get(3, 2) != 0 ||
        ctm.Get(3, 3) != 1) {
      // W not identity row, therefore perspective is applied
      return ctm;
    }

    skity::Matrix result = ctm;
    result.Set(0, 3, roundf(ctm.Get(0, 3)));
    result.Set(1, 3, roundf(ctm.Get(1, 3)));
    // No need to worry about Z translation because it has no effect
    // without perspective entries...
    return result;
  }

  /**
   * Calculates the uniform scale factor from a similarity transformation
   * matrix.
   *
   * @param similarity_ctm A transformation matrix that must be a similarity
   * transform
   * @return The absolute scale factor of the transformation
   */
  static float GetScaleFactor(const skity::Matrix& similarity_ctm) {
    FML_DCHECK(similarity_ctm.IsSimilarity());

    // For simple cases where the matrix only contains scale and translate
    if (similarity_ctm.OnlyScaleAndTranslate()) {
      // The scale factor is directly available in the ScaleX component
      return similarity_ctm.GetScaleX();
    } else {
      // For matrices that include rotation:
      // In a similarity transform with rotation, the scale factor is the
      // length of the transformed unit vector, which can be calculated
      // using the Pythagorean theorem on the scale and skew components
      float scale_x = similarity_ctm.GetScaleX();
      float skew_y = similarity_ctm.GetSkewY();
      return std::sqrt(scale_x * scale_x + skew_y * skew_y);
    }
  }

  /**
   * @brief Check if the matrix is a similarity matrix and the scale factor is
   * positive.
   *
   * The matrix is a similarity matrix if it has only translation, uniform scale
   * and rotation transformations.
   * This method is important for raster caching because similarity transforms
   * are "friendly" for cached images - they maintain the visual quality of the
   * cached content since they don't distort the shape. This helps ensure that
   * using cached content will still look good when transformed.
   *
   * @param ctm the current transformation matrix.
   * @return true if the matrix is a similarity matrix and the scale factor is
   * positive.
   */
  static bool IsMatrixSimilarity(const skity::Matrix& ctm) {
    return ctm.IsSimilarity() && GetScaleFactor(ctm) > 0.f;
  }
};

}  // namespace clay

#endif  // CLAY_FLOW_RASTER_CACHE_UTIL_H_
