// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxViewportMetrics.h>

@implementation LynxViewportMetrics

- (instancetype)initWithSize:(CGSize)size
                   widthMode:(LynxViewSizeMode)widthMode
                  heightMode:(LynxViewSizeMode)heightMode {
  self = [super init];
  if (self) {
    _size = size;
    _widthMode = widthMode;
    _heightMode = heightMode;
  }
  return self;
}

@end
