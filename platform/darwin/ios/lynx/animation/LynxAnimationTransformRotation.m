// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxAnimationTransformRotation.h"

static const float kEPSILON = 0.001;
@implementation LynxAnimationTransformRotation

- (id)init {
  if ([super init]) {
    _rotationX = _rotationY = _rotationZ = 0.0;
  }
  return self;
}

- (BOOL)isEqualToTransformRotation:(LynxAnimationTransformRotation *)transformRotation {
  if (!transformRotation) {
    return NO;
  }
  return (fabs(_rotationX - transformRotation.rotationX) < kEPSILON &&
          fabs(_rotationY - transformRotation.rotationY) < kEPSILON &&
          fabs(_rotationZ - transformRotation.rotationZ) < kEPSILON);
}

@end
