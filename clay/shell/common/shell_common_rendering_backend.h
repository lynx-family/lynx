/*
 * Copyright 2025 The Lynx Authors. All rights reserved.
 * Licensed under the Apache License Version 2.0 that can be found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CLAY_SHELL_COMMON_SHELL_COMMON_RENDERING_BACKEND_H_
#define CLAY_SHELL_COMMON_SHELL_COMMON_RENDERING_BACKEND_H_

#ifndef ENABLE_SKITY
#ifndef USE_SYSTEM_ICU
#include "clay/fml/icu_util.h"
#endif  // USE_SYSTEM_ICU
#include "clay/flow/layers/offscreen_surface.h"
#include "clay/gfx/skity_to_skia_utils.h"
#else
#include "clay/gfx/paint_image_skity.h"
#endif  // ENABLE_SKITY

#endif  // CLAY_SHELL_COMMON_SHELL_COMMON_RENDERING_BACKEND_H_
