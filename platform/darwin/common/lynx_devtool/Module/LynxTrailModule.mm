// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTrailModule.h"

#import "Lynx/LynxLog.h"
#import "Lynx/LynxService.h"

@implementation LynxTrailModule {
  __weak LynxContext *context_;
}

+ (NSString *)name {
  return @"LynxTrailModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"getSettings" : NSStringFromSelector(@selector(getSettings)),
  };
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    context_ = context;
  }
  return self;
}

- (NSDictionary *)getSettings {
  return [LynxTrail getAllValues];
}

@end
