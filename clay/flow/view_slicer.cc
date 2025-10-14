// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/view_slicer.h"

#include <list>

#include "clay/flow/embedded_views.h"
#include "clay/fml/logging.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

std::unordered_map<int64_t, skity::Rect> SliceViews(
    clay::GrCanvas* background_canvas,
    const std::vector<int64_t>& composition_order,
    const std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>>&
        slices,
    const std::unordered_map<int64_t, skity::Rect>& view_rects) {
  std::unordered_map<int64_t, skity::Rect> overlay_layers;

  auto current_frame_view_count = composition_order.size();

  // Restore the clip context after exiting this method since it's changed
  // below.

  clay::GrAutoCanvasRestore save_restore(background_canvas, true);

  for (size_t i = 0; i < current_frame_view_count; i++) {
    int64_t view_id = composition_order[i];
    EmbedderViewSlice* slice = slices.at(view_id).get();
    if (slice->canvas() == nullptr) {
      continue;
    }

    slice->end_recording();

    skity::Rect full_joined_rect = skity::Rect::MakeEmpty();

    // Determinate if Flutter UI intersects with any of the previous
    // platform views stacked by z position.
    //
    // This is done by querying the r-tree that holds the records for the
    // picture recorder corresponding to the flow layers added after a platform
    // view layer.
    for (int j = i; j >= 0; j--) {
      int64_t current_view_id = composition_order[j];
      auto maybe_rect = view_rects.find(current_view_id);
      FML_DCHECK(maybe_rect != view_rects.end());
      if (maybe_rect == view_rects.end()) {
        continue;
      }

      skity::Rect current_view_rect = maybe_rect->second;
      skity::Rect rounded_in_platform_view_rect = current_view_rect;
      rounded_in_platform_view_rect.RoundIn();

      // Each rect corresponds to a native view that renders Flutter UI.
      std::list<skity::Rect> intersection_rects =
          slice->searchNonOverlappingDrawnRects(current_view_rect);

      // Ignore intersections of single width/height on the edge of the platform
      // view.
      // This is to address the following performance issue when interleaving
      // adjacent platform views and layers: Since we `roundOut` both platform
      // view rects and the layer rects, as long as the coordinate is
      // fractional, there will be an intersection of a single pixel width (or
      // height) after rounding out, even if they do not intersect before
      // rounding out. We have to round out both platform view rect and the
      // layer rect. Rounding in platform view rect will result in missing pixel
      // on the intersection edge. Rounding in layer rect will result in missing
      // pixel on the edge of the layer on top of the platform view.
      for (auto it = intersection_rects.begin(); it != intersection_rects.end();
           /*no-op*/) {
        // If intersection_rect does not intersect with the *rounded in*
        // platform view rect, then the intersection must be a single pixel
        // width (or height) on edge.
        if (!skity::Rect::Intersect(*it, rounded_in_platform_view_rect)) {
          it = intersection_rects.erase(it);
        } else {
          ++it;
        }
      }

      // Limit the number of native views, so it doesn't grow forever.
      //
      // In this case, the rects are merged into a single one that is the union
      // of all the rects.
      skity::Rect partial_joined_rect = skity::Rect::MakeEmpty();
      for (const skity::Rect& rect : intersection_rects) {
        partial_joined_rect.Join(rect);
      }

      current_view_rect.RoundOut();
      // Get the intersection rect with the `current_view_rect`,
      if (partial_joined_rect.Intersect(current_view_rect)) {
        // Join the `partial_joined_rect` into `full_joined_rect` to get the
        // rect above the current `slice`, only if it intersects the indicated
        // view. This should always be the case because we just deleted any
        // rects that don't intersect the "rounded-in" view, so they must
        // all intersect the "rounded-out" view (or the partial join could
        // be empty in which case this would be a NOP). Either way, the
        // penalty for not checking the return value of the intersect method
        // would be to join a non-overlapping rectangle into the overlay
        // bounds - if the above implementation ever changes - so we check it.
        full_joined_rect.Join(partial_joined_rect);
      }
    }

    if (!full_joined_rect.IsEmpty()) {
      // Subpixels in the platform may not align with the canvas subpixels.
      //
      // To workaround it, round the floating point bounds and make the rect
      // slightly larger.
      //
      // For example, {0.3, 0.5, 3.1, 4.7} becomes {0, 0, 4, 5}.
      full_joined_rect.RoundOut();
      overlay_layers.insert({view_id, full_joined_rect});

      // Clip the background canvas, so it doesn't contain any of the pixels
      // drawn on the overlay layer.
      CANVAS_CLIP_RECT_WITH_OP(background_canvas, full_joined_rect,
                               clay::GrClipOp::kDifference);
    }
    slice->render_into(background_canvas);
  }

  // Manually trigger the DlAutoCanvasRestore before we submit the frame
  save_restore.restore();

  return overlay_layers;
}

}  // namespace clay
