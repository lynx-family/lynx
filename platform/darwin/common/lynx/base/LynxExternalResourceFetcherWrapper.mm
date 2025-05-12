// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxExternalResourceFetcherWrapper.h>

#import <Lynx/LynxError.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxTraceEventDef.h>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

@implementation LynxExternalResourceFetcherWrapper {
  id<LynxDynamicComponentFetcher> _component_fetcher;
}

- (instancetype)initWithDynamicComponentFetcher:(id<LynxDynamicComponentFetcher>)fetcher {
  if (self = [super init]) {
    _component_fetcher = fetcher;
  }
  return self;
}

- (void)fetchResource:(NSString*)url withLoadedBlock:(LoadedBlock)callback {
  if (_component_fetcher) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, DYNAMIC_COMPONENT_FETCHER_LOAD_COMPONENT, "url",
                [url UTF8String]);
    [_component_fetcher loadDynamicComponent:url withLoadedBlock:callback];
    return;
  }

  // No available provider or fetcher
  callback(nil, [LynxError lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                 description:@"No available provider or fetcher"]);
}

@end
