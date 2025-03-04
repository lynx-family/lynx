// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

static const CGFloat LYNX_BORDER_THREAHOLD = 0.001f;

static inline CGRect LynxGetBoundsAutoAdjust(CGRect bounds) {
  return CGRectMake(CGRectGetMinX(bounds), CGRectGetMinY(bounds), MAX(0, bounds.size.width),
                    MAX(0, bounds.size.height));
}

static inline BOOL LynxBorderInsetsAreEqual(UIEdgeInsets borderInsets) {
  return ABS(borderInsets.left - borderInsets.right) < LYNX_BORDER_THREAHOLD &&
         ABS(borderInsets.left - borderInsets.bottom) < LYNX_BORDER_THREAHOLD &&
         ABS(borderInsets.left - borderInsets.top) < LYNX_BORDER_THREAHOLD;
}

static inline CGFloat LynxBorderUnitValToFloatA(LynxBorderUnitValue val, CGFloat reference,
                                                LynxPlatformLength* calc) {
  if (val.unit == LynxBorderValueUnitDefault) {
    return MAX(0, val.val);
  } else if (val.unit == LynxBorderValueUnitPercent) {
    return MAX(0, val.val * reference);
  } else if (calc) {
    return [calc valueWithParentValue:reference];
  }
  return .0f;
}

static inline CGFloat LynxBorderUnitValToFloat(LynxBorderUnitValue val, CGFloat percentBase) {
  return LynxBorderUnitValToFloatA(val, percentBase, NULL);
}

static inline BOOL LynxBorderColorsAreEqual(LynxBorderColors borderColors) {
  return CGColorEqualToColor(borderColors.left, borderColors.right) &&
         CGColorEqualToColor(borderColors.left, borderColors.top) &&
         CGColorEqualToColor(borderColors.left, borderColors.bottom);
}

static inline BOOL LynxBorderStylesAreEqual(LynxBorderStyles borderStyles) {
  return borderStyles.left == borderStyles.right && borderStyles.left == borderStyles.top &&
         borderStyles.left == borderStyles.bottom;
}

static inline void LynxEllipseGetIntersectionsWithLine(CGRect ellipseBounds, CGPoint lineStart,
                                                       CGPoint lineEnd, CGPoint intersections[2]) {
  const CGPoint ellipseCenter = {CGRectGetMidX(ellipseBounds), CGRectGetMidY(ellipseBounds)};

  lineStart.x -= ellipseCenter.x;
  lineStart.y -= ellipseCenter.y;
  lineEnd.x -= ellipseCenter.x;
  lineEnd.y -= ellipseCenter.y;

  const CGFloat m = (lineEnd.y - lineStart.y) / (lineEnd.x - lineStart.x);
  const CGFloat a = ellipseBounds.size.width / 2;
  const CGFloat b = ellipseBounds.size.height / 2;
  const CGFloat c = lineStart.y - m * lineStart.x;
  const CGFloat A = (b * b + a * a * m * m);
  const CGFloat B = 2 * a * a * c * m;
  const CGFloat D = sqrt((a * a * (b * b - c * c)) / A + pow(B / (2 * A), 2));

  const CGFloat x_ = -B / (2 * A);
  const CGFloat x1 = x_ + D;
  const CGFloat x2 = x_ - D;
  const CGFloat y1 = m * x1 + c;
  const CGFloat y2 = m * x2 + c;

  intersections[0] = (CGPoint){x1 + ellipseCenter.x, y1 + ellipseCenter.y};
  intersections[1] = (CGPoint){x2 + ellipseCenter.x, y2 + ellipseCenter.y};
}

static inline LynxCornerInsetPoints LynxCalculateCornerInsetPoints(UIEdgeInsets borderInsets,
                                                                   LynxCornerInsets cornerInsets,
                                                                   CGSize size) {
  LynxCornerInsetPoints ret;

  ret.topLeft = (CGPoint){borderInsets.left, borderInsets.top};
  if (cornerInsets.topLeft.width > 0 && cornerInsets.topLeft.height > 0) {
    CGPoint points[2];
    LynxEllipseGetIntersectionsWithLine(
        (CGRect){ret.topLeft, {2 * cornerInsets.topLeft.width, 2 * cornerInsets.topLeft.height}},
        CGPointZero, ret.topLeft, points);
    if (!isnan(points[1].x) && !isnan(points[1].y)) {
      ret.topLeft = points[1];
    }
  }

  ret.bottomLeft = (CGPoint){borderInsets.left, size.height - borderInsets.bottom};
  if (cornerInsets.bottomLeft.width > 0 && cornerInsets.bottomLeft.height > 0) {
    CGPoint points[2];
    LynxEllipseGetIntersectionsWithLine(
        (CGRect){{ret.bottomLeft.x, ret.bottomLeft.y - 2 * cornerInsets.bottomLeft.height},
                 {2 * cornerInsets.bottomLeft.width, 2 * cornerInsets.bottomLeft.height}},
        (CGPoint){0, size.height}, ret.bottomLeft, points);
    if (!isnan(points[1].x) && !isnan(points[1].y)) {
      ret.bottomLeft = points[1];
    }
  }

  ret.topRight = (CGPoint){size.width - borderInsets.right, borderInsets.top};
  if (cornerInsets.topRight.width > 0 && cornerInsets.topRight.height > 0) {
    CGPoint points[2];
    LynxEllipseGetIntersectionsWithLine(
        (CGRect){{ret.topRight.x - 2 * cornerInsets.topRight.width, ret.topRight.y},
                 {2 * cornerInsets.topRight.width, 2 * cornerInsets.topRight.height}},
        (CGPoint){size.width, 0}, ret.topRight, points);
    if (!isnan(points[0].x) && !isnan(points[0].y)) {
      ret.topRight = points[0];
    }
  }

  ret.bottomRight = (CGPoint){size.width - borderInsets.right, size.height - borderInsets.bottom};
  if (cornerInsets.bottomRight.width > 0 && cornerInsets.bottomRight.height > 0) {
    CGPoint points[2];
    LynxEllipseGetIntersectionsWithLine(
        (CGRect){{ret.bottomRight.x - 2 * cornerInsets.bottomRight.width,
                  ret.bottomRight.y - 2 * cornerInsets.bottomRight.height},
                 {2 * cornerInsets.bottomRight.width, 2 * cornerInsets.bottomRight.height}},
        (CGPoint){size.width, size.height}, ret.bottomRight, points);
    if (!isnan(points[0].x) && !isnan(points[0].y)) {
      ret.bottomRight = points[0];
    }
  }
  return ret;
}

static inline CGColorRef LynxCreateDarkenColor(CGColorRef color, bool darken) {
  CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
  CGColorRef result = 0;
  const CGFloat mul = (darken ? 0.618 : 1.0);
  const size_t count = CGColorGetNumberOfComponents(color);
  const CGFloat* arr = CGColorGetComponents(color);
  if (arr != 0 && (count == 2 || count == 4)) {
    if (count == 2) {
      CGFloat newVal[4] = {arr[0] * mul, arr[0] * mul, arr[0] * mul, arr[1]};
      result = CGColorCreate(space, newVal);
    } else {
      CGFloat newVal[4] = {arr[0] * mul, arr[1] * mul, arr[2] * mul, arr[3]};
      result = CGColorCreate(space, newVal);
    }
  }
  if (result == 0) {
    if (color == 0) {
      const CGFloat gray = mul * 0.618;
      CGFloat newVal[4] = {gray, gray, gray, 1};
      result = CGColorCreate(space, newVal);
    } else {
      result = CGColorCreateCopy(color);
    }
  }
  CGColorSpaceRelease(space);
  return result;
}

static inline CGSize LynxRoundViewSize(CGSize viewSize) {
  return CGSizeMake(
      round(viewSize.width * [UIScreen mainScreen].scale) / [UIScreen mainScreen].scale,
      round(viewSize.height * [UIScreen mainScreen].scale) / [UIScreen mainScreen].scale);
}
