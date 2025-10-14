// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/rubberband_distance.h"

#include <cmath>

namespace clay {

/**
 Alternative formula: f(x) = (x * dimension * resistance) / (dimension *
 resistance + |x|)
 */
float RubberBandDistance(float x, float dimension,
                         float resistance /* = 0.55 */) {
  float res =
      (x * dimension * resistance) / (std::abs(x) * resistance + dimension);
  return (isnan(res) || isinf(res)) ? 0 : res;
}

float ReverseRubberBandDistance(float x, float dimension,
                                float resistance /* = 0.55 */) {
  float res = (x * dimension) / (resistance * (dimension - std::abs(x)));
  return (isnan(res) || isinf(res)) ? 0 : res;
}

FloatPoint RubberBandDistance(FloatPoint offset, float width, float height) {
  return FloatPoint(RubberBandDistance(offset.x(), width),
                    RubberBandDistance(offset.y(), height));
}

FloatPoint ReverseRubberBandDistance(FloatPoint offset, float width,
                                     float height) {
  return FloatPoint(ReverseRubberBandDistance(offset.x(), width),
                    ReverseRubberBandDistance(offset.y(), height));
}

}  // namespace clay
