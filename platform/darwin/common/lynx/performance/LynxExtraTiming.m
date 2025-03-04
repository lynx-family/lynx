// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxExtraTiming.h"

@implementation LynxExtraTiming

- (NSDictionary *)toDictionary {
  return @{
    @"open_time" : @(self.openTime),
    @"container_init_start" : @(self.containerInitStart),
    @"container_init_end" : @(self.containerInitEnd),
    @"prepare_template_start" : @(self.prepareTemplateStart),
    @"prepare_template_end" : @(self.prepareTemplateEnd)
  };
}

@end
