// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTransformOriginRaw.h"
#import "LynxCSSType.h"

@implementation LynxTransformOriginRaw {
  LynxPlatformLengthUnit _p0Unit;
  LynxPlatformLengthUnit _p1Unit;
}

+ (LynxTransformOriginRaw*)convertToLynxTransformOriginRaw:(id)value {
  if ([value isEqual:[NSNull null]] || nil == value || ![value isKindOfClass:[NSArray class]]) {
    return nil;
  }
  NSArray* arr = (NSArray*)value;
  if (arr.count < 2) {
    return nil;
  }
  LynxTransformOriginRaw* ltor = [[LynxTransformOriginRaw alloc] init];
  if (arr.count == 2) {
    ltor.p0 = [arr[0] floatValue];
    ltor->_p0Unit = (LynxPlatformLengthUnit)[arr[1] intValue];
  } else if (arr.count == 4) {
    ltor.p0 = [arr[0] floatValue];
    ltor->_p0Unit = (LynxPlatformLengthUnit)[arr[1] intValue];
    ltor.p1 = [arr[2] floatValue];
    ltor->_p1Unit = (LynxPlatformLengthUnit)[arr[3] intValue];
  }
  return ltor;
}

- (instancetype)init {
  if (self = [super init]) {
    _p0 = 0.5f;
    _p0Unit = LynxPlatformLengthUnitPercentage;
    _p1 = 0.5f;
    _p1Unit = LynxPlatformLengthUnitPercentage;
  }
  return self;
}

- (bool)isValid {
  return [self isP0Valid] || [self isP1Valid];
}
- (bool)isP0Valid {
  return !(fabs(_p0 - 0.5f) < 0.001 && _p0Unit == LynxPlatformLengthUnitPercentage);
}
- (bool)isP1Valid {
  return !(fabs(_p1 - 0.5f) < 0.001 && _p1Unit == LynxPlatformLengthUnitPercentage);
}
- (bool)isP0Percent {
  return _p0Unit == LynxPlatformLengthUnitPercentage;
}
- (bool)isP1Percent {
  return _p1Unit == LynxPlatformLengthUnitPercentage;
}
- (bool)hasPercent {
  return [self isP0Percent] || [self isP1Percent];
}

- (bool)isDefault {
  return fabs(_p0 - 0.5f) < 0.001 && fabs(_p1 - 0.5f) < 0.001;
}

@end
