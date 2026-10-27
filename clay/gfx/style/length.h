// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_LENGTH_H_
#define CLAY_GFX_STYLE_LENGTH_H_

namespace clay {

enum class LengthUnit { kNum, kPercent };

class Length {
 public:
  Length() : value_(0), unit_(LengthUnit::kNum) {}
  explicit Length(float v) : value_(v), unit_(LengthUnit::kNum) {}
  Length(float v, LengthUnit u) : value_(v), unit_(u) {}
  Length(float v, int u) : value_(v), unit_(static_cast<LengthUnit>(u)) {}
  bool IsPercent() const { return LengthUnit::kPercent == unit_; }
  float GetValue(float v) const { return IsPercent() ? value_ * v : value_; }
  float GetRawValue() const { return value_; }
  void SetValue(float v) { value_ = v; }
  void SetUnit(LengthUnit unit) { unit_ = unit; }
  bool operator==(const Length& other) const {
    return value_ == other.value_ && unit_ == other.unit_;
  }
  bool operator!=(const Length& other) const { return !(*this == other); }

 private:
  float value_;
  LengthUnit unit_;
};

}  // namespace clay

#endif  // CLAY_GFX_STYLE_LENGTH_H_
