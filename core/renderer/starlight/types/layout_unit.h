// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_UNIT_H_
#define CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_UNIT_H_

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"

namespace lynx {
namespace starlight {

class LayoutUnit {
 public:
  LayoutUnit() : value_(0.f), is_indefinite_(true) {}
  // FIXME: add back the explict
  explicit LayoutUnit(float value) : value_(value), is_indefinite_(false) {}

  static LayoutUnit Indefinite() { return LayoutUnit(0.f, true); }
  static LayoutUnit Zero() { return LayoutUnit(0.f, false); }

  LayoutUnit(const LayoutUnit& other)
      : value_(other.value_), is_indefinite_(other.is_indefinite_) {}

  bool IsIndefinite() const { return is_indefinite_; }
  bool IsDefinite() const { return !IsIndefinite(); }

  LayoutUnit& operator=(const LayoutUnit& other) {
    value_ = other.value_;

    is_indefinite_ = other.is_indefinite_;
    return *this;
  }

  LayoutUnit& ClampIndefiniteToZero() {
    if (is_indefinite_) {
      value_ = 0.f;
      is_indefinite_ = false;
    }
    return *this;
  }

  LayoutUnit& operator=(float other) {
    value_ = other;
    is_indefinite_ = false;
    return *this;
  }

  LayoutUnit& AssignIfIndefinite(const LayoutUnit& other) {
    if (IsIndefinite()) {
      *this = other;
    }
    return *this;
  }

  LayoutUnit& Override(const LayoutUnit& other) {
    if (other.IsDefinite()) {
      *this = other;
    }
    return *this;
  }

  bool operator==(const LayoutUnit& other) const {
    if (is_indefinite_ && other.is_indefinite_) {
      return true;
    }
    return is_indefinite_ == other.is_indefinite_ && value_ == other.value_;
  }

  bool operator!=(const LayoutUnit& other) const { return !(*this == other); }

  LayoutUnit operator+(const LayoutUnit& other) const {
    return LayoutUnit(other.value_ + value_,
                      is_indefinite_ || other.is_indefinite_);
  }

  LayoutUnit operator+(float other) const {
    return LayoutUnit(other + value_, is_indefinite_);
  }

  LayoutUnit operator-(const LayoutUnit& other) const {
    return LayoutUnit(value_ - other.value_,
                      is_indefinite_ || other.is_indefinite_);
  }

  LayoutUnit operator-(float other) const {
    return LayoutUnit(value_ - other, is_indefinite_);
  }

  LayoutUnit operator/(float other) const {
    if (IsIndefinite() || other == 0.f) {
      return Indefinite();
    }
    return LayoutUnit(value_ / other, is_indefinite_);
  }

  float ToFloat() const {
    DCHECK(!is_indefinite_);

    return value_;
  }

  static LayoutUnit LesserLayoutUnit(const LayoutUnit& a, const LayoutUnit& b) {
    if (!a.IsIndefinite() && !b.IsIndefinite()) {
      return a.value_ > b.value_ ? b : a;
    }
    if (!a.IsIndefinite()) {
      return a;
    }
    return b;
  }

  static LayoutUnit LargerLayoutUnit(const LayoutUnit& a, const LayoutUnit& b) {
    if (!a.IsIndefinite() && !b.IsIndefinite()) {
      return a.value_ < b.value_ ? b : a;
    }
    if (!a.IsIndefinite()) {
      return a;
    }
    return b;
  }

  static LayoutUnit ClampLayoutUnitWithMinMax(const LayoutUnit& target,
                                              const LayoutUnit& min,
                                              const LayoutUnit& max) {
    if (target.IsIndefinite()) {
      return LayoutUnit::Indefinite();
    }
    return LayoutUnit::LesserLayoutUnit(
        max, LayoutUnit::LargerLayoutUnit(min, target));
  };

 private:
  LayoutUnit(float value, float is_indefinite)
      : value_(value), is_indefinite_(is_indefinite) {}

  float value_ = 0.f;
  bool is_indefinite_;
  friend LayoutUnit operator*(float, const LayoutUnit&);
};

inline LayoutUnit operator*(float value, const LayoutUnit& unit) {
  return LayoutUnit(unit.value_ * value, unit.IsIndefinite());
}

inline LayoutUnit operator*(const LayoutUnit& unit, float value) {
  return value * unit;
}

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_UNIT_H_
