// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEventReporter.h"
#import "LynxService.h"
#import "LynxServiceEventReporterProtocol.h"
#import "LynxVersion.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/services/event_report/event_tracker_platform_impl.h"

// if you don't need instance id, just use kUnknownInstanceId.
int32_t const kUnknownInstanceId = -1;
// Lynx SDK's Version, set by LynxEnv.
static NSString *const kPropLynxSDKVersion = @"lynx_sdk_version";
NSString *const kLynxSDKErrorEvent = @"lynxsdk_error_event";

@interface LynxEventReporter ()

/**
 * Universal parameters generated internally by Lynx and can be used for all events.
 *
 * Note: Must be accessed on report thread.
 */
@property(nonatomic, strong)
    NSMutableDictionary<NSNumber *, NSMutableDictionary<NSString *, NSObject *> *> *allGenericInfo;
/**
 * allExtraParams is used to store all the extended parameters related to LynxView.These extended
 * parameters are set through LynxView.setExtraParams:forKey.The storage structure uses the address
 * of LynxView as the key and ExtraParams as the value.
 *
 *  Note: Must be accessed on report thread.
 */
@property(nonatomic, strong)
    NSMutableDictionary<NSNumber *, NSMutableDictionary<NSString *, NSObject *> *> *allExtraParams;

/// All external observers of event report.
@property(nonatomic, strong) NSHashTable<id<LynxEventReportObserverProtocol>> *observerList;

@end

@implementation LynxEventReporter

#pragma mark - Public Methods

+ (void)onEvent:(nonnull NSString *)eventName
     instanceId:(int32_t)instanceId
          props:(nullable NSDictionary<NSString *, NSObject *> *)props {
  if (!eventName || (instanceId < 0 && !props)) {
    // If instance id is unknown, props of event must be not null.
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEventReporter::onEvent",
              [&](lynx::perfetto::EventContext ctx) {
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("instanceId");
                  debug->set_string_value(std::to_string(instanceId));
                }
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("eventName");
                  debug->set_string_value([eventName UTF8String]);
                }
              });
  [self runOnReportThread:^{
    [[self sharedInstance] handleEvent:eventName props:props instanceId:instanceId];
  }];
}

+ (void)onEvent:(NSString *)eventName
      instanceId:(int32_t)instanceId
    propsBuilder:(NSDictionary<NSString *, NSObject *> * (^)(void))propsBuilder {
  if (!eventName || (instanceId < 0 && !propsBuilder)) {
    // If instance id is unknown, props of event must be not null.
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEventReporter::onEvent",
              [&](lynx::perfetto::EventContext ctx) {
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("instanceId");
                  debug->set_string_value(std::to_string(instanceId));
                }
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("eventName");
                  debug->set_string_value([eventName UTF8String]);
                }
              });
  [self runOnReportThread:^{
    NSDictionary<NSString *, NSObject *> *props = nil;
    if (propsBuilder) {
      props = propsBuilder();
    }
    if ((instanceId < 0 && !props)) {
      // If instance id is unknown, props of event must be not null.
      return;
    }
    [[self sharedInstance] handleEvent:eventName props:props instanceId:instanceId];
  }];
}

+ (void)updateGenericInfo:(NSObject *)value key:(NSString *)key instanceId:(int32_t)instanceId {
  if (!value || !key || instanceId < 0) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEventReporter::updateGenericInfo",
              [&](lynx::perfetto::EventContext ctx) {
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("instanceId");
                  debug->set_string_value(std::to_string(instanceId));
                }
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("key");
                  debug->set_string_value([key UTF8String]);
                }
              });
  [self runOnReportThread:^{
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEventReporter::updateGenericInfo.run",
                [&](lynx::perfetto::EventContext ctx) {
                  {
                    auto *debug = ctx.event()->add_debug_annotations();
                    debug->set_name("instanceId");
                    debug->set_string_value(std::to_string(instanceId));
                  }
                  {
                    auto *debug = ctx.event()->add_debug_annotations();
                    debug->set_name("key");
                    debug->set_string_value([key UTF8String]);
                  }
                });
    NSNumber *instanceIdNum = @(instanceId);
    LynxEventReporter *reporter = [self sharedInstance];
    NSMutableDictionary<NSString *, NSObject *> *genericInfo =
        [[reporter allGenericInfo] objectForKey:instanceIdNum];
    if (!genericInfo) {
      genericInfo = [NSMutableDictionary dictionary];
      [genericInfo setObject:[LynxVersion versionString] forKey:kPropLynxSDKVersion];
      [[reporter allGenericInfo] setObject:genericInfo forKey:instanceIdNum];
    }
    [genericInfo setObject:value forKey:key];
  }];
}

+ (void)removeGenericInfo:(int32_t)instanceId {
  if (instanceId < 0) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEventReporter::removeGenericInfo",
              [&](lynx::perfetto::EventContext ctx) {
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("instanceId");
                debug->set_string_value(std::to_string(instanceId));
              });
  [self runOnReportThread:^{
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEventReporter::removeGenericInfo.run",
                [&](lynx::perfetto::EventContext ctx) {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("instanceId");
                  debug->set_string_value(std::to_string(instanceId));
                });
    [[[self sharedInstance] allGenericInfo] removeObjectForKey:@(instanceId)];
  }];
}

+ (void)moveExtraParams:(int32_t)originInstanceId toInstanceId:(int32_t)targetInstanceId {
  if (originInstanceId < 0 || targetInstanceId < 0) {
    return;
  }
  [self runOnReportThread:^{
    LynxEventReporter *reporter = [self sharedInstance];
    NSNumber *originInstanceIdNum = @(originInstanceId);
    NSMutableDictionary<NSString *, NSObject *> *extraParams =
        [[reporter allExtraParams] objectForKey:originInstanceIdNum];
    if (extraParams) {
      NSNumber *targetInstanceIdNum = @(targetInstanceId);
      NSMutableDictionary<NSString *, NSObject *> *targetExtraParams =
          [[reporter allExtraParams] objectForKey:targetInstanceIdNum];
      if (targetExtraParams) {
        [extraParams addEntriesFromDictionary:targetExtraParams];
      }
      [[reporter allExtraParams] setObject:extraParams forKey:@(targetInstanceId)];
      [[reporter allExtraParams] removeObjectForKey:originInstanceIdNum];
    }
  }];
}

+ (void)putExtraParams:(NSDictionary<NSString *, NSObject *> *)params
         forInstanceId:(int32_t)instanceId {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEventReporter::setExtraParams",
              [instanceId](lynx::perfetto::EventContext ctx) {
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("instanceId");
                  debug->set_string_value(std::to_string(instanceId));
                }
              });
  if (!params || instanceId < 0) {
    return;
  }
  [self runOnReportThread:^{
    LynxEventReporter *reporter = [self sharedInstance];
    NSNumber *key = @(instanceId);
    NSMutableDictionary<NSString *, NSObject *> *extraParams =
        [[reporter allExtraParams] objectForKey:key];
    if (extraParams) {
      [extraParams addEntriesFromDictionary:params];
    } else {
      [[reporter allExtraParams] setObject:[params mutableCopy] forKey:key];
    }
  }];
}

+ (void)clearCacheForInstanceId:(int32_t)instanceId {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEventReporter::clearCacheByInstanceId",
              [instanceId](lynx::perfetto::EventContext ctx) {
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("instanceId");
                  debug->set_string_value(std::to_string(instanceId));
                }
              });
  if (instanceId < 0) {
    return;
  }
  [self runOnReportThread:^{
    LynxEventReporter *reporter = [self sharedInstance];
    NSNumber *key = @(instanceId);
    [[reporter allGenericInfo] removeObjectForKey:key];
    [[reporter allExtraParams] removeObjectForKey:key];
  }];
}

+ (void)addEventReportObserver:(id<LynxEventReportObserverProtocol>)observer {
  if (!observer) {
    return;
  }
  [self runOnReportThread:^{
    [[self sharedInstance] addObserver:observer];
  }];
}

- (void)addObserver:(id<LynxEventReportObserverProtocol>)observer {
  if (observer && ![_observerList containsObject:observer]) {
    [_observerList addObject:observer];
  }
}

+ (void)removeEventReportObserver:(id<LynxEventReportObserverProtocol>)observer {
  if (!observer) {
    return;
  }
  [self runOnReportThread:^{
    [[self sharedInstance] removeObserver:observer];
  }];
}

- (void)removeObserver:(id<LynxEventReportObserverProtocol>)observer {
  if (observer) {
    [_observerList removeObject:observer];
  }
}

#pragma mark - Private Methods
+ (void)runOnReportThread:(void (^)(void))task {
  if (!task) {
    return;
  }
  auto taskRunner = lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner();
  if (taskRunner->RunsTasksOnCurrentThread()) {
    task();
  } else {
    taskRunner->PostTask([task]() { task(); });
  }
}

- (void)handleEvent:(NSString *)eventName
              props:(NSDictionary<NSString *, NSObject *> *)props
         instanceId:(int32_t)instanceId {
  if (!eventName || (instanceId < 0 && !props)) {
    // If instance id is unknown, props of event must be not null.
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEventReporter::handleEvent",
              [&](lynx::perfetto::EventContext ctx) {
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("instanceId");
                  debug->set_string_value(std::to_string(instanceId));
                }
                {
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("eventName");
                  debug->set_string_value([eventName UTF8String]);
                }
              });
  NSNumber *instanceIdNum = @(instanceId);
  NSMutableDictionary *propsM = [NSMutableDictionary dictionary];
  NSMutableDictionary *genericInfo = [[self allGenericInfo] objectForKey:instanceIdNum];
  if (genericInfo) {
    [propsM addEntriesFromDictionary:genericInfo];
  }
  if (props) {
    [propsM addEntriesFromDictionary:props];
  }
  props = [propsM copy];
  NSMutableDictionary *extraParams = [[self allExtraParams] objectForKey:instanceIdNum];
  NSDictionary *extraData = [extraParams copy];
  for (id<LynxEventReportObserverProtocol> observer in self.observerList.copy) {
    [observer onReportEvent:eventName instanceId:instanceId props:props extraData:extraData];
  }
}

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  static LynxEventReporter *reporter;
  dispatch_once(&onceToken, ^{
    reporter = [[LynxEventReporter alloc] init];
  });
  return reporter;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _allGenericInfo = [NSMutableDictionary dictionary];
    _allExtraParams = [NSMutableDictionary dictionary];
    _observerList = [NSHashTable hashTableWithOptions:NSPointerFunctionsWeakMemory];
    id<LynxServiceEventReporterProtocol> service = LynxService(LynxServiceEventReporterProtocol);
    if (service) {
      [_observerList addObject:service];
    }
  }
  return self;
}

@end
