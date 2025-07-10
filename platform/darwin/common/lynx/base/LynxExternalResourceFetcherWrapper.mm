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

- (BOOL)fetchResource:(NSString*)url withLoadedBlock:(LoadedBlock)callback sync:(BOOL)sync {
  if (!_component_fetcher) {
    // false if component fetcher not set;
    return false;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, DYNAMIC_COMPONENT_FETCHER_LOAD_COMPONENT, "url",
              [url UTF8String]);
  if (!sync) {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^() {
      [self->_component_fetcher loadDynamicComponent:url withLoadedBlock:callback];
    });
  } else {
    [_component_fetcher loadDynamicComponent:url withLoadedBlock:callback];
  }
  return true;
}

@end
