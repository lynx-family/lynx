// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundUtils.h"
#import "LynxBackgroundImageLayerInfo.h"
#import "LynxBackgroundRenderer.h"
#import "LynxUI+Internal.h"
#import "LynxUIUnitUtils.h"

void LynxDrawBackgroundToImageContext(LynxBorderRadii cornerRadii, UIEdgeInsets borderInsets,
                                      CGColorRef backgroundColor, BOOL drawToEdge,
                                      NSArray* bgImageInfoArr, CGRect rect, CGContextRef ctx) {
  CGPathRef path;
  if (drawToEdge) {
    path = CGPathCreateWithRect((rect), NULL);
  } else {
    path = [LynxBackgroundUtils createBezierPathWithRoundedRect:rect
                                                    borderRadii:cornerRadii
                                                     edgeInsets:borderInsets];
  }

  // draw backgrounds
  if (backgroundColor) {
    CGContextSetFillColorWithColor(ctx, backgroundColor);
    CGContextAddPath(ctx, path);
    CGContextFillPath(ctx);
  }
  CGPathRelease(path);

  if (bgImageInfoArr != nil) {
    const int count = (int)bgImageInfoArr.count;
    for (int i = count - 1; i >= 0; --i) {
      LynxBackgroundImageLayerInfo* layerInfo = [bgImageInfoArr objectAtIndex:i];
      [layerInfo drawInContext:ctx];
    }
  }
}

void LynxDrawBorderToImageContext(LynxBorderStyles borderStyles, CGSize viewSize,
                                  LynxBorderRadii cornerRadii, UIEdgeInsets borderInsets,
                                  LynxBorderColors borderColors, BOOL drawToEdge, CGRect rect,
                                  const LynxCornerInsets cornerInsets, const BOOL hasCornerRadii,
                                  CGContextRef ctx) {
  CGPathRef path;
  if (drawToEdge) {
    path = CGPathCreateWithRect(rect, NULL);
  } else {
    path = LynxPathCreateWithRoundedRect(rect, cornerInsets);
  }

  const LynxCornerInsets insetCornerInsets = LynxGetCornerInsets(rect, cornerRadii, borderInsets);
  CGPathRef insetPath = LynxPathCreateWithRoundedRect(LynxGetRectWithEdgeInsets(rect, borderInsets),
                                                      insetCornerInsets);
  LynxDrawBorders(ctx, borderStyles, viewSize, cornerRadii, borderInsets, borderColors, drawToEdge,
                  hasCornerRadii, path, insetPath, cornerInsets);

  CGPathRelease(insetPath);
  CGPathRelease(path);
}

@implementation LynxBackgroundUtils
+ (CGPathRef)createBezierPathWithRoundedRect:(CGRect)bounds
                                 borderRadii:(LynxBorderRadii)borderRadii
                                  edgeInsets:(UIEdgeInsets)edgeInsets {
  const CGRect rect = LynxGetBoundsAutoAdjust(bounds);
  const UIEdgeInsets borders = LynxGetEdgeInsets(rect, edgeInsets, 1.0);
  return LynxPathCreateWithRoundedRect(LynxGetRectWithEdgeInsets(rect, borders),
                                       LynxGetCornerInsets(rect, borderRadii, borders));
}

+ (CGPathRef)createBezierPathWithRoundedRect:(CGRect)bounds
                                 borderRadii:(LynxBorderRadii)borderRadii {
  const CGRect rect = LynxGetBoundsAutoAdjust(bounds);
  return LynxPathCreateWithRoundedRect(rect,
                                       LynxGetCornerInsets(rect, borderRadii, UIEdgeInsetsZero));
}

#pragma mark path painting

void LynxPathAddEllipticArc(CGMutablePathRef path, CGPoint origin, CGFloat width, CGFloat height,
                            CGFloat startAngle, CGFloat endAngle, BOOL clockwise) {
  CGFloat xScale = 1, yScale = 1, radius = 0;
  if (width != 0) {
    xScale = 1;
    yScale = height / width;
    radius = width;
  } else if (height != 0) {
    xScale = width / height;
    yScale = 1;
    radius = height;
  }

  CGAffineTransform t = CGAffineTransformMakeTranslation(origin.x, origin.y);
  t = CGAffineTransformScale(t, xScale, yScale);

  CGPathAddArc(path, &t, 0, 0, radius, startAngle, endAngle, clockwise);
}

void LynxPathAddRect(CGMutablePathRef path, CGRect bounds, bool reverse) {
  const CGFloat minX = bounds.origin.x;
  const CGFloat minY = bounds.origin.y;
  const CGFloat maxX = minX + bounds.size.width;
  const CGFloat maxY = minY + bounds.size.height;
  CGPathMoveToPoint(path, nil, minX, minY);
  if (reverse) {
    CGPathAddLineToPoint(path, nil, minX, maxY);
    CGPathAddLineToPoint(path, nil, maxX, maxY);
    CGPathAddLineToPoint(path, nil, maxX, minY);
  } else {
    CGPathAddLineToPoint(path, nil, maxX, minY);
    CGPathAddLineToPoint(path, nil, maxX, maxY);
    CGPathAddLineToPoint(path, nil, minX, maxY);
  }
  CGPathCloseSubpath(path);
}

void LynxPathAddRoundedRect(CGMutablePathRef path, CGRect bounds, LynxCornerInsets ci) {
  const CGFloat minX = bounds.origin.x;
  const CGFloat minY = bounds.origin.y;
  const CGFloat maxX = minX + bounds.size.width;
  const CGFloat maxY = minY + bounds.size.height;

  CGPathMoveToPoint(path, nil, minX, minY + ci.topLeft.height);
  LynxPathAddEllipticArc(path, (CGPoint){minX + ci.topLeft.width, minY + ci.topLeft.height},
                         ci.topLeft.width, ci.topLeft.height, M_PI, 3 * M_PI_2, NO);
  LynxPathAddEllipticArc(path, (CGPoint){maxX - ci.topRight.width, minY + ci.topRight.height},
                         ci.topRight.width, ci.topRight.height, 3 * M_PI_2, 0, NO);
  LynxPathAddEllipticArc(path, (CGPoint){maxX - ci.bottomRight.width, maxY - ci.bottomRight.height},
                         ci.bottomRight.width, ci.bottomRight.height, 0, M_PI_2, NO);
  LynxPathAddEllipticArc(path, (CGPoint){minX + ci.bottomLeft.width, maxY - ci.bottomLeft.height},
                         ci.bottomLeft.width, ci.bottomLeft.height, M_PI_2, M_PI, NO);
  CGPathCloseSubpath(path);
}

CGPathRef LynxPathCreateWithRoundedRect(CGRect bounds, LynxCornerInsets ci) {
  CGMutablePathRef path = CGPathCreateMutable();
  LynxPathAddRoundedRect(path, bounds, ci);
  return path;
}

#pragma mark insets judgement and adjustment
BOOL LynxCornerInsetsAreAboveThreshold(const LynxCornerInsets cornerInsets) {
  const CGFloat val = LYNX_BORDER_THREAHOLD;
  return (cornerInsets.topLeft.width > val || cornerInsets.topLeft.height > val ||
          cornerInsets.topRight.width > val || cornerInsets.topRight.height > val ||
          cornerInsets.bottomLeft.width > val || cornerInsets.bottomLeft.height > val ||
          cornerInsets.bottomRight.width > val || cornerInsets.bottomRight.height > val);
}

UIEdgeInsets LynxGetEdgeInsets(CGRect bounds, UIEdgeInsets edgeInsets, CGFloat mul) {
  if (mul <= 1e-6) {
    UIEdgeInsets border = {0, 0, 0, 0};
    return border;
  }

  UIEdgeInsets border = edgeInsets;
  if (border.left + border.right > bounds.size.width) {
    CGFloat tmp = bounds.size.width / (border.left + border.right);
    border.left *= tmp;
    border.right *= tmp;
  }
  if (border.top + border.bottom > bounds.size.height) {
    CGFloat tmp = bounds.size.height / (border.top + border.bottom);
    border.top *= tmp;
    border.bottom *= tmp;
  }

  border.top *= mul;
  border.right *= mul;
  border.bottom *= mul;
  border.left *= mul;
  return border;
}

// follows the order in CSS computed style
#define TOP_LEFT_X 0
#define TOP_LEFT_Y 1
#define TOP_RIGHT_X 2
#define TOP_RIGHT_Y 3
#define BOTTOM_RIGHT_X 4
#define BOTTOM_RIGHT_Y 5
#define BOTTOM_LEFT_X 6
#define BOTTOM_LEFT_Y 7

LynxCornerInsets LynxGetCornerInsetsA(CGRect bounds, LynxBorderRadii cornerRadii,
                                      UIEdgeInsets edgeInsets,
                                      LynxPlatformLength* __strong cornerRadiiCalc[8]) {
  const CGFloat width = MAX(0, bounds.size.width);
  const CGFloat height = MAX(0, bounds.size.height);

  LynxCornerInsets cornerInsets = {
      .topLeft =
          {
              LynxBorderUnitValToFloatA(cornerRadii.topLeftX, width, cornerRadiiCalc[TOP_LEFT_X]),
              LynxBorderUnitValToFloatA(cornerRadii.topLeftY, height, cornerRadiiCalc[TOP_LEFT_Y]),
          },
      .topRight =
          {
              LynxBorderUnitValToFloatA(cornerRadii.topRightX, width, cornerRadiiCalc[TOP_RIGHT_X]),
              LynxBorderUnitValToFloatA(cornerRadii.topRightY, height,
                                        cornerRadiiCalc[TOP_RIGHT_Y]),
          },
      .bottomLeft =
          {
              LynxBorderUnitValToFloatA(cornerRadii.bottomLeftX, width,
                                        cornerRadiiCalc[BOTTOM_LEFT_X]),
              LynxBorderUnitValToFloatA(cornerRadii.bottomLeftY, height,
                                        cornerRadiiCalc[BOTTOM_LEFT_Y]),
          },
      .bottomRight = {
          LynxBorderUnitValToFloatA(cornerRadii.bottomRightX, width,
                                    cornerRadiiCalc[BOTTOM_RIGHT_X]),
          LynxBorderUnitValToFloatA(cornerRadii.bottomRightY, height,
                                    cornerRadiiCalc[BOTTOM_RIGHT_Y]),
      }};

  CGFloat val = 1.0;
  const CGFloat top = cornerInsets.topLeft.width + cornerInsets.topRight.width;
  if (top > width) {
    CGFloat tmp = width / top;
    if (tmp < val) val = tmp;
  }

  const CGFloat bottom = cornerInsets.bottomLeft.width + cornerInsets.bottomRight.width;
  if (bottom > width) {
    CGFloat tmp = width / bottom;
    if (tmp < val) val = tmp;
  }

  const CGFloat left = cornerInsets.topLeft.height + cornerInsets.bottomLeft.height;
  if (left > height) {
    CGFloat tmp = height / left;
    if (tmp < val) val = tmp;
  }

  const CGFloat right = cornerInsets.topRight.height + cornerInsets.bottomRight.height;
  if (right > height) {
    CGFloat tmp = height / right;
    if (tmp < val) val = tmp;
  }

  return (LynxCornerInsets){
      .topLeft =
          {
              MAX(0, cornerInsets.topLeft.width * val - edgeInsets.left),
              MAX(0, cornerInsets.topLeft.height * val - edgeInsets.top),
          },
      .topRight =
          {
              MAX(0, cornerInsets.topRight.width * val - edgeInsets.right),
              MAX(0, cornerInsets.topRight.height * val - edgeInsets.top),
          },
      .bottomLeft =
          {
              MAX(0, cornerInsets.bottomLeft.width * val - edgeInsets.left),
              MAX(0, cornerInsets.bottomLeft.height * val - edgeInsets.bottom),
          },
      .bottomRight = {
          MAX(0, cornerInsets.bottomRight.width * val - edgeInsets.right),
          MAX(0, cornerInsets.bottomRight.height * val - edgeInsets.bottom),
      }};
}

#undef TOP_LEFT_X
#undef TOP_LEFT_Y
#undef TOP_RIGHT_X
#undef TOP_RIGHT_Y
#undef BOTTOM_RIGHT_X
#undef BOTTOM_RIGHT_Y
#undef BOTTOM_LEFT_X
#undef BOTTOM_LEFT_Y

LynxCornerInsets LynxGetCornerInsets(CGRect bounds, LynxBorderRadii cornerRadii,
                                     UIEdgeInsets edgeInsets) {
  LynxPlatformLength* emptyCalc[8] = {NULL};
  return LynxGetCornerInsetsA(bounds, cornerRadii, edgeInsets, emptyCalc);
}

void adjustInsets(UIImage* image, CALayer* layer, UIEdgeInsets insets) {
  if (!UIEdgeInsetsEqualToEdgeInsets(insets, UIEdgeInsetsZero)) {
    image = [image resizableImageWithCapInsets:insets];
  }

  CGRect contentsCenter = CGRectMake(
      image.capInsets.left / image.size.width, image.capInsets.top / image.size.height,
      (image.size.width - image.capInsets.right - image.capInsets.left) / image.size.width,
      (image.size.height - image.capInsets.bottom - image.capInsets.top) / image.size.height);

  layer.contents = (id)image.CGImage;
  layer.contentsScale = image.scale;

  const BOOL isResizable = !UIEdgeInsetsEqualToEdgeInsets(image.capInsets, UIEdgeInsetsZero);
  layer.contentsCenter = isResizable ? contentsCenter : CGRectMake(0.0, 0.0, 1.0, 1.0);
}

CGRect LynxGetRectWithEdgeInsets(CGRect bounds, UIEdgeInsets border) {
  return CGRectMake(bounds.origin.x + border.left, bounds.origin.y + border.top,
                    MAX(0, bounds.size.width - border.left - border.right),
                    MAX(0, bounds.size.height - border.top - border.bottom));
}

BOOL LynxBorderInsetsNotLargeThan(UIEdgeInsets borderInsets, float val) {
  return borderInsets.left <= val && borderInsets.top <= val && borderInsets.bottom <= val &&
         borderInsets.right <= val;
}

#pragma mark update layer
bool LynxUpdateOutlineLayer(CALayer* layer, CGSize viewSize, LynxBorderStyle style, UIColor* color,
                            float width) {
  CGRect rect = {.size = viewSize};
  rect = LynxGetBoundsAutoAdjust(rect);

  const CGRect frameRect = CGRectInset(rect, -width, -width);
  layer.frame = frameRect;
  const CGRect frameBound = {.size = frameRect.size};
  CGContextRef oldCtx = UIGraphicsGetCurrentContext();
  if (oldCtx) {
    UIGraphicsPushContext(oldCtx);
  }
  UIGraphicsBeginImageContextWithOptions(frameRect.size, NO, 0.0);
  CGContextRef ctx = UIGraphicsGetCurrentContext();
  const LynxRenderBorderStyle renderStyle = LynxToRenderBorderStyle(style);
  const UIEdgeInsets borderInsets = {width, width, width, width};
  switch (renderStyle) {
    case LynxRenderBorderStyleDashedOrDotted:
      LynxDrawDashedOrDottedRectWithSameColor(ctx, style, frameRect.size, borderInsets,
                                              color.CGColor);
      break;
    case LynxRenderBorderStyleSolidInsetOrOutset:
      LynxDoDrawOutlineSubRect(ctx, style, color, width, frameBound);
      break;
    case LynxRenderBorderStyleDoubleGrooveOrRidge:
      if (style == LynxBorderStyleDouble) {
        const float subWidth = width / 3.0f;
        LynxDoDrawOutlineSubRect(ctx, style, color, subWidth, frameBound);
        LynxDoDrawOutlineSubRect(ctx, style, color, subWidth,
                                 CGRectInset(frameBound, width / 1.5f, width / 1.5f));
      } else if (style == LynxBorderStyleRidge) {
        const float subWidth = width / 2.0f;
        LynxDoDrawOutlineSubRect(ctx, LynxBorderStyleOutset, color, subWidth, frameBound);
        LynxDoDrawOutlineSubRect(ctx, LynxBorderStyleInset, color, subWidth,
                                 CGRectInset(frameBound, subWidth, subWidth));
      } else if (style == LynxBorderStyleGroove) {
        const float subWidth = width / 2.0f;
        LynxDoDrawOutlineSubRect(ctx, LynxBorderStyleInset, color, subWidth, frameBound);
        LynxDoDrawOutlineSubRect(ctx, LynxBorderStyleOutset, color, subWidth,
                                 CGRectInset(frameBound, subWidth, subWidth));
      }
      break;
    case LynxRenderBorderStyleNone:
    default:
      UIGraphicsEndImageContext();
      if (oldCtx) {
        UIGraphicsPopContext();
      }
      return false;
  }

  UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  if (oldCtx) {
    UIGraphicsPopContext();
  }

  LynxUpdateLayerWithImage(layer, image);
  return true;
}

void LynxUpdateLayerWithImage(CALayer* layer, UIImage* image) {
  layer.contents = (id)image.CGImage;
  layer.contentsScale = image.scale;
  const BOOL isResizable = !UIEdgeInsetsEqualToEdgeInsets(image.capInsets, UIEdgeInsetsZero);
  if (isResizable) {
    layer.contentsCenter = ({
      CGSize size = image.size;
      UIEdgeInsets insets = image.capInsets;
      CGRectMake(insets.left / size.width, insets.top / size.height, 1.0 / size.width,
                 1.0 / size.height);
    });
  } else {
    layer.contentsCenter = CGRectMake(0.0, 0.0, 1.0, 1.0);
  }
}

#pragma mark get images
UIImage* LynxGetBackgroundImage(CGSize viewSize, LynxBorderRadii cornerRadii,
                                UIEdgeInsets borderInsets, CGColorRef backgroundColor,
                                BOOL drawToEdge, NSArray* bgImageInfoArr, BOOL pixelated) {
  return LynxGetBackgroundImageWithClip(viewSize, cornerRadii, borderInsets, backgroundColor,
                                        drawToEdge, bgImageInfoArr, LynxBackgroundClipBorderBox,
                                        UIEdgeInsetsZero, pixelated);
}

UIImage* LynxGetBackgroundImageWithClip(CGSize viewSize, LynxBorderRadii cornerRadii,
                                        UIEdgeInsets borderInsets, CGColorRef backgroundColor,
                                        BOOL drawToEdge, NSArray* bgImageInfoArr,
                                        LynxBackgroundClipType clipType, UIEdgeInsets paddingInsets,
                                        BOOL pixelated) {
  viewSize = LynxRoundViewSize(viewSize);
  CGRect rect = {.size = viewSize};
  rect = LynxGetBoundsAutoAdjust(rect);

  switch (clipType) {
    case LynxBackgroundClipPaddingBox:
      borderInsets = LynxGetEdgeInsets(rect, borderInsets, 1.0);
      break;
    case LynxBackgroundClipBorderBox:
      borderInsets = UIEdgeInsetsZero;
      break;
    case LynxBackgroundClipContentBox:
      borderInsets = UIEdgeInsetsMake(
          borderInsets.top + paddingInsets.top, borderInsets.left + paddingInsets.left,
          borderInsets.bottom + paddingInsets.bottom, borderInsets.right + paddingInsets.bottom);
      break;
  }

  const LynxCornerInsets cornerInsets = LynxGetCornerInsets(rect, cornerRadii, UIEdgeInsetsZero);
  const BOOL hasCornerRadii = LynxCornerInsetsAreAboveThreshold(cornerInsets);
  const CGFloat alpha = CGColorGetAlpha(backgroundColor);
  BOOL opaque =
      (drawToEdge || !hasCornerRadii) && alpha == 1.0 && clipType == LynxBackgroundClipBorderBox;

  UIImage* image = [LynxUI
      imageWithActionBlock:^(CGContextRef ctx) {
        if (pixelated) {
          CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
        }
        LynxDrawBackgroundToImageContext(cornerRadii, borderInsets, backgroundColor, drawToEdge,
                                         bgImageInfoArr, rect, ctx);
      }
                    opaque:opaque
                     scale:[LynxUIUnitUtils screenScale]
                      size:rect.size];
  return image;
}

UIImage* LynxGetBorderLayerImage(LynxBorderStyles borderStyles, CGSize viewSize,
                                 LynxBorderRadii cornerRadii, UIEdgeInsets borderInsets,
                                 LynxBorderColors borderColors, BOOL drawToEdge) {
  CGRect rect = {.size = viewSize};
  rect = LynxGetBoundsAutoAdjust(rect);
  borderInsets = LynxGetEdgeInsets(rect, borderInsets, 1.0);

  const LynxCornerInsets cornerInsets = LynxGetCornerInsets(rect, cornerRadii, UIEdgeInsetsZero);
  const BOOL hasCornerRadii = LynxCornerInsetsAreAboveThreshold(cornerInsets);

  UIImage* image = [LynxUI
      imageWithActionBlock:^(CGContextRef _Nonnull context) {
        LynxDrawBorderToImageContext(borderStyles, viewSize, cornerRadii, borderInsets,
                                     borderColors, drawToEdge, rect, cornerInsets, hasCornerRadii,
                                     context);
      }
                    opaque:NO
                     scale:[LynxUIUnitUtils screenScale]
                      size:rect.size];
  return image;
}

BOOL internalHasSameBorderRadius(LynxBorderRadii radii) {
  return radii.topLeftX.unit == LynxBorderValueUnitDefault &&
         isBorderUnitEqual(radii.topLeftX, radii.topLeftY) &&
         isBorderUnitEqual(radii.topLeftX, radii.topRightX) &&
         isBorderUnitEqual(radii.topLeftX, radii.topRightY) &&
         isBorderUnitEqual(radii.topLeftX, radii.bottomLeftX) &&
         isBorderUnitEqual(radii.topLeftX, radii.bottomLeftY) &&
         isBorderUnitEqual(radii.topLeftX, radii.bottomRightX) &&
         isBorderUnitEqual(radii.topLeftX, radii.bottomRightY);
}

CGPathRef LynxCreateBorderCenterPath(CGSize viewSize, LynxBorderRadii cornerRadius,
                                     UIEdgeInsets borderWidth) {
  CGRect rect = {.size = viewSize};
  rect = LynxGetBoundsAutoAdjust(rect);
  const UIEdgeInsets centerInsets = LynxGetEdgeInsets(rect, borderWidth, 0.5);

  CGPathRef path =
      LynxPathCreateWithRoundedRect(LynxGetRectWithEdgeInsets(rect, centerInsets),
                                    LynxGetCornerInsets(rect, cornerRadius, centerInsets));
  return path;
}

void LynxUpdateBorderLayerWithPath(CAShapeLayer* borderLayer, CGPathRef path,
                                   LynxBackgroundInfo* info) {
  // TODO(renzhongyue): Apply styles according styles in info.
  UIColor* strokeColor = [UIColor clearColor];
  CGFloat lineWidth = 0;
  if (info) {
    strokeColor = [info borderLeftColor] ? [info borderLeftColor] : [UIColor blackColor];
    lineWidth = [info borderWidth].left;
  }
  borderLayer.contents = nil;
  borderLayer.path = path;
  borderLayer.lineWidth = lineWidth;
  borderLayer.fillColor = [[UIColor clearColor] CGColor];
  borderLayer.strokeStart = 0;
  borderLayer.strokeEnd = 1;
  borderLayer.strokeColor = [strokeColor CGColor];
}

void LBRGetBorderValueOrLength(NSArray* valArray, int index, LynxBorderUnitValue* value,
                               LynxPlatformLength** length) {
  LynxPlatformLengthUnit type =
      (LynxPlatformLengthUnit)[LynxConverter toNSUInteger:[valArray objectAtIndex:index + 1]];
  value->unit = (int)type;
  if (type == LynxPlatformLengthUnitCalc) {
    *length = [[LynxPlatformLength alloc] initWithValue:[valArray objectAtIndex:index] type:type];
  } else {
    value->val = [LynxConverter toCGFloat:[valArray objectAtIndex:index]];
  }
}

@end
