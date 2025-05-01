// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxMemoryRecord.h"

@implementation LynxMemoryRecord

- (instancetype)initWithCategory:(NSString*)category
                          sizeKb:(float)sizeKb
                          detail:(NSDictionary<NSString*, NSString*>*)detail {
  if ([self init]) {
    _category = category;
    _sizeKb = sizeKb;
    _detail = detail;
  }
  return self;
}

@end
