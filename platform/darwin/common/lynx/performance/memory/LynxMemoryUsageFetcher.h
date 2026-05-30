// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxMemoryUsageResult.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxMemoryUsageFetcherCallback)(LynxInstanceMemoryUsage *_Nullable result);

@protocol LynxMemoryUsageFetcher <NSObject>

// Reports one instance memory sample for a global memory query. Implementations may invoke the
// callback on any thread, but they should invoke it exactly once; passing nil is treated as a
// completed fetch with no instance sample. The collector defensively ignores later invocations from
// the same fetcher for the same query.
- (void)queryMemoryUsageAsync:(LynxMemoryUsageFetcherCallback)callback;

@end

@interface LynxMemoryUsageFetchers : NSObject

// Registers a fetcher by object identity. The registry stores weak references, so callers should
// unregister during teardown but do not need the registry to keep the fetcher alive.
+ (BOOL)registerFetcher:(nullable id<LynxMemoryUsageFetcher>)fetcher;
+ (BOOL)unregisterFetcher:(nullable id<LynxMemoryUsageFetcher>)fetcher;

// Returns a strong, fixed snapshot for one query. Later registration changes do not affect the
// caller's expected fetch count.
+ (NSArray<id<LynxMemoryUsageFetcher>> *)fetchersForCurrentQuery;

@end

NS_ASSUME_NONNULL_END
