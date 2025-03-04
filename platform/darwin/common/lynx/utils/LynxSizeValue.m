// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxSizeValue.h"
#import "LynxUnitUtils.h"

@implementation LynxSizeValue

- (instancetype)init {
  return [self initWithType:LynxSizeValueTypeUnknown value:0.f];
}

- (instancetype)initWithType:(LynxSizeValueType)type value:(CGFloat)value {
  self = [super init];
  if (self) {
    _type = type;
    _value = value;
  }
  return self;
}

+ (instancetype)sizeValueFromCSSString:(NSString *)valueStr {
  // The data from the frontend might not be of NSString type
  if (![valueStr isKindOfClass:[NSString class]]) {
    return nil;
  }
  if ([valueStr hasSuffix:@"%"]) {
    CGFloat value = [[valueStr substringToIndex:valueStr.length - 1] floatValue];
    value /= 100.f;
    return [[LynxSizeValue alloc] initWithType:LynxSizeValueTypePercent value:value];
  } else if ([valueStr hasSuffix:@"px"]) {
    CGFloat value = [LynxUnitUtils toPtFromUnitValue:valueStr];
    return [[LynxSizeValue alloc] initWithType:LynxSizeValueTypeDevicePt value:value];
  }
  return nil;
}

- (CGFloat)convertToDevicePtWithFullSize:(CGFloat)fullSize {
  if (self.type == LynxSizeValueTypePercent) {
    return self.value * fullSize;
  } else if (self.type == LynxSizeValueTypeDevicePt) {
    return self.value;
  }
  return 0.f;
}

@end
