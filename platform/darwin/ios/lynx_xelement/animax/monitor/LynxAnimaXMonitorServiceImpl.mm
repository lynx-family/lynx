// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxAnimaXMonitorServiceImpl.h>

#import <AnimaX/AnimaXURLUtils.h>
#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxUIContext.h>
#include "src/player/animax_monitor_event.h"

static NSString *const AnimaXMonitorIntegrationTypeLynx = @"lynx";

@interface LynxAnimaXMonitorServiceImpl ()
@property(nonatomic, assign) int32_t instanceId;
@property(nonatomic, copy) NSString *pageUrl;

- (NSDictionary *)appendURLInfo:(NSDictionary *)params;
- (void)reportEvent:(NSString *)eventName params:(NSDictionary *)params;
@end

@implementation LynxAnimaXMonitorServiceImpl

- (instancetype)initWithLynxUIContext:(nullable LynxUIContext *)context {
  if (self = [super init]) {
    _instanceId = -1;
    _pageUrl = AnimaXMonitorUnknownUrl;

    if (context) {
      _instanceId = context.rootView.instanceId;

      NSString *viewUrl = context.rootView.url;
      if (viewUrl) {
        _pageUrl = [viewUrl copy];
      }
    }
  }
  return self;
}

- (NSDictionary *)appendURLInfo:(NSDictionary *)params {
  NSString *sourceUrl = [AnimaXURLUtils clearUrlQuery:[self.urlHolder getCurrentUrl]];
  NSString *pageUrl = [AnimaXURLUtils clearUrlQuery:self.pageUrl];

  NSMutableDictionary *newParams = [NSMutableDictionary dictionaryWithDictionary:params];
  [newParams addEntriesFromDictionary:@{
    AnimaXMonitorSourceUrl : sourceUrl,
    AnimaXMonitorPageUrl : pageUrl,
    AnimaXMonitorIntegrationType : AnimaXMonitorIntegrationTypeLynx
  }];

  return newParams;
}

- (void)reportEvent:(NSString *)eventName params:(NSDictionary *)params {
  NSDictionary *newParams = [self appendURLInfo:params];
  [LynxEventReporter onEvent:eventName instanceId:self.instanceId props:newParams];
}

- (void)reportError:(NSDictionary<NSString *, NSObject *> *)params {
  if (params.count == 0) {
    return;
  }

  [self reportEvent:@(lynx::animax::kErrorEventName.c_str()) params:params];
}

- (void)reportPerformance:(NSDictionary<NSString *, NSObject *> *)params {
  if (params.count == 0) {
    return;
  }

  [self reportEvent:@(lynx::animax::kPerformanceEventName.c_str()) params:params];
}

@end
