// Copyright 2025 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxGestureExtraBundle.h"

@implementation LynxGestureExtraBundle

- (instancetype)init {
  self = [super init];
  if (self) {
    _gestureDirection = 0;
    _simultaneousDeltaX = 0;
    _simultaneousDeltaY = 0;
    _isNeedConsumedSimultaneousGesture = NO;
  }
  return self;
}

- (void)resetSimultaneousDelta {
  self.simultaneousDeltaX = 0;
  self.simultaneousDeltaY = 0;
  self.isNeedConsumedSimultaneousGesture = NO;
}

@end
