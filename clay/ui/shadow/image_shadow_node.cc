// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/image_shadow_node.h"

#include <limits>
#include <mutex>

#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/component/base_image_view.h"
#include "clay/ui/component/component_constants.h"

namespace clay {

ImageShadowNode::ImageShadowNode(ShadowNodeOwner* owner, std::string tag,
                                 int id)
    : ShadowNode(owner, tag, id) {}

// Called from UI thread
void ImageShadowNode::AdjustSizeIfNeeded(bool auto_size, float bitmap_width,
                                         float bitmap_height) {
  bool need_update_layout = false;
  {
    std::lock_guard<std::mutex> l(mutex_);
    need_update_layout = auto_size || (auto_size_ != auto_size);

    if (auto_size_ == auto_size && bitmap_width_ == bitmap_width &&
        bitmap_height_ == bitmap_height) {
      return;
    }

    bitmap_width_ = bitmap_width;
    bitmap_height_ = bitmap_height;
    auto_size_ = auto_size;
  }
  if (need_update_layout) {
    MarkDirty();
  }
}

MeasureResult ImageShadowNode::Measure(const MeasureConstraint& constraint) {
  if (!constraint.IsValid()) {
    return {};
  }

  float constraint_width = constraint.width.value();
  float constraint_height = constraint.height.value();
  if (constraint.width_mode == MeasureMode::kDefinite &&
      constraint.height_mode == MeasureMode::kDefinite) {
    return {constraint_width, constraint_height, 0.f};
  }

  float bitmap_width, bitmap_height;
  bool auto_size;
  {
    std::lock_guard<std::mutex> l(mutex_);
    bitmap_width = bitmap_width_;
    bitmap_height = bitmap_height_;
    auto_size = auto_size_;
  }

  if (!auto_size) {
    return {constraint.width_mode == MeasureMode::kDefinite ? constraint_width
                                                            : 0.f,
            constraint.height_mode == MeasureMode::kDefinite ? constraint_height
                                                             : 0.f,
            0.f};
  }

  float result_width = constraint_width;
  float result_height = constraint_height;
  if (constraint.width_mode == MeasureMode::kDefinite) {
    // Height is determined by width
    float expected_height = constraint_width * (bitmap_height / bitmap_width);
    if (constraint.height_mode == MeasureMode::kAtMost &&
        expected_height > constraint_height) {
      result_height = constraint_height;
    } else {
      result_height = expected_height;
    }
  } else if (constraint.height_mode == MeasureMode::kDefinite) {
    // Width is determined by height
    float expected_width = constraint_height * (bitmap_width / bitmap_height);
    if (constraint.width_mode == MeasureMode::kAtMost &&
        expected_width > constraint_width) {
      result_width = constraint_width;
    } else {
      result_width = expected_width;
    }
  } else {
    if (constraint.width_mode == MeasureMode::kIndefinite) {
      result_width = std::numeric_limits<float>::max();
    }
    if (constraint.height_mode == MeasureMode::kIndefinite) {
      result_height = std::numeric_limits<float>::max();
    }

    if (result_width >= bitmap_width && result_height >= bitmap_height) {
      // The bitmap size is smaller than the constraint.
      result_width = bitmap_width;
      result_height = bitmap_height;
    } else {
      // Keep the aspect ratio of bitmap
      float aspect_ratio = bitmap_height / bitmap_width;
      if (result_height / result_width < aspect_ratio) {
        result_width = result_height / aspect_ratio;
      } else {
        result_height = aspect_ratio * result_width;
      }
    }
  }

  return {result_width, result_height, 0.f};
}

}  // namespace clay
