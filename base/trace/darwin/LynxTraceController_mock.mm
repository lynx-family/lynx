// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTraceController.h"

static LynxTraceController* lynxTraceController = nil;

@implementation LynxTraceController {
}

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    lynxTraceController = [[self alloc] init];
  });
  return lynxTraceController;
}

- (intptr_t)getTraceController {
  return 0;
}

- (void)startTrace {
}

- (void)stopTrace {
}

- (void)startTracing:(completeBlockType)completeBlock config:(NSDictionary*)config {
}

- (void)startTracing:(completeBlockType)completeBlock jsonConfig:(NSString*)config {
}

- (void)stopTracing {
}

- (void)startStartupTracingIfNeeded {
}

- (void)onTracingComplete:(NSString*)traceFile {
}
@end
