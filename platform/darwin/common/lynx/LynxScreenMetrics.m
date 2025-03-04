// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import "LynxScreenMetrics.h"

@implementation LynxScreenMetrics

+ (LynxScreenMetrics*)getDefaultLynxScreenMetrics {
  static dispatch_once_t onceToken;
  static LynxScreenMetrics* defaultScreenMetrics = nil;
  dispatch_once(&onceToken, ^{
    defaultScreenMetrics = [[LynxScreenMetrics alloc] init];
  });
  return defaultScreenMetrics;
}

- (instancetype)init {
  if (self = [super init]) {
#if OS_IOS
    _screenSize = [UIScreen mainScreen].bounds.size;
    _scale = [UIScreen mainScreen].scale;
#else
    _screenSize = [[NSScreen mainScreen] frame].size;
    _scale = 1.0f;
#endif
  }
  return self;
}

- (instancetype)initWithScreenSize:(CGSize)screenSize scale:(CGFloat)scale {
  if (self = [self init]) {
    [self setLynxScreenSize:screenSize];
    _scale = scale;
  }
  return self;
}

- (void)setLynxScreenSize:(CGSize)screenSize {
  _screenSize = screenSize;
}

@end
