// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/platform_length.h"

#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace harmony {

static float GetValueInternal(const fml::RefPtr<lepus::CArray>& calc_array,
                              float value, const PlatformLengthType unit,
                              float parent_value) {
  if (unit == PlatformLengthType::kPercentage) {
    return value * parent_value;
  }
  if (unit == PlatformLengthType::kNumber) {
    return value;
  }
  if (unit == PlatformLengthType::kCalc) {
    float ret = 0;
    size_t i = 0;
    PlatformLengthType item_type = PlatformLengthType::kNumber;
    for (i = 0, item_type = static_cast<PlatformLengthType>(
                    calc_array->get(i + 1).Number());
         i < calc_array->size(); i += 2) {
      fml::RefPtr<lepus::CArray> item_array{nullptr};
      float item_value = 0;
      if (PlatformLengthType::kCalc == item_type) {
        item_array = calc_array->get(i).Array();
      } else if (item_type == PlatformLengthType::kNumber ||
                 item_type == PlatformLengthType::kPercentage) {
        item_value = calc_array->get(i).Number();
      }
      ret += GetValueInternal(item_array, item_value, item_type, parent_value);
    }
    return ret;
  }
  return 0;
}

PlatformLength::PlatformLength() = default;

PlatformLength::PlatformLength(const lepus::Value& value,
                               const PlatformLengthType type) {
  type_ = type;
  if (type_ == PlatformLengthType::kCalc) {
    calc_array_ = value.Array();
  } else if (PlatformLengthType::kNumber == type ||
             PlatformLengthType::kPercentage == type) {
    val_ = value.Number();
  }
}

float PlatformLength::GetValue(float parent) const {
  return GetValueInternal(calc_array_, val_, type_, parent);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
