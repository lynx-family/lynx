// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/types/nlength.h"

#include <utility>

#include "base/include/string/string_number_convert.h"

namespace lynx {
namespace starlight {

using lynx::base::StringToFloat;

namespace {
// auto unit percentage vw vh
std::string NumericLengthToString(const BaseLength& length) {
  constexpr const char* kUnit = "unit";
  constexpr const char* kPercentageMark = "%";
  if (!length.HasValue()) {
    return std::string("0");
  } else if (length.ContainsFixedValue() && !length.ContainsPercentage()) {
    return std::to_string(length.GetFixedPart()) + kUnit;
  } else if (!length.ContainsFixedValue() && length.ContainsPercentage()) {
    return std::to_string(length.GetPercentagePart()) + kPercentageMark;
  } else {
    return std::to_string(length.GetFixedPart()) + kUnit + "+" +
           std::to_string(length.GetPercentagePart()) + kPercentageMark;
  }

  return "";
}
}  // namespace
bool NLength::operator==(const NLength& o) const {
  return type_ == o.type_ && numeric_length_ == o.numeric_length_;
}

bool NLength::operator!=(const NLength& o) const { return !(*this == o); }

NLength NLength::MakeAutoNLength() {
  return NLength(NLengthType::kNLengthAuto);
}

NLength NLength::MakeMaxContentNLength() {
  return NLength(NLengthType::kNLengthMaxContent);
}

NLength NLength::MakeFitContentNLength(const BaseLength& nLength) {
  return NLength(nLength, NLengthType::kNLengthFitContent);
}

NLength NLength::MakeUnitNLength(float value) {
  return NLength(value, NLengthType::kNLengthUnit);
}

NLength NLength::MakePercentageNLength(float value) {
  return NLength(value, NLengthType::kNLengthPercentage);
}

NLength NLength::MakeFrNLength(float value) {
  return NLength(value, NLengthType::kNLengthFr);
}

// TODO(zhixuan): Rename calc type;
NLength NLength::MakeCalcNLength(float fixed, float percentage) {
  return NLength(BaseLength(fixed, percentage), NLengthType::kNLengthCalc);
}

// TODO(zhixuan): Remove this constructor
NLength::NLength(float value, NLengthType type)
    : type_(type),
      numeric_length_(type == kNLengthPercentage ? BaseLength(0, value)
                                                 : BaseLength(value)) {}

NLength::NLength(const BaseLength& base_length, NLengthType type)
    : type_(type), numeric_length_(base_length) {}

std::string NLength::ToString() const {
  std::string result;
  switch (GetType()) {
    case NLengthType::kNLengthAuto: {
      result = "auto";
      break;
    }

    case NLengthType::kNLengthUnit:
    case NLengthType::kNLengthPercentage: {
      result = NumericLengthToString(numeric_length_);
    } break;
    case NLengthType::kNLengthCalc: {
      result = "calc(" + NumericLengthToString(numeric_length_) + ")";
    } break;
    case NLengthType::kNLengthMaxContent: {
      result = "max-content";
    } break;
    case NLengthType::kNLengthFitContent: {
      result = "fit-content";
      if (numeric_length_.HasValue()) {
        result += "(" + NumericLengthToString(numeric_length_) + ")";
      }
    } break;
    default:
      break;
  }
  return result + ";";
}

LayoutUnit NLengthToFakeLayoutUnit(const NLength& length) {
  return NLengthToLayoutUnit(length, LayoutUnit::Indefinite());
}

// static
// TODO(zhixuan): Refactor to BaseLength to LayoutUnit
LayoutUnit NLengthToLayoutUnit(const NLength& length,
                               const LayoutUnit& parent_value) {
  if (!length.NumericLength().HasValue()) {
    return LayoutUnit::Indefinite();
  } else {
    const auto& numeric_length = length.NumericLength();
    return numeric_length.ContainsPercentage()
               ? LayoutUnit(numeric_length.GetFixedPart()) +
                     parent_value *
                         (numeric_length.GetPercentagePart() / 100.0f)
               : LayoutUnit(numeric_length.GetFixedPart());
  }
}

}  // namespace starlight
}  // namespace lynx
