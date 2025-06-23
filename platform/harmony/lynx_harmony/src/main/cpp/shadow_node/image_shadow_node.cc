// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/image_shadow_node.h"

#include "base/include/float_comparison.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/lynx_trace_categories.h"
namespace lynx {
namespace tasm {
namespace harmony {
ImageShadowNode::ImageShadowNode(int sign, const std::string& tag)
    : ShadowNode(sign, tag) {
  SetCustomMeasureFunc(this);
}

LayoutResult ImageShadowNode::Measure(float width, MeasureMode width_mode,
                                      float height, MeasureMode height_mode,
                                      bool final_measure) {
  exactly_ = width_mode == MeasureMode::Definite &&
             height_mode == MeasureMode::Definite;
  if (exactly_) {
    return LayoutResult(width, height, 0);
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, IMAGE_SHADOW_NODE_MEASURE);
  auto bitmap_w = bitmap_width_;
  auto bitmap_h = bitmap_height_;
  exactly_ = (width == 0 && width_mode != MeasureMode::Indefinite) ||
             (height == 0 && height_mode != MeasureMode::Indefinite);
  if (!auto_size_ || bitmap_w <= 0 || bitmap_h <= 0 || exactly_) {
    return LayoutResult(width_mode == MeasureMode::Definite ? width : 0.f,
                        height_mode == MeasureMode::Definite ? height : 0.f, 0);
  }

  if (width_mode == MeasureMode::Definite) {
    // Height is determined by width
    auto expect_height = bitmap_h / bitmap_w * width;
    if (height_mode == MeasureMode::AtMost) {
      if (base::FloatsLarger(height, expect_height)) {
        height = expect_height;
      }
    } else if (height_mode == MeasureMode::Indefinite) {
      height = expect_height;
    } else {
      // will never reach this point
    }
  } else {
    auto max_size = 65535.f;
    if (width_mode == MeasureMode::Indefinite) {
      width = max_size;
    }
    if (height_mode == MeasureMode::Indefinite) {
      height = max_size;
    }

    if (height_mode == MeasureMode::Definite) {
      // width is determined by height
      auto expect_width = bitmap_w / bitmap_h * height;
      if (base::FloatsLarger(width, expect_width)) {
        width = expect_width;
      }
    } else {
      // If the bitmap is smaller than the image view, the image view size is
      // the bitmap size
      if (base::FloatsLargerOrEqual(width, bitmap_w) &&
          base::FloatsLargerOrEqual(height, bitmap_h)) {
        width = bitmap_w;
        height = bitmap_h;
      } else {
        // Otherwise, the short side of the image view shall prevail
        auto aspect_ratio = bitmap_h / bitmap_w;
        if (base::FloatsLarger(aspect_ratio, height / width)) {
          width = height / aspect_ratio;
        } else {
          height = aspect_ratio * width;
        }
      }
    }
  }

  return LayoutResult(width, height, 0);
}

void ImageShadowNode::JustSize(bool auto_size, float bitmap_width,
                               float bitmap_height, float width, float height) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, IMAGE_SHADOW_NODE_JUST_SIZE);
  auto last_state = auto_size_;
  auto_size_ = auto_size;
  bitmap_height_ = bitmap_height;
  bitmap_width_ = bitmap_width;
  if (last_state != auto_size_) {
    RequestLayout();
    return;
  }
  if (exactly_) {
    return;
  }
  if (auto_size) {
    if (bitmap_width > 0 && bitmap_height > 0 &&
        (width == 0 || height == 0 ||
         base::FloatsLarger(
             std::abs(width / height - bitmap_width / bitmap_height), 0.05))) {
      RequestLayout();
    }
  }
}

void ImageShadowNode::Align() {}
}  // namespace harmony
}  // namespace tasm

}  // namespace lynx
