// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUnitUtils.h"
#import "LynxLog.h"

@implementation LynxUnitUtils

+ (CGFloat)toPtFromIDUnitValue:(id)unitValue withDefaultPt:(CGFloat)defaultPt {
  if ([unitValue isKindOfClass:NSString.class]) {
    return [LynxUnitUtils toPtFromUnitValue:(NSString *)unitValue];
  } else if ([unitValue isKindOfClass:NSNumber.class]) {
    return ((NSNumber *)unitValue).floatValue;
  }
  return defaultPt;
}

+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue {
  return [self toPtFromUnitValue:unitValue withDefaultPt:0];
}

+ (CGFloat)toPhysicalPixelFromPt:(CGFloat)valuePt {
  LynxScreenMetrics *screenMetrics = [LynxScreenMetrics getDefaultLynxScreenMetrics];
  return screenMetrics.scale * valuePt;
}

+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue withDefaultPt:(CGFloat)defaultPt {
  return [self toPtFromUnitValue:unitValue
                    rootFontSize:0
                     curFontSize:0
                       rootWidth:0
                      rootHeight:0
                   withDefaultPt:defaultPt];
}

+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue
                rootFontSize:(CGFloat)rootFontSize
                 curFontSize:(CGFloat)curFontSize
                   rootWidth:(int)rootWidth
                  rootHeight:(int)rootHeight {
  return [self toPtFromUnitValue:unitValue
                    rootFontSize:rootFontSize
                     curFontSize:curFontSize
                       rootWidth:rootWidth
                      rootHeight:rootHeight
                   withDefaultPt:0];
}

+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue
                rootFontSize:(CGFloat)rootFontSize
                 curFontSize:(CGFloat)curFontSize
                   rootWidth:(int)rootWidth
                  rootHeight:(int)rootHeight
                    viewSize:(CGFloat)viewSize
               withDefaultPt:(CGFloat)defaultPt {
  LynxScreenMetrics *screenMetrics = [LynxScreenMetrics getDefaultLynxScreenMetrics];
  return [self toPtWithScreenMetrics:screenMetrics
                           unitValue:unitValue
                        rootFontSize:rootFontSize
                         curFontSize:curFontSize
                           rootWidth:rootWidth
                          rootHeight:rootHeight
                       withDefaultPt:0];
}

+ (CGFloat)toPtWithScreenMetrics:(LynxScreenMetrics *)screenMetrics
                       unitValue:(NSString *)unitValue
                    rootFontSize:(CGFloat)rootFontSize
                     curFontSize:(CGFloat)curFontSize
                       rootWidth:(int)rootWidth
                      rootHeight:(int)rootHeight
                        viewSize:(CGFloat)viewSize
                   withDefaultPt:(CGFloat)defaultPt {
  if (!unitValue || unitValue.length == 0) {
    return defaultPt;
  }
  @try {
    if ([self isPercentage:unitValue]) {
      return [[unitValue substringToIndex:unitValue.length - 1] floatValue] * viewSize / 100;
    }
  } @catch (NSException *exception) {
    LLogError(@"LynxUnitUtils has error while make unit value to pt!");
  }
  return [self toPtWithScreenMetrics:screenMetrics
                           unitValue:unitValue
                        rootFontSize:rootFontSize
                         curFontSize:curFontSize
                           rootWidth:rootWidth
                          rootHeight:rootHeight
                       withDefaultPt:0];
}

+ (CGFloat)toPtFromUnitValue:(NSString *)unitValue
                rootFontSize:(CGFloat)rootFontSize
                 curFontSize:(CGFloat)curFontSize
                   rootWidth:(int)rootWidth
                  rootHeight:(int)rootHeight
               withDefaultPt:(CGFloat)defaultPt {
  LynxScreenMetrics *screenMetrics = [LynxScreenMetrics getDefaultLynxScreenMetrics];
  return [self toPtWithScreenMetrics:screenMetrics
                           unitValue:unitValue
                        rootFontSize:rootFontSize
                         curFontSize:curFontSize
                           rootWidth:rootWidth
                          rootHeight:rootHeight
                       withDefaultPt:defaultPt];
}

+ (CGFloat)toPtWithScreenMetrics:(LynxScreenMetrics *)screenMetrics
                       unitValue:(NSString *)unitValue
                    rootFontSize:(CGFloat)rootFontSize
                     curFontSize:(CGFloat)curFontSize
                       rootWidth:(int)rootWidth
                      rootHeight:(int)rootHeight
                   withDefaultPt:(CGFloat)defaultPt {
  if (!unitValue || unitValue.length == 0) {
    return defaultPt;
  }
  @try {
    if (unitValue.length > 3 && [unitValue hasSuffix:@"rpx"]) {
      CGFloat f = [[unitValue substringToIndex:unitValue.length - 3] floatValue];
      return f * screenMetrics.screenSize.width / 750;
    } else if (unitValue.length > 3 && [unitValue hasSuffix:@"ppx"]) {
      return [[unitValue substringToIndex:unitValue.length - 3] floatValue] / screenMetrics.scale;
    } else if (unitValue.length > 2 && [unitValue hasSuffix:@"px"]) {
      return [[unitValue substringToIndex:unitValue.length - 2] floatValue];
    } else if (unitValue.length > 3 && [unitValue hasSuffix:@"rem"]) {
      return [[unitValue substringToIndex:unitValue.length - 3] floatValue] * rootFontSize;
    } else if (unitValue.length > 2 && [unitValue hasSuffix:@"em"]) {
      return [[unitValue substringToIndex:unitValue.length - 2] floatValue] *
             (isnan(curFontSize) ? rootFontSize : curFontSize);
    } else if (unitValue.length > 2 && [unitValue hasSuffix:@"vw"]) {
      return [[unitValue substringToIndex:unitValue.length - 2] floatValue] * rootWidth / 100;
    } else if (unitValue.length > 2 && [unitValue hasSuffix:@"vh"]) {
      return [[unitValue substringToIndex:unitValue.length - 2] floatValue] * rootHeight / 100;
    } else {
      return [unitValue floatValue];
    }
  } @catch (NSException *exception) {
    LLogError(@"LynxUnitUtils has error while make unit value to pt!");
  }
  return defaultPt;
}

+ (BOOL)isPercentage:(NSString *)unitValue {
  return [unitValue hasSuffix:@"%"];
}

+ (CGFloat)clamp:(CGFloat)value min:(CGFloat)minValue max:(CGFloat)maxValue {
  NSAssert(minValue <= maxValue, @"minValue should less or equal than maxValue");
  return MIN(MAX(value, minValue), maxValue);
}

@end
