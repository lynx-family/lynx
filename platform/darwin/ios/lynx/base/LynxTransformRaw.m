// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTransformRaw.h"
#import "LynxCSSType.h"
#import "LynxLog.h"

@implementation LynxTransformRaw {
  LynxPlatformLengthUnit _p0Unit;
  LynxPlatformLengthUnit _p1Unit;
  LynxPlatformLengthUnit _p2Unit;
}

- (instancetype)initWithArray:(NSArray*)arr {
  if (self = [super init]) {
    _type = [arr[0] intValue];
    if ([self isMatrix]) {
      [self initTransformMatrixWithArray:arr];
    } else {
      _p0Unit = (LynxPlatformLengthUnit)[arr[2] intValue];
      _p1Unit = (LynxPlatformLengthUnit)[arr[4] intValue];
      _p2Unit = (LynxPlatformLengthUnit)[arr[6] intValue];
      if ([self isTranslate]) {
        _platformLengthP0 = [[LynxPlatformLength alloc] initWithValue:arr[1] type:_p0Unit];
        _platformLengthP1 = [[LynxPlatformLength alloc] initWithValue:arr[3] type:_p1Unit];
        _platformLengthP2 = [[LynxPlatformLength alloc] initWithValue:arr[5] type:_p2Unit];
      } else {
        _p0 = [arr[1] doubleValue];
        _p1 = [arr[3] doubleValue];
        _p2 = [arr[5] doubleValue];
      }
    }
  }
  return self;
}

- (void)initTransformMatrixWithArray:(NSArray*)array {
  if (array.count == 7) {
    self.transformMatrix = (CATransform3D){[array[1] doubleValue],
                                           0,
                                           0,
                                           0,
                                           0,
                                           [array[2] doubleValue],
                                           0,
                                           0,
                                           0,
                                           0,
                                           [array[3] doubleValue],
                                           [array[4] doubleValue],
                                           [array[5] doubleValue],
                                           [array[6] doubleValue],
                                           0,
                                           1};
  } else if (array.count == 17) {
    self.transformMatrix =
        (CATransform3D){[array[1] doubleValue],  [array[2] doubleValue],  [array[3] doubleValue],
                        [array[4] doubleValue],  [array[5] doubleValue],  [array[6] doubleValue],
                        [array[7] doubleValue],  [array[8] doubleValue],  [array[9] doubleValue],
                        [array[10] doubleValue], [array[11] doubleValue], [array[12] doubleValue],
                        [array[13] doubleValue], [array[14] doubleValue], [array[15] doubleValue],
                        [array[16] doubleValue]};
  } else {
    LLogError(@"Error: array contains 7 or 17!");
  }
}

- (bool)isP0Percent {
  return _p0Unit == LynxPlatformLengthUnitPercentage;
}
- (bool)isP1Percent {
  return _p1Unit == LynxPlatformLengthUnitPercentage;
}
- (bool)isP2Percent {
  return _p2Unit == LynxPlatformLengthUnitPercentage;
}

- (bool)isMatrix {
  LynxTransformType type = (LynxTransformType)_type;
  return type == LynxTransformTypeMatrix || type == LynxTransformTypeMatrix3d;
}

- (bool)isTranslate {
  LynxTransformType type = (LynxTransformType)_type;
  return type == LynxTransformTypeTranslate || type == LynxTransformTypeTranslate3d ||
         type == LynxTransformTypeTranslateX || type == LynxTransformTypeTranslateY ||
         type == LynxTransformTypeTranslateZ;
}

- (bool)isRotate {
  LynxTransformType type = (LynxTransformType)_type;
  return type == LynxTransformTypeRotate || type == LynxTransformTypeRotateX ||
         type == LynxTransformTypeRotateY || type == LynxTransformTypeRotateZ;
}
- (bool)isRotateXY {
  LynxTransformType type = (LynxTransformType)_type;
  return type == LynxTransformTypeRotateX || type == LynxTransformTypeRotateY;
}

+ (NSArray<LynxTransformRaw*>*)toTransformRaw:(NSArray*)arr {
  if ([arr isEqual:[NSNull null]] || nil == arr || [arr count] == 0) {
    return nil;
  }
  NSMutableArray<LynxTransformRaw*>* result = [[NSMutableArray alloc] init];
  for (NSUInteger i = 0; i < arr.count; i++) {
    if ([arr[i] isKindOfClass:[NSArray class]]) {
      NSArray* item = (NSArray*)arr[i];
      if (item != nil && item.count >= 4) {
        [result addObject:[[LynxTransformRaw alloc] initWithArray:item]];
      }
    }
  }
  return [result copy];
}

+ (bool)hasPercent:(NSArray<LynxTransformRaw*>*)arr {
  if (nil == arr || arr.count == 0) {
    return false;
  }
  for (LynxTransformRaw* raw in arr) {
    if ([raw isP0Percent] || [raw isP1Percent]) {
      return true;
    }
  }
  return false;
}

// TODO: will remove in release/2.2
+ (CGFloat)getRotateZRad:(NSArray<LynxTransformRaw*>*)arr {
  CGFloat rotateZ = 0;
  if (nil == arr || arr.count == 0) {
    return rotateZ;
  }
  for (LynxTransformRaw* raw in arr) {
    LynxTransformType type = (LynxTransformType)raw.type;
    if (type == LynxTransformTypeRotateZ || type == LynxTransformTypeRotate) {
      rotateZ = raw.p0 * M_PI / 180;
    }
  }
  return rotateZ;
}

+ (bool)hasScaleOrRotate:(NSArray<LynxTransformRaw*>*)arr {
  if (nil == arr || arr.count == 0) {
    return false;
  }
  for (LynxTransformRaw* raw in arr) {
    LynxTransformType type = (LynxTransformType)raw.type;
    if (type == LynxTransformTypeScale || type == LynxTransformTypeScaleX ||
        type == LynxTransformTypeScaleY || type == LynxTransformTypeRotate ||
        type == LynxTransformTypeRotateX || type == LynxTransformTypeRotateY ||
        type == LynxTransformTypeRotateZ) {
      return true;
    }
  }
  return false;
}

+ (CGFloat)getTranslateX:(NSArray<LynxTransformRaw*>*)arr {
  if (nil == arr || arr.count == 0) {
    return 0;
  }

  CGFloat res = 0;

  for (LynxTransformRaw* raw in arr) {
    LynxTransformType type = (LynxTransformType)raw.type;
    switch (type) {
      case LynxTransformTypeTranslateX:
      case LynxTransformTypeTranslate3d:
        res = [raw.platformLengthP0 numberValue];
        break;
      default:
        break;
    }
  }

  return res;
}

+ (CGFloat)getTranslateY:(NSArray<LynxTransformRaw*>*)arr {
  if (nil == arr || arr.count == 0) {
    return 0;
  }

  CGFloat res = 0;

  for (LynxTransformRaw* raw in arr) {
    LynxTransformType type = (LynxTransformType)raw.type;
    switch (type) {
      case LynxTransformTypeTranslateY:
        res = [raw.platformLengthP0 numberValue];
      case LynxTransformTypeTranslate3d:
        res = [raw.platformLengthP1 numberValue];
        break;
      default:
        break;
    }
  }

  return res;
}
@end
