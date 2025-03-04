// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxExternalResourceFetcherWrapper.h"

#import "LynxError.h"
#import "LynxSubErrorCode.h"

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

@implementation LynxExternalResourceFetcherWrapper {
  id<LynxResourceFetcher> _lynx_service_fetcher;
  id<LynxDynamicComponentFetcher> _component_fetcher;
}

- (instancetype)initWithDynamicComponentFetcher:(id<LynxDynamicComponentFetcher>)fetcher {
  if (self = [super init]) {
    _component_fetcher = fetcher;
    _lynx_service_fetcher = [LynxResourceServiceFetcher ensureLynxService]
                                ? [[LynxResourceServiceFetcher alloc] init]
                                : nil;
    _enableLynxService = false;
  }
  return self;
}

- (void)fetchResource:(NSString*)url withLoadedBlock:(LoadedBlock)callback {
  // firstly, try lynx resource service via enableLynxService set by front-end
  if ([self enableLynxService]) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxResourceServiceFetcher::fetchResource", "url",
                [url UTF8String]);
    // if lynx resource service fetcher exists, use it, otherwise, try other fetchers
    if (_lynx_service_fetcher) {
      [_lynx_service_fetcher
          loadResourceWithURLString:url
                         completion:^(BOOL isSyncCallback, NSData* _Nullable data,
                                      NSError* _Nullable error, NSURL* _Nullable resURL) {
                           callback(data, error);
                         }];
      return;
    } else {
      // try other fetchers
      LOGW("LynxResourceServiceFetcher is nil, switch to the fetchers registered in by host. ");
    }
  }

  // secondly, try dynamic component fetcher registered by host
  if (_component_fetcher) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "DynamicComponentFetcher::loadDynamicComponent", "url",
                [url UTF8String]);
    [_component_fetcher loadDynamicComponent:url withLoadedBlock:callback];
    return;
  }

  // No available provider or fetcher
  callback(nil, [LynxError lynxErrorWithCode:ECLynxResourceExternalResourceRequestFailed
                                 description:@"No available provider or fetcher"]);
}

@end
