// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_MEASURABLE_H_
#define CLAY_UI_COMPONENT_MEASURABLE_H_

#include "clay/ui/common/measure_constraint.h"

namespace clay {

class Measurable {
 public:
  virtual void Measure(const MeasureConstraint& constraint,
                       MeasureResult& result) = 0;
};

class CustomMeasurable {
 public:
  virtual MeasureResult Measure(const MeasureConstraint& constraint) = 0;
  virtual void Align() = 0;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_MEASURABLE_H_
