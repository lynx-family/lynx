// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundRenderer+Internal.h"

// Currently, the dashed / dotted implementation only supports a single colour +
// single width, as that's currently required and supported on Android.
//
// Supporting individual widths + colours on each side is possible by modifying
// the current implementation. The idea is that we will draw four different
// lines
// and clip appropriately for each side (might require adjustment of phase so
// that
// they line up but even browsers don't do a good job at that).
//
// Firstly, create two paths for the outer and inner paths. The inner path is
// generated exactly the same way as the outer, just given an inset rect,
// derived
// from the insets on each side. Then clip using the odd-even rule
// (CGContextEOClip()). This will give us a nice rounded (possibly) clip mask.
//
// +----------------------------------+
// |@@@@@@@@  Clipped Space  @@@@@@@@@|
// |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
// |@@+----------------------+@@@@@@@@|
// |@@|                      |@@@@@@@@|
// |@@|                      |@@@@@@@@|
// |@@|                      |@@@@@@@@|
// |@@+----------------------+@@@@@@@@|
// |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
// +----------------------------------+
//
// Afterwards, we create a clip path for each border side (CGContextSaveGState()
// and CGContextRestoreGState() when drawing each side). The clip mask for each
// segment is a trapezoid connecting corresponding edges of the inner and outer
// rects. For example, in the case of the top edge, the points would be:
// - (MinX(outer), MinY(outer))
// - (MaxX(outer), MinY(outer))
// - (MinX(inner) + topLeftRadius, MinY(inner) + topLeftRadius)
// - (MaxX(inner) - topRightRadius, MinY(inner) + topRightRadius)
//
//         +------------------+
//         |\                /|
//         | \              / |
//         |  \    top     /  |
//         |   \          /   |
//         |    \        /    |
//         |     +------+     |
//         |     |      |     |
//         |     |      |     |
//         |     |      |     |
//         |left |      |right|
//         |     |      |     |
//         |     |      |     |
//         |     +------+     |
//         |    /        \    |
//         |   /          \   |
//         |  /            \  |
//         | /    bottom    \ |
//         |/                \|
//         +------------------+
//
//
// Note that this approach will produce discontinous colour changes at the edge
// (which is okay). The reason is that Quartz does not currently support drawing
// of gradients _along_ a path (NB: clipping a path and drawing a linear
// gradient
// is _not_ equivalent).

static inline CGContextRef LynxUIGraphicsBeginImageContext(CGSize size, CGColorRef backgroundColor,
                                                           BOOL hasCornerRadii, BOOL drawToEdge,
                                                           BOOL* hasOldCtx,
                                                           LynxBackgroundClipType clipType) {
  const CGFloat alpha = CGColorGetAlpha(backgroundColor);
  const BOOL opaque =
      (drawToEdge || !hasCornerRadii) && alpha == 1.0 && clipType == LynxBackgroundClipBorderBox;
  CGContextRef oldCtx = UIGraphicsGetCurrentContext();
  if (oldCtx) {
    UIGraphicsPushContext(oldCtx);
    *hasOldCtx = YES;
  }
  UIGraphicsBeginImageContextWithOptions(size, opaque, 0.0);
  return UIGraphicsGetCurrentContext();
}

static inline void LynxDrawSolidInsetOrOutsetBorder(
    CGContextRef ctx, LynxBorderStyle borderStyle, CGSize size, LynxBorderRadii cornerRadii,
    UIEdgeInsets borderInsets, LynxBorderColors borderColors, BOOL drawToEdge, BOOL hasCornerRadii,
    CGPathRef path, CGPathRef insetPath, const LynxCornerInsets cornerInsets) {
  BOOL hasEqualColors = FALSE, toReleaseBorderColors = FALSE;
  switch (borderStyle) {
    case LynxBorderStyleInset:
      borderColors.left = LynxCreateDarkenColor(borderColors.left, true);
      borderColors.top = LynxCreateDarkenColor(borderColors.top, true);
      borderColors.bottom = LynxCreateDarkenColor(borderColors.bottom, false);
      borderColors.right = LynxCreateDarkenColor(borderColors.right, false);
      toReleaseBorderColors = TRUE;
      break;
    case LynxBorderStyleOutset:
      borderColors.left = LynxCreateDarkenColor(borderColors.left, false);
      borderColors.top = LynxCreateDarkenColor(borderColors.top, false);
      borderColors.bottom = LynxCreateDarkenColor(borderColors.bottom, true);
      borderColors.right = LynxCreateDarkenColor(borderColors.right, true);
      toReleaseBorderColors = TRUE;
      break;
    default:
      hasEqualColors = LynxBorderColorsAreEqual(borderColors);
      break;
  }

  if (hasEqualColors && (drawToEdge || !hasCornerRadii)) {
    CGContextSetFillColorWithColor(ctx, borderColors.left);
    const CGRect rect = {.size = size};
    CGContextAddRect(ctx, rect);
    CGContextAddPath(ctx, insetPath);
    CGContextEOFillPath(ctx);
    return;
  }

  const int sides = 4;
  LynxCornerInsetPoints insetCorners =
      LynxCalculateCornerInsetPoints(borderInsets, cornerInsets, size);
  const CGPoint points[sides][4] = {
      {CGPointZero, insetCorners.topLeft, insetCorners.bottomLeft,
       (CGPoint){0, size.height}},                                                           // left
      {CGPointZero, insetCorners.topLeft, insetCorners.topRight, (CGPoint){size.width, 0}},  // top
      {
          (CGPoint){size.width, 0},
          insetCorners.topRight,
          insetCorners.bottomRight,
          (CGPoint){size.width, size.height},
      },  // right
      {
          (CGPoint){0, size.height},
          insetCorners.bottomLeft,
          insetCorners.bottomRight,
          (CGPoint){size.width, size.height},
      }  // bottom
  };
  const CGFloat insets[sides] = {borderInsets.left, borderInsets.top, borderInsets.right,
                                 borderInsets.bottom};
  CGColorRef colors[sides] = {borderColors.left, borderColors.top, borderColors.right,
                              borderColors.bottom};

  if (hasCornerRadii && LynxBorderInsetsNotLargeThan(borderInsets, 1.1f)) {
    // use stroke mode
    const CGRect rect = {.size = size};
    const UIEdgeInsets centerInsets = LynxGetEdgeInsets(rect, borderInsets, 0.5);
    CGPathRef centerPathForStroke =
        LynxPathCreateWithRoundedRect(UIEdgeInsetsInsetRect(rect, centerInsets),
                                      LynxGetCornerInsets(rect, cornerRadii, centerInsets));
    for (int i = 0; i < sides; ++i) {
      if (insets[i] <= 0) continue;
      CGContextSaveGState(ctx);
      CGContextSetAllowsAntialiasing(ctx, true);
      CGContextAddLines(ctx, points[i], 4);
      CGContextClip(ctx);
      CGContextSetStrokeColorWithColor(ctx, colors[i]);
      CGContextSetLineWidth(ctx, insets[i]);
      CGContextAddPath(ctx, centerPathForStroke);
      CGContextStrokePath(ctx);
      CGContextRestoreGState(ctx);
    }
    CGPathRelease(centerPathForStroke);

  } else {
    // add clip
    if (path != NULL) {
      CGContextAddPath(ctx, path);
      CGContextAddPath(ctx, insetPath);
      CGContextEOClip(ctx);
    }

    CGColorRef currentColor = NULL;
    for (int i = 0; i < sides; ++i) {
      if (i > 0 && !CGColorEqualToColor(currentColor, colors[i])) {
        CGContextSetFillColorWithColor(ctx, currentColor);
        CGContextFillPath(ctx);
        currentColor = colors[i];
      }
      if (insets[i] > 0) {
        currentColor = colors[i];
        CGContextAddLines(ctx, points[i], 4);
      }
    }
    CGContextSetFillColorWithColor(ctx, currentColor);
    CGContextFillPath(ctx);
  }

  if (toReleaseBorderColors) {
    CGColorRelease(borderColors.top);
    CGColorRelease(borderColors.right);
    CGColorRelease(borderColors.bottom);
    CGColorRelease(borderColors.left);
  }
}

static inline void LynxStrokeDashedOrDottedBorderLine(CGContextRef ctx, bool isDotted,
                                                      CGColorRef borderColor, CGPoint startPoint,
                                                      CGPoint endPoint, float borderLength,
                                                      float borderWidth) {
  float sectionLen = (borderWidth >= 1 ? borderWidth : 1) * (isDotted ? 2 : 6) * 0.5f;
  int newSectionCount = ((int)((borderLength / sectionLen - 0.5f) * 0.5f)) * 2 + 1;
  if (newSectionCount > 1) {
    CGFloat dashLengths[2];
    dashLengths[0] = dashLengths[1] = borderLength / newSectionCount;
    CGContextSetLineDash(ctx, 0, dashLengths, 2);
  }

  CGContextSetStrokeColorWithColor(ctx, borderColor);
  CGContextBeginPath(ctx);
  CGContextSetLineWidth(ctx, borderWidth);
  CGContextMoveToPoint(ctx, startPoint.x, startPoint.y);
  CGContextAddLineToPoint(ctx, endPoint.x, endPoint.y);
  CGContextStrokePath(ctx);

  CGContextSetLineDash(ctx, 0, 0, 0);
}

static inline void LynxSaveAndClipQuadrilateralFromPoints(CGContextRef ctx,
                                                          const CGPoint* pointsForClip) {
  CGContextSaveGState(ctx);

  CGContextAddLines(ctx, pointsForClip, 4);
  CGContextClosePath(ctx);
  CGContextClip(ctx);
}

static inline void LynxStrokeDashDottedCenterPath(CGContextRef ctx, BOOL isDotted, CGPathRef path,
                                                  CGColorRef borderColor,
                                                  CGFloat borderWidthForEffect,
                                                  CGFloat borderWidthForStroke) {
  CGContextSetStrokeColorWithColor(ctx, borderColor);
  CGContextSetLineWidth(ctx, borderWidthForStroke);

  CGFloat dashLengths[2];
  dashLengths[0] = dashLengths[1] = (isDotted ? 1 : 3) * borderWidthForEffect;
  CGContextSetLineDash(ctx, 0, dashLengths, 2);

  CGContextAddPath(ctx, path);
  CGContextStrokePath(ctx);

  CGContextSetLineDash(ctx, 0, 0, 0);
}

static inline void LynxDrawDashedOrDottedRectWithSameColor(CGContextRef ctx,
                                                           LynxBorderStyle borderStyle, CGSize size,
                                                           UIEdgeInsets borderInsets,
                                                           CGColorRef borderColor) {
  NSCParameterAssert(borderStyle == LynxBorderStyleDashed || borderStyle == LynxBorderStyleDotted);

  const BOOL isDotted = (borderStyle == LynxBorderStyleDotted);

  // stroke borders (top->right->bottom->left)
  if (borderInsets.top > 0) {
    const float topCenter = borderInsets.top * 0.5f;
    const float topEnd = size.width - (borderInsets.right > 0 ? borderInsets.right : 0);
    LynxStrokeDashedOrDottedBorderLine(ctx, isDotted, borderColor, CGPointMake(0, topCenter),
                                       CGPointMake(topEnd, topCenter), size.width,
                                       borderInsets.top);
  }

  if (borderInsets.right > 0) {
    const float rightCenter = size.width - borderInsets.right * 0.5f;
    const float rightEnd = size.height - (borderInsets.bottom > 0 ? borderInsets.bottom : 0);
    LynxStrokeDashedOrDottedBorderLine(ctx, isDotted, borderColor, CGPointMake(rightCenter, 0),
                                       CGPointMake(rightCenter, rightEnd), size.height,
                                       borderInsets.right);
  }

  if (borderInsets.bottom > 0) {
    const float bottomCenter = size.height - borderInsets.bottom * 0.5f;
    const float bottomEnd = (borderInsets.left > 0 ? borderInsets.left : 0);
    LynxStrokeDashedOrDottedBorderLine(
        ctx, isDotted, borderColor, CGPointMake(size.width, bottomCenter),
        CGPointMake(bottomEnd, bottomCenter), size.width, borderInsets.bottom);
  }

  if (borderInsets.left > 0) {
    const float leftCenter = borderInsets.left * 0.5f;
    const float leftEnd = (borderInsets.top > 0 ? borderInsets.top : 0);
    LynxStrokeDashedOrDottedBorderLine(
        ctx, isDotted, borderColor, CGPointMake(leftCenter, size.height),
        CGPointMake(leftCenter, leftEnd), size.height, borderInsets.left);
  }
}

static inline void LynxDrawDashedOrDottedRoundRectWithSameColor(
    CGContextRef ctx, LynxBorderStyle borderStyle, CGSize size, LynxBorderRadii cornerRadii,
    UIEdgeInsets borderInsets, LynxBorderColors borderColors, BOOL drawToEdge, BOOL hasCornerRadii,
    CGPathRef insetPath, const LynxCornerInsets cornerInsets) {
  NSCParameterAssert(borderStyle == LynxBorderStyleDashed || borderStyle == LynxBorderStyleDotted);
  const BOOL isDotted = (borderStyle == LynxBorderStyleDotted);
  const CGRect rect = {.size = size};
  const UIEdgeInsets centerInsets = LynxGetEdgeInsets(rect, borderInsets, 0.5);
  CGPathRef newPath =
      LynxPathCreateWithRoundedRect(UIEdgeInsetsInsetRect(rect, centerInsets),
                                    LynxGetCornerInsets(rect, cornerRadii, centerInsets));
  if (newPath) {
    LynxStrokeDashDottedCenterPath(ctx, isDotted, newPath, borderColors.left, borderInsets.left,
                                   borderInsets.left);

    CGPathRelease(newPath);
  }
}

static inline LynxRenderBorderStyle LynxToRenderBorderStyle(LynxBorderStyle style) {
  switch (style) {
    case LynxBorderStyleNone:
    case LynxBorderStyleHidden:
      return LynxRenderBorderStyleNone;

    case LynxBorderStyleDashed:
    case LynxBorderStyleDotted:
      return LynxRenderBorderStyleDashedOrDotted;

    case LynxBorderStyleDouble:
    case LynxBorderStyleGroove:
    case LynxBorderStyleRidge:
      return LynxRenderBorderStyleDoubleGrooveOrRidge;

    case LynxBorderStyleSolid:
    case LynxBorderStyleInset:
    case LynxBorderStyleOutset:
    default:
      return LynxRenderBorderStyleSolidInsetOrOutset;
  }
}

static inline void LynxDrawBorderSideDoubleGrooveOrRidge(CGContextRef ctx,
                                                         LynxRenderBorderSideInfo* const info,
                                                         CGPathRef* _centerInsetPathArr) {
  CGFloat lineWidth = info->width * 0.5f;
  bool toReleaseColor = false;
  CGColorRef colors[2] = {info->color, info->color};
  LynxBorderPathId pathId[2] = {LynxBorderPathId14, LynxBorderPathId34};
  switch (info->style) {
    case LynxBorderStyleGroove:
      toReleaseColor = true;
      colors[0] = LynxCreateDarkenColor(info->color, info->isLeftOrTop);
      colors[1] = LynxCreateDarkenColor(info->color, !info->isLeftOrTop);
      break;
    case LynxBorderStyleRidge:
      toReleaseColor = true;
      colors[0] = LynxCreateDarkenColor(info->color, !info->isLeftOrTop);
      colors[1] = LynxCreateDarkenColor(info->color, info->isLeftOrTop);
      break;
    case LynxBorderStyleDouble:
      lineWidth = info->width / 3.0f;
      pathId[0] = LynxBorderPathId16;
      pathId[1] = LynxBorderPathId56;
      break;
    default:
      return;
  }

  LynxSaveAndClipQuadrilateralFromPoints(ctx, info->clipPoints);
  for (int i = 0; i < 2; ++i) {
    CGContextSetStrokeColorWithColor(ctx, colors[i]);
    CGContextSetLineWidth(ctx, lineWidth);
    CGPathRef path = _centerInsetPathArr[pathId[i]];
    if (path) {
      CGContextAddPath(ctx, path);
    }
    CGContextStrokePath(ctx);
  }
  if (toReleaseColor) {
    CGColorRelease(colors[0]);
    CGColorRelease(colors[1]);
  }
  CGContextRestoreGState(ctx);
}

static inline void LynxInitBorderCenterPaths(CGPathRef* _centerInsetPath, CGRect rect,
                                             LynxBorderStyles borderStyles,
                                             UIEdgeInsets borderInsets, LynxBorderRadii cornerRadii,
                                             BOOL drawRoundRect) {
  BOOL pathIdArr[LynxBorderPathIdCount];
  memset(pathIdArr, 0, sizeof(pathIdArr));
  const LynxBorderStyle styles[4] = {borderStyles.top, borderStyles.right, borderStyles.bottom,
                                     borderStyles.left};
  for (int i = 0; i < 4; ++i) {
    switch (styles[i]) {
      case LynxBorderStyleDashed:
      case LynxBorderStyleDotted:
        if (drawRoundRect) {
          pathIdArr[LynxBorderPathId12] = TRUE;
        }
        break;
      case LynxBorderStyleDouble:
        pathIdArr[LynxBorderPathId16] = pathIdArr[LynxBorderPathId56] = TRUE;
        break;
      case LynxBorderStyleGroove:
      case LynxBorderStyleRidge:
      case LynxBorderStyleInset:
      case LynxBorderStyleOutset:
        pathIdArr[LynxBorderPathId14] = pathIdArr[LynxBorderPathId34] = TRUE;
        break;
      default:
        break;
    }
  }
  const CGFloat mulArr[] = {1.0 / 6.0, 0.25, 0.5, 0.75, 5.0 / 6.0};
  for (int index = 0; index < LynxBorderPathIdCount; ++index) {
    if (!pathIdArr[index] || _centerInsetPath[index] != 0) continue;
    const CGFloat mul = mulArr[index];
    const UIEdgeInsets centerInsets = LynxGetEdgeInsets(rect, borderInsets, mul);
    CGPathRef newPath =
        LynxPathCreateWithRoundedRect(UIEdgeInsetsInsetRect(rect, centerInsets),
                                      LynxGetCornerInsets(rect, cornerRadii, centerInsets));
    _centerInsetPath[index] = newPath;
  }
}

static inline void LynxDrawBorderSide(CGContextRef ctx, LynxRenderBorderSideInfo* const info,
                                      bool drawRoundRect, CGPathRef* centerInsetPathArr) {
  const LynxRenderBorderStyle renderStyle = LynxToRenderBorderStyle(info->style);
  switch (renderStyle) {
    case LynxRenderBorderStyleDashedOrDotted: {
      LynxSaveAndClipQuadrilateralFromPoints(ctx, info->clipPoints);
      const BOOL isDotted = (info->style == LynxBorderStyleDotted);
      if (drawRoundRect) {
        CGPathRef path = centerInsetPathArr[LynxBorderPathId12];
        if (path) {
          LynxStrokeDashDottedCenterPath(ctx, isDotted, path, info->color, info->width,
                                         info->maxWidth);
        }
      } else {
        LynxStrokeDashedOrDottedBorderLine(ctx, isDotted, info->color, info->linePoints[0],
                                           info->linePoints[1], info->length, info->width);
      }
      CGContextRestoreGState(ctx);
    } break;
    case LynxRenderBorderStyleSolidInsetOrOutset: {
      CGColorRef colorToRelease = 0;
      CGContextBeginPath(ctx);
      CGContextAddLines(ctx, info->clipPoints, 4);
      switch (info->style) {
        case LynxBorderStyleInset:
          colorToRelease = LynxCreateDarkenColor(info->color, info->isLeftOrTop);
          break;
        case LynxBorderStyleOutset:
          colorToRelease = LynxCreateDarkenColor(info->color, !info->isLeftOrTop);
          break;
        default:
          break;
      }
      CGContextSetFillColorWithColor(ctx, (colorToRelease ? colorToRelease : info->color));
      CGContextFillPath(ctx);
      if (colorToRelease) {
        CGColorRelease(colorToRelease);
      }
    } break;
    case LynxRenderBorderStyleDoubleGrooveOrRidge:
      LynxDrawBorderSideDoubleGrooveOrRidge(ctx, info, centerInsetPathArr);
      break;
    case LynxRenderBorderStyleNone:
    default:
      break;
  }
}

static inline void LynxDrawBorders(CGContextRef ctx, LynxBorderStyles borderStyles, CGSize size,
                                   LynxBorderRadii cornerRadii, UIEdgeInsets borderInsets,
                                   LynxBorderColors borderColors, BOOL drawToEdge,
                                   BOOL hasCornerRadii, CGPathRef path, CGPathRef insetPath,
                                   const LynxCornerInsets cornerInsets) {
  const BOOL drawRoundRect = (!drawToEdge && hasCornerRadii);
  const BOOL hasEqualColors = LynxBorderColorsAreEqual(borderColors);

  if (LynxBorderStylesAreEqual(borderStyles)) {
    const LynxRenderBorderStyle renderStyle = LynxToRenderBorderStyle(borderStyles.left);
    switch (renderStyle) {
      case LynxRenderBorderStyleNone:
        return;
      case LynxRenderBorderStyleDashedOrDotted:
        if (hasEqualColors) {
          if (!drawRoundRect) {
            LynxDrawDashedOrDottedRectWithSameColor(ctx, borderStyles.left, size, borderInsets,
                                                    borderColors.left);
            return;
          } else if (LynxBorderInsetsAreEqual(borderInsets)) {
            LynxDrawDashedOrDottedRoundRectWithSameColor(ctx, borderStyles.left, size, cornerRadii,
                                                         borderInsets, borderColors, drawToEdge,
                                                         hasCornerRadii, insetPath, cornerInsets);
            return;
          }
        } else {
          break;
        }
      case LynxRenderBorderStyleSolidInsetOrOutset:
        LynxDrawSolidInsetOrOutsetBorder(ctx, borderStyles.left, size, cornerRadii, borderInsets,
                                         borderColors, drawToEdge, hasCornerRadii, path, insetPath,
                                         cornerInsets);
        return;
      default:
        break;
    }
  }

  // add clip
  CGContextAddPath(ctx, path);
  CGContextAddPath(ctx, insetPath);
  CGContextEOClip(ctx);

  // separate all sides
  const CGRect rect = {.size = size};
  LynxCornerInsetPoints insetCorners =
      LynxCalculateCornerInsetPoints(borderInsets, cornerInsets, rect.size);

  CGFloat maxBorderWidth = MAX(0, MAX(MAX(borderInsets.left, borderInsets.right),
                                      MAX(borderInsets.top, borderInsets.bottom)));
  CGPathRef centerInsetPathArr[LynxBorderPathIdCount];
  memset(centerInsetPathArr, 0, sizeof(centerInsetPathArr));
  LynxInitBorderCenterPaths(centerInsetPathArr, rect, borderStyles, borderInsets, cornerRadii,
                            drawRoundRect);

  // TOP
  if (borderInsets.top > 0) {
    const float topCenter = borderInsets.top * 0.5f;
    LynxRenderBorderSideInfo info = {.style = borderStyles.top,
                                     .color = borderColors.top,
                                     .width = borderInsets.top,
                                     .length = size.width,
                                     .maxWidth = maxBorderWidth,
                                     .isLeftOrTop = true,
                                     .clipPoints =
                                         {
                                             CGPointZero,
                                             insetCorners.topLeft,
                                             insetCorners.topRight,
                                             (CGPoint){size.width, 0},
                                         },
                                     .linePoints = {{0, topCenter}, {size.width, topCenter}}};
    LynxDrawBorderSide(ctx, &info, drawRoundRect, centerInsetPathArr);
  }

  // RIGHT
  if (borderInsets.right > 0) {
    const float rightCenter = size.width - borderInsets.right * 0.5f;
    LynxRenderBorderSideInfo info = {.style = borderStyles.right,
                                     .color = borderColors.right,
                                     .width = borderInsets.right,
                                     .length = size.height,
                                     .maxWidth = maxBorderWidth,
                                     .isLeftOrTop = false,
                                     .clipPoints =
                                         {
                                             (CGPoint){size.width, 0},
                                             insetCorners.topRight,
                                             insetCorners.bottomRight,
                                             (CGPoint){size.width, size.height},
                                         },
                                     .linePoints = {
                                         {rightCenter, 0},
                                         {rightCenter, size.height},
                                     }};
    LynxDrawBorderSide(ctx, &info, drawRoundRect, centerInsetPathArr);
  }

  // BOTTOM
  if (borderInsets.bottom > 0) {
    const float bottomCenter = size.height - borderInsets.bottom * 0.5f;
    LynxRenderBorderSideInfo info = {.style = borderStyles.bottom,
                                     .color = borderColors.bottom,
                                     .width = borderInsets.bottom,
                                     .length = size.width,
                                     .maxWidth = maxBorderWidth,
                                     .isLeftOrTop = false,
                                     .clipPoints =
                                         {
                                             (CGPoint){0, size.height},
                                             insetCorners.bottomLeft,
                                             insetCorners.bottomRight,
                                             (CGPoint){size.width, size.height},
                                         },
                                     .linePoints = {{size.width, bottomCenter}, {0, bottomCenter}}};
    LynxDrawBorderSide(ctx, &info, drawRoundRect, centerInsetPathArr);
  }

  // LEFT
  if (borderInsets.left > 0) {
    const float leftCenter = borderInsets.left * 0.5f;
    LynxRenderBorderSideInfo info = {.style = borderStyles.left,
                                     .color = borderColors.left,
                                     .width = borderInsets.left,
                                     .length = size.height,
                                     .maxWidth = maxBorderWidth,
                                     .isLeftOrTop = true,
                                     .clipPoints =
                                         {
                                             CGPointZero,
                                             insetCorners.topLeft,
                                             insetCorners.bottomLeft,
                                             (CGPoint){0, size.height},
                                         },
                                     .linePoints = {{leftCenter, size.height}, {leftCenter, 0}}};
    LynxDrawBorderSide(ctx, &info, drawRoundRect, centerInsetPathArr);
  }
  for (int i = 0; i < LynxBorderPathIdCount; ++i) {
    CGPathRelease(centerInsetPathArr[i]);
  }
}

static inline void LynxDoDrawOutlineSubRect(CGContextRef ctx, LynxBorderStyle style, UIColor* color,
                                            float width, CGRect rect) {
  LynxBorderRadii cornerRadii;
  LynxCornerInsets cornerInsets;
  memset(&cornerRadii, 0, sizeof(cornerRadii));
  memset(&cornerInsets, 0, sizeof(cornerInsets));
  LynxBorderColors colors;
  colors.bottom = colors.left = colors.right = colors.top = color.CGColor;
  CGRect insetRect =
      CGRectMake(width, width, rect.size.width - 2 * width, rect.size.height - 2 * width);
  CGPathRef insetPath = CGPathCreateWithRect(insetRect, nil);
  const UIEdgeInsets borderInsets = {width, width, width, width};
  CGContextSaveGState(ctx);
  CGContextTranslateCTM(ctx, rect.origin.x, rect.origin.y);
  LynxDrawSolidInsetOrOutsetBorder(ctx, style, rect.size, cornerRadii, borderInsets, colors, YES,
                                   NO, NULL, insetPath, cornerInsets);
  CGContextRestoreGState(ctx);
  CGPathRelease(insetPath);
}
