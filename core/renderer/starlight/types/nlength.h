// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_TYPES_NLENGTH_H_
#define CORE_RENDERER_STARLIGHT_TYPES_NLENGTH_H_

#include <stack>
#include <string>
#include <vector>

#include "base/include/log/logging.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/types/layout_unit.h"

namespace lynx {
namespace starlight {

enum NLengthType {
  kNLengthAuto,
  kNLengthUnit,
  kNLengthPercentage,
  kNLengthCalc,
  kNLengthMaxContent,
  kNLengthFitContent,
  kNLengthFr,
};

// Numeric length is made of two parts
// 1. The fixed length in px unit
// 2. Percentage that relative to percentage base.
// To resolve length, add the fixed part with the percentage part multipled by
// percantage base and divided by 100.f.
class BaseLength {
 public:
  BaseLength(float fixed_part) : has_value_(true), fixed_(fixed_part) {}
  BaseLength(float fixed_part, float percentage_part)
      : has_value_(true),
        fixed_(fixed_part),
        percentage_(percentage_part),
        has_percentage_(true) {}
  BaseLength() = default;
  bool operator==(const BaseLength& o) const {
    return has_value_ == o.has_value_ && fixed_ == o.fixed_ &&
           has_percentage_ == o.has_percentage_ && percentage_ == o.percentage_;
  }
  bool operator!=(const BaseLength& o) const { return !(*this == o); }

  bool HasValue() const { return has_value_; }

  // That the percentage part of a length is 0 semantically different from
  // that a length does not contains the percentage part.
  bool ContainsPercentage() const { return has_percentage_ && has_value_; }

  // When a length has percentage part and fixed part is zero,
  // treat the length as a percentage only length.
  // When a length do has value but does not contains percent part and fixed
  // part is 0, treat the length as a fixed 0.
  bool ContainsFixedValue() const {
    return (fixed_ != 0 || !has_percentage_) && has_value_;
  }

  float GetFixedPart() const { return fixed_; }
  float GetPercentagePart() const { return percentage_; }

 private:
  bool has_value_ = false;
  float fixed_ = 0, percentage_ = 0;
  bool has_percentage_ = false;
};

class NLength {
 public:
  static NLength MakeAutoNLength();
  static NLength MakeMaxContentNLength();
  static NLength MakeFitContentNLength() { return NLength(kNLengthFitContent); }
  static NLength MakeFitContentNLength(const BaseLength& nLength);
  static NLength MakeUnitNLength(float value);
  static NLength MakeFrNLength(float value);
  static NLength MakePercentageNLength(float value);
  static NLength MakeCalcNLength(float fixed) {
    return NLength(BaseLength(fixed), NLengthType::kNLengthCalc);
  }
  static NLength MakeCalcNLength(float fixed, float percentage);

  ~NLength() {}

  std::string ToString() const;

  // TODO(zhixuan): Remove raw value getters.
  float GetRawValue() const {
    return type_ == kNLengthPercentage ? numeric_length_.GetPercentagePart()
                                       : numeric_length_.GetFixedPart();
  }

  NLengthType GetType() const { return type_; }
  const BaseLength& NumericLength() const { return numeric_length_; }

  bool IsAuto() const { return GetType() == NLengthType::kNLengthAuto; }
  bool IsUnit() const { return GetType() == NLengthType::kNLengthUnit; }
  bool IsPercent() const {
    return GetType() == NLengthType::kNLengthPercentage;
  }
  bool IsCalc() const { return GetType() == NLengthType::kNLengthCalc; }
  bool IsUnitOrResolvableValue() const {
    return GetType() == NLengthType::kNLengthUnit ||
           GetType() == NLengthType::kNLengthPercentage ||
           GetType() == NLengthType::kNLengthCalc;
  }
  bool IsMaxContent() const {
    return GetType() == NLengthType::kNLengthMaxContent;
  }
  bool IsFr() const { return GetType() == NLengthType::kNLengthFr; }
  bool IsFitContent() const {
    return GetType() == NLengthType::kNLengthFitContent;
  }

  bool IsIntrinsic() const { return IsFitContent() || IsMaxContent(); }
  // Including Percentage/Calc Type, e.g., width:calc(10% + 1px), width:10%
  bool ContainsPercentage() const {
    return numeric_length_.ContainsPercentage();
  }

  bool operator==(const NLength& o) const;
  bool operator!=(const NLength& o) const;

 private:
  NLength(NLengthType type) : type_(type) {}
  // Fixed
  NLength(float value, NLengthType type);
  // Combined
  NLength(const BaseLength& base_length, NLengthType type);

  NLengthType type_;

  BaseLength numeric_length_;
};

// WARNING!!! Don't use this method
LayoutUnit NLengthToFakeLayoutUnit(const NLength& length);

LayoutUnit NLengthToLayoutUnit(const NLength& length,
                               const LayoutUnit& parent_value);

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_TYPES_NLENGTH_H_
