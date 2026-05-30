// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxGlobalMemoryUsageCollectionContext.h"

@interface LynxGlobalMemoryUsageCollectionContext ()

@property(nonatomic, strong, readonly) NSMutableArray<id> *callbacks;

@end

@implementation LynxGlobalMemoryUsageCollectionContext

- (instancetype)initWithCollectionId:(uint64_t)collectionId
                   collectionStartMs:(int64_t)collectionStartMs
               expectedInstanceCount:(NSInteger)expectedInstanceCount
                            callback:(LynxGlobalMemoryUsageCallback)callback {
  self = [super init];
  if (self) {
    _collectionId = collectionId;
    _collectionStartMs = collectionStartMs;
    _expectedInstanceCount = expectedInstanceCount;
    _callbacks = [NSMutableArray array];
    _instances = [NSMutableArray array];
    [_callbacks addObject:[callback copy]];
  }
  return self;
}

- (BOOL)matchesCollectionId:(uint64_t)collectionId {
  return _collectionId == collectionId;
}

- (void)addCallback:(LynxGlobalMemoryUsageCallback)callback {
  [_callbacks addObject:[callback copy]];
}

- (void)recordInstanceResult:(LynxInstanceMemoryUsage *_Nullable)result {
  _receivedFetchResultCount++;
  if (result) {
    [_instances addObject:result];
  }
}

- (BOOL)hasReceivedAllFetchResults {
  return _receivedFetchResultCount >= _expectedInstanceCount;
}

- (NSArray<id> *)copyCallbacksForCompletion {
  return [_callbacks copy];
}

@end
