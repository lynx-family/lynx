// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/layout_global.h"

bool IsSLDefiniteMode(SLMeasureMode mode) {
  return mode == SLMeasureModeDefinite;
}

bool IsSLIndefiniteMode(SLMeasureMode mode) {
  return mode == SLMeasureModeIndefinite;
}

bool IsSLAtMostMode(SLMeasureMode mode) { return mode == SLMeasureModeAtMost; }
