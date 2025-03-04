// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundInfo.h"
#import "LynxCSSType.h"

@class LynxBackgroundImageLayerInfo;

typedef struct {
  CGColorRef top, left, bottom, right;
} LynxBorderColors;

typedef struct {
  CGSize topLeft, topRight, bottomLeft, bottomRight;
} LynxCornerInsets;

typedef struct {
  CGPoint topLeft, topRight, bottomLeft, bottomRight;
} LynxCornerInsetPoints;

typedef struct {
  LynxBorderStyle top, left, bottom, right;
} LynxBorderStyles;

typedef NS_ENUM(NSInteger, LynxRenderBorderStyle) {
  LynxRenderBorderStyleSolidInsetOrOutset = 0,
  LynxRenderBorderStyleDashedOrDotted,
  LynxRenderBorderStyleDoubleGrooveOrRidge,
  LynxRenderBorderStyleNone,
};

typedef struct _LynxRenderBorderSideData {
  LynxBorderStyle style;
  CGColorRef color;
  CGFloat length, width, maxWidth;
  CGPoint clipPoints[4];
  CGPoint linePoints[2];
  BOOL isLeftOrTop;
} LynxRenderBorderSideInfo;

typedef NS_ENUM(NSInteger, LynxBorderPathId) {
  LynxBorderPathId16 = 0,
  LynxBorderPathId14,
  LynxBorderPathId12,
  LynxBorderPathId34,
  LynxBorderPathId56,
  LynxBorderPathIdCount,
};

@interface LynxBackgroundUtils : NSObject

+ (CGPathRef)createBezierPathWithRoundedRect:(CGRect)bounds
                                 borderRadii:(LynxBorderRadii)borderRadii;
+ (CGPathRef)createBezierPathWithRoundedRect:(CGRect)bounds
                                 borderRadii:(LynxBorderRadii)borderRadii
                                  edgeInsets:(UIEdgeInsets)edgeInsets;

void LynxPathAddEllipticArc(CGMutablePathRef path, CGPoint origin, CGFloat width, CGFloat height,
                            CGFloat startAngle, CGFloat endAngle, BOOL clockwise);
void LynxPathAddRect(CGMutablePathRef path, CGRect bounds, bool reverse);
void LynxPathAddRoundedRect(CGMutablePathRef path, CGRect bounds, LynxCornerInsets ci);
CGPathRef LynxPathCreateWithRoundedRect(CGRect bounds, LynxCornerInsets ci);

BOOL LynxCornerInsetsAreAboveThreshold(const LynxCornerInsets cornerInsets);
UIEdgeInsets LynxGetEdgeInsets(CGRect bounds, UIEdgeInsets edgeInsets, CGFloat mul);
LynxCornerInsets LynxGetCornerInsets(CGRect bounds, LynxBorderRadii cornerRadii,
                                     UIEdgeInsets edgeInsets);

LynxCornerInsets LynxGetCornerInsetsA(CGRect bounds, LynxBorderRadii cornerRadii,
                                      UIEdgeInsets edgeInsets,
                                      LynxPlatformLength* __strong cornerRadiiCalc[8]);
void adjustInsets(UIImage* image, CALayer* layer, UIEdgeInsets insets);
CGRect LynxGetRectWithEdgeInsets(CGRect bounds, UIEdgeInsets border);
BOOL LynxBorderInsetsNotLargeThan(UIEdgeInsets borderInsets, float val);

bool LynxUpdateOutlineLayer(CALayer* layer, CGSize viewSize, LynxBorderStyle style, UIColor* color,
                            float width);
void LynxUpdateLayerWithImage(CALayer* layer, UIImage* image);
UIImage* LynxGetBorderLayerImage(LynxBorderStyles borderStyles, CGSize viewSize,
                                 LynxBorderRadii cornerRadii, UIEdgeInsets borderInsets,
                                 LynxBorderColors borderColors, BOOL drawToEdge);
UIImage* LynxGetBackgroundImage(CGSize viewSize, LynxBorderRadii cornerRadii,
                                UIEdgeInsets borderInsets, CGColorRef backgroundColor,
                                BOOL drawToEdge, NSArray* bgImageInfoArr, BOOL pixelated);
UIImage* LynxGetBackgroundImageWithClip(CGSize viewSize, LynxBorderRadii cornerRadii,
                                        UIEdgeInsets borderInsets, CGColorRef backgroundColor,
                                        BOOL drawToEdge, NSArray* bgImageInfoArr,
                                        LynxBackgroundClipType clip, UIEdgeInsets paddingInsets,
                                        BOOL pixelated);

CGPathRef LynxCreateBorderCenterPath(CGSize viewSize, LynxBorderRadii cornerRadius,
                                     UIEdgeInsets borderWidth);
void LynxUpdateBorderLayerWithPath(CAShapeLayer* borderLayer, CGPathRef path,
                                   LynxBackgroundInfo* info);

BOOL internalHasSameBorderRadius(LynxBorderRadii radii);
void LBRGetBorderValueOrLength(NSArray* valArray, int index, LynxBorderUnitValue* value,
                               LynxPlatformLength** length);
@end  // LynxBackgroundUtils
