// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/border_radius.h"

#include "base/include/float_comparison.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace harmony {

bool BorderRadius::IsIdentical() const {
  return base::FloatsEqual(computed_value_[0], computed_value_[2]) &&
         base::FloatsEqual(computed_value_[0], computed_value_[4]) &&
         base::FloatsEqual(computed_value_[0], computed_value_[6]) &&
         base::FloatsEqual(computed_value_[1], computed_value_[3]) &&
         base::FloatsEqual(computed_value_[1], computed_value_[5]) &&
         base::FloatsEqual(computed_value_[1], computed_value_[7]);
}

bool BorderRadius::IsZero() const {
  return IsIdentical() && base::FloatsEqual(computed_value_[0], .0f) &&
         base::FloatsEqual(computed_value_[1], .0f);
}

void BorderRadius::Compute(float width, float height) {
  if (base::FloatsEqual(width, .0f) || base::FloatsEqual(height, .0f)) {
    memset(radius_, 0, 8 * sizeof(float));
    return;
  }

  for (int i = 0; i < 8; i += 2) {
    computed_value_[i] = radius_[i].GetValue(width);
    computed_value_[i + 1] = radius_[i + 1].GetValue(height);
  }
  float val = 1.0;
  float tmp_w = 0.f;
  float tmp_h = 0.f;
  float tmp;
  if (base::FloatsLarger(computed_value_[0] + computed_value_[2],
                         computed_value_[4] + computed_value_[6])) {
    tmp_w = computed_value_[0] + computed_value_[2];
  } else {
    tmp_w = computed_value_[4] + computed_value_[6];
  }

  if (base::FloatsLarger(computed_value_[1] + computed_value_[7],
                         computed_value_[3] + computed_value_[5])) {
    tmp_h = computed_value_[1] + computed_value_[7];
  } else {
    tmp_h = computed_value_[3] + computed_value_[5];
  }
  if (base::FloatsLarger(tmp_w, width) || base::FloatsLarger(tmp_h, height)) {
    if (base::FloatsLarger(tmp_w * height, tmp_h * width)) {
      tmp = width / tmp_w;
    } else {
      tmp = height / tmp_h;
    }
    if (base::FloatsLarger(val, tmp)) {
      val = tmp;
    }

    if (base::FloatsLarger(1.0f, val)) {
      for (float& value : computed_value_) {
        value = value * val;
      }
    }
  }
}

void BorderRadius::SetRadius(CornerPosition position, const lepus::Value& value,
                             int offset) {
  const auto* val_array = value.Array().get();
  size_t index = static_cast<size_t>(position) * 2;
  radius_[index] = (PlatformLength){
      val_array->get(offset),
      static_cast<PlatformLengthType>(val_array->get(offset + 1).Number())};
  radius_[index + 1] = (PlatformLength){
      val_array->get(offset + 2),
      static_cast<PlatformLengthType>(val_array->get(offset + 3).Number())};
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
