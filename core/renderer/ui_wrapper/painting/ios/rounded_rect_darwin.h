// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_ROUNDED_RECT_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_ROUNDED_RECT_DARWIN_H_

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>

static inline void LynxCGPathAddEllipticArc(CGMutablePathRef path, CGPoint origin, CGFloat width,
                                            CGFloat height, CGFloat start_angle, CGFloat end_angle,
                                            bool clockwise) {
  CGFloat x_scale = 1;
  CGFloat y_scale = 1;
  CGFloat radius = 0;
  if (width != 0) {
    y_scale = height / width;
    radius = width;
  } else if (height != 0) {
    x_scale = width / height;
    radius = height;
  }

  CGAffineTransform transform = CGAffineTransformMakeTranslation(origin.x, origin.y);
  transform = CGAffineTransformScale(transform, x_scale, y_scale);
  CGPathAddArc(path, &transform, 0, 0, radius, start_angle, end_angle, clockwise);
}

// Radii are already resolved and ordered top-left, top-right, bottom-right,
// bottom-left.
static inline void LynxCGPathAddRoundedRect(CGMutablePathRef path, CGRect bounds,
                                            const CGSize radii[4]) {
  const CGFloat min_x = bounds.origin.x;
  const CGFloat min_y = bounds.origin.y;
  const CGFloat max_x = min_x + bounds.size.width;
  const CGFloat max_y = min_y + bounds.size.height;
  const CGSize tl = radii[0], tr = radii[1], br = radii[2], bl = radii[3];
  const CGFloat quarter_turn = 1.57079632679489661923;

  CGPathMoveToPoint(path, NULL, min_x, min_y + tl.height);
  LynxCGPathAddEllipticArc(path, CGPointMake(min_x + tl.width, min_y + tl.height), tl.width,
                           tl.height, 2 * quarter_turn, 3 * quarter_turn, false);
  LynxCGPathAddEllipticArc(path, CGPointMake(max_x - tr.width, min_y + tr.height), tr.width,
                           tr.height, 3 * quarter_turn, 0, false);
  LynxCGPathAddEllipticArc(path, CGPointMake(max_x - br.width, max_y - br.height), br.width,
                           br.height, 0, quarter_turn, false);
  LynxCGPathAddEllipticArc(path, CGPointMake(min_x + bl.width, max_y - bl.height), bl.width,
                           bl.height, quarter_turn, 2 * quarter_turn, false);
  CGPathCloseSubpath(path);
}

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_ROUNDED_RECT_DARWIN_H_
