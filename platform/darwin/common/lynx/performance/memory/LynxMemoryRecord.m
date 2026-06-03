// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryRecord.h>

@implementation LynxMemoryRecord

- (instancetype)initWithCategory:(NSString*)category
                       sizeBytes:(int64_t)sizeBytes
                          detail:(NSDictionary<NSString*, NSString*>* _Nullable)detail {
  if ([self init]) {
    _category = category;
    _sizeBytes = sizeBytes;
    _detail = detail;
  }
  return self;
}

- (id)copyWithZone:(NSZone*)zone {
  LynxMemoryRecord* recordCopy =
      [[[self class] allocWithZone:zone] initWithCategory:[_category copy] ?: @""
                                                sizeBytes:_sizeBytes
                                                   detail:[_detail copy]];
  recordCopy.instanceCount = _instanceCount;
  return recordCopy;
}

@end
