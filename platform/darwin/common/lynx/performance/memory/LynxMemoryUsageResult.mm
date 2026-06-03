// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryUsageResult.h>

#import <Lynx/LynxEventReporter.h>

@implementation LynxInstanceMemoryUsage

- (instancetype)init {
  self = [super init];
  if (self) {
    _instanceId = kUnknownInstanceId;
    _pageId = @"";
    _url = @"";
    _viewDetail = @{};
    _btsRuntimeGroupId = @"";
  }
  return self;
}

@end

@implementation LynxGlobalMemoryUsageResult

- (instancetype)init {
  self = [super init];
  if (self) {
    _instances = @[];
  }
  return self;
}

@end
