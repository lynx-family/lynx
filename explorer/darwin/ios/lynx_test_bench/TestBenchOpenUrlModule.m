// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchOpenUrlModule.h"
#import "TestBenchPageManager.h"

@implementation TestBenchOpenUrlModule

+ (NSString *)name {
  return @"TestBenchOpenUrlModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"openSchema" : NSStringFromSelector(@selector(openSchema:)),
  };
}

- (void)openSchema:(NSDictionary *)params {
  [[TestBenchPageManager sharedInstance] replayPageFromOpenSchema:params];
}

@end
