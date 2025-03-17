// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTraceController.h>
#import <LynxDevtool/TestBenchTraceProfileHelper.h>

@interface TestBenchTraceProfileHelper ()

@end

@implementation TestBenchTraceProfileHelper {
  LynxTraceController *_controller;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _controller = [LynxTraceController sharedInstance];
  }
  return self;
}

- (void)startTrace {
  if (_controller) {
    [_controller startTracing:nullptr config:@{}];
  }
}

- (void)startTrace:(int)bufferSize {
  if (_controller) {
    [_controller startTracing:nullptr config:@{@"buffer_size" : @(bufferSize)}];
  }
}

- (void)stopTrace {
  if (_controller) {
    [_controller stopTracing];
  }
}
@end
