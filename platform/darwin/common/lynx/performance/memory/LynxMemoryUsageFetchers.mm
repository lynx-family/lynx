// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxMemoryUsageFetcher.h"

namespace {

NSHashTable<id<LynxMemoryUsageFetcher>> *LynxMemoryUsageFetcherStorage() {
  static NSHashTable<id<LynxMemoryUsageFetcher>> *fetchers = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    // Weak storage keeps the registry from extending a fetcher's lifecycle.
    // The owner that registers the fetcher remains responsible for retaining it.
    //
    // Pointer personality makes registration identity-based, even if a fetcher overrides
    // hash/isEqual: to compare equal to another fetcher object.
    NSPointerFunctionsOptions options =
        NSPointerFunctionsWeakMemory | NSPointerFunctionsObjectPointerPersonality;
    fetchers = [NSHashTable hashTableWithOptions:options];
  });
  return fetchers;
}

}  // namespace

@implementation LynxMemoryUsageFetchers

+ (BOOL)registerFetcher:(nullable id<LynxMemoryUsageFetcher>)fetcher {
  if (!fetcher) {
    return NO;
  }
  NSHashTable<id<LynxMemoryUsageFetcher>> *storage = LynxMemoryUsageFetcherStorage();
  @synchronized(storage) {
    if ([storage containsObject:fetcher]) {
      return NO;
    }
    [storage addObject:fetcher];
    return YES;
  }
}

+ (BOOL)unregisterFetcher:(nullable id<LynxMemoryUsageFetcher>)fetcher {
  if (!fetcher) {
    return NO;
  }
  NSHashTable<id<LynxMemoryUsageFetcher>> *storage = LynxMemoryUsageFetcherStorage();
  @synchronized(storage) {
    BOOL didUnregister = [storage containsObject:fetcher];
    [storage removeObject:fetcher];
    return didUnregister;
  }
}

+ (NSArray<id<LynxMemoryUsageFetcher>> *)fetchersForCurrentQuery {
  NSMutableArray<id<LynxMemoryUsageFetcher>> *fetchers = [NSMutableArray array];
  NSHashTable<id<LynxMemoryUsageFetcher>> *storage = LynxMemoryUsageFetcherStorage();
  @synchronized(storage) {
    // Enumerating the weak table also drops fetchers that were already deallocated. The returned
    // NSArray is a strong, fixed request snapshot, so expectedInstanceCount is not affected by
    // later lifecycle unregister calls.
    for (id<LynxMemoryUsageFetcher> fetcher in storage) {
      if (fetcher) {
        [fetchers addObject:fetcher];
      }
    }
  }
  return [fetchers copy];
}

@end
