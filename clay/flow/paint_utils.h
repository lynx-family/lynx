// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_PAINT_UTILS_H_
#define CLAY_FLOW_PAINT_UTILS_H_

#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/rect.hpp"

namespace clay {

typedef void (*CheckerboardFunc)(clay::GrCanvas*, const skity::Rect&);

void DrawCheckerboard(clay::GrCanvas* canvas, const skity::Rect& rect);

#if !defined(NDEBUG)
void DrawRasterCacheTag(GrCanvas* canvas, float x, float y, int use_count);
void DrawDebugBorders(GrCanvas* canvas, const skity::Rect& bounds);
#endif

}  // namespace clay

#endif  // CLAY_FLOW_PAINT_UTILS_H_
