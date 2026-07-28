// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLogContext+Internal.h"

@implementation LynxLogContext

- (instancetype)initWithViewId:(int64_t)viewId
                      engineId:(int64_t)engineId
                     runtimeId:(int64_t)runtimeId {
  if (self = [super init]) {
    _viewId = viewId;
    _engineId = engineId;
    _runtimeId = runtimeId;
  }
  return self;
}

- (NSString*)description {
  return [NSString stringWithFormat:@"[%lld,%lld,%lld]", (long long)_viewId, (long long)_engineId,
                                    (long long)_runtimeId];
}

@end
