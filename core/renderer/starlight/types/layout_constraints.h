// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_CONSTRAINTS_H_
#define CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_CONSTRAINTS_H_

#include <algorithm>
#include <limits>

#include "base/include/compiler_specific.h"
#include "base/include/float_comparison.h"
#include "base/include/log/logging.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/types/layout_directions.h"
#include "core/renderer/starlight/types/layout_unit.h"

namespace lynx {
namespace starlight {

class OneSideConstraint {
 public:
  OneSideConstraint(float size, SLMeasureMode mode)
      : size_(size), mode_(mode) {}
  OneSideConstraint() : OneSideConstraint(Indefinite()) {}

  static OneSideConstraint Indefinite() {
    return OneSideConstraint(10E7, SLMeasureModeIndefinite);
  }
  static OneSideConstraint Definite(float size) {
    return OneSideConstraint(size, SLMeasureModeDefinite);
  }
  static OneSideConstraint AtMost(float size) {
    return OneSideConstraint(size, SLMeasureModeAtMost);
  }

  OneSideConstraint& operator=(const OneSideConstraint&) = default;
  OneSideConstraint(const OneSideConstraint&) = default;
  OneSideConstraint(OneSideConstraint&&) = default;

  SLMeasureMode Mode() const { return mode_; }
  float Size() const {
    // For whatever reason DCHECK does not work on iOS,
    // following check should not mess up release code.
    // Let's do the check in UT and android.
    DCHECK(mode_ != SLMeasureModeIndefinite);
    return size_;
  }

  bool operator==(const OneSideConstraint& other) const {
    return (mode_ == SLMeasureModeIndefinite &&
            other.mode_ == SLMeasureModeIndefinite) ||
           (mode_ == other.mode_ && size_ == other.size_);
  }
  bool Near(const OneSideConstraint& other) const {
    return (mode_ == SLMeasureModeIndefinite &&
            other.mode_ == SLMeasureModeIndefinite) ||
           (mode_ == other.mode_ && base::FloatsEqual(size_, other.size_));
  }

  LayoutUnit ToPercentBase() const {
    return mode_ == SLMeasureModeDefinite ? LayoutUnit(size_)
                                          : LayoutUnit::Indefinite();
  }

  void ApplySize(const LayoutUnit& size) {
    if (size.IsDefinite()) {
      mode_ = SLMeasureModeDefinite;
      size_ = size.ToFloat();
    }
  }

 private:
  friend struct CompactConstraints;

  float size_;
  SLMeasureMode mode_;
};

using Constraints = DimensionValue<OneSideConstraint>;

// Compose ranges are independent of CSS measure modes. Native units are layout
// units; infinity is represented explicitly, never by an arbitrary large size.
struct ComposeConstraints {
  static constexpr float kInfinity = std::numeric_limits<float>::infinity();
  float min_width = 0;
  float max_width = kInfinity;
  float min_height = 0;
  float max_height = kInfinity;

  bool operator==(const ComposeConstraints& other) const {
    return min_width == other.min_width && max_width == other.max_width &&
           min_height == other.min_height && max_height == other.max_height;
  }

  float ConstrainWidth(float width) const {
    return std::max(min_width, std::min(width, max_width));
  }
  float ConstrainHeight(float height) const {
    return std::max(min_height, std::min(height, max_height));
  }
  ComposeConstraints Inset(float width, float height) const {
    return {std::max(0.f, min_width - width), std::max(0.f, max_width - width),
            std::max(0.f, min_height - height),
            std::max(0.f, max_height - height)};
  }
  Constraints ToCssConstraints() const {
    auto axis = [](float min, float max) {
      return min == max         ? OneSideConstraint::Definite(max)
             : max == kInfinity ? OneSideConstraint::Indefinite()
                                : OneSideConstraint::AtMost(max);
    };
    return {axis(min_width, max_width), axis(min_height, max_height)};
  }
};

// Layout optimized struct for cache.
struct __attribute__((packed, aligned(1))) CompactConstraints {
  DimensionValue<float> sizes;
  DimensionValue<SLMeasureMode> modes;

  CompactConstraints() { *this = Constraints(); }

  CompactConstraints& operator=(const Constraints& other) {
    sizes[kHorizontal] = other[kHorizontal].size_;
    sizes[kVertical] = other[kVertical].size_;
    modes[kHorizontal] = other[kHorizontal].mode_;
    modes[kVertical] = other[kVertical].mode_;
    return *this;
  }

  bool operator==(const Constraints& other) const {
    return (*this)[kHorizontal] == other[kHorizontal] &&
           (*this)[kVertical] == other[kVertical];
  }

  OneSideConstraint operator[](Dimension dim) const {
    return OneSideConstraint(sizes[dim], modes[dim]);
  }
};

}  // namespace starlight
}  // namespace lynx
#endif  // CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_CONSTRAINTS_H_
