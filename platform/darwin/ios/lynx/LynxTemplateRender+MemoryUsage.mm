// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateRender+Internal.h>

#import "LynxTemplateRender+Protected.h"
#import "platform/darwin/common/lynx/performance/memory/LynxMemoryUsageFetcher.h"

#include "core/shell/performance/native_memory_usage_query.h"

#include <utility>

typedef void (^LynxTemplateRenderNativeMemoryUsageCallback)(
    const lynx::tasm::performance::NativeMemoryUsageSnapshot& snapshot);

@interface LynxTemplateRenderMemoryUsageFetcher : NSObject <LynxMemoryUsageFetcher>

- (instancetype)initWithTemplateRender:(LynxTemplateRender*)templateRender;

@end

@implementation LynxTemplateRender (MemoryUsage)

- (void)registerMemoryUsageFetcherIfNeeded {
  if (!_memoryUsageFetcher) {
    _memoryUsageFetcher =
        [[LynxTemplateRenderMemoryUsageFetcher alloc] initWithTemplateRender:self];
  }
  [LynxMemoryUsageFetchers registerFetcher:_memoryUsageFetcher];
}

- (void)unregisterMemoryUsageFetcherIfNeeded {
  // Unregistering only removes this instance from future global requests. If a
  // request already captured this fetcher in its snapshot, the per-instance
  // query path is still responsible for returning nil or a complete best-effort
  // result exactly once.
  [LynxMemoryUsageFetchers unregisterFetcher:_memoryUsageFetcher];
}

- (NSString*)memoryUsageTemplateUrl {
  return _url ?: @"";
}

- (BOOL)queryNativeMemoryUsageForGlobalCollectorAsync:
    (LynxTemplateRenderNativeMemoryUsageCallback)callback {
  if (!callback || shell_ == nullptr || shell_->IsDestroyed()) {
    return NO;
  }

  LynxTemplateRenderNativeMemoryUsageCallback callbackCopy = [callback copy];
  auto nativeCallback =
      [callbackCopy](const lynx::tasm::performance::NativeMemoryUsageSnapshot& snapshot) {
        // NativeMemoryUsageQuery returns snapshots on the report thread. Let the
        // fetcher mutate its per-instance query inline there so native and view
        // completion stay serialized without another report-thread hop.
        callbackCopy(snapshot);
      };
  shell_->QueryNativeMemoryUsageAsync(std::move(nativeCallback));
  return YES;
}

@end
