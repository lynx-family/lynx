// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_RUBBERBAND_DISTANCE_H_
#define CLAY_UI_COMPONENT_RUBBERBAND_DISTANCE_H_

#include "clay/gfx/geometry/float_point.h"

namespace clay {

float RubberBandDistance(float x, float dimension, float resistance = 0.55);
float ReverseRubberBandDistance(float x, float dimension,
                                float resistance = 0.55);
FloatPoint RubberBandDistance(FloatPoint offset, float width_dimension,
                              float height_dimension);
FloatPoint ReverseRubberBandDistance(FloatPoint offset, float width_dimension,
                                     float height_dimension);

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_RUBBERBAND_DISTANCE_H_
