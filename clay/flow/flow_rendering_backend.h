// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_FLOW_RENDERING_BACKEND_H_
#define CLAY_FLOW_FLOW_RENDERING_BACKEND_H_

#ifndef ENABLE_SKITY
#include "clay/flow/layers/offscreen_surface.h"
#include "clay/flow/layers/picture_complexity_helper_skia.h"
#else
#include "clay/flow/layers/picture_complexity_calculator_skity.h"
#include "clay/flow/layers/picture_complexity_helper_skity.h"
#include "clay/gfx/skity/picture_skity.h"
#endif  // ENABLE_SKITY

#endif  // CLAY_FLOW_FLOW_RENDERING_BACKEND_H_
