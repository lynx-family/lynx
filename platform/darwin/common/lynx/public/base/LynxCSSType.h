// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXCSSTYPE_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXCSSTYPE_H_

#import <Foundation/Foundation.h>

#import "LynxAutoGenCSSType.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * This macro is used for creating converters for enum types.
 */
#define LYNX_ENUM_CONVERTER(type, values, default, getter)                                   \
  +(type)type : (id)value LYNX_DYNAMIC {                                                     \
    static NSDictionary *mapping;                                                            \
    static dispatch_once_t onceToken;                                                        \
    dispatch_once(&onceToken, ^{                                                             \
      mapping = values;                                                                      \
    });                                                                                      \
    return _RCT_CAST(type, [RCTConvertEnumValue(#type, mapping, @(default), value) getter]); \
  }

#define DECLARE_ENUM_CONVERTER(type) \
  @interface LynxConverter (type)    \
  +(type)to##type : (id)value;       \
  @end

#define DEFINE_ENUM_CONVERTER(type, defaultType)   \
  @implementation LynxConverter (type)             \
  +(type)to##type : (id)value {                    \
    if (!value || [value isEqual:[NSNull null]]) { \
      return defaultType;                          \
    }                                              \
    return (type)[value intValue];                 \
  }                                                \
  @end

// NOTE:the enum's value should equals to css_type.h's defines.

typedef NS_ENUM(NSUInteger, LynxOverflowType) {
  LynxOverflowVisible,
  LynxOverflowHidden,
  LynxOverflowScroll,
};

typedef NS_ENUM(NSUInteger, LynxTimingFunctionType) {
  LynxTimingFunctionLinear,
  LynxTimingFunctionEaseIn,
  LynxTimingFunctionEaseOut,
  LynxTimingFunctionEaseInEaseOut,
  LynxTimingFunctionSquareBezier,
  LynxTimingFunctionCubicBezier,
  LynxTimingFunctionSteps,
};

typedef NS_ENUM(NSUInteger, LynxAnimationDirectionType) {
  LynxAnimationDirectionNormal = 0,            // version:1.0
  LynxAnimationDirectionReverse = 1,           // version:1.0
  LynxAnimationDirectionAlternate = 2,         // version:1.0
  LynxAnimationDirectionAlternateReverse = 3,  // version:1.0
};
typedef NS_ENUM(NSUInteger, LynxAnimationFillModeType) {
  LynxAnimationFillModeNone = 0,       // version:1.0
  LynxAnimationFillModeForwards = 1,   // version:1.0
  LynxAnimationFillModeBackwards = 2,  // version:1.0
  LynxAnimationFillModeBoth = 3,       // version:1.0
};
typedef NS_ENUM(NSUInteger, LynxAnimationPlayStateType) {
  LynxAnimationPlayStatePaused = 0,   // version:1.0
  LynxAnimationPlayStateRunning = 1,  // version:1.0
};

typedef NS_ENUM(NSUInteger, LynxTransformType) {
  LynxTransformTypeNone = 0,
  LynxTransformTypeTranslate = 1,
  LynxTransformTypeTranslateX = 1 << 1,
  LynxTransformTypeTranslateY = 1 << 2,
  LynxTransformTypeTranslateZ = 1 << 3,
  LynxTransformTypeTranslate3d = 1 << 4,
  LynxTransformTypeRotate = 1 << 5,
  LynxTransformTypeRotateX = 1 << 6,
  LynxTransformTypeRotateY = 1 << 7,
  LynxTransformTypeRotateZ = 1 << 8,
  LynxTransformTypeScale = 1 << 9,
  LynxTransformTypeScaleX = 1 << 10,
  LynxTransformTypeScaleY = 1 << 11,
  LynxTransformTypeSkew = 1 << 12,
  LynxTransformTypeSkewX = 1 << 13,
  LynxTransformTypeSkewY = 1 << 14,
  LynxTransformTypeMatrix = 1 << 15,
  LynxTransformTypeMatrix3d = 1 << 16
};

typedef NS_ENUM(NSUInteger, LynxBoxShadowOption) {
  LynxBoxShadowOptionNone = 0,
  LynxBoxShadowOptionInset = 1,
  LynxBoxShadowOptionInitial = 2,
  LynxBoxShadowOptionInherit = 3
};

typedef NS_ENUM(NSUInteger, LynxPlatformLengthUnit) {
  LynxPlatformLengthUnitNumber = 0,
  LynxPlatformLengthUnitPercentage = 1,
  LynxPlatformLengthUnitCalc = 2
};

typedef NS_ENUM(NSUInteger, LynxPerspectiveLengthUnit) {
  LynxPerspectiveLengthUnitNumber = 0,
  LynxPerspectiveLengthUnitVw = 1,
  LynxPerspectiveLengthUnitVh = 2,
  LynxPerspectiveLengthUnitDefault = 3,
  LynxPerspectiveLengthUnitPX = 4
};

typedef NS_ENUM(NSUInteger, LynxDirectionType) {
  LynxDirectionNormal = 0,
  LynxDirectionRtl = 2,
  LynxDirectionLtr = 3,
  LynxDirectionLynxRtl __attribute__((deprecated)) = LynxDirectionRtl,
};

typedef NS_ENUM(NSInteger, LynxBackgroundClipType) {
  LynxBackgroundClipPaddingBox = 0,  // 0
  LynxBackgroundClipBorderBox,       // 1
  LynxBackgroundClipContentBox,      // 2
};

typedef NS_ENUM(NSInteger, LynxBackgroundSizeType) {
  LynxBackgroundSizeAuto = -(1 << 5),
  LynxBackgroundSizeCover = -(1 << 5) - 1,
  LynxBackgroundSizeContain = -(1 << 5) - 2,
};

typedef NS_ENUM(NSInteger, LynxBackgroundPositionType) {
  LynxBackgroundPositionTop = -(1 << 5),
  LynxBackgroundPositionRight = -(1 << 5) - 1,
  LynxBackgroundPositionBottom = -(1 << 5) - 2,
  LynxBackgroundPositionLeft = -(1 << 5) - 3,
  LynxBackgroundPositionCenter = -(1 << 5) - 4,
};

typedef NS_ENUM(NSUInteger, LynxBackgroundRepeatType) {
  LynxBackgroundRepeatRepeat,    // 0
  LynxBackgroundRepeatNoRepeat,  // 1
  LynxBackgroundRepeatRepeatX,   // 2
  LynxBackgroundRepeatRepeatY,   // 3
  LynxBackgroundRepeatRound,     // 4
  LynxBackgroundRepeatSpace,     // 5
};

typedef NS_ENUM(NSUInteger, LynxBackgroundOriginType) {
  LynxBackgroundOriginPaddingBox = 0,
  LynxBackgroundOriginBorderBox = 1,
  LynxBackgroundOriginContentBox = 2,
};

typedef NS_ENUM(NSUInteger, LynxTextAlignType) {
  LynxTextAlignLeft = 0,
  LynxTextAlignCenter = 1,
  LynxTextAlignRight = 2,
  LynxTextAlignStart = 3,
  LynxTextAlignEnd = 4,
  LynxTextAlignJustify = 5,
  LynxTextAlignAuto __attribute__((deprecated)) = LynxTextAlignStart,
};

typedef NS_ENUM(NSUInteger, LynxTextDecorationType) {
  LynxTextDecorationNone = 0,
  LynxTextDecorationUnderLine = 1 << 0,
  LynxTextDecorationLineThrough = 1 << 1,
  LynxTextDecorationSolid = 1 << 2,
  LynxTextDecorationDouble = 1 << 3,
  LynxTextDecorationDotted = 1 << 4,
  LynxTextDecorationDashed = 1 << 5,
  LynxTextDecorationWavy = 1 << 6,
  LynxTextDecorationColor = 1 << 7,
};

typedef NS_ENUM(NSUInteger, LynxBackgroundImageType) {
  LynxBackgroundImageNone = 0,
  // LynxBackgroundImageURL is used in ElementManager::ResolveStaticAssetPath by number. Make sure
  // to keep consistent if you change the value of LynxBackgroundImageURL.
  LynxBackgroundImageURL = 1,
  LynxBackgroundImageLinearGradient = 2,
  LynxBackgroundImageRadialGradient = 3,
};

typedef NS_ENUM(NSUInteger, LynxRadialGradientShapeType) {
  LynxRadialGradientShapeEllipse = 0,
  LynxRadialGradientShapeCircle = 1,
};

typedef NS_ENUM(NSUInteger, LynxRadialGradientSizeType) {
  LynxRadialGradientSizeFarthestCorner = 0,
  LynxRadialGradientSizeFarthestSide = 1,
  LynxRadialGradientSizeClosestCorner = 2,
  LynxRadialGradientSizeClosestSide = 3,
  LynxRadialGradientSizeLength = 4,
};

typedef NS_ENUM(NSInteger, LynxVerticalAlign) {
  LynxVerticalAlignDefault = 0,
  LynxVerticalAlignBaseline,
  LynxVerticalAlignSub,
  LynxVerticalAlignSuper,
  LynxVerticalAlignTop,
  LynxVerticalAlignTextTop,
  LynxVerticalAlignMiddle,
  LynxVerticalAlignBottom,
  LynxVerticalAlignTextBottom,
  LynxVerticalAlignLength,
  LynxVerticalAlignPercent,
  LynxVerticalAlignCenter
};

typedef NS_ENUM(NSInteger, LynxBorderStyle) {
  LynxBorderStyleSolid = 0,
  LynxBorderStyleDashed,
  LynxBorderStyleDotted,
  LynxBorderStyleDouble,
  LynxBorderStyleGroove,
  LynxBorderStyleRidge,
  LynxBorderStyleInset,
  LynxBorderStyleOutset,
  LynxBorderStyleHidden,
  LynxBorderStyleNone,
};

typedef NS_ENUM(NSInteger, LynxFlexDirection) {
  LynxFlexDirectionColumn = 0,
  LynxFlexDirectionColumnReverse,
  LynxFlexDirectionRow,
  LynxFlexDirectionRowReverse
};

typedef NS_ENUM(NSInteger, LynxBorderWidth) {
  LynxBorderWidthThin = 1,
  LynxBorderWidthMedium = 3,
  LynxBorderWidthThick = 5
};

typedef NS_ENUM(NSInteger, LynxFilterType) {
  LynxFilterTypeNone = 0,
  LynxFilterTypeGrayScale = 1,
  LynxFilterTypeBlur = 2,
};

typedef NS_ENUM(NSInteger, LynxBasicShapeType) {
  LynxBasicShapeTypeUnknown = 0,
  LynxBasicShapeTypeCircle = 1,
  LynxBasicShapeTypeEllipse = 2,
  LynxBasicShapeTypePath = 3,
  LynxBasicShapeTypeSuperEllipse = 4,
  LynxBasicShapeTypeInset = 5,
};

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXCSSTYPE_H_
