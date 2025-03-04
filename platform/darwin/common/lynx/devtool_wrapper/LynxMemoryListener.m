// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "LynxMemoryListener.h"

@implementation LynxMemoryListener

+ (instancetype)shareInstance {
  static LynxMemoryListener* instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[self alloc] init];
  });
  return instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    self.memoryReporters = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void)addMemoryReporter:(id<LynxMemoryReporter>)report {
  [self.memoryReporters addObject:report];
}

- (void)removeMemoryReporter:(id<LynxMemoryReporter>)report {
  [self.memoryReporters removeObject:report];
}

- (void)uploadImageInfo:(NSDictionary*)data {
  for (id<LynxMemoryReporter> report in self.memoryReporters) {
    [report uploadImageInfo:data];
  }
}

@end
