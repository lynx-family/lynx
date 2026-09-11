// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNX_PLATFORM_DARWIN_IOS_LYNX_BASE_BACKGROUND_LYNXLAYERCORNERRADII_H_
#define LYNX_PLATFORM_DARWIN_IOS_LYNX_BASE_BACKGROUND_LYNXLAYERCORNERRADII_H_

#import <Lynx/LynxBackgroundUtils.h>
#import <QuartzCore/QuartzCore.h>
#include <math.h>

typedef struct {
  BOOL eligible;
  CGFloat radius;
  NSUInteger corners;
} LynxLayerCornerRadii;

// Requires untransformed bounds; unresolved or overlapping partial radii use paths.
static inline LynxLayerCornerRadii LynxGetLayerCornerRadii(LynxBorderRadii radii, CGSize size) {
  LynxLayerCornerRadii result = {NO, 0, 0};
  if (internalHasSameBorderRadius(radii)) {
    result.eligible = YES;
    result.radius = MIN(radii.topLeftX.val, MIN(size.width, size.height) * 0.5);
    return result;
  }
  if (@available(iOS 11.0, *)) {
    // A square corner's nonzero axis still participates in CSS overlap scaling.
    if (radii.topLeftX.val + radii.topRightX.val > size.width ||
        radii.bottomLeftX.val + radii.bottomRightX.val > size.width ||
        radii.topLeftY.val + radii.bottomLeftY.val > size.height ||
        radii.topRightY.val + radii.bottomRightY.val > size.height) {
      return result;
    }
    const LynxBorderUnitValue x[] = {radii.topLeftX, radii.topRightX, radii.bottomLeftX,
                                     radii.bottomRightX};
    const LynxBorderUnitValue y[] = {radii.topLeftY, radii.topRightY, radii.bottomLeftY,
                                     radii.bottomRightY};
    const CACornerMask corners[] = {kCALayerMinXMinYCorner, kCALayerMaxXMinYCorner,
                                    kCALayerMinXMaxYCorner, kCALayerMaxXMaxYCorner};
    CGFloat radius = 0;
    NSUInteger selected = 0;
    for (NSUInteger i = 0; i < 4; ++i) {
      if (x[i].unit != LynxBorderValueUnitDefault || y[i].unit != LynxBorderValueUnitDefault ||
          !isfinite(x[i].val) || !isfinite(y[i].val) || x[i].val < 0 || y[i].val < 0) {
        return result;
      }
      if (x[i].val == 0 || y[i].val == 0) continue;
      if (x[i].val != y[i].val || (radius != 0 && radius != x[i].val)) return result;
      radius = x[i].val;
      selected |= corners[i];
    }
    if (radius > MIN(size.width, size.height) * 0.5) return result;
    result = (LynxLayerCornerRadii){YES, radius, selected};
  }
  return result;
}

static inline void LynxSetLayerCornerRadii(CALayer *layer, LynxLayerCornerRadii radii) {
  layer.cornerRadius = radii.eligible ? radii.radius : 0;
  if (@available(iOS 11.0, *)) {
    layer.maskedCorners = radii.eligible && radii.corners
                              ? radii.corners
                              : kCALayerMinXMinYCorner | kCALayerMaxXMinYCorner |
                                    kCALayerMinXMaxYCorner | kCALayerMaxXMaxYCorner;
  }
}

#endif  // LYNX_PLATFORM_DARWIN_IOS_LYNX_BASE_BACKGROUND_LYNXLAYERCORNERRADII_H_
