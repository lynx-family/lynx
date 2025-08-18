// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxLayoutTick.h>

@implementation LynxLayoutTick {
  LynxOnLayoutBlock _block;
  BOOL _enableLayout;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _block = nil;
  }
  return self;
}

- (void)setLayoutBlock:(LynxOnLayoutBlock)block {
  _block = block;
}

- (void)requestLayout {
  _enableLayout = YES;
}

- (void)requestLayout:(LynxOnLayoutBlock)block {
  _enableLayout = YES;
  _block = block;
}

- (void)triggerLayout {
  if (_enableLayout) {
    if (_block) {
      _block();
    } else {
      [NSException raise:@"LynxLayoutTickException"
                  format:@"triggerLayout called without a layout block set"];
    }
    _enableLayout = NO;
  }
}

- (void)cancelLayoutRequest {
  _enableLayout = NO;
}

@end
