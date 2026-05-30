// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxMemoryUsageQuery.h>

NS_ASSUME_NONNULL_BEGIN

// Request-scoped state for one global memory collection. The collector only touches this object on
// the report thread, so the context does not need its own locking.
@interface LynxGlobalMemoryUsageCollectionContext : NSObject

@property(nonatomic, assign, readonly) uint64_t collectionId;
@property(nonatomic, assign, readonly) int64_t collectionStartMs;
@property(nonatomic, assign, readonly) NSInteger expectedInstanceCount;
@property(nonatomic, assign, readonly) NSInteger receivedFetchResultCount;
@property(nonatomic, strong, readonly) NSMutableArray<LynxInstanceMemoryUsage *> *instances;

- (instancetype)initWithCollectionId:(uint64_t)collectionId
                   collectionStartMs:(int64_t)collectionStartMs
               expectedInstanceCount:(NSInteger)expectedInstanceCount
                            callback:(LynxGlobalMemoryUsageCallback)callback;
- (BOOL)matchesCollectionId:(uint64_t)collectionId;
- (void)addCallback:(LynxGlobalMemoryUsageCallback)callback;
- (void)recordInstanceResult:(LynxInstanceMemoryUsage *_Nullable)result;
- (BOOL)hasReceivedAllFetchResults;

// Returns a snapshot of callbacks to invoke after the collector clears its active request pointer.
// This keeps user callbacks free to re-enter the public API and start a new request.
- (NSArray<id> *)copyCallbacksForCompletion;

@end

NS_ASSUME_NONNULL_END
