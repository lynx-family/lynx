// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/object_painter.h"

#include "clay/gfx/geometry/float_rounded_rect.h"

namespace clay {

void ObjectPainter::DrawDefaultFocusRing(GraphicsContext* context,
                                         const FloatRoundedRect& round_rect,
                                         int thickness, Color color) {
  Paint paint;
  paint.setColor(color);
  paint.setDrawStyle(DrawStyle::kStroke);
  paint.setStrokeWidth(static_cast<float>(thickness));

  context->DrawRRect(round_rect, paint);
}

}  // namespace clay
