// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skity/picture_skity.h"

#include "skity/graphic/color.hpp"
#include "skity/graphic/paint.hpp"

namespace clay {

uint32_t PictureSkity::NextUniqueID() {
  static std::atomic<uint32_t> next_id(1);
  uint32_t id;
  do {
    id = next_id.fetch_add(1, std::memory_order_relaxed);
  } while (id == 0);  // 0 is reserved for an invalid id.
  return id;
}

void PictureSkity::DispatchToWorklet(ClayAnimationPropertyType type,
                                     const clay::Color& value) {
  if (!picture_) {
    return;
  }
  for (auto op : dynamic_ops_) {
    if ((ClayAnimationPropertyType::kColor == type &&
         op.first == DynamicOpType::kSetTextColor) ||
        (ClayAnimationPropertyType::kBackgroundColor == type &&
         op.first == DynamicOpType::kSetBackgroundColor)) {
      skity::Paint* paint = picture_->GetOpPaintByOffset(op.second);
      if (paint) {
        paint->SetColor(skity::ColorSetARGB(value.Alpha(), value.Red(),
                                            value.Green(), value.Blue()));
      }
    }
  }
}

clay::Color PictureSkity::ObtainWorkletValue(
    ClayAnimationPropertyType type) const {
  if (!picture_) {
    return Color();
  }
  for (auto op : dynamic_ops_) {
    if ((ClayAnimationPropertyType::kColor == type &&
         op.first == DynamicOpType::kSetTextColor) ||
        (ClayAnimationPropertyType::kBackgroundColor == type &&
         op.first == DynamicOpType::kSetBackgroundColor)) {
      skity::Paint* paint = picture_->GetOpPaintByOffset(op.second);
      if (paint) {
        // 32-bit ARGB color value
        return Color(paint->GetColor());
      }
    }
  }
  return Color();
}

}  // namespace clay
