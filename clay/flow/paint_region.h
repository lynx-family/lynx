// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_FLOW_PAINT_REGION_H_
#define CLAY_FLOW_PAINT_REGION_H_

#include <memory>
#include <vector>

#include "clay/fml/logging.h"
#include "skity/geometry/rect.hpp"

namespace clay {

// Corresponds to area on the screen where the layer subtree has painted to.
//
// The area is used when adding damage of removed or dirty layer to overall
// damage.
//
// Because there is a PaintRegion for each layer, it must be able to represent
// the area with minimal overhead. This is accomplished by having one
// vector<skity::Rect> shared between all paint regions, and each paint region
// keeping begin and end index of rects relevant to particular subtree.
//
// All rects are in screen coordinates.
class PaintRegion {
 public:
  PaintRegion() = default;
  PaintRegion(std::shared_ptr<std::vector<skity::Rect>> rects, size_t from,
              size_t to, bool has_readback, bool has_drawable_image,
              bool has_animation, bool has_deferred_image)
      : rects_(rects),
        from_(from),
        to_(to),
        has_readback_(has_readback),
        has_drawable_image_(has_drawable_image),
        has_animation_(has_animation),
        has_deferred_image_(has_deferred_image) {}

  std::vector<skity::Rect>::const_iterator begin() const {
    FML_DCHECK(is_valid());
    return rects_->begin() + from_;
  }

  std::vector<skity::Rect>::const_iterator end() const {
    FML_DCHECK(is_valid());
    return rects_->begin() + to_;
  }

  // Compute bounds for this region
  skity::Rect ComputeBounds() const;

  bool is_valid() const { return rects_ != nullptr; }

  // Returns true if there is a layer in subtree represented by this region
  // that performs readback
  bool has_readback() const { return has_readback_; }

  // Returns whether there is a DrawableImageLayer in subtree represented by
  // this region.
  bool has_drawable_image() const { return has_drawable_image_; }

  bool has_animation() const { return has_animation_; }

  bool has_deferred_image() const { return has_deferred_image_; }

 private:
  std::shared_ptr<std::vector<skity::Rect>> rects_;
  size_t from_ = 0;
  size_t to_ = 0;
  bool has_readback_ = false;
  bool has_drawable_image_ = false;
  bool has_animation_ = false;
  bool has_deferred_image_ = false;
};

}  // namespace clay

#endif  // CLAY_FLOW_PAINT_REGION_H_
