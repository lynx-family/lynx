// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_BASIC_SHAPE_DATA_H_
#define CORE_STYLE_BASIC_SHAPE_DATA_H_

#include <algorithm>
#include <array>
#include <cstddef>

#include "base/include/value/base_string.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx::starlight {

struct BasicShapeData {
  enum class CornerType { kRect, kRounded, kSuperEllipse };
  static constexpr size_t kInsetCount = 4;
  static constexpr size_t kCornerRadiusCount = 8;

  BasicShapeType type = BasicShapeType::kUnknown;
  CornerType corner_type = CornerType::kRect;
  double exponent_x = 2;
  double exponent_y = 2;
  base::String path_data;

  NLength& RadiusX() { return lengths_[0]; }
  const NLength& RadiusX() const { return lengths_[0]; }
  NLength& RadiusY() { return lengths_[1]; }
  const NLength& RadiusY() const { return lengths_[1]; }
  NLength& CenterX() { return lengths_[2]; }
  const NLength& CenterX() const { return lengths_[2]; }
  NLength& CenterY() { return lengths_[3]; }
  const NLength& CenterY() const { return lengths_[3]; }
  // Top, right, bottom, left; radial shapes use these slots for radii/center.
  NLength& Inset(size_t index) { return lengths_[index]; }
  const NLength& Inset(size_t index) const { return lengths_[index]; }
  // Alternating x/y radii, clockwise from the top-left corner.
  NLength& CornerRadius(size_t index) { return lengths_[kInsetCount + index]; }
  const NLength& CornerRadius(size_t index) const {
    return lengths_[kInsetCount + index];
  }

  bool operator==(const BasicShapeData& rhs) const {
    if (type != rhs.type) return false;
    switch (type) {
      case BasicShapeType::kPath:
        return path_data == rhs.path_data;
      case BasicShapeType::kInset:
        return std::equal(lengths_.begin(), lengths_.begin() + kInsetCount,
                          rhs.lengths_.begin()) &&
               corner_type == rhs.corner_type &&
               (corner_type == CornerType::kRect ||
                (std::equal(lengths_.begin() + kInsetCount, lengths_.end(),
                            rhs.lengths_.begin() + kInsetCount) &&
                 (corner_type != CornerType::kSuperEllipse ||
                  (exponent_x == rhs.exponent_x &&
                   exponent_y == rhs.exponent_y))));
      case BasicShapeType::kCircle:
      case BasicShapeType::kEllipse:
      case BasicShapeType::kSuperEllipse:
        return RadiusX() == rhs.RadiusX() && CenterX() == rhs.CenterX() &&
               CenterY() == rhs.CenterY() &&
               (type == BasicShapeType::kCircle ||
                RadiusY() == rhs.RadiusY()) &&
               (type != BasicShapeType::kSuperEllipse ||
                (exponent_x == rhs.exponent_x && exponent_y == rhs.exponent_y));
      case BasicShapeType::kUnknown:
        return true;
    }
    return false;
  }
  bool operator!=(const BasicShapeData& rhs) const { return !(*this == rhs); }

  using AlwaysUseFlexOptionalMemSave = bool;

 private:
  std::array<NLength, kInsetCount + kCornerRadiusCount> lengths_ = {
      NLength::MakeUnitNLength(0), NLength::MakeUnitNLength(0),
      NLength::MakeUnitNLength(0), NLength::MakeUnitNLength(0),
      NLength::MakeUnitNLength(0), NLength::MakeUnitNLength(0),
      NLength::MakeUnitNLength(0), NLength::MakeUnitNLength(0),
      NLength::MakeUnitNLength(0), NLength::MakeUnitNLength(0),
      NLength::MakeUnitNLength(0), NLength::MakeUnitNLength(0)};
};

}  // namespace lynx::starlight

#endif  // CORE_STYLE_BASIC_SHAPE_DATA_H_
