// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_PLATFORM_LENGTH_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_PLATFORM_LENGTH_H_

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace tasm {
namespace harmony {

enum class PlatformLengthType : uint32_t {
  kNumber = 0,
  kPercentage = 1,
  kCalc = 2,
};

class PlatformLength {
 public:
  PlatformLength();
  explicit PlatformLength(const lepus::Value& value, PlatformLengthType type);
  PlatformLength(const float value, const PlatformLengthType unit)
      : type_(unit), val_(value) {}
  float GetValue(float parent, float density = 1.f) const;

  float AsNumber() const {
    return type_ == PlatformLengthType::kNumber ? val_ : .0f;
  }

 private:
  PlatformLengthType type_ = PlatformLengthType::kNumber;
  float val_ = .0f;
  fml::RefPtr<lepus::CArray> calc_array_{nullptr};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_PLATFORM_LENGTH_H_
