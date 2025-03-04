// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_CONSTRAINTS_H_
#define CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_CONSTRAINTS_H_

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
  float size_;
  SLMeasureMode mode_;
};

using Constraints = DimensionValue<OneSideConstraint>;

}  // namespace starlight
}  // namespace lynx
#endif  // CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_CONSTRAINTS_H_
