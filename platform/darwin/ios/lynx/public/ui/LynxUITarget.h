// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxUITarget <NSObject>
@optional
- (void)targetOnScreen;
- (void)freeMemoryCache;
- (void)targetOffScreen;
- (int64_t)memoryUsageBytes;
- (NSDictionary<NSString *, NSString *> *)memoryUsageDetail;
/**
 * Returns bytes to count in this collection. Shared allocations should be counted
 * only when their owner is first added to sharedOwners, which compares by identity.
 * The default implementation calls memoryUsageBytes.
 */
- (int64_t)memoryUsageBytesWithSharedOwners:(NSHashTable *)sharedOwners;
@end

NS_ASSUME_NONNULL_END
