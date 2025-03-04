// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <QuartzCore/QuartzCore.h>
#import "LynxConverter+LynxCSSType.h"
#import "LynxLog.h"

DEFINE_ENUM_CONVERTER(LynxOverflowType, LynxOverflowHidden)
DEFINE_ENUM_CONVERTER(LynxAnimationDirectionType, LynxAnimationDirectionNormal)
DEFINE_ENUM_CONVERTER(LynxAnimationPlayStateType, LynxAnimationPlayStateRunning)
DEFINE_ENUM_CONVERTER(LynxVisibilityType, LynxVisibilityVisible)
DEFINE_ENUM_CONVERTER(LynxDirectionType, LynxDirectionLtr)
DEFINE_ENUM_CONVERTER(LynxFontWeightType, LynxFontWeightNormal)
DEFINE_ENUM_CONVERTER(LynxWordBreakType, LynxWordBreakNormal)
DEFINE_ENUM_CONVERTER(LynxBackgroundClipType, LynxBackgroundClipBorderBox)
DEFINE_ENUM_CONVERTER(LynxFontStyleType, LynxFontStyleNormal)
DEFINE_ENUM_CONVERTER(LynxTextAlignType, LynxTextAlignLeft)
DEFINE_ENUM_CONVERTER(LynxWhiteSpaceType, LynxWhiteSpaceNormal)
DEFINE_ENUM_CONVERTER(LynxTextOverflowType, LynxTextOverflowClip)
DEFINE_ENUM_CONVERTER(LynxTextDecorationType, LynxTextDecorationNone)
DEFINE_ENUM_CONVERTER(LynxVerticalAlign, LynxVerticalAlignDefault)

#undef DEFINE_ENUM_CONVERTER

@implementation LynxConverter (CAMediaTimingFunction)
+ (CAMediaTimingFunction*)toCAMediaTimingFunction:(id)value {
  CAMediaTimingFunctionName name = kCAMediaTimingFunctionLinear;
  if (!value || [value isEqual:[NSNull null]]) {
    return [CAMediaTimingFunction functionWithName:name];
  }
  if ([value isKindOfClass:[NSArray class]]) {
    NSArray* arr = (NSArray*)value;
    LynxTimingFunctionType tfType = (LynxTimingFunctionType)[arr[0] intValue];
    // TODO(liyanbo): support steps animation.
    //    starlight::StepsType stType = (starlight::StepsType)[arr[1] intValue];
    switch (tfType) {
      case LynxTimingFunctionLinear:
        name = kCAMediaTimingFunctionLinear;
        break;
      case LynxTimingFunctionEaseInEaseOut:
        name = kCAMediaTimingFunctionEaseInEaseOut;
        break;
      case LynxTimingFunctionEaseIn:
        name = kCAMediaTimingFunctionEaseIn;
        break;
      case LynxTimingFunctionEaseOut:
        name = kCAMediaTimingFunctionEaseOut;
        break;
      case LynxTimingFunctionSquareBezier:
        return [CAMediaTimingFunction functionWithControlPoints:[arr[2] floatValue
        ]:[arr[3] floatValue
        ]:1.0:1.0];
      case LynxTimingFunctionCubicBezier:
        return [CAMediaTimingFunction functionWithControlPoints:[arr[2] floatValue
        ]:[arr[3] floatValue
        ]:[arr[4] floatValue
        ]:[arr[5] floatValue]];
      case LynxTimingFunctionSteps:
        LLogWarn(@"steps timing function don't support.");
        break;
    }
  }
  return [CAMediaTimingFunction functionWithName:name];
}
@end

@implementation LynxConverter (CAMediaTimingFillMode)
+ (CAMediaTimingFillMode)toCAMediaTimingFillMode:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return kCAFillModeRemoved;
  }
  LynxAnimationFillModeType type = (LynxAnimationFillModeType)[value intValue];
  if (type == LynxAnimationFillModeNone) {
    return kCAFillModeRemoved;
  } else if (type == LynxAnimationFillModeForwards) {
    return kCAFillModeForwards;
  } else if (type == LynxAnimationFillModeBackwards) {
    return kCAFillModeBackwards;
  } else if (type == LynxAnimationFillModeBoth) {
    return kCAFillModeBoth;
  } else {
    return kCAFillModeRemoved;
  }
}
@end
