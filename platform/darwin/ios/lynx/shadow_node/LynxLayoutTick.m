// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLayoutTick.h"

@implementation LynxLayoutTick {
  LynxOnLayoutBlock _block;
  BOOL _enableLayout;
}

- (instancetype)initWithBlock:(LynxOnLayoutBlock)block {
  self = [super init];
  if (self) {
    _block = block;
  }
  return self;
}

- (void)requestLayout {
  _enableLayout = YES;
}

- (void)triggerLayout {
  if (_enableLayout) {
    _block();
    _enableLayout = NO;
  }
}

- (void)cancelLayoutRequest {
  _enableLayout = NO;
}

@end
