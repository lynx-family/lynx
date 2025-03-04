// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIUnitUtils.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxCSSType.h"

@implementation LynxUIUnitUtils

+ (CGFloat)screenScale {
  static CGFloat __scale = 0.0;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    CGContextRef oldCtx = UIGraphicsGetCurrentContext();
    if (oldCtx) {
      UIGraphicsPushContext(oldCtx);
    }
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(1, 1), YES, 0);
    __scale = CGContextGetCTM(UIGraphicsGetCurrentContext()).a;
    UIGraphicsEndImageContext();
    if (oldCtx) {
      UIGraphicsPopContext();
    }
  });
  return __scale;
}

// The function is needed here to avoid the loss of precision which may be caused by float
// calculation.
+ (void)roundToPhysicalPixel:(CGFloat*)number {
  CGFloat scale = [self screenScale];
  *number = round(*number * scale) / scale;
}

+ (void)roundRectToPhysicalPixelGrid:(CGRect*)rect {
  [LynxUIUnitUtils roundToPhysicalPixel:&(rect->origin.x)];
  [LynxUIUnitUtils roundToPhysicalPixel:&(rect->origin.y)];
  [LynxUIUnitUtils roundToPhysicalPixel:&(rect->size.width)];
  [LynxUIUnitUtils roundToPhysicalPixel:&(rect->size.height)];
}

+ (void)roundInsetsToPhysicalPixelGrid:(UIEdgeInsets*)insets {
  [LynxUIUnitUtils roundToPhysicalPixel:&(insets->top)];
  [LynxUIUnitUtils roundToPhysicalPixel:&(insets->left)];
  [LynxUIUnitUtils roundToPhysicalPixel:&(insets->bottom)];
  [LynxUIUnitUtils roundToPhysicalPixel:&(insets->right)];
}

+ (CGFloat)roundPtToPhysicalPixel:(CGFloat)number {
  CGFloat scale = [self screenScale];
  return round(number * scale) / scale;
}

@end

#pragma mark LynxPlatformLength

@implementation LynxPlatformLength

- (instancetype)init {
  return [self initWithValue:@0 type:LynxPlatformLengthUnitNumber];
}

- (id)copyWithZone:(NSZone*)zone {
  LynxPlatformLength* copy = [[self.class allocWithZone:zone] init];

  if (copy) {
    copy.value = self.value;
    copy.calcArray = [self.calcArray copy];
    copy.type = self.type;
  }

  return copy;
}

- (instancetype)initWithValue:(id)value type:(LynxPlatformLengthUnit)type {
  self = [super init];

  if (self) {
    self.type = type;
    if (self.type == LynxPlatformLengthUnitCalc) {
      self.calcArray = value;
    } else if (type == LynxPlatformLengthUnitNumber || type == LynxPlatformLengthUnitPercentage) {
      self.value = [value floatValue];
    }
  }
  return self;
}

- (CGFloat)valueWithParentValue:(CGFloat)parentValue {
  return getValueInternal(self.calcArray, self.value, self.type, parentValue);
}

static CGFloat getValueInternal(NSArray* calcArray, CGFloat value, LynxPlatformLengthUnit type,
                                CGFloat parentValue) {
  if (type == LynxPlatformLengthUnitPercentage) {
    return value * parentValue;
  } else if (type == LynxPlatformLengthUnitNumber) {
    return value;
  } else if (type == LynxPlatformLengthUnitCalc) {
    float ret = 0;

    for (uint i = 0; i < [calcArray count]; i += 2) {
      NSArray* itemArray = nil;
      float itemValue = 0;
      LynxPlatformLengthUnit itemType = [[calcArray objectAtIndex:i + 1] intValue];

      if (itemType == LynxPlatformLengthUnitCalc) {
        itemArray = [calcArray objectAtIndex:i];
      } else if (itemType == LynxPlatformLengthUnitNumber ||
                 itemType == LynxPlatformLengthUnitPercentage) {
        itemValue = [[calcArray objectAtIndex:i] floatValue];
      }
      ret += getValueInternal(itemArray, itemValue, itemType, parentValue);
    }
    return ret;
  } else {
    return 0;
  }
}

- (CGFloat)numberValue {
  return self.type == LynxPlatformLengthUnitNumber ? self.value : .0f;
}

static inline bool LynxCGFloatEqual(CGFloat a, CGFloat b) {
  return (fabs((a) - (b)) < CGFLOAT_EPSILON);
}

- (BOOL)isEqual:(id)object {
  if (self == object) {
    return YES;
  }
  if (![object isKindOfClass:[LynxPlatformLength class]]) {
    return NO;
  }
  LynxPlatformLength* other = (LynxPlatformLength*)object;
  return [self type] == [other type] && ([self type] != LynxPlatformLengthUnitCalc
                                             ? LynxCGFloatEqual([self value], [other value])
                                             : [[self calcArray] isEqualToArray:[other calcArray]]);
}

// Ensure two equal object have same hash.

- (NSUInteger)hash {
  if (self.type == LynxPlatformLengthUnitCalc) {
    return [self.calcArray hash];
  }

  return self.type * (NSUInteger)self.value;
}

@end
